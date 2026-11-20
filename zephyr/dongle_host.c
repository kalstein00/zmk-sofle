#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/hid.h>
#include "dongle_host.h"
#include "dongle_log.h"
#include "dongle_host_state.h"
#include "dongle_status.h"
#include "dongle_menu.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum command_type { HOST_SELECT, HOST_NEXT, HOST_PREVIOUS, HOST_CLEAR_ALL,
                    HOST_OUTPUT, HOST_CONNECTED, HOST_MENU, HOST_CLEAR_SLOT };
enum wait_stage { WAIT_NONE, WAIT_INPUT_IDLE, WAIT_INPUT_BARRIER, WAIT_PC_DISCONNECT };
struct command { enum command_type type; uint8_t value; };
K_MSGQ_DEFINE(host_commands, sizeof(struct command), 16, 4);
K_THREAD_STACK_DEFINE(host_stack, 2048);
static struct k_work_q host_queue;
static struct k_work_delayable host_work;
static struct k_spinlock state_lock;
K_MUTEX_DEFINE(input_lock);
static struct dongle_host_state state;
static struct dongle_menu menu;
static struct command pending;
static bool command_active;
static bool disconnecting;
static int64_t disconnect_deadline;
static bool initialized;
static bool input_barrier_queued;
static enum wait_stage wait_stage;
K_SEM_DEFINE(input_barrier, 0, 1);

/* Store profile and transport as one settings value. Old settings remain the
 * fallback until the first successful write by this firmware. */
static uint8_t restored_selection[2];
static bool have_restored_selection;

static int load_selection(const char *name, size_t len, settings_read_cb read, void *arg) {
    if (strcmp(name, "selection") != 0) return -ENOENT;
    if (len != sizeof(restored_selection)) return -EINVAL;
    uint8_t data[2];
    int count = read(arg, data, sizeof(data));
    if (count < 0) return count;
    if (count != sizeof(data) || data[0] >= ZMK_BLE_PROFILE_COUNT ||
        (data[1] != ZMK_TRANSPORT_USB && data[1] != ZMK_TRANSPORT_BLE)) return -EINVAL;
    memcpy(restored_selection, data, sizeof(data));
    have_restored_selection = true;
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(sofle_host, "sofle_host", NULL, load_selection, NULL, NULL);

void dongle_host_restore_selection(uint8_t *profile) {
    if (!have_restored_selection) {
        dongle_log_note(DONGLE_LOG_HOST_DEFAULT, *profile, zmk_endpoints_get_preferred_transport());
        return;
    }
    *profile = restored_selection[0];
    dongle_endpoints_select_transport(restored_selection[1]);
    dongle_log_note(DONGLE_LOG_HOST_RESTORED, *profile, restored_selection[1]);
}

static int save_selection(uint8_t profile, enum zmk_transport transport) {
    const uint8_t data[2] = {profile, transport};
    return settings_save_one("sofle_host/selection", data, sizeof(data));
}

/* A device-less event reaches our listener after earlier input callbacks finish. */
static void input_barrier_received(struct input_event *event) {
    if (!event->dev && event->type == INPUT_EV_MSC && event->code == 0) {
        k_sem_give(&input_barrier);
    }
}
INPUT_CALLBACK_DEFINE(NULL, input_barrier_received);

void dongle_host_input_lock(void) { k_mutex_lock(&input_lock, K_FOREVER); }
void dongle_host_input_unlock(void) { k_mutex_unlock(&input_lock); }

bool dongle_host_reports_allowed(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    dongle_menu_expire(&menu, k_uptime_get());
    bool allowed = !state.switching && menu.page == MENU_CLOSED;
    k_spin_unlock(&state_lock, key);
    return allowed;
}

bool dongle_output_is_ble(void) {
    return zmk_endpoints_selected().transport == ZMK_TRANSPORT_BLE;
}

bool dongle_output_switching(void) {
    return !dongle_host_reports_allowed();
}

uint32_t dongle_host_generation(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    uint32_t generation = state.generation;
    k_spin_unlock(&state_lock, key);
    return generation;
}

bool dongle_host_transport_allowed(enum zmk_transport transport) {
    return dongle_host_reports_allowed() &&
           zmk_endpoints_get_preferred_transport() == transport;
}

static int request(enum command_type type, uint8_t value);

struct dongle_menu dongle_menu_snapshot(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    dongle_menu_expire(&menu, k_uptime_get());
    struct dongle_menu snapshot = menu;
    k_spin_unlock(&state_lock, key);
    return snapshot;
}

bool dongle_host_position(uint8_t source, uint32_t position, bool pressed) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    int64_t now = k_uptime_get();
    dongle_menu_expire(&menu, now);
    bool in_menu = menu.page != MENU_CLOSED;
    bool switching = state.switching;
    if (in_menu) state.switching = true;
    bool swallow = dongle_host_state_position(&state, source, position, pressed);
    state.switching = switching;
    enum dongle_menu_action action = MENU_NO_ACTION;
    uint8_t profile = menu.profile;
    if (in_menu && !switching && pressed && swallow) {
        action = dongle_menu_key(&menu, position, now);
        if (action != MENU_NO_ACTION) state.switching = true;
    }
    k_spin_unlock(&state_lock, key);
    if (in_menu && pressed && swallow) {
        /* The pointing activity listener observes this without producing HID. */
        input_report(NULL, INPUT_EV_MSC, 1, 0, false, K_NO_WAIT);
    }
    int err = 0;
    if (action == MENU_USE) err = request(HOST_SELECT, profile);
    else if (action == MENU_USE_USB) err = request(HOST_OUTPUT, ZMK_TRANSPORT_USB);
    else if (action == MENU_DELETE) err = request(HOST_CLEAR_SLOT, profile);
    else if (action == MENU_DELETE_ALL) err = request(HOST_CLEAR_ALL, 0);
    if (err) {
        key = k_spin_lock(&state_lock);
        state.switching = k_msgq_num_used_get(&host_commands) != 0;
        k_spin_unlock(&state_lock, key);
    }
    return swallow;
}

static int request(enum command_type type, uint8_t value) {
    if (!initialized) {
        dongle_log_note(DONGLE_LOG_HOST_COMMAND_DROPPED, type, -EAGAIN);
        return -EAGAIN;
    }
    struct command command = {type, value};
    /* Arm the output gate before returning to the keymap/macro caller. */
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    int err = k_msgq_put(&host_commands, &command, K_NO_WAIT);
    if (!err && (type != HOST_CLEAR_SLOT || value == zmk_ble_active_profile_index())) {
        dongle_host_state_begin(&state);
    }
    dongle_log_note(err ? DONGLE_LOG_HOST_COMMAND_DROPPED : DONGLE_LOG_HOST_COMMAND_QUEUED,
                    type, err ? err : value);
    k_spin_unlock(&state_lock, key);
    if (!err) {
        k_work_reschedule_for_queue(&host_queue, &host_work, K_NO_WAIT);
    }
    return err;
}

int zmk_ble_prof_select(uint8_t index) {
    return index < ZMK_BLE_PROFILE_COUNT ? request(HOST_SELECT, index) : -ERANGE;
}
int dongle_menu_request(void) { return request(HOST_MENU, 0); }
int zmk_ble_prof_next(void) { return request(HOST_NEXT, 0); }
int zmk_ble_prof_prev(void) { return request(HOST_PREVIOUS, 0); }
int dongle_host_clear_profile(uint8_t profile) {
    return profile < ZMK_BLE_PROFILE_COUNT ? request(HOST_CLEAR_SLOT, profile) : -ERANGE;
}
void zmk_ble_clear_bonds(void) {
    int err = dongle_host_clear_profile(zmk_ble_active_profile_index());
    if (err) LOG_ERR("Host clear request failed: %d", err);
}
void zmk_ble_clear_all_bonds(void) {
    int err = request(HOST_CLEAR_ALL, 0);
    if (err) LOG_ERR("Host clear-all request failed: %d", err);
}
int zmk_endpoints_select_transport(enum zmk_transport transport) {
    if (transport != ZMK_TRANSPORT_USB && transport != ZMK_TRANSPORT_BLE) return -EINVAL;
    return request(HOST_OUTPUT, transport);
}
void dongle_host_connected(void) {
    /* A background BLE reconnect must not release or gate selected USB input. */
    if (zmk_endpoints_get_preferred_transport() != ZMK_TRANSPORT_BLE) return;
    int err = request(HOST_CONNECTED, 0);
    if (err) LOG_ERR("Host connection gate failed: %d", err);
}

static bool input_idle(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    bool released = dongle_host_state_released(&state);
    k_spin_unlock(&state_lock, key);
    return released && dongle_behavior_queue_idle() && dongle_sticky_idle() && dongle_mouse_idle();
}

static void release_latched_input(void) {
    /* Toggle and sticky keys can outlive physical releases. Retire their timers
     * and reference-counted HID state before opening another endpoint. */
    dongle_sticky_reset();
    zmk_hid_implicit_modifiers_release();
    zmk_hid_masked_modifiers_clear();
    for (unsigned mod = 0; mod < 8; mod++) {
        while (zmk_hid_mod_is_pressed(mod)) zmk_hid_unregister_mod(mod);
    }
    zmk_hid_keyboard_clear();
    zmk_hid_consumer_clear();
    for (unsigned button = 0; button < ZMK_HID_MOUSE_NUM_BUTTONS; button++) {
        while (zmk_hid_get_mouse_report()->body.buttons & BIT(button)) {
            zmk_hid_mouse_button_release(button);
        }
    }
    zmk_hid_mouse_clear();
}

static void disconnect_pc(struct bt_conn *conn, void *data) {
    int *result = data;
    struct bt_conn_info info;
    if (!bt_conn_get_info(conn, &info) && info.role == BT_CONN_ROLE_PERIPHERAL &&
        info.state == BT_CONN_STATE_CONNECTED) {
        int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err && err != -ENOTCONN && !*result) *result = err;
    }
}

static void count_pc(struct bt_conn *conn, void *data) {
    struct bt_conn_info info;
    if (!bt_conn_get_info(conn, &info) && info.role == BT_CONN_ROLE_PERIPHERAL &&
        info.state != BT_CONN_STATE_DISCONNECTED) {
        (*(unsigned *)data)++;
    }
}

static int begin_pc_disconnect(void) {
    dongle_ble_pause();
    int err = 0;
    bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_pc, &err);
    if (err) {
        dongle_ble_resume();
        return err;
    }
    disconnecting = true;
    disconnect_deadline = k_uptime_get() + 4500;
    return 0;
}

static void finish_command(int result) {
    dongle_log_note(DONGLE_LOG_HOST_COMMAND_RESULT, pending.type, result);
    command_active = false;
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    if (k_msgq_num_used_get(&host_commands) == 0 && state.switching) {
        state.switching = false;
        dongle_log_note(DONGLE_LOG_HOST_GATE_OPEN, zmk_ble_active_profile_index(),
                        menu.page != MENU_CLOSED);
    }
    k_spin_unlock(&state_lock, key);
    k_work_reschedule_for_queue(&host_queue, &host_work, K_NO_WAIT);
}

static void wait_for(enum wait_stage stage) {
    if (wait_stage == stage) return;
    wait_stage = stage;
    dongle_log_note(DONGLE_LOG_HOST_GATE_WAIT, pending.type, stage);
}

static void process(struct k_work *work) {
    if (!command_active) {
        if (k_msgq_get(&host_commands, &pending, K_NO_WAIT)) return;
        wait_stage = WAIT_NONE;
        dongle_log_note(DONGLE_LOG_HOST_COMMAND_STARTED, pending.type, pending.value);
        if (pending.type == HOST_CLEAR_SLOT && pending.value != zmk_ble_active_profile_index()) {
            /* Erasing an inactive bond must not touch the live HID endpoint. */
            int err = dongle_ble_clear_slot(pending.value);
            if (err) LOG_ERR("Host clear failed for profile %u: %d", pending.value, err);
            finish_command(err);
            return;
        }
        /* A previously queued USB selection may have taken effect in the meantime. */
        if (pending.type == HOST_CONNECTED &&
            zmk_endpoints_get_preferred_transport() != ZMK_TRANSPORT_BLE) {
            finish_command(-ECANCELED);
            return;
        }
        command_active = true;
        disconnecting = false;
        input_barrier_queued = false;
        /* Send neutral reports before clearing HID reference counts. */
        struct zmk_endpoint_instance old = zmk_endpoints_selected();
        int err = old.transport == ZMK_TRANSPORT_BLE ? dongle_hog_release() : dongle_usb_release();
        if (err) dongle_log_note(DONGLE_LOG_HOST_RELEASE_RESULT, old.transport, err);
        if (err && err != -ENOTCONN && err != -ENODEV && pending.type != HOST_CONNECTED) {
            bool usb_recovery = pending.type == HOST_OUTPUT && pending.value == ZMK_TRANSPORT_USB;
            if (old.transport == ZMK_TRANSPORT_BLE && (usb_recovery || pending.type == HOST_MENU)) {
                /* Closing the failed BLE link clears the old PC before opening the menu or USB. */
                err = begin_pc_disconnect();
            }
            if (err) {
                finish_command(err);
                return;
            }
        }
    }
    dongle_host_input_lock();
    if (!input_idle()) {
        wait_for(WAIT_INPUT_IDLE);
        dongle_host_input_unlock();
        k_work_reschedule_for_queue(&host_queue, &host_work, K_MSEC(10));
        return;
    }
    if (!input_barrier_queued) {
        dongle_mouse_reset();
        k_sem_reset(&input_barrier);
        input_barrier_queued = input_report(NULL, INPUT_EV_MSC, 0, 0, false, K_NO_WAIT) == 0;
    }
    if (!input_barrier_queued || k_sem_count_get(&input_barrier) == 0) {
        wait_for(WAIT_INPUT_BARRIER);
        dongle_host_input_unlock();
        k_work_reschedule_for_queue(&host_queue, &host_work, K_MSEC(10));
        return;
    }
    release_latched_input();
    uint8_t profile = zmk_ble_active_profile_index();
    bool selecting = pending.type == HOST_SELECT || pending.type == HOST_NEXT ||
                     pending.type == HOST_PREVIOUS;
    if (pending.type == HOST_SELECT) profile = pending.value;
    else if (pending.type == HOST_NEXT) profile = (profile + 1) % ZMK_BLE_PROFILE_COUNT;
    else if (pending.type == HOST_PREVIOUS) {
        profile = (profile + ZMK_BLE_PROFILE_COUNT - 1) % ZMK_BLE_PROFILE_COUNT;
    }
    bool change_profile = pending.type != HOST_OUTPUT && pending.type != HOST_CONNECTED &&
                          pending.type != HOST_MENU;
    bool wait_for_pc = change_profile || disconnecting;
    if (wait_for_pc) {
        if (!disconnecting) {
            int err = begin_pc_disconnect();
            if (err) {
                finish_command(err);
                dongle_host_input_unlock();
                return;
            }
        }
        unsigned connected = 0;
        bt_conn_foreach(BT_CONN_TYPE_LE, count_pc, &connected);
        if (connected && k_uptime_get() >= disconnect_deadline) {
            dongle_ble_resume();
            finish_command(-ETIMEDOUT);
            dongle_host_input_unlock();
            return;
        }
        if (connected) {
            wait_for(WAIT_PC_DISCONNECT);
            dongle_host_input_unlock();
            k_work_reschedule_for_queue(&host_queue, &host_work, K_MSEC(10));
            return;
        }
    }
    int err = 0;
    switch (pending.type) {
    case HOST_SELECT:
    case HOST_NEXT:
    case HOST_PREVIOUS:
        err = save_selection(profile, ZMK_TRANSPORT_BLE);
        if (!err) err = dongle_ble_select_profile(profile);
        if (!err) err = dongle_endpoints_select_transport(ZMK_TRANSPORT_BLE);
        break;
    case HOST_CLEAR_ALL:
        err = dongle_ble_clear_profiles(true);
        if (!err) err = save_selection(0, zmk_endpoints_get_preferred_transport());
        if (!err) err = dongle_ble_apply_profile(0);
        break;
    case HOST_OUTPUT:
        err = save_selection(profile, pending.value);
        if (!err) err = dongle_endpoints_select_transport(pending.value);
        break;
    case HOST_CONNECTED: break;
    case HOST_CLEAR_SLOT: err = dongle_ble_clear_slot(pending.value); break;
    case HOST_MENU: {
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        dongle_menu_open(&menu, profile, k_uptime_get());
        k_spin_unlock(&state_lock, key);
        break;
    }
    }
    if (wait_for_pc) dongle_ble_resume();
    if (err) LOG_ERR("Host command %d failed: %d", pending.type, err);
    else LOG_INF("Host selected: profile %d, output %d", zmk_ble_active_profile_index(),
                 zmk_endpoints_get_preferred_transport());
    finish_command(err);
    dongle_host_input_unlock();
}

static int log_usb_state(const zmk_event_t *event) {
    const struct zmk_usb_conn_state_changed *usb = as_zmk_usb_conn_state_changed(event);
    dongle_log_note(DONGLE_LOG_USB_STATE, -1, usb->conn_state);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(sofle_host_usb_log, log_usb_state);
ZMK_SUBSCRIPTION(sofle_host_usb_log, zmk_usb_conn_state_changed);

static int init(void) {
    dongle_log_note(DONGLE_LOG_USB_STATE, -1, zmk_usb_get_conn_state());
    k_work_init_delayable(&host_work, process);
    k_work_queue_start(&host_queue, host_stack, K_THREAD_STACK_SIZEOF(host_stack), 5, NULL);
    initialized = true;
    return 0;
}
SYS_INIT(init, APPLICATION, 89);
