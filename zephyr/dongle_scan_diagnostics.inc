#include <zephyr/sys/atomic.h>

int dongle_split_known_slot(const bt_addr_le_t *addr);

enum dongle_scan_flag {
    SCAN_UNDIRECTED = BIT(0),
    SCAN_DIRECTED = BIT(1),
    SCAN_OTHER = BIT(2),
    SCAN_RESERVATION_REJECTED = BIT(3),
};

static atomic_t scan_reports;
static atomic_t scan_flags[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static void dongle_scan_mark(const bt_addr_le_t *addr, enum dongle_scan_flag flag) {
    int slot = dongle_split_known_slot(addr);
    if (slot >= 0) {
        atomic_or(&scan_flags[slot], flag);
    }
}

static void dongle_scan_report(const bt_addr_le_t *addr, uint8_t type) {
    if (atomic_get(&scan_reports) < INT16_MAX) {
        atomic_inc(&scan_reports);
    }
    if (type == BT_GAP_ADV_TYPE_ADV_IND) {
        dongle_scan_mark(addr, SCAN_UNDIRECTED);
    } else if (type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        dongle_scan_mark(addr, SCAN_DIRECTED);
    } else {
        dongle_scan_mark(addr, SCAN_OTHER);
    }
}

static void dongle_scan_reset(void) {
    atomic_clear(&scan_reports);
    for (int i = 0; i < ARRAY_SIZE(scan_flags); i++) {
        atomic_clear(&scan_flags[i]);
    }
}

static void dongle_scan_sample(struct k_work *work) {
    ARG_UNUSED(work);
    if (!is_scanning) {
        return;
    }
    dongle_log_note(DONGLE_LOG_SCAN_REPORTS, -1, atomic_get(&scan_reports));
    for (int i = 0; i < ARRAY_SIZE(scan_flags); i++) {
        if (peripherals[i].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            dongle_log_note(DONGLE_LOG_SCAN_SLOT, i, atomic_get(&scan_flags[i]));
        }
    }
}

K_WORK_DELAYABLE_DEFINE(dongle_scan_sample_work, dongle_scan_sample);
