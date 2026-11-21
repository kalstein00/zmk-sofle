"""Apply #5 snapshot hooks to the pinned ZMK central without editing west sources."""
import hashlib
import pathlib
import sys
src, out = map(pathlib.Path, sys.argv[1:])
s = src.read_text(encoding="utf-8")
if hashlib.sha256(s.encode()).hexdigest() != "535fa1e0c7e5795a8fb271df8cf79a59f68916e8c247d241fef97bc22f4fbd2a":
    raise SystemExit("Unexpected ZMK central source; review the snapshot hooks")
def replace(old, new, count=1):
    global s
    if s.count(old) != count:
        raise SystemExit(f"Unexpected ZMK central hook count for {old[:100]!r}: {s.count(old)}")
    s = s.replace(old, new)
replace('#include <zmk/physical_layouts.h>', '#include <zmk/physical_layouts.h>\n#include "dongle_battery_snapshot.h"\n#include "dongle_sleep.h"\n#include "dongle_log.h"')
replace('static bool split_central_eir_found(const bt_addr_le_t *addr) {',
        '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
#include "dongle_scan_diagnostics.inc"
#endif

static bool split_central_eir_found(const bt_addr_le_t *addr) {''')
replace('    if (slot_idx < 0) {\n        LOG_INF("Unable to reserve peripheral slot',
        '''    if (slot_idx < 0) {
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        dongle_scan_mark(addr, SCAN_RESERVATION_REJECTED);
#endif
        LOG_INF("Unable to reserve peripheral slot''')
replace("    /* We're only interested in connectable events */",
        '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    dongle_scan_report(addr, type);
#endif
    /* We're only interested in connectable events */''')
replace('static int start_scanning(void) {', '''static void dongle_scan_retry(struct k_work *work) {
    ARG_UNUSED(work);
    start_scanning();
}

K_WORK_DELAYABLE_DEFINE(dongle_scan_retry_work, dongle_scan_retry);

static int start_scanning(void) {''')
replace('''    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);
    if (err < 0) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }''', '''    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);
    if (err < 0) {
        is_scanning = false;
        dongle_log_note(DONGLE_LOG_SCAN_FAILED, -1, err);
        LOG_ERR("Scanning failed to start (err %d)", err);
        k_work_reschedule(&dongle_scan_retry_work, K_MSEC(500));
        return err;
    }
    dongle_log_note(DONGLE_LOG_SCAN_STARTED, -1, 0);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    k_work_reschedule(&dongle_scan_sample_work, K_SECONDS(8));
#endif''')
replace('    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);',
        '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    dongle_scan_reset();
#endif
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, split_central_device_found);''')
replace('''    int err = stop_scanning();
    if (err < 0) {
        return false;
    }

    LOG_DBG("Initiating new connection");''', '''    int err = stop_scanning();
    if (err < 0) {
        dongle_log_note(DONGLE_LOG_SCAN_FAILED, slot_idx, err);
        return false;
    }

    dongle_log_note(DONGLE_LOG_SPLIT_CONNECTING, slot_idx, 0);
    LOG_DBG("Initiating new connection");''')
replace('''    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &slot->conn);
    if (err < 0) {''', '''    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &slot->conn);
    if (err < 0) {
        dongle_log_note(DONGLE_LOG_SPLIT_CONNECT_FAILED, slot_idx, err);''')
replace('    if (conn_err) {\n        LOG_ERR("Failed to connect', '    if (conn_err) {\n        dongle_log_note(DONGLE_LOG_SPLIT_CONNECT_FAILED, peripheral_slot_index_for_conn(conn), conn_err);\n        LOG_ERR("Failed to connect')
replace('    struct bt_gatt_discover_params discover_params;', '''    struct bt_gatt_discover_params discover_params;
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    bool key_discovery_in_progress;
#endif''')
replace('    slot->subscribe_params.value_handle = 0;', '''    slot->subscribe_params.value_handle = 0;
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    slot->key_discovery_in_progress = false;
#endif''')
replace('static uint8_t split_central_notify_func(', '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
static void dongle_key_subscription_failed(struct bt_conn *conn, struct peripheral_slot *slot);
#endif

static uint8_t split_central_notify_func(''')
replace('''        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {''', '''        params->value_handle = 0U;
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        params->ccc_handle = 0U;
        dongle_key_subscription_failed(conn, slot);
#endif
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[NOTIFICATION] data %p length %u", data, length);

    for (int i = 0; i < POSITION_STATE_DATA_LEN; i++) {''')
replace('static uint8_t split_central_chrc_discovery_func(', '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
#define DONGLE_KEY_DISCOVERY_RETRIES 2

static uint8_t split_central_service_discovery_func(struct bt_conn *conn,
                                                   const struct bt_gatt_attr *attr,
                                                   struct bt_gatt_discover_params *params);

struct dongle_key_discovery_retry {
    struct k_work_delayable work;
    struct bt_conn *conn;
    struct bt_gatt_discover_params params;
    int slot;
    uint8_t attempts;
};

static struct dongle_key_discovery_retry key_discovery_work_by_conn[CONFIG_BT_MAX_CONN];
static struct k_work_q key_discovery_queue;
K_THREAD_STACK_DEFINE(key_discovery_stack, 2048);

static void dongle_retry_key_discovery(struct bt_conn *conn, struct peripheral_slot *slot);

static void dongle_key_discovery_retry_work(struct k_work *work) {
    struct dongle_key_discovery_retry *retry =
        CONTAINER_OF(k_work_delayable_from_work(work), struct dongle_key_discovery_retry, work);
    struct bt_conn *conn = retry->conn;
    retry->conn = NULL;
    int index = retry->slot;
    int err = bt_gatt_discover(conn, &retry->params);
    if (err && err != -ENOTCONN) {
        dongle_log_note(DONGLE_LOG_DISCOVERY_FAILED, index, err);
    }
    if (err == -ENOMEM) {
        struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
        if (slot) {
            dongle_retry_key_discovery(conn, slot);
        }
    }
    bt_conn_unref(conn);
}

static void dongle_retry_key_discovery(struct bt_conn *conn, struct peripheral_slot *slot) {
    struct bt_conn_info info;
    if (slot->key_discovery_in_progress || bt_conn_get_info(conn, &info) ||
        info.state != BT_CONN_STATE_CONNECTED) {
        return;
    }
    struct dongle_key_discovery_retry *retry = &key_discovery_work_by_conn[bt_conn_index(conn)];
    if (retry->conn) {
        return;
    }
    int index = peripheral_slot_index_for_conn(conn);
    if (index < 0) {
        return;
    }
    dongle_log_note(DONGLE_LOG_KEY_CHARACTERISTIC_MISSING, index, retry->attempts);
    if (retry->attempts >= DONGLE_KEY_DISCOVERY_RETRIES) {
        return;
    }

    // Use a dedicated queue: ATT allocation may block while split input is delivered.
    // The reference pins the connection index even if this peripheral disconnects.
    retry->attempts++;
    retry->conn = bt_conn_ref(conn);
    retry->slot = index;
    retry->params = (struct bt_gatt_discover_params){
        .uuid = &split_service_uuid.uuid,
        .func = split_central_service_discovery_func,
        .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
        .type = BT_GATT_DISCOVER_PRIMARY,
    };
    int err = k_work_schedule_for_queue(&key_discovery_queue, &retry->work, K_MSEC(150));
    if (err < 0) {
        bt_conn_unref(retry->conn);
        retry->conn = NULL;
        dongle_log_note(DONGLE_LOG_DISCOVERY_FAILED, index, err);
    }
}

static void dongle_key_subscription_failed(struct bt_conn *conn, struct peripheral_slot *slot) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) || info.state != BT_CONN_STATE_CONNECTED) {
        return;
    }
    if (slot->key_discovery_in_progress) {
        // CCC can fail while characteristic discovery still owns its parameters.
        // Zephyr completes a cancelled discovery through the same callback.
        bt_gatt_cancel(conn, &slot->discover_params);
        slot->key_discovery_in_progress = false;
    }
    dongle_retry_key_discovery(conn, slot);
}

static void dongle_key_ccc_result(struct bt_conn *conn, uint8_t err,
                                  struct bt_gatt_subscribe_params *params) {
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (slot) {
        dongle_log_note(DONGLE_LOG_KEY_CCC_RESULT, peripheral_slot_index_for_conn(conn), err);
        if (err) {
            params->value_handle = 0;
            params->ccc_handle = 0;
            dongle_key_subscription_failed(conn, slot);
        }
    }
}
#endif

static uint8_t split_central_chrc_discovery_func(''')
replace('            split_central_subscribe(conn, &slot->subscribe_params);',
        '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
            slot->subscribe_params.subscribe = dongle_key_ccc_result;
            int err = split_central_subscribe(conn, &slot->subscribe_params);
            dongle_log_note(DONGLE_LOG_KEY_SUBSCRIBE_RESULT, peripheral_slot_index_for_conn(conn), err);
            if (err && err != -EALREADY) {
                slot->subscribe_params.value_handle = 0;
                slot->subscribe_params.ccc_handle = 0;
                slot->key_discovery_in_progress = false;
                dongle_retry_key_discovery(conn, slot);
                return BT_GATT_ITER_STOP;
            }
#else
            split_central_subscribe(conn, &slot->subscribe_params);
#endif''')
replace('''    if (!attr) {
        LOG_DBG("Discover complete");
        return BT_GATT_ITER_STOP;
    }''', '''    if (!attr) {
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
        if (slot) {
            slot->key_discovery_in_progress = false;
            if (!slot->subscribe_params.value_handle) {
                dongle_retry_key_discovery(conn, slot);
            }
        }
#endif
        LOG_DBG("Discover complete");
        return BT_GATT_ITER_STOP;
    }''')
replace('    if (bt_uuid_cmp(slot->discover_params.uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)) !=',
        '    if (bt_uuid_cmp(params->uuid, BT_UUID_DECLARE_128(ZMK_SPLIT_BT_SERVICE_UUID)) !=')
replace('    return subscribed ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;', '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    if (subscribed) {
        slot->key_discovery_in_progress = false;
    }
#endif
    return subscribed ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;''')
replace('    LOG_DBG("Found split service");', '''    LOG_DBG("Found split service");
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    slot->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    slot->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    slot->key_discovery_in_progress = true;
#endif''')
replace('static int zmk_split_bt_central_init(void) {', '''static int zmk_split_bt_central_init(void) {
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    k_work_queue_start(&key_discovery_queue, key_discovery_stack,
                       K_THREAD_STACK_SIZEOF(key_discovery_stack), 6, NULL);
    for (size_t i = 0; i < ARRAY_SIZE(key_discovery_work_by_conn); i++) {
        k_work_init_delayable(&key_discovery_work_by_conn[i].work, dongle_key_discovery_retry_work);
    }
#endif''')
replace('''    if (!attr) {
        LOG_DBG("Discover complete");
        (void)memset(params, 0, sizeof(*params));''', '''    if (!attr) {
        dongle_log_note(DONGLE_LOG_SPLIT_SERVICE_MISSING, peripheral_slot_index_for_conn(conn), 0);
        LOG_DBG("Discover complete");
        (void)memset(params, 0, sizeof(*params));''')
replace('''    if (!slot || !slot->selected_physical_layout_handle) {''', '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    if (slot && err) {
        dongle_log_note(DONGLE_LOG_SPLIT_SECURITY_FAILED, peripheral_slot_index_for_conn(conn), err);
    }
#endif
    if (!slot || !slot->selected_physical_layout_handle) {''')
replace('''            LOG_ERR("Discover failed(err %d)", err);
            return;''', '''            dongle_log_note(DONGLE_LOG_DISCOVERY_FAILED, peripheral_slot_index_for_conn(conn), err);
            LOG_ERR("Discover failed(err %d)", err);
            return;''')
replace('''        LOG_ERR("Failed to start discovering split service characteristics (err %d)", err);''', '''        dongle_log_note(DONGLE_LOG_DISCOVERY_FAILED, peripheral_slot_index_for_conn(conn), err);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        slot->key_discovery_in_progress = false;
        if (err == -ENOMEM && !slot->subscribe_params.value_handle) {
            dongle_retry_key_discovery(conn, slot);
        }
#endif
        LOG_ERR("Failed to start discovering split service characteristics (err %d)", err);''')
replace('    uint8_t battery_level = ((uint8_t *)data)[0];', '    uint8_t battery_level = ((uint8_t *)data)[0];\n    dongle_battery_sample(peripheral_slot_index_for_conn(conn), conn, battery_level);', 2)
replace('''    struct peripheral_event_wrapper event_wrapper = {
        .source = peripheral_slot_index_for_conn(conn),
        .event = {.type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_SENSOR_EVENT,''', '''    dongle_sleep_input_received(peripheral_slot_index_for_conn(conn), conn);
    struct peripheral_event_wrapper event_wrapper = {
        .source = peripheral_slot_index_for_conn(conn),
        .event = {.type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_SENSOR_EVENT,''')
replace('''                bool pressed = slot->position_state[i] & BIT(j);
                struct peripheral_event_wrapper ev = {''', '''                bool pressed = slot->position_state[i] & BIT(j);
                if (pressed) dongle_sleep_input_received(peripheral_slot_index_for_conn(conn), conn);
                struct peripheral_event_wrapper ev = {''')
replace('''        zmk_split_transport_central_peripheral_event_handler(&bt_central, ev.source, ev.event);
''', '''        zmk_split_transport_central_peripheral_event_handler(&bt_central, ev.source, ev.event);
        if (ev.source < ZMK_SPLIT_BLE_PERIPHERAL_COUNT &&
            ev.event.type == ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_KEY_POSITION_EVENT &&
            ev.event.data.key_position_event.pressed) {
            dongle_battery_position(ev.source, peripherals[ev.source].conn,
                                    ev.event.data.key_position_event.position);
        }
''')
replace('    confirm_peripheral_slot_conn(conn);\n    split_central_process_connection(conn);', '    confirm_peripheral_slot_conn(conn);\n    dongle_log_note(DONGLE_LOG_SPLIT_CONNECTED, peripheral_slot_index_for_conn(conn), 0);\n    dongle_battery_connect(peripheral_slot_index_for_conn(conn), conn);\n    dongle_sleep_connection(peripheral_slot_index_for_conn(conn), conn, true);\n    split_central_process_connection(conn);')
replace('    confirm_peripheral_slot_conn(conn);', '''    confirm_peripheral_slot_conn(conn);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    key_discovery_work_by_conn[bt_conn_index(conn)].attempts = 0;
#endif''')
replace('    err = release_peripheral_slot_for_conn(conn);', '''#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    int log_slot = peripheral_slot_index_for_conn(conn);
    if (log_slot >= 0) dongle_log_note(DONGLE_LOG_SPLIT_DISCONNECTED, log_slot, reason);
#endif
    dongle_battery_disconnect(peripheral_slot_index_for_conn(conn), conn);
    dongle_sleep_connection(peripheral_slot_index_for_conn(conn), conn, false);
    err = release_peripheral_slot_for_conn(conn);''')
replace('void split_central_split_run_callback(struct k_work *work) {', '''static bool dongle_sleep_command(struct central_cmd_wrapper *p) {
    return p->cmd.type == ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_INVOKE_BEHAVIOR &&
           strcmp(p->cmd.data.invoke_behavior.behavior_dev, "z_so_off") == 0;
}
static void dongle_sleep_command_result(struct central_cmd_wrapper *p, int result) {
    if (dongle_sleep_command(p))
        dongle_sleep_send_result(p->source, p->cmd.data.invoke_behavior.param1, result);
}
void split_central_split_run_callback(struct k_work *work) {''')
replace('''        if (peripherals[payload_wrapper.source].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            LOG_ERR("Source not connected");
            continue;
        }''', '''        if (peripherals[payload_wrapper.source].state != PERIPHERAL_SLOT_STATE_CONNECTED) {
            LOG_ERR("Source not connected");
            dongle_sleep_command_result(&payload_wrapper, -ENOTCONN);
            continue;
        }''')
replace('''                LOG_ERR("Run behavior handle not found");
                continue;''', '''                LOG_ERR("Run behavior handle not found");
                dongle_sleep_command_result(&payload_wrapper, -ENODEV);
                continue;''')
replace('''            if (err) {
                LOG_ERR("Failed to write the behavior characteristic (err %d)", err);''', '''            dongle_sleep_command_result(&payload_wrapper, err);
            if (err) {
                LOG_ERR("Failed to write the behavior characteristic (err %d)", err);''')
replace('''            int err = bt_gatt_write_without_response(
                peripherals[payload_wrapper.source].conn,
                peripherals[payload_wrapper.source].run_behavior_handle, &payload,
                sizeof(struct zmk_split_run_behavior_payload), true);''', '''            struct bt_conn *sleep_conn = dongle_sleep_command(&payload_wrapper) ?
                dongle_sleep_target_ref(payload_wrapper.source, payload_wrapper.cmd.data.invoke_behavior.param1) : NULL;
            if (dongle_sleep_command(&payload_wrapper) && !sleep_conn) {
                dongle_sleep_command_result(&payload_wrapper, -ENOTCONN);
                continue;
            }
            int err = bt_gatt_write_without_response(
                sleep_conn ? sleep_conn : peripherals[payload_wrapper.source].conn,
                peripherals[payload_wrapper.source].run_behavior_handle, &payload,
                sizeof(struct zmk_split_run_behavior_payload), true);
            if (sleep_conn) bt_conn_unref(sleep_conn);''')
replace('''            k_msgq_get(&zmk_split_central_split_run_msgq, &discarded_report, K_NO_WAIT);
            return split_bt_invoke_behavior_payload(payload_wrapper);''', '''            if (k_msgq_get(&zmk_split_central_split_run_msgq, &discarded_report, K_NO_WAIT) == 0)
                dongle_sleep_command_result(&discarded_report, -ENOBUFS);
            return split_bt_invoke_behavior_payload(payload_wrapper);''')
out.write_text(s, encoding="utf-8", newline="\n")
