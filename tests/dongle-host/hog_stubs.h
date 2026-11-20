#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define K_NO_WAIT 0
#define K_MSEC(x) (x)
#define CONFIG_ZMK_BLE_THREAD_STACK_SIZE 64
#define CONFIG_ZMK_BLE_KEYBOARD_REPORT_QUEUE_SIZE 4
#define CONFIG_ZMK_BLE_CONSUMER_REPORT_QUEUE_SIZE 4
#define CONFIG_ZMK_BLE_MOUSE_REPORT_QUEUE_SIZE 4
#define CONFIG_ZMK_POINTING 1
#define IS_ENABLED(x) (x)
#define BT_SECURITY_L2 2
#define BT_GATT_CCC_NOTIFY 1
#define LOG_DBG(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define K_THREAD_STACK_DEFINE(name, size) static char name[size]

struct k_work { void (*handler)(struct k_work *); bool queued; };
struct k_work_q { int unused; };
struct k_msgq { void *storage; size_t size; unsigned capacity, used, head, tail; };
#define K_MSGQ_DEFINE(name, size_, count_, align_) \
    static unsigned char name##_storage[(size_) * (count_)]; \
    struct k_msgq name = {.storage=name##_storage, .size=(size_), .capacity=(count_)}
#define K_WORK_DEFINE(name, fn) struct k_work name = {.handler=(fn)}
static struct k_work *scheduled[32];
static unsigned scheduled_count;
static int k_msgq_put(struct k_msgq *q, const void *data, int timeout) {
    if (q->used == q->capacity) return -ENOMSG;
    memcpy((char *)q->storage + q->tail * q->size, data, q->size);
    q->tail = (q->tail + 1) % q->capacity; q->used++; return 0;
}
static int k_msgq_get(struct k_msgq *q, void *data, int timeout) {
    if (!q->used) return -ENOMSG;
    memcpy(data, (char *)q->storage + q->head * q->size, q->size);
    q->head = (q->head + 1) % q->capacity; q->used--; return 0;
}
static int k_work_submit_to_queue(struct k_work_q *q, struct k_work *work) {
    if (!work->queued) { work->queued=true; scheduled[scheduled_count++]=work; }
    return 0;
}
static void flush_work(void) {
    while (scheduled_count) {
        struct k_work *work=scheduled[0];
        memmove(scheduled, scheduled+1, (--scheduled_count)*sizeof(*scheduled));
        work->queued=false; work->handler(work);
    }
}
static int k_work_queue_drain(struct k_work_q *q, bool plug) { flush_work(); return 0; }

enum zmk_transport { ZMK_TRANSPORT_USB, ZMK_TRANSPORT_BLE };
static bool output_blocked;
static enum zmk_transport selected=ZMK_TRANSPORT_BLE;
static uint32_t generation=1;
static bool dongle_host_transport_allowed(enum zmk_transport transport) {
    return !output_blocked && selected == transport;
}
static uint32_t dongle_host_generation(void) { return generation; }
struct zmk_hid_keyboard_report_body { uint8_t modifiers, reserved, keys[6]; };
struct zmk_hid_consumer_report_body { uint16_t keys[2]; };
struct zmk_hid_mouse_report_body { uint8_t buttons; int16_t x,y,wheel,pan; };
struct bt_conn { int host, refs, security; bool connected; };
static struct bt_conn home={0,1,2,true}, office={1,1,2,true};
static struct bt_conn *active=&home;
static struct bt_conn *bt_conn_ref(struct bt_conn *conn) { conn->refs++; return conn; }
static void bt_conn_unref(struct bt_conn *conn) { assert(conn->refs>1); conn->refs--; }
static struct bt_conn *zmk_ble_active_profile_conn(void) {
    return active && active->connected ? bt_conn_ref(active) : NULL;
}
static int bt_conn_get_security(struct bt_conn *conn) { return conn->security; }
static int bt_conn_set_security(struct bt_conn *conn, int security) { return 0; }
struct bt_gatt_attr { int id; };
static struct { struct bt_gatt_attr attrs[20]; } hog_svc;
struct bt_gatt_notify_params {
    const struct bt_gatt_attr *attr;
    const void *data;
    size_t len;
    void (*func)(struct bt_conn *, void *);
    void *user_data;
};
typedef int32_t atomic_t;
typedef int32_t atomic_val_t;
static void atomic_set(atomic_t *value, atomic_val_t next) { *value = next; }
static atomic_val_t atomic_get(atomic_t *value) { return *value; }
static bool atomic_cas(atomic_t *value, atomic_val_t expected, atomic_val_t next) {
    if (*value != expected) return false;
    *value = next;
    return true;
}
struct k_sem { unsigned count; };
#define K_SEM_DEFINE(name, initial, max) static struct k_sem name = {initial}
static int64_t now;
static int64_t k_uptime_get(void) { return now; }
static void k_sem_reset(struct k_sem *sem) { sem->count = 0; }
static void k_sem_give(struct k_sem *sem) { sem->count = 1; }
static struct bt_gatt_notify_params pending_tx, stale_tx;
static struct bt_conn *pending_conn;
static bool suppress_completion;
static unsigned completed_notifications;
static int k_sem_take(struct k_sem *sem, int timeout) {
    if (stale_tx.func) {
        stale_tx.func(pending_conn, stale_tx.user_data);
        stale_tx.func = NULL;
        assert(!sem->count);
    }
    if (pending_tx.func && !suppress_completion) {
        pending_tx.func(pending_conn, pending_tx.user_data);
        pending_tx.func = NULL;
        completed_notifications++;
    }
    if (sem->count) { sem->count = 0; return 0; }
    stale_tx = pending_tx;
    pending_tx.func = NULL;
    now += timeout;
    return -EAGAIN;
}
static bool bt_gatt_is_subscribed(struct bt_conn *conn, const struct bt_gatt_attr *attr, int value) {
    return conn->connected;
}
static struct { int host, report; uint8_t data[32]; size_t len; } sent[256];
static unsigned sent_count;
static int bt_gatt_notify_cb(struct bt_conn *conn, const struct bt_gatt_notify_params *params) {
    if (!conn->connected) return -ENOTCONN;
    unsigned i=sent_count++; assert(i<256);
    sent[i].host=conn->host; sent[i].report=params->attr->id; sent[i].len=params->len;
    memcpy(sent[i].data,params->data,params->len);
    if (params->func) {
        assert(!pending_tx.func);
        pending_tx = *params;
        pending_conn = conn;
    }
    return 0;
}
