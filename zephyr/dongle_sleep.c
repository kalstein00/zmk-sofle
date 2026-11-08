#define DT_DRV_COMPAT zmk_behavior_dongle_soft_off
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/display.h>
#include <zmk/split/central.h>
#include "dongle_sleep_state.h"
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct dongle_sleep_state state;
static struct bt_conn *connections[2];
static struct k_spinlock lock;
extern struct k_work unblank_display_work;
void blank_display_cb(struct k_work *work);

bool dongle_sleep_blanked(void) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    bool result = state.blanked;
    k_spin_unlock(&lock, key);
    return result;
}
static void deep_blank(struct k_work *work) {
    if (dongle_sleep_blanked()) blank_display_cb(work);
}
K_WORK_DEFINE(deep_blank_work, deep_blank);
void dongle_sleep_connection(int slot, struct bt_conn *conn, bool connected) {
    if (slot < 0 || slot >= 2) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    struct bt_conn *old = NULL;
    if (connected) {
        old = connections[slot];
        connections[slot] = bt_conn_ref(conn);
        dongle_sleep_connected(&state, slot);
    } else if (connections[slot] == conn) {
        old = connections[slot];
        connections[slot] = NULL;
        dongle_sleep_disconnected(&state, slot);
    }
    k_spin_unlock(&lock, key);
    if (old) bt_conn_unref(old);
}
struct bt_conn *dongle_sleep_target_ref(unsigned slot, uint32_t generation) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    struct bt_conn *conn = dongle_sleep_target_current_state(&state, slot, generation) ?
                           bt_conn_ref(connections[slot]) : NULL;
    k_spin_unlock(&lock, key);
    return conn;
}
void dongle_sleep_send_result(unsigned slot, uint32_t generation, int result) {
    if (result >= 0) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    bool wake = dongle_sleep_target_current_state(&state, slot, generation) &&
                dongle_sleep_send_failed(&state, slot);
    k_spin_unlock(&lock, key);
    if (wake) k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);
}
void dongle_sleep_input_received(int slot, struct bt_conn *conn) {
    if (slot < 0 || slot >= 2) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    bool wake = connections[slot] == conn && dongle_sleep_key(&state, slot, true);
    k_spin_unlock(&lock, key);
    if (wake) k_work_submit_to_queue(zmk_display_work_q(), &unblank_display_work);
}

static struct zmk_behavior_binding_event held_event;
static void sleep_expired(struct k_work *work) {
    ARG_UNUSED(work);
    k_spinlock_key_t key = k_spin_lock(&lock);
    uint8_t connected = dongle_sleep_begin(&state);
    uint32_t generations[2] = {state.generation[0], state.generation[1]};
    k_spin_unlock(&lock, key);
    k_work_submit_to_queue(zmk_display_work_q(), &deep_blank_work);
    struct zmk_behavior_binding remote = {.behavior_dev = "z_so_off"};
    for (unsigned i = 0; i < 2; ++i) {
        if (!(connected & (1u << i))) continue;
        remote.param1 = generations[i];
        int err = zmk_split_central_invoke_behavior(i, &remote, held_event, true);
        if (err < 0) {
            LOG_ERR("Failed to queue soft off for slot %u (%d)", i, err);
            dongle_sleep_send_result(i, generations[i], err);
        }
    }
}
K_WORK_DELAYABLE_DEFINE(sleep_work, sleep_expired);
static int press(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    held_event = event;
    k_work_reschedule(&sleep_work, K_SECONDS(2));
    return ZMK_BEHAVIOR_OPAQUE;
}
static int release(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding); ARG_UNUSED(event);
    k_work_cancel_delayable(&sleep_work);
    return ZMK_BEHAVIOR_OPAQUE;
}
static const struct behavior_driver_api api = {
    .binding_pressed = press, .binding_released = release,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
