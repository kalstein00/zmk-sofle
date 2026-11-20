#define DT_DRV_COMPAT zmk_behavior_host_clear
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>
#include "dongle_host.h"

static bool all_profiles;
static uint32_t clear_position;
static bool armed;
static uint8_t target_profile;
static int64_t clear_deadline;

static void clear_work(struct k_work *work) {
    dongle_host_input_lock();
    if (armed && k_uptime_get() >= clear_deadline) {
        armed = false;
        if (all_profiles) zmk_ble_clear_all_bonds();
        else dongle_host_clear_profile(target_profile);
    }
    dongle_host_input_unlock();
}
K_WORK_DELAYABLE_DEFINE(clear_timer, clear_work);

static int pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    if (binding->param1 > 1) return -EINVAL;
    all_profiles = binding->param1;
    target_profile = zmk_ble_active_profile_index();
    clear_position = event.position;
    clear_deadline = k_uptime_get() + 2000;
    armed = true;
    k_work_reschedule(&clear_timer, K_SECONDS(2));
    return ZMK_BEHAVIOR_OPAQUE;
}
static int released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    if (event.position == clear_position) {
        armed = false;
        k_work_cancel_delayable(&clear_timer);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}
static const struct behavior_driver_api api = {.binding_pressed = pressed, .binding_released = released};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
