#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define ZMK_BLE_PROFILE_COUNT 4
#define BT_ID_DEFAULT 0
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN 0x13
#define BT_CONN_ROLE_PERIPHERAL 1
#define K_FOREVER (-1)
#define K_MSEC(ms) (ms)
#define K_SECONDS(s) ((s) * 1000)
#define LOG_INF(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define K_MUTEX_DEFINE(name) static int name
static void k_mutex_lock(int *mutex, int timeout) {}
static void k_mutex_unlock(int *mutex) {}
struct k_work { int unused; };
struct k_work_delayable { int64_t deadline; void (*handler)(struct k_work *); };
#define K_WORK_DELAYABLE_DEFINE(name, fn) static struct k_work_delayable name = {.handler = fn}
static int64_t now;
static int64_t k_uptime_get(void) { return now; }
static void k_work_reschedule(struct k_work_delayable *work, int64_t delay) {
    work->deadline = now + delay;
}
static void k_work_cancel_delayable(struct k_work_delayable *work) { work->deadline = 0; }

typedef unsigned bt_addr_le_t;
static const bt_addr_le_t empty;
#define BT_ADDR_LE_ANY (&empty)
struct bt_conn { bt_addr_le_t addr; uint8_t role; bool connected; unsigned refs; };
struct bt_conn_info { uint8_t role; };
enum bt_security_err { BT_SECURITY_ERR_UNSPECIFIED, BT_SECURITY_ERR_PAIR_NOT_ALLOWED };
static unsigned unpaired;
static unsigned accepted_connections;
static void dongle_host_connected(void) { accepted_connections++; }
static const bt_addr_le_t *bt_conn_get_dst(struct bt_conn *conn) { return &conn->addr; }
static int bt_conn_get_info(struct bt_conn *conn, struct bt_conn_info *info) {
    info->role = conn->role;
    return 0;
}
static int bt_addr_le_cmp(const bt_addr_le_t *a, const bt_addr_le_t *b) { return *a != *b; }
static struct bt_conn *bt_conn_ref(struct bt_conn *conn) { conn->refs++; return conn; }
static void bt_conn_unref(struct bt_conn *conn) { assert(conn->refs > 1); conn->refs--; }
static void bt_conn_auth_cancel(struct bt_conn *conn) {}
static void bt_conn_disconnect(struct bt_conn *conn, int reason) { conn->connected = false; }
static int bt_unpair(int id, const bt_addr_le_t *addr) {
    assert(*addr > 0 && *addr <= 3); /* Addresses 10 and 11 are split bonds. */
    unpaired |= 1u << *addr;
    return 0;
}
static struct { bt_addr_le_t peer; } profiles[ZMK_BLE_PROFILE_COUNT] = {{1}};
static uint8_t active_profile;
static enum advertising_type { ZMK_ADV_NONE } advertising_status;
static bool zmk_ble_profile_is_open(uint8_t index) { return !profiles[index].peer; }
static bool zmk_ble_active_profile_is_open(void) { return zmk_ble_profile_is_open(active_profile); }
static bt_addr_le_t *zmk_ble_active_profile_addr(void) { return &profiles[active_profile].peer; }
static int zmk_ble_profile_index(const bt_addr_le_t *addr) {
    for (int i = 0; i < ZMK_BLE_PROFILE_COUNT; i++) {
        if (profiles[i].peer == *addr) return i;
    }
    return -ENODEV;
}
static void set_profile_address(uint8_t index, const bt_addr_le_t *addr) {
    profiles[index].peer = *addr;
}
static struct bt_conn *active_connection;
#define BT_SECURITY_L2 2
static bool zmk_ble_active_profile_is_connected(void) {
    return active_connection && active_connection->connected;
}
static struct bt_conn *zmk_ble_active_profile_conn(void) {
    return bt_conn_ref(active_connection);
}
static int bt_conn_get_security(struct bt_conn *conn) { return BT_SECURITY_L2; }

#include "dongle_ble_policy.inc"
static void dongle_raw_pairing_complete(struct bt_conn *conn, bool bonded) {
    set_profile_address(active_profile, bt_conn_get_dst(conn));
}
#include "pairing.generated.c"
#include "dongle_ble_commands.inc"

static int dongle_raw_update_advertising(void) { return 0; }
static int dongle_raw_prof_select(uint8_t index) { active_profile = index; return 0; }

int main(void) {
    struct bt_conn home = {1, BT_CONN_ROLE_PERIPHERAL, true, 1};
    struct bt_conn office = {2, BT_CONN_ROLE_PERIPHERAL, true, 1};
    struct bt_conn split = {10, 0, true, 1};
    assert(dongle_host_accept_connection(&home));
    assert(!dongle_host_accept_connection(&office));
    dongle_ble_pause();
    assert(!dongle_ble_select_profile(1));
    dongle_ble_resume();
    assert(dongle_pairing_open());
    assert(dongle_pc_snapshot().status == DONGLE_PC_PAIRING);
    assert(!dongle_host_accept_connection(&home)); /* Already registered in slot 0. */
    office.connected = true;
    assert(dongle_host_accept_connection(&office));
    assert(dongle_host_pairing_allowed(&office));
    auth_pairing_complete(&split, true);
    assert(zmk_ble_active_profile_is_open() && !unpaired);
    auth_pairing_complete(&office, true);
    assert(profiles[1].peer == 2 && !dongle_pairing_open() && office.refs == 1);
    assert(dongle_pc_snapshot().status == DONGLE_PC_PAIRED);
    active_connection = &office;
    assert(dongle_pc_snapshot().status == DONGLE_PC_CONNECTED && office.refs == 1);
    active_connection = NULL;

    struct bt_conn spare = {3, BT_CONN_ROLE_PERIPHERAL, true, 1};
    assert(!dongle_ble_select_profile(2));
    assert(dongle_host_accept_connection(&spare));
    now = 120000;
    dongle_pairing_work.handler(NULL);
    assert(!dongle_pairing_open() && !spare.connected && spare.refs == 1);
    assert(dongle_pc_snapshot().status == DONGLE_PC_TIMEOUT);
    auth_pairing_complete(&spare, true); /* Completion from the expired window. */
    assert(zmk_ble_active_profile_is_open() && unpaired == (1u << 3));
    assert(!dongle_ble_select_profile(2) && dongle_pairing_open());
    spare.connected = true;
    assert(dongle_host_accept_connection(&spare));
    dongle_ble_pause();
    assert(!spare.connected && !dongle_host_pairing_allowed(&spare));
    assert(!dongle_ble_select_profile(1));
    dongle_ble_resume();
    assert(!dongle_ble_clear_profiles(false));
    assert(profiles[0].peer == 1 && zmk_ble_profile_is_open(1));
    assert(!dongle_ble_clear_profiles(true));
    assert(zmk_ble_profile_is_open(0) && zmk_ble_profile_is_open(1));
    assert(!dongle_ble_apply_profile(0));
    assert(active_profile == 0 && split.connected && unpaired == ((1u << 1) | (1u << 2) | (1u << 3)));
    puts("PASS: pairing isolates registered hosts and split roles; timeout, retry, cancellation and PC bond deletion");
}
