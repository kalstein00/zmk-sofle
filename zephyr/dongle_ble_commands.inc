/* Included after the pinned BLE implementation so its helpers remain private. */
void dongle_ble_pause(void) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    dongle_ble_paused = true;
    dongle_pairing_cancel();
    update_advertising();
    k_mutex_unlock(&dongle_ble_mutex);
}

void dongle_ble_resume(void) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    dongle_ble_paused = false;
    update_advertising();
    k_mutex_unlock(&dongle_ble_mutex);
}

int dongle_ble_select_profile(uint8_t index) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    dongle_pairing_cancel();
    dongle_pairing_result = DONGLE_PC_EMPTY;
    int err = dongle_raw_prof_select(index);
    if (!err && zmk_ble_active_profile_is_open()) {
        dongle_pairing_slot = index;
        dongle_pairing_deadline = k_uptime_get() + 120000;
        k_work_reschedule(&dongle_pairing_work, K_SECONDS(120));
        dongle_log_note(DONGLE_LOG_HOST_PAIRING_OPEN, index, 120);
        LOG_INF("PC profile %u pairing window: 120 seconds", index);
    }
    k_mutex_unlock(&dongle_ble_mutex);
    return err;
}

/* Caller owns dongle_ble_mutex. */
static int clear_pc_bond(uint8_t profile) {
    if (zmk_ble_profile_is_open(profile)) return 0;
    int err = bt_unpair(BT_ID_DEFAULT, &profiles[profile].peer);
    if (!err) set_profile_address(profile, BT_ADDR_LE_ANY);
    return err;
}

int dongle_ble_clear_profiles(bool all) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    dongle_pairing_cancel();
    dongle_pairing_result = DONGLE_PC_EMPTY;
    int err = 0;
    for (unsigned i = 0; i < ZMK_BLE_PROFILE_COUNT; i++) {
        if (!all && i != active_profile) continue;
        err = clear_pc_bond(i);
        if (err) break;
    }
    /* Deletion does not open a new pairing window. Explicitly select the empty
     * slot to pair; neither split addresses nor keymap settings are touched. */
    k_mutex_unlock(&dongle_ble_mutex);
    return err;
}

int dongle_ble_apply_profile(uint8_t profile) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    int err = dongle_raw_prof_select(profile);
    k_mutex_unlock(&dongle_ble_mutex);
    return err;
}

struct dongle_pc_snapshot dongle_pc_snapshot(void) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    struct dongle_pc_snapshot snapshot = {.profile = active_profile};
    if (!zmk_ble_active_profile_is_open()) {
        snapshot.status = DONGLE_PC_PAIRED;
        if (zmk_ble_active_profile_is_connected()) {
            struct bt_conn *conn = zmk_ble_active_profile_conn();
            if (conn) {
                if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
                    snapshot.status = DONGLE_PC_CONNECTED;
                }
                bt_conn_unref(conn);
            }
        }
    } else if (dongle_pairing_result != DONGLE_PC_EMPTY) {
        snapshot.status = dongle_pairing_result;
    } else if (dongle_pairing_open()) {
        snapshot.status = DONGLE_PC_PAIRING;
    }
    k_mutex_unlock(&dongle_ble_mutex);
    return snapshot;
}

enum dongle_pc_status dongle_pc_profile_status(uint8_t profile) {
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    enum dongle_pc_status status =
        profile == active_profile ? dongle_pc_snapshot().status :
        zmk_ble_profile_is_open(profile) ? DONGLE_PC_EMPTY : DONGLE_PC_PAIRED;
    k_mutex_unlock(&dongle_ble_mutex);
    return status;
}

int dongle_ble_clear_slot(uint8_t profile) {
    if (profile >= ZMK_BLE_PROFILE_COUNT) return -ERANGE;
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    int err = profile == active_profile ? dongle_ble_clear_profiles(false) : clear_pc_bond(profile);
    k_mutex_unlock(&dongle_ble_mutex);
    return err;
}
