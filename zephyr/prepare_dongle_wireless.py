"""Adapt only the pinned ZMK sources for the optional wireless dongle build."""
import hashlib
import pathlib
import sys

# Hash normalized text, as in prepare_dongle_central.py (works with CRLF clones).
HASHES = {
    'battery': '152753e9646ba980f33f79184f4c7b08b79716ff3b2da3a61f4c9f78d23a1fc4',
    'studio_rpc': 'ff12de4ec2805f7d4564fe66c253f453c4f3ae6d70a3fc3c8e24374626b19b5d',
    'ble': '9c85d07c07f5e9f3b482527bdd78d6228f3f5b37961fbab308f5195d2b20178e',
    'endpoints': 'f2af0b3649d47edb62f3c2654273aa85a74786590ad5ebdb44637b5ce9ea181a',
    'hog': '2e8c12bb4197173d238a6be1e33abf2db5dedbd4a6cf29a83d5e2f6c79d4919b',
    'usb_hid': '3c7a934a2b5eac808889cd14cbc2d8b40e1dcc35a4ce0fbf6e6ac3a12b51ac8a',
    'behavior_queue': 'b5201256f674a83f92705630919fbb0237765b93a01b147c4b34bde816f8a05c',
    'split_central': '7181393a973e4dc7ba628866aec9a6dd6b87336d7b8ec0bbc2cd0138d755cff8',
    'sticky_key': 'b475c76f8a4df5d527291d4424718a29d6e6e11b95abcce62f241a7459079d73',
    'two_axis': '59cc6a23e390c9fe48d647dd4553cdb54adbcb5ff83533737b8bd73736eb51f2',
}


def replace(text, old, new, count=1):
    if text.count(old) != count:
        raise SystemExit(f"Wireless hook mismatch: {old[:100]!r}: {text.count(old)}")
    return text.replace(old, new)


def ble(text):
    text = replace(text, '#include <zmk/ble.h>', '#include <zmk/ble.h>\n#include "dongle_host.h"')
    text = replace(text, 'static bool directed_advertising_enabled = false;',
                   'static bool directed_advertising_enabled = false;\n#include "dongle_ble_policy.inc"')
    text = replace(text, 'int update_advertising(void) {', 'static int dongle_raw_update_advertising(void) {')
    text = replace(text, '        desired_adv = ZMK_ADV_CONN;\n    } else if',
                   '        desired_adv = dongle_pairing_open() && !dongle_pairing_conn ? ZMK_ADV_CONN : ZMK_ADV_NONE;\n    } else if')
    text = replace(text, '    LOG_DBG("advertising from %d to %d", advertising_status, desired_adv);',
                   '    if (dongle_ble_paused) desired_adv = ZMK_ADV_NONE;\n    LOG_DBG("advertising from %d to %d", advertising_status, desired_adv);')
    # The controller owns public commands; no internal call may bypass it.
    start = text.index('static void clear_profile_bond(')
    end = text.index('int zmk_ble_active_profile_index(', start)
    text = text[:start] + text[end:]
    start = text.index('int zmk_ble_prof_next(')
    end = text.index('int zmk_ble_prof_disconnect(', start)
    text = text[:start] + text[end:]
    text = replace(text, 'int zmk_ble_prof_select(uint8_t index) {', 'static int dongle_raw_prof_select(uint8_t index) {')
    start = text.index('#if IS_ENABLED(CONFIG_SETTINGS)\nstatic void ble_save_profile_work(')
    end = text.index('static int dongle_raw_prof_select(', start)
    text = text[:start] + text[end:]
    text = replace(text, '    ble_save_profile();\n', '')
    text = replace(text, '    k_work_init_delayable(&ble_save_work, ble_save_profile_work);\n', '')
    text = replace(text, '    zmk_ble_ready(0);', '    dongle_host_restore_selection(&active_profile);\n    dongle_log_note(DONGLE_LOG_HOST_BOOT_BOND, active_profile, !zmk_ble_active_profile_is_open());\n    zmk_ble_ready(0);')
    text = replace(text, '        LOG_WRN("Failed to connect to %s (%u)", addr, err);',
                   '        dongle_log_note(DONGLE_LOG_HOST_CONNECT_FAILED, active_profile, err);\n'
                   '        LOG_WRN("Failed to connect to %s (%u)", addr, err);')
    text = replace(text, '    LOG_DBG("Connected %s", addr);',
                   '    if (!dongle_host_accept_connection(conn)) { update_advertising(); return; }\n    LOG_DBG("Connected %s", addr);')
    text = replace(text, '    // We need to do this in a work callback,',
                   '    dongle_log_note(DONGLE_LOG_HOST_DISCONNECTED, zmk_ble_profile_index(bt_conn_get_dst(conn)), reason);\n'
                   '    dongle_host_pairing_failed(conn, BT_SECURITY_ERR_UNSPECIFIED);\n\n    // We need to do this in a work callback,')
    text = replace(text, '    if (!err) {\n        LOG_DBG("Security changed:', '''    struct bt_conn_info info;
    if (!bt_conn_get_info(conn, &info) && info.role == BT_CONN_ROLE_PERIPHERAL) {
        int profile = zmk_ble_profile_index(bt_conn_get_dst(conn));
        dongle_log_note(err ? DONGLE_LOG_HOST_SECURITY_FAILED : DONGLE_LOG_HOST_SECURITY_LEVEL,
                        profile, err ? err : level);
        if (!err && level >= BT_SECURITY_L2) dongle_host_connected();
    }
    if (!err) {
        LOG_DBG("Security changed:''')
    start = text.index('    return zmk_ble_active_profile_is_open() ||')
    end = text.index('\n}', start)
    text = text[:start] + '    return dongle_host_pairing_allowed(conn);' + text[end:]
    text = replace(text, 'static void auth_pairing_complete(struct bt_conn *conn, bool bonded) {',
                   'static void dongle_raw_pairing_complete(struct bt_conn *conn, bool bonded) {')
    marker = 'static struct bt_conn_auth_cb zmk_ble_auth_cb_display = {'
    wrapper = '''static void auth_pairing_complete(struct bt_conn *conn, bool bonded) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) || info.role != BT_CONN_ROLE_PERIPHERAL) return;
    k_mutex_lock(&dongle_ble_mutex, K_FOREVER);
    if (bonded && dongle_host_pairing_allowed(conn)) {
        dongle_raw_pairing_complete(conn, bonded);
        dongle_log_note(DONGLE_LOG_HOST_PAIRING_RESULT, active_profile, 0);
        dongle_pairing_deadline = 0;
        k_work_cancel_delayable(&dongle_pairing_work);
        bt_conn_unref(dongle_pairing_conn);
        dongle_pairing_conn = NULL;
        dongle_host_connected();
    } else {
        dongle_log_note(DONGLE_LOG_HOST_PAIRING_RESULT, active_profile, BT_SECURITY_ERR_PAIR_NOT_ALLOWED);
        /* Only a rejected PC connection is unpaired; split roles never enter. */
        bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
    update_advertising();
    k_mutex_unlock(&dongle_ble_mutex);
}

'''
    text = replace(text, marker, wrapper + marker)
    text = replace(text, '    .pairing_complete = auth_pairing_complete,',
                   '    .pairing_complete = auth_pairing_complete,\n    .pairing_failed = dongle_host_pairing_failed,')
    text = replace(text, '        int err = read_cb(cb_arg, &active_profile, sizeof(active_profile));',
                   '        int err = read_cb(cb_arg, &active_profile, sizeof(active_profile));\n        if (active_profile >= ZMK_BLE_PROFILE_COUNT) active_profile = 0;')
    return text + '''
int dongle_split_known_slot(const bt_addr_le_t *addr) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (bt_addr_le_cmp(&peripheral_addrs[i], BT_ADDR_LE_ANY) != 0 &&
            bt_addr_le_cmp(&peripheral_addrs[i], addr) == 0) {
            return i;
        }
    }
    return -1;
}

#include "dongle_ble_commands.inc"
'''


def endpoints(text):
    text = replace(text, '#include <zmk/endpoints.h>', '#include <zmk/endpoints.h>\n#include "dongle_host.h"')
    text = replace(text, 'int zmk_endpoints_select_transport(enum zmk_transport transport) {',
                   'int dongle_endpoints_select_transport(enum zmk_transport transport) {')
    start = text.index('#if IS_ENABLED(CONFIG_SETTINGS)\nstatic void endpoints_save_preferred_work(')
    end = text.index('bool zmk_endpoint_instance_eq(', start)
    text = text[:start] + text[end:]
    text = replace(text, '    endpoints_save_preferred();\n', '')
    text = replace(text, '#if IS_ENABLED(CONFIG_SETTINGS)\n    k_work_init_delayable(&endpoints_save_work, endpoints_save_preferred_work);\n#endif\n', '')
    text = replace(text, '    if (preferred_transport == transport) {\n        return 0;\n    }\n', '')
    start = text.index('static enum zmk_transport get_selected_transport(void) {')
    end = text.index('\nenum zmk_transport zmk_endpoints_get_preferred_transport', start)
    text = text[:start] + '''static enum zmk_transport get_selected_transport(void) {
    return preferred_transport;
}
''' + text[end:]
    text = replace(text, '        zmk_endpoints_clear_current();',
                   '        /* The host controller releases the old endpoint without clearing HID counters. */')
    for name in ('send_keyboard_report', 'send_consumer_report', 'zmk_endpoints_send_mouse_report'):
        # Mouse in this pinned fork has an old-style empty parameter list.
        signature = f'{name}(' + (') {' if name.startswith('zmk_') else 'void) {')
        text = replace(text, signature, signature + '\n    if (!dongle_host_reports_allowed()) return 0;')
    text = replace(text, '        update_current_endpoint();\n    }',
                   '        if (preferred_transport != ZMK_TRANSPORT_USB && preferred_transport != ZMK_TRANSPORT_BLE) preferred_transport = ZMK_TRANSPORT_USB;\n        update_current_endpoint();\n    }')
    return text


def hog(text):
    text = replace(text, '#include <zmk/hog.h>',
                   '#include <zmk/hog.h>\n#include <zephyr/sys/atomic.h>\n#include "dongle_host.h"\n#include "dongle_log.h"')
    text = replace(text, '    host_requests_notification = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;',
                   '    host_requests_notification = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;\n'
                   '    dongle_log_note(DONGLE_LOG_HOST_HID_CCC, zmk_ble_active_profile_index(), value);')
    for kind, attr, report_kind in (('keyboard', 5, 0), ('consumer', 9, 1), ('mouse', 13, 2)):
        body = f'struct zmk_hid_{kind}_report_body'
        # Protect GATT reads as well as notifications from inactive USB/BLE output.
        declaration = f'    {body} *report_body = &zmk_hid_get_{kind}_report()->body;'
        text = replace(text, declaration,
                       f'    static {body} neutral;\n    {body} *report_body = dongle_host_transport_allowed(ZMK_TRANSPORT_BLE) ? &zmk_hid_get_{kind}_report()->body : &neutral;')
        queue_type = f'struct dongle_{kind}_packet'
        marker = f'K_MSGQ_DEFINE(zmk_hog_{kind}_msgq, sizeof({body}),'
        text = replace(text, marker,
                       f'{queue_type} {{ struct bt_conn *conn; uint32_t generation; {body} report; }};\nK_MSGQ_DEFINE(zmk_hog_{kind}_msgq, sizeof({queue_type}),')
        text = replace(text, f'    {body} report;\n',
                       f'    {queue_type} packet;\n    static uint32_t last_generation = UINT32_MAX;\n    static int last_notify_error;\n')
        text = replace(text, f'while (k_msgq_get(&zmk_hog_{kind}_msgq, &report, K_NO_WAIT) == 0) {{\n        struct bt_conn *conn = zmk_ble_active_profile_conn();\n        if (conn == NULL) {{\n            return;\n        }}',
                       f'''while (k_msgq_get(&zmk_hog_{kind}_msgq, &packet, K_NO_WAIT) == 0) {{
        struct bt_conn *conn = packet.conn;
        if (!dongle_host_transport_allowed(ZMK_TRANSPORT_BLE) || packet.generation != dongle_host_generation()) {{
            bt_conn_unref(conn);
            continue;
        }}''')
        text = replace(text, f'            .attr = &hog_svc.attrs[{attr}],\n            .data = &report,\n            .len = sizeof(report),',
                       f'            .attr = &hog_svc.attrs[{attr}],\n            .data = &packet.report,\n            .len = sizeof(packet.report),')
        hook = f'            .attr = &hog_svc.attrs[{attr}],\n            .data = &packet.report,\n            .len = sizeof(packet.report),\n        }};\n\n        int err = bt_gatt_notify_cb(conn, &notify_params);'
        text = replace(text, hook, hook + f'''
        if (packet.generation != last_generation || err != last_notify_error) {{
            dongle_log_note(DONGLE_LOG_HOST_HID_NOTIFY_RESULT, {report_kind}, err);
            last_generation = packet.generation;
            last_notify_error = err;
        }}''')
        text = replace(text, f'int zmk_hog_send_{kind}_report({body} *report) {{',
                       f'''int zmk_hog_send_{kind}_report({body} *report) {{
    if (!dongle_host_transport_allowed(ZMK_TRANSPORT_BLE)) return 0;
    struct bt_conn *conn = zmk_ble_active_profile_conn();
    if (!conn) return -ENOTCONN;
    if (bt_conn_get_security(conn) < BT_SECURITY_L2) {{
        bt_conn_set_security(conn, BT_SECURITY_L2);
        bt_conn_unref(conn);
        return -EACCES;
    }}
    {queue_type} packet = {{ .conn = conn, .generation = dongle_host_generation(), .report = *report }};''')
        text = replace(text, f'k_msgq_put(&zmk_hog_{kind}_msgq, report, K_MSEC(100))',
                       f'k_msgq_put(&zmk_hog_{kind}_msgq, &packet, K_NO_WAIT)')
        text = replace(text, f'            {body} discarded_report;\n            k_msgq_get(&zmk_hog_{kind}_msgq, &discarded_report, K_NO_WAIT);\n            return zmk_hog_send_{kind}_report(report);',
                       f'''            {queue_type} discarded;
            if (!k_msgq_get(&zmk_hog_{kind}_msgq, &discarded, K_NO_WAIT)) bt_conn_unref(discarded.conn);
            bt_conn_unref(conn);
            return zmk_hog_send_{kind}_report(report);''')
        text = replace(text, f'            LOG_WRN("Failed to queue {kind} report to send (%d)", err);\n            return err;',
                       f'            LOG_WRN("Failed to queue {kind} report to send (%d)", err);\n            bt_conn_unref(conn);\n            return err;')
    text = replace(text, '        case -EAGAIN: {', '        case -ENOMSG:\n        case -EAGAIN: {', 3)
    return text + '''
K_SEM_DEFINE(dongle_release_sent, 0, 1);
static atomic_t pending_release;
/* Release reports are submitted serially by the host command queue. */
static uint32_t release_sequence;

static void release_sent(struct bt_conn *conn, void *data) {
    if (atomic_cas(&pending_release, (atomic_val_t)(uintptr_t)data, 0)) {
        k_sem_give(&dongle_release_sent);
    }
}

static int send_release(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *report, size_t len) {
    if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
        return 0; /* No notification subscriber for this report. */
    }
    if (!++release_sequence) ++release_sequence;
    atomic_val_t token = (atomic_val_t)release_sequence;
    k_sem_reset(&dongle_release_sent);
    atomic_set(&pending_release, token);
    struct bt_gatt_notify_params params = {
        .attr = attr, .data = report, .len = len,
        .func = release_sent, .user_data = (void *)(uintptr_t)release_sequence,
    };
    int err = bt_gatt_notify_cb(conn, &params);
    if (!err) {
        int64_t deadline = k_uptime_get() + 500;
        while (atomic_get(&pending_release) == token) {
            int64_t remaining = deadline - k_uptime_get();
            if (remaining <= 0 || k_sem_take(&dongle_release_sent, K_MSEC(remaining))) {
                if (atomic_cas(&pending_release, token, 0)) err = -ETIMEDOUT;
                break;
            }
        }
    } else {
        atomic_cas(&pending_release, token, 0);
    }
    return err;
}

int dongle_hog_release(void) {
    k_work_queue_drain(&hog_work_q, false);
    struct bt_conn *conn = zmk_ble_active_profile_conn();
    if (!conn) return -ENOTCONN;
    const struct zmk_hid_keyboard_report_body keyboard = {0};
    const struct zmk_hid_consumer_report_body consumer = {0};
    const struct zmk_hid_mouse_report_body mouse = {0};
    int err = send_release(conn, &hog_svc.attrs[5], &keyboard, sizeof(keyboard));
    if (!err) err = send_release(conn, &hog_svc.attrs[9], &consumer, sizeof(consumer));
    if (!err) err = send_release(conn, &hog_svc.attrs[13], &mouse, sizeof(mouse));
    bt_conn_unref(conn);
    return err;
}
'''


def usb_hid(text):
    text = replace(text, '#include <zmk/hid.h>', '#include <zmk/hid.h>\n#include "dongle_host.h"')
    text = replace(text, 'static uint8_t *get_keyboard_report(size_t *len) {', '''static uint8_t *get_keyboard_report(size_t *len) {
    if (!dongle_host_transport_allowed(ZMK_TRANSPORT_USB)) {
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
        static zmk_hid_boot_report_t boot;
        if (hid_protocol != HID_PROTOCOL_REPORT) { *len = sizeof(boot); return (uint8_t *)&boot; }
#endif
        static struct zmk_hid_keyboard_report neutral = {.report_id = ZMK_HID_REPORT_ID_KEYBOARD};
        *len = sizeof(neutral);
        return (uint8_t *)&neutral;
    }''')
    text = replace(text, '            struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();',
                   '            static struct zmk_hid_consumer_report neutral = {.report_id = ZMK_HID_REPORT_ID_CONSUMER};\n            struct zmk_hid_consumer_report *report = dongle_host_transport_allowed(ZMK_TRANSPORT_USB) ? zmk_hid_get_consumer_report() : &neutral;')
    start = text.index('static int zmk_usb_hid_send_report(')
    end = text.index('int zmk_usb_hid_send_keyboard_report(void)', start)
    text = text[:start] + '''static int dongle_usb_send_report(const uint8_t *report, size_t len,
                                  bool wait_for_completion) {
    switch (zmk_usb_get_status()) {
    case USB_DC_SUSPEND: {
        int err = usb_wakeup_request();
        return !err && wait_for_completion ? -EAGAIN : err;
    }
    case USB_DC_ERROR:
    case USB_DC_RESET:
    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return -ENODEV;
    default:
        break;
    }

    int err = k_sem_take(&hid_sem, K_MSEC(30));
    if (err) return err;
    err = hid_int_ep_write(hid_dev, report, len, NULL);
    if (err) {
        k_sem_give(&hid_sem);
        return err;
    }
    if (!wait_for_completion) return 0;

    err = k_sem_take(&hid_sem, K_MSEC(100));
    if (err) return -ETIMEDOUT;
    k_sem_give(&hid_sem);
    return 0;
}

static int zmk_usb_hid_send_report(const uint8_t *report, size_t len) {
    return dongle_usb_send_report(report, len, false);
}

''' + text[end:]
    return text + '''
int dongle_usb_release(void) {
    static struct zmk_hid_keyboard_report keyboard = {.report_id = ZMK_HID_REPORT_ID_KEYBOARD};
    static struct zmk_hid_consumer_report consumer = {.report_id = ZMK_HID_REPORT_ID_CONSUMER};
    static struct zmk_hid_mouse_report mouse = {.report_id = ZMK_HID_REPORT_ID_MOUSE};
#if IS_ENABLED(CONFIG_ZMK_USB_BOOT)
    if (hid_protocol != HID_PROTOCOL_REPORT) {
        static zmk_hid_boot_report_t boot;
        return dongle_usb_send_report((const uint8_t *)&boot, sizeof(boot), true);
    }
#endif
    int err = dongle_usb_send_report((const uint8_t *)&keyboard, sizeof(keyboard), true);
    if (!err) err = dongle_usb_send_report((const uint8_t *)&consumer, sizeof(consumer), true);
    if (!err) err = dongle_usb_send_report((const uint8_t *)&mouse, sizeof(mouse), true);
    return err;
}
'''


def behavior_queue(text):
    text = replace(text, '#include <zmk/behavior.h>', '#include <zmk/behavior.h>\n#include "dongle_host.h"')
    text = replace(text, '        if (item.press) {', '        dongle_host_input_lock();\n        if (item.press) {')
    text = replace(text, '        LOG_DBG("Processing next queued behavior in %dms", item.wait);',
                   '        dongle_host_input_unlock();\n        LOG_DBG("Processing next queued behavior in %dms", item.wait);')
    return text + '''
bool dongle_behavior_queue_idle(void) {
    return k_msgq_num_used_get(&zmk_behavior_queue_msgq) == 0 &&
           k_work_busy_get(&queue_work.work) == 0;
}
'''


def split_central(text):
    text = replace(text, '#include <errno.h>', '#include <errno.h>\n#include "dongle_host.h"')
    text = replace(text, '        struct zmk_position_state_changed state_ev =', '''        if (dongle_host_position(source, ev.data.key_position_event.position,
                                 ev.data.key_position_event.pressed)) return 0;
        struct zmk_position_state_changed state_ev =''')
    for event in ('INPUT_EVENT', 'SENSOR_EVENT'):
        marker = f'case ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_{event}: {{'
        text = replace(text, marker, marker + '\n        if (!dongle_host_reports_allowed()) return 0;')
    text = replace(text, 'int zmk_split_transport_central_peripheral_event_handler(',
                   'static int dongle_raw_peripheral_event_handler(')
    return text + '''
int zmk_split_transport_central_peripheral_event_handler(
    const struct zmk_split_transport_central *transport, uint8_t source,
    struct zmk_split_transport_peripheral_event ev) {
    dongle_host_input_lock();
    int err = dongle_raw_peripheral_event_handler(transport, source, ev);
    dongle_host_input_unlock();
    return err;
}
'''


def sticky_key(text):
    text = replace(text, '#include <zmk/behavior.h>', '#include <zmk/behavior.h>\n#include "dongle_host.h"')
    text = replace(text, 'void behavior_sticky_key_timer_handler(struct k_work *item) {',
                   'static void dongle_raw_sticky_timer(struct k_work *item) {')
    marker = 'static int behavior_sticky_key_init(const struct device *dev) {'
    text = replace(text, marker, '''void behavior_sticky_key_timer_handler(struct k_work *item) {
    dongle_host_input_lock();
    dongle_raw_sticky_timer(item);
    dongle_host_input_unlock();
}

''' + marker)
    return text + '''
bool dongle_sticky_idle(void) {
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
    for (unsigned i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        if (k_work_busy_get(&active_sticky_keys[i].release_timer.work) & K_WORK_RUNNING) return false;
    }
#endif
    return true;
}
void dongle_sticky_reset(void) {
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
    for (unsigned i = 0; i < ZMK_BHV_STICKY_KEY_MAX_HELD; i++) {
        struct active_sticky_key *key = &active_sticky_keys[i];
        if (key->position == ZMK_BHV_STICKY_KEY_POSITION_FREE) continue;
        k_work_cancel_delayable(&key->release_timer);
        if (!key->config->lazy || key->modified_key_keycode) release_sticky_key_behavior(key, k_uptime_get());
        else clear_sticky_key(key);
        key->timer_cancelled = false;
    }
#endif
}
'''


def two_axis(text):
    text = replace(text, '#include <zmk/behavior.h>', '#include <zmk/behavior.h>\n#include "dongle_host.h"')
    text = replace(text, 'static void tick_work_cb(struct k_work *work) {',
                   'static void dongle_raw_mouse_tick(struct k_work *work) {')
    marker = 'static void set_start_times_for_activity_1d('
    text = replace(text, marker, '''static void tick_work_cb(struct k_work *work) {
    dongle_host_input_lock();
    dongle_raw_mouse_tick(work);
    dongle_host_input_unlock();
}

''' + marker)
    return text + '''
#define DONGLE_MOUSE_IDLE(n) && !(k_work_busy_get(&behavior_input_two_axis_data_##n.tick_work.work) & K_WORK_RUNNING)
bool dongle_mouse_idle(void) {
    return true DT_INST_FOREACH_STATUS_OKAY(DONGLE_MOUSE_IDLE);
}
#define DONGLE_MOUSE_RESET(n) \\
    k_work_cancel_delayable(&behavior_input_two_axis_data_##n.tick_work); \\
    memset(&behavior_input_two_axis_data_##n.state, 0, sizeof(behavior_input_two_axis_data_##n.state));
void dongle_mouse_reset(void) {
    DT_INST_FOREACH_STATUS_OKAY(DONGLE_MOUSE_RESET)
}
'''


def studio_rpc(text):
    return replace(text, 'enum zmk_transport transport = zmk_endpoints_selected().transport;',
                   '/* Studio uses USB CDC independently of the HID output. */\n'
                   '    const enum zmk_transport transport = ZMK_TRANSPORT_USB;')


def battery(text):
    text = replace(text, '#include <zmk/battery.h>', '''#include <zmk/battery.h>
#include <zephyr/sys/atomic.h>
#include "dongle_status.h"

static atomic_t dongle_battery_percent = ATOMIC_INIT(-1);

int dongle_local_battery_percent(void) {
    return atomic_get(&dongle_battery_percent);
}''')
    marker = '    if (last_state_of_charge != state_of_charge.val1) {'
    return replace(text, marker, '''    atomic_set(&dongle_battery_percent,
               state_of_charge.val1 >= 0 && state_of_charge.val1 <= 100 ? state_of_charge.val1 : -1);
''' + marker)


TRANSFORMS = {'battery': battery, 'studio_rpc': studio_rpc, 'ble': ble, 'endpoints': endpoints, 'hog': hog, 'usb_hid': usb_hid,
              'behavior_queue': behavior_queue, 'split_central': split_central,
              'sticky_key': sticky_key, 'two_axis': two_axis}


def main():
    name, source, target = sys.argv[1:]
    text = pathlib.Path(source).read_text(encoding='utf-8')
    digest = hashlib.sha256(text.encode()).hexdigest()
    if digest != HASHES[name]:
        raise SystemExit(f'Unexpected pinned {name} source: {digest}; review wireless hooks')
    pathlib.Path(target).write_text(TRANSFORMS[name](text), encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
