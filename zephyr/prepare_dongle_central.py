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
replace('#include <zmk/physical_layouts.h>', '#include <zmk/physical_layouts.h>\n#include "dongle_battery_snapshot.h"\n#include "dongle_sleep.h"')
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
replace('    confirm_peripheral_slot_conn(conn);\n    split_central_process_connection(conn);', '    confirm_peripheral_slot_conn(conn);\n    dongle_battery_connect(peripheral_slot_index_for_conn(conn), conn);\n    dongle_sleep_connection(peripheral_slot_index_for_conn(conn), conn, true);\n    split_central_process_connection(conn);')
replace('    err = release_peripheral_slot_for_conn(conn);', '    dongle_battery_disconnect(peripheral_slot_index_for_conn(conn), conn);\n    dongle_sleep_connection(peripheral_slot_index_for_conn(conn), conn, false);\n    err = release_peripheral_slot_for_conn(conn);')
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
