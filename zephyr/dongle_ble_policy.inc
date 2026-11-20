/* Included by generated, hash-checked ble.c; owns only PC-host pairing. */
#include "dongle_status.h"
#include "dongle_log.h"
K_MUTEX_DEFINE(dongle_ble_mutex);
static enum dongle_pc_status dongle_pairing_result;
static bool dongle_ble_paused;
static uint8_t dongle_pairing_slot;
static int64_t dongle_pairing_deadline;
static struct bt_conn *dongle_pairing_conn;
static int dongle_raw_update_advertising(void);
static int dongle_raw_prof_select(uint8_t index);
static void dongle_pairing_timeout(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(dongle_pairing_work, dongle_pairing_timeout);

static bool dongle_pairing_open(void) {
    return !dongle_ble_paused && active_profile == dongle_pairing_slot &&
           dongle_pairing_deadline > k_uptime_get();
}

int update_advertising(void) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    enum advertising_type previous = advertising_status;
    int err = dongle_raw_update_advertising();
    if (err) {
        dongle_log_note(DONGLE_LOG_HOST_ADV_FAILED, active_profile, err);
    } else if (previous != advertising_status) {
        dongle_log_note(DONGLE_LOG_HOST_ADV_MODE, active_profile, advertising_status);
    }
    k_mutex_unlock(&dongle_ble_mutex);
    return err;
}

static void dongle_pairing_cancel(void) {
    dongle_pairing_deadline = 0;
    k_work_cancel_delayable(&dongle_pairing_work);
    if (dongle_pairing_conn) {
        bt_conn_auth_cancel(dongle_pairing_conn);
        bt_conn_disconnect(dongle_pairing_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(dongle_pairing_conn);
        dongle_pairing_conn = NULL;
    }
}

static void dongle_pairing_timeout(struct k_work *work) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    if (!dongle_pairing_deadline) {
        k_mutex_unlock(&dongle_ble_mutex);
        return;
    }
    /* A queued old timeout must not expire a newly opened window. */
    int64_t remaining = dongle_pairing_deadline - k_uptime_get();
    if (remaining > 0) {
        k_work_reschedule(&dongle_pairing_work, K_MSEC(remaining));
    } else {
        dongle_pairing_result = DONGLE_PC_TIMEOUT;
        dongle_log_note(DONGLE_LOG_HOST_PAIRING_TIMEOUT, dongle_pairing_slot, 0);
        dongle_pairing_cancel();
        update_advertising();
        LOG_INF("PC pairing window expired; select the empty profile to retry");
    }
    k_mutex_unlock(&dongle_ble_mutex);
}

static bool dongle_host_accept_connection(struct bt_conn *conn) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    bool accepted;
    if (!zmk_ble_active_profile_is_open()) {
        accepted = !dongle_ble_paused &&
                   !bt_addr_le_cmp(zmk_ble_active_profile_addr(), bt_conn_get_dst(conn));
    } else {
        accepted = dongle_pairing_open() &&
                   zmk_ble_profile_index(bt_conn_get_dst(conn)) < 0 &&
                   (!dongle_pairing_conn || dongle_pairing_conn == conn);
        if (accepted && !dongle_pairing_conn) dongle_pairing_conn = bt_conn_ref(conn);
    }
    dongle_log_note(accepted ? DONGLE_LOG_HOST_LINK_ACCEPTED : DONGLE_LOG_HOST_LINK_REJECTED,
                    active_profile, dongle_ble_paused ? 2 : zmk_ble_active_profile_is_open());
    k_mutex_unlock(&dongle_ble_mutex);
    if (!accepted) bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    else dongle_host_connected();
    return accepted;
}

static bool dongle_host_pairing_allowed(struct bt_conn *conn) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    bool allowed = dongle_pairing_open() && zmk_ble_active_profile_is_open() &&
                   dongle_pairing_conn == conn;
    k_mutex_unlock(&dongle_ble_mutex);
    return allowed;
}

static void dongle_host_pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    if (dongle_pairing_conn == conn) {
        dongle_log_note(DONGLE_LOG_HOST_PAIRING_RESULT, dongle_pairing_slot, reason);
        dongle_pairing_result = DONGLE_PC_FAILED;
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(dongle_pairing_conn);
        dongle_pairing_conn = NULL;
        LOG_WRN("PC pairing failed: %d", reason);
        /* Continue the current 120-second window for a retry. */
        update_advertising();
    }
    k_mutex_unlock(&dongle_ble_mutex);
}
