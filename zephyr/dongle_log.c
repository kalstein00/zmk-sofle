#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include "dongle_log.h"
#include "dongle_log_ring.h"

struct log_record {
    uint32_t sequence;
    uint32_t uptime_ms;
    int16_t value;
    int8_t slot;
    uint8_t event;
};

static struct log_record records[DONGLE_LOG_CAPACITY];
static uint32_t latest;
static bool have_records;
static atomic_t dropped;
static atomic_t write_failures;
static const struct device *const log_uart = DEVICE_DT_GET(DT_NODELABEL(sofle_log_uart));
K_MSGQ_DEFINE(log_queue, sizeof(struct log_record), 48, 4);
K_SEM_DEFINE(log_ready, 0, 1);

static int load_record(const char *name, size_t len, settings_read_cb read, void *arg) {
    if (len != sizeof(struct log_record)) return -EINVAL;
    char *end;
    unsigned long slot = strtoul(name, &end, 10);
    if (end == name || *end || slot >= DONGLE_LOG_CAPACITY) return -EINVAL;
    struct log_record record;
    int size = read(arg, &record, sizeof(record));
    if (size < 0) return size;
    if (size != sizeof(record) || !record.event || record.event >= DONGLE_LOG_END) return -EINVAL;
    unsigned index = dongle_log_slot(record.sequence);
    /* Accept records written before the ring grew to its current size. */
    unsigned legacy_slot = record.sequence - 1u;
    if (index != slot && legacy_slot % 64u != slot && legacy_slot % 32u != slot) {
        return -EINVAL;
    }
    if (!records[index].event || dongle_log_is_newer(record.sequence, records[index].sequence)) {
        records[index] = record;
    }
    if (!have_records || dongle_log_is_newer(record.sequence, latest)) latest = record.sequence;
    have_records = true;
    return 0;
}

static int start_log(void) {
    k_sem_give(&log_ready);
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(sofle_log, "sofle_log", NULL, load_record, start_log, NULL);

void dongle_log_note(enum dongle_log_event event, int slot, int value) {
    struct log_record record = {
        .uptime_ms = k_uptime_get_32(), .event = event, .slot = slot, .value = value,
    };
    if (k_msgq_put(&log_queue, &record, K_NO_WAIT)) atomic_inc(&dropped);
}

static void save_record(struct log_record record) {
    record.sequence = latest + 1u;
    unsigned slot = dongle_log_slot(record.sequence);
    char key[sizeof("sofle_log/255")];
    snprintf(key, sizeof(key), "sofle_log/%02u", slot);
    if (settings_save_one(key, &record, sizeof(record))) {
        atomic_inc(&write_failures);
        return;
    }
    records[slot] = record;
    latest = record.sequence;
    have_records = true;
}

static void serial_write(const char *line) {
    for (; *line; line++) uart_poll_out(log_uart, *line);
}

/* CDC poll-out can overwrite unread bytes when its TX ring fills. */
#define SOFLE_LOG_LINE_SIZE 96
BUILD_ASSERT(DONGLE_LOG_CAPACITY * SOFLE_LOG_LINE_SIZE + 128 <=
             CONFIG_USB_CDC_ACM_RINGBUF_SIZE);

static void dump_records(void) {
    static const char *const events[] = {
        [DONGLE_LOG_BOOT] = "boot",
        [DONGLE_LOG_SCAN_STARTED] = "scan_started",
        [DONGLE_LOG_SCAN_FAILED] = "scan_failed",
        [DONGLE_LOG_SPLIT_CONNECTING] = "split_connecting",
        [DONGLE_LOG_SPLIT_CONNECT_FAILED] = "split_connect_failed",
        [DONGLE_LOG_SPLIT_CONNECTED] = "split_connected",
        [DONGLE_LOG_SPLIT_DISCONNECTED] = "split_disconnected",
        [DONGLE_LOG_DISCOVERY_FAILED] = "discovery_failed",
        [DONGLE_LOG_SPLIT_SIDE] = "split_side",
        [DONGLE_LOG_DROPPED] = "events_dropped",
        [DONGLE_LOG_SCAN_REPORTS] = "scan_reports",
        [DONGLE_LOG_SCAN_SLOT] = "scan_slot",
        [DONGLE_LOG_KEY_SUBSCRIBE_RESULT] = "key_subscribe_result",
        [DONGLE_LOG_KEY_CCC_RESULT] = "key_ccc_result",
        [DONGLE_LOG_SPLIT_SECURITY_FAILED] = "split_security_failed",
        [DONGLE_LOG_SPLIT_SERVICE_MISSING] = "split_service_missing",
        [DONGLE_LOG_KEY_CHARACTERISTIC_MISSING] = "key_characteristic_missing",
        [DONGLE_LOG_USB_STATE] = "usb_state",
        [DONGLE_LOG_HOST_RESTORED] = "host_restored",
        [DONGLE_LOG_HOST_DEFAULT] = "host_default",
        [DONGLE_LOG_HOST_ADV_MODE] = "host_adv_mode",
        [DONGLE_LOG_HOST_ADV_FAILED] = "host_adv_failed",
        [DONGLE_LOG_HOST_LINK_ACCEPTED] = "host_link_accepted",
        [DONGLE_LOG_HOST_LINK_REJECTED] = "host_link_rejected",
        [DONGLE_LOG_HOST_CONNECT_FAILED] = "host_connect_failed",
        [DONGLE_LOG_HOST_DISCONNECTED] = "host_disconnected",
        [DONGLE_LOG_HOST_SECURITY_LEVEL] = "host_security_level",
        [DONGLE_LOG_HOST_SECURITY_FAILED] = "host_security_failed",
        [DONGLE_LOG_HOST_PAIRING_OPEN] = "host_pairing_open",
        [DONGLE_LOG_HOST_PAIRING_TIMEOUT] = "host_pairing_timeout",
        [DONGLE_LOG_HOST_PAIRING_RESULT] = "host_pairing_result",
        [DONGLE_LOG_HOST_COMMAND_QUEUED] = "host_command_queued",
        [DONGLE_LOG_HOST_COMMAND_DROPPED] = "host_command_dropped",
        [DONGLE_LOG_HOST_COMMAND_STARTED] = "host_command_started",
        [DONGLE_LOG_HOST_COMMAND_RESULT] = "host_command_result",
        [DONGLE_LOG_HOST_GATE_WAIT] = "host_gate_wait",
        [DONGLE_LOG_HOST_GATE_OPEN] = "host_gate_open",
        [DONGLE_LOG_HOST_RELEASE_RESULT] = "host_release_result",
        [DONGLE_LOG_HOST_BOOT_BOND] = "host_boot_bond",
        [DONGLE_LOG_HOST_HID_CCC] = "host_hid_ccc",
        [DONGLE_LOG_HOST_HID_NOTIFY_RESULT] = "host_hid_notify_result",
    };
    char line[SOFLE_LOG_LINE_SIZE];
    snprintf(line, sizeof(line), "SOFLE LOG v1 (flash errors since boot: %ld)\r\n",
             (long)atomic_get(&write_failures));
    serial_write(line);
    for (unsigned i = DONGLE_LOG_CAPACITY; have_records && i > 0; i--) {
        uint32_t seq = latest - (i - 1);
        const struct log_record *record = &records[dongle_log_slot(seq)];
        if (!record->event || record->sequence != seq) continue;
        snprintf(line, sizeof(line), "%lu ms=%lu %s slot=%d code=%d\r\n",
                 (unsigned long)seq, (unsigned long)record->uptime_ms, events[record->event],
                 record->slot, record->value);
        serial_write(line);
    }
    serial_write("END SOFLE LOG\r\n");
}

static void log_worker(void *a, void *b, void *c) {
    k_sem_take(&log_ready, K_FOREVER);
    save_record((struct log_record){.event = DONGLE_LOG_BOOT, .slot = -1});
    uint32_t last_scan_failure = 0;
    bool seen_scan_failure = false;
    bool dtr_was_on = false;
    for (;;) {
        struct log_record record;
        if (!k_msgq_get(&log_queue, &record, K_SECONDS(1))) {
            /* Persistent scan failures retry twice per second. Bound writes for
             * that loop while keeping the first failure and subsequent recovery. */
            if (record.event != DONGLE_LOG_SCAN_FAILED || !seen_scan_failure ||
                record.uptime_ms - last_scan_failure >= 60000u) {
                save_record(record);
                if (record.event == DONGLE_LOG_SCAN_FAILED) {
                    seen_scan_failure = true;
                    last_scan_failure = record.uptime_ms;
                }
            }
            if (record.event == DONGLE_LOG_SCAN_STARTED) seen_scan_failure = false;
        }
        atomic_val_t lost = atomic_set(&dropped, 0);
        if (lost) {
            save_record((struct log_record){.event = DONGLE_LOG_DROPPED, .slot = -1,
                        .uptime_ms = k_uptime_get_32(), .value = MIN(lost, INT16_MAX)});
        }
        uint32_t dtr = 0;
        if (device_is_ready(log_uart)) uart_line_ctrl_get(log_uart, UART_LINE_CTRL_DTR, &dtr);
        if (!dtr) dtr_was_on = false;
        else if (k_msgq_num_used_get(&log_queue) == 0 && !dtr_was_on) {
            dump_records();
            dtr_was_on = true;
        }
    }
}
K_THREAD_DEFINE(sofle_log_worker, 1536, log_worker, NULL, NULL, NULL, 8, 0, 0);
