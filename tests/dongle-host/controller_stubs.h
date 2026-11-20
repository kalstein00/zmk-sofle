#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "dongle_host_state.h"
#include "dongle_menu.h"
static int64_t now;
static int64_t k_uptime_get(void) { return now; }

#define K_NO_WAIT 0
#define K_FOREVER (-1)
#define K_MSEC(x) (x)
#define BIT(x) (1u << (x))
#define LOG_INF(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define LOG_MODULE_DECLARE(...)
#define ZMK_BLE_PROFILE_COUNT 4
#define ZMK_HID_MOUSE_NUM_BUTTONS 5
#define BT_CONN_ROLE_PERIPHERAL 1
#define BT_CONN_TYPE_LE 1
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN 0x13
#define BT_CONN_STATE_DISCONNECTED 0
#define BT_CONN_STATE_CONNECTED 1
#define K_THREAD_STACK_DEFINE(name, size) static char name[size]
#define K_THREAD_STACK_SIZEOF(name) sizeof(name)
#define SYS_INIT(fn, level, priority) static int (*startup)(void) = fn
#define K_MUTEX_DEFINE(name) static struct k_mutex name
#define INPUT_EV_MSC 4
struct input_event { const void *dev; uint8_t type; uint16_t code; };
#define INPUT_CALLBACK_DEFINE(dev, fn) static void (*input_callback)(struct input_event *) = fn
static struct input_event queued_input;
static bool have_queued_input;
static int input_report(const void *dev, uint8_t type, uint16_t code, int value, bool sync, int timeout) {
    if (code == 1) return 0;
    assert(!have_queued_input);
    queued_input = (struct input_event){dev, type, code};
    have_queued_input = true;
    return 0;
}
static void drain_input(void (*callback)(struct input_event *)) {
    assert(have_queued_input);
    have_queued_input = false;
    callback(&queued_input);
}
struct k_sem { unsigned count; };
#define K_SEM_DEFINE(name, initial, max) static struct k_sem name = {initial}
static void k_sem_give(struct k_sem *sem) { sem->count = 1; }
static void k_sem_reset(struct k_sem *sem) { sem->count = 0; }
static unsigned k_sem_count_get(struct k_sem *sem) { return sem->count; }

struct k_spinlock { int unused; };
typedef int k_spinlock_key_t;
struct k_mutex { int unused; };
static int k_mutex_lock(struct k_mutex *m, int timeout) { return 0; }
static int k_mutex_unlock(struct k_mutex *m) { return 0; }
static int k_spin_lock(struct k_spinlock *m) { return 0; }
static void k_spin_unlock(struct k_spinlock *m, int key) {}
struct k_work { void (*handler)(struct k_work *); };
struct k_work_delayable { struct k_work work; };
struct k_work_q { int unused; };
static struct k_work *scheduled;
static void k_work_init_delayable(struct k_work_delayable *work, void (*handler)(struct k_work *)) {
    work->work.handler=handler;
}
static int k_work_reschedule_for_queue(struct k_work_q *q, struct k_work_delayable *work, int delay) {
    scheduled=&work->work; return 0;
}
static void k_work_queue_start(struct k_work_q *q, void *stack, size_t size, int priority, void *config) {}
struct k_msgq { void *storage; size_t size; unsigned capacity, used, head, tail; };
#define K_MSGQ_DEFINE(name, size_, count_, align_) \
    static unsigned char name##_storage[(size_) * (count_)]; \
    struct k_msgq name = {.storage=name##_storage, .size=(size_), .capacity=(count_)}
static int k_msgq_put(struct k_msgq *q, const void *data, int timeout) {
    if (q->used==q->capacity) return -ENOMSG;
    memcpy((char *)q->storage+q->tail*q->size,data,q->size);
    q->tail=(q->tail+1)%q->capacity; q->used++; return 0;
}
static int k_msgq_get(struct k_msgq *q, void *data, int timeout) {
    if (!q->used) return -ENOMSG;
    memcpy(data,(char *)q->storage+q->head*q->size,q->size);
    q->head=(q->head+1)%q->capacity; q->used--; return 0;
}
static unsigned k_msgq_num_used_get(struct k_msgq *q) { return q->used; }
static void tick(void) {
    if (scheduled) { struct k_work *work=scheduled; scheduled=NULL; work->handler(work); }
}

enum zmk_transport { ZMK_TRANSPORT_USB, ZMK_TRANSPORT_BLE };
struct zmk_endpoint_instance { enum zmk_transport transport; struct { int profile_index; } ble; };
static enum zmk_transport preferred=ZMK_TRANSPORT_USB;
static uint8_t selected_profile;
typedef int (*settings_read_cb)(void *, void *, size_t);
#define SETTINGS_STATIC_HANDLER_DEFINE(id, name, get, set, commit, export) \
    static int (*settings_set)(const char *, size_t, settings_read_cb, void *) = set
static uint8_t saved_selection[2];
static unsigned selection_writes;
static bool fail_selection_save;
static int settings_save_one(const char *key, const void *data, size_t len) {
    assert(!strcmp(key, "sofle_host/selection") && len == sizeof(saved_selection));
    if (fail_selection_save) return -EIO;
    memcpy(saved_selection, data, len);
    selection_writes++;
    return 0;
}
static int read_selection(void *src, void *dst, size_t len) {
    memcpy(dst, src, len);
    return len;
}
static int zmk_ble_active_profile_index(void) { return selected_profile; }
static bool pc_bonds[4]={true,true,true,true};
static bool split_bonds[2]={true,true};
static bool paused;
static unsigned usb_releases, ble_releases;
#ifndef TEST_REAL_ENDPOINTS
static enum zmk_transport zmk_endpoints_get_preferred_transport(void) { return preferred; }
static struct zmk_endpoint_instance zmk_endpoints_selected(void) {
    return (struct zmk_endpoint_instance){.transport=preferred,.ble.profile_index=selected_profile};
}
static int dongle_endpoints_select_transport(enum zmk_transport transport) { preferred=transport; return 0; }
#endif
static int dongle_ble_select_profile(uint8_t index) { selected_profile=index; return 0; }
static int dongle_ble_apply_profile(uint8_t index) { selected_profile=index; return 0; }
static int dongle_ble_clear_profiles(bool all) {
    if (all) memset(pc_bonds,0,sizeof(pc_bonds));
    else pc_bonds[selected_profile]=false;
    return 0;
}
static int dongle_ble_clear_slot(uint8_t profile) { pc_bonds[profile] = false; return 0; }
static void dongle_ble_pause(void) { paused=true; }
static void dongle_ble_resume(void) { paused=false; }
static bool dongle_behavior_queue_idle(void) { return true; }
static bool dongle_sticky_idle(void) { return true; }
static bool dongle_mouse_idle(void) { return true; }
static void dongle_sticky_reset(void) {}
static void dongle_mouse_reset(void) {}
static int usb_release_result;
static int dongle_usb_release(void) { usb_releases++; return usb_release_result; }
static int ble_release_result;
static int dongle_hog_release(void) { ble_releases++; return ble_release_result; }
struct zmk_hid_keyboard_report_body { uint8_t modifiers, reserved, keys[6]; };
struct zmk_hid_consumer_report_body { uint16_t keys[2]; };
struct zmk_hid_mouse_report_body { uint8_t buttons; int16_t x,y,wheel,pan; };
struct zmk_hid_keyboard_report { struct zmk_hid_keyboard_report_body body; };
struct zmk_hid_consumer_report { struct zmk_hid_consumer_report_body body; };
struct zmk_hid_mouse_report { struct zmk_hid_mouse_report_body body; };
static struct zmk_hid_keyboard_report keyboard;
static struct zmk_hid_consumer_report consumer;
static struct zmk_hid_mouse_report mouse;
static unsigned modifier_counts[8];
static bool zmk_hid_mod_is_pressed(unsigned mod) { return modifier_counts[mod]>0; }
static int zmk_hid_unregister_mod(unsigned mod) {
    assert(modifier_counts[mod]);
    if (!--modifier_counts[mod]) keyboard.body.modifiers &= ~BIT(mod);
    return 0;
}
static void zmk_hid_implicit_modifiers_release(void) {}
static void zmk_hid_masked_modifiers_clear(void) {}
static void zmk_hid_keyboard_clear(void) { memset(&keyboard,0,sizeof(keyboard)); }
static void zmk_hid_consumer_clear(void) { memset(&consumer,0,sizeof(consumer)); }
static void zmk_hid_mouse_clear(void) { memset(&mouse,0,sizeof(mouse)); }
static void zmk_hid_mouse_button_release(unsigned button) { mouse.body.buttons &= ~BIT(button); }
static struct zmk_hid_mouse_report *zmk_hid_get_mouse_report(void) { return &mouse; }

struct bt_conn { int role, state; };
struct bt_conn_info { int role, state; };
static struct bt_conn connections[3]={{1,1},{0,1},{0,1}};
static int bt_conn_get_info(struct bt_conn *conn, struct bt_conn_info *info) {
    info->role=conn->role; info->state=conn->state; return 0;
}
static int disconnect_result;
static bool hold_pc_connection;
static int bt_conn_disconnect(struct bt_conn *conn, int reason) {
    if (disconnect_result) return disconnect_result;
    if (!hold_pc_connection) conn->state=0;
    return 0;
}
static void bt_conn_foreach(int type, void (*callback)(struct bt_conn *, void *), void *data) {
    for (unsigned i=0;i<3;i++) callback(&connections[i],data);
}
