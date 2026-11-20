#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS 1
#include "dongle_log.h"
#include "dongle_log_ring.h"

#define DT_NODELABEL(name) 0
#define DEVICE_DT_GET(node) NULL
#define CONFIG_USB_CDC_ACM_RINGBUF_SIZE 24832
#define BUILD_ASSERT(condition) _Static_assert(condition, #condition)
#define K_FOREVER (-1)
#define K_NO_WAIT 0
#define K_SECONDS(s) (s)
#define UART_LINE_CTRL_DTR 0
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define K_MSGQ_DEFINE(name, size, count, alignment) static int name
#define K_SEM_DEFINE(name, initial, max) static int name
#define K_THREAD_DEFINE(name, stack, fn, a, b, c, prio, opt, delay)
#define SETTINGS_STATIC_HANDLER_DEFINE(id, prefix, get, set, commit, export)
struct device;
typedef int (*settings_read_cb)(void *, void *, size_t);
typedef int atomic_val_t;
typedef int atomic_t;
static int atomic_get(atomic_t *a) { return *a; }
static int atomic_set(atomic_t *a, int next) { int old = *a; *a = next; return old; }
static void atomic_inc(atomic_t *a) { ++*a; }
static void k_sem_give(int *s) {}
static void k_sem_take(int *s, int timeout) {}
static int k_msgq_get(int *q, void *data, int timeout) { return -EAGAIN; }
static unsigned k_msgq_num_used_get(int *q) { return 0; }
static int k_msgq_put(int *q, const void *data, int timeout) { return 0; }
static uint32_t k_uptime_get_32(void) { return 0; }
static bool device_is_ready(const struct device *dev) { return true; }
static int uart_line_ctrl_get(const struct device *dev, int type, uint32_t *state) { return 0; }

static char output[CONFIG_USB_CDC_ACM_RINGBUF_SIZE];
static size_t output_size;
static void uart_poll_out(const struct device *dev, char value) {
    assert(output_size + 1 < sizeof(output));
    output[output_size++] = value;
}
static uint8_t flash[DONGLE_LOG_CAPACITY][32];
static unsigned flash_count;
static bool fail_write;
static int settings_save_one(const char *key, const void *record, size_t len) {
    assert(!strncmp(key, "sofle_log/", 10));
    unsigned slot = strtoul(key + 10, NULL, 10);
    assert(slot < DONGLE_LOG_CAPACITY && len <= sizeof(flash[slot]));
    if (fail_write) return -EIO;
    if (!memcmp(flash[slot], (uint8_t[32]){0}, sizeof(flash[slot]))) flash_count++;
    memcpy(flash[slot], record, len);
    return 0;
}
static int read_record(void *src, void *dst, size_t len) {
    memcpy(dst, src, len);
    return len;
}

#include "log.generated.c"

int main(void) {
    for (unsigned i = 1; i <= DONGLE_LOG_CAPACITY + 8; i++) {
        save_record((struct log_record){.event = DONGLE_LOG_SPLIT_CONNECTED, .value = i});
    }
    assert(flash_count == DONGLE_LOG_CAPACITY);
    fail_write = true;
    save_record((struct log_record){.event = DONGLE_LOG_SPLIT_DISCONNECTED});
    assert(latest == DONGLE_LOG_CAPACITY + 8 && write_failures == 1);

    /* Simulate reboot: reset RAM and reload the retained settings in reverse slot order. */
    memset(records, 0, sizeof(records));
    latest = 0;
    have_records = false;
    for (unsigned i = DONGLE_LOG_CAPACITY; i > 0; i--) {
        char key[4];
        snprintf(key, sizeof(key), "%02u", i - 1);
        assert(!load_record(key, sizeof(struct log_record), read_record, flash[i - 1]));
    }
    assert(latest == DONGLE_LOG_CAPACITY + 8);
    dump_records();
    char *first = strchr(output, '\n');
    assert(first && !strncmp(first + 1, "9 ms=", 5));
    char last[96];
    snprintf(last, sizeof(last), "%u ms=0 split_connected slot=0 code=%u\r\nEND SOFLE LOG",
             latest, latest);
    assert(strstr(output, last));
    unsigned lines = 0;
    for (const char *line = output; (line = strchr(line, '\n')); line++) lines++;
    assert(lines == DONGLE_LOG_CAPACITY + 2);
    assert(!strstr(output, "split_disconnected"));
    assert(dongle_log_slot(0) == DONGLE_LOG_CAPACITY - 1 && dongle_log_is_newer(0, UINT32_MAX));
    /* An upgraded firmware can read v1 records written at the old modulo. */
    struct log_record legacy = {.sequence = 34, .event = DONGLE_LOG_SPLIT_CONNECT_FAILED};
    memset(records, 0, sizeof(records));
    assert(!load_record("01", sizeof(legacy), read_record, &legacy));
    assert(records[dongle_log_slot(34)].event == DONGLE_LOG_SPLIT_CONNECT_FAILED);
    legacy.sequence = 66;
    assert(!load_record("01", sizeof(legacy), read_record, &legacy));
    assert(records[dongle_log_slot(66)].event == DONGLE_LOG_SPLIT_CONNECT_FAILED);
    puts("PASS: log overwrites oldest, survives settings reload and dumps in sequence order");
}
