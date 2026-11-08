#include <zephyr/kernel.h>
#include <zmk/ble.h>
#include "dongle_battery_snapshot.h"
static struct dongle_slot_state slots[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
static struct k_spinlock lock;
static bool valid(int n) { return n >= 0 && n < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; }
void dongle_battery_connect(int n, struct bt_conn *conn) {
    if (!valid(n)) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    dongle_slot_connect(&slots[n], (uintptr_t)conn);
    k_spin_unlock(&lock, key);
}
void dongle_battery_disconnect(int n, struct bt_conn *conn) {
    if (!valid(n)) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    dongle_slot_disconnect(&slots[n], (uintptr_t)conn);
    k_spin_unlock(&lock, key);
}
void dongle_battery_sample(int n, struct bt_conn *conn, uint8_t level) {
    if (!valid(n)) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    dongle_slot_battery(&slots[n], (uintptr_t)conn, level);
    k_spin_unlock(&lock, key);
}
struct dongle_slot_state dongle_battery_get(unsigned n) {
    struct dongle_slot_state result = {0};
    k_spinlock_key_t key = k_spin_lock(&lock);
    if (n < ZMK_SPLIT_BLE_PERIPHERAL_COUNT) result = slots[n];
    k_spin_unlock(&lock, key);
    return result;
}
