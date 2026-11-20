#include "controller_stubs.h"
#define CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS 1
#include "dongle_log.h"
static enum dongle_log_event last_log_event;
static int last_log_slot, last_log_code;
void dongle_log_note(enum dongle_log_event event, int slot, int code) {
    last_log_event = event;
    last_log_slot = slot;
    last_log_code = code;
}
typedef int zmk_event_t;
struct zmk_usb_conn_state_changed { int conn_state; };
static int zmk_usb_get_conn_state(void) { return 0; }
static const struct zmk_usb_conn_state_changed *as_zmk_usb_conn_state_changed(const zmk_event_t *event) {
    static const struct zmk_usb_conn_state_changed usb;
    return &usb;
}
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_LISTENER(name, callback) static int (*name)(const zmk_event_t *) = callback
#define ZMK_SUBSCRIPTION(name, event)
#include "controller.generated.c"

static void menu_tap(unsigned position) {
    assert(dongle_host_position(0, position, true));
    assert(dongle_host_position(0, position, false));
}

static void settle(void) {
    for (unsigned i = 0; (scheduled || have_queued_input) && i < 100; i++) {
        if (have_queued_input) drain_input(input_callback);
        tick();
    }
    assert(!scheduled && !have_queued_input);
}

int main(void) {
    assert(!startup());
    assert(!dongle_host_position(1,61,true));
    assert(!zmk_ble_prof_select(1));
    tick();
    assert(!dongle_host_reports_allowed() && selected_profile==0 && usb_releases==1);
    assert(dongle_host_position(0,14,true));
    assert(dongle_host_position(0,14,false));
    assert(!dongle_host_position(1,61,false));
    tick();
    assert(have_queued_input && !dongle_host_reports_allowed() && selected_profile == 0);
    tick();
    assert(!dongle_host_reports_allowed() && selected_profile == 0);
    mouse.body.buttons = 1; /* An old mouse callback completed before the barrier. */
    settle();
    assert(selected_profile==1 && preferred==ZMK_TRANSPORT_BLE && dongle_host_reports_allowed());
    assert(!connections[0].state && connections[1].state && connections[2].state);
    assert(!paused && split_bonds[0] && split_bonds[1]);
    assert(!mouse.body.buttons);
    assert(zmk_ble_prof_select(4)==-ERANGE);
    assert(!zmk_ble_prof_prev()); settle(); assert(selected_profile==0);
    assert(!zmk_ble_prof_prev()); settle(); assert(selected_profile==3);
    assert(!zmk_ble_prof_next()); settle(); assert(selected_profile==0);

    /* &kt keeps a key/modifier down after its physical key has been released. */
    keyboard.body.keys[0]=4;
    keyboard.body.modifiers=2; modifier_counts[1]=2;
    mouse.body.buttons=1;
    assert(!zmk_ble_prof_select(2));
    settle();
    assert(selected_profile==2 && dongle_host_reports_allowed());
    assert(!keyboard.body.keys[0] && !keyboard.body.modifiers && !modifier_counts[1] && !mouse.body.buttons);

    unsigned before = usb_releases;
    assert(!zmk_endpoints_select_transport(ZMK_TRANSPORT_USB));
    dongle_host_connected(); /* BLE callback happens before the queued USB selection. */
    settle();
    assert(preferred == ZMK_TRANSPORT_USB && dongle_host_reports_allowed());
    assert(usb_releases == before);
    assert(!dongle_host_position(0,53,true));
    keyboard.body.modifiers=1; modifier_counts[0]=1;
    dongle_host_connected();
    settle();
    assert(dongle_host_reports_allowed() && usb_releases==before && keyboard.body.modifiers==1);
    assert(!dongle_host_position(0,53,false));
    keyboard.body.modifiers=0; modifier_counts[0]=0;
    assert(!dongle_host_position(1, 49, true));
    assert(!dongle_menu_request());
    tick();
    assert(dongle_menu_snapshot().page == MENU_CLOSED);
    assert(!dongle_host_position(1, 49, false));
    settle();
    assert(dongle_menu_snapshot().page == MENU_HOSTS);
    assert(!dongle_host_reports_allowed());
    menu_tap(19); /* P3 */
    menu_tap(51); /* Options */
    menu_tap(19); /* Delete */
    menu_tap(51); /* Confirm prompt */
    assert(dongle_menu_snapshot().page == MENU_CONFIRM);
    menu_tap(51); /* Default Cancel */
    assert(pc_bonds[3] && dongle_menu_snapshot().page == MENU_ACTIONS);
    menu_tap(19);
    menu_tap(51);
    menu_tap(19); /* Explicit Delete */
    menu_tap(51);
    settle();
    assert(!pc_bonds[3] && pc_bonds[2] && selected_profile == 2);
    assert(dongle_host_reports_allowed() && dongle_menu_snapshot().page == MENU_CLOSED);
    assert(!dongle_menu_request());
    settle();
    assert(dongle_host_position(0, 19, true));
    now += 30000;
    assert(dongle_menu_snapshot().page == MENU_CLOSED);
    assert(dongle_host_position(0, 19, false));
    assert(!dongle_host_position(0, 19, true));
    assert(!dongle_host_position(0, 19, false));
    assert(!dongle_menu_request());
    settle();
    menu_tap(15); /* W moves up to P1. */
    assert(dongle_menu_snapshot().profile == 1);
    menu_tap(28);
    menu_tap(28); /* S moves down to P3. */
    menu_tap(16); /* E opens options. */
    menu_tap(14); /* Q returns to profiles. */
    assert(dongle_menu_snapshot().page == MENU_HOSTS);
    menu_tap(16);
    menu_tap(16);
    settle();
    assert(selected_profile == 3 && preferred == ZMK_TRANSPORT_BLE);
    assert(dongle_host_reports_allowed());

    /* Q armed on P3, then P1 was queued while Q was still held. */
    pc_bonds[3] = true;
    assert(!dongle_host_position(0, 14, true));
    assert(!zmk_ble_prof_select(1));
    tick();
    assert(selected_profile == 3);
    zmk_ble_clear_bonds();
    assert(!dongle_host_position(0, 14, false));
    for (unsigned i = 0; selected_profile != 1 && i < 100; i++) {
        tick();
        if (have_queued_input) drain_input(input_callback);
    }
    assert(selected_profile == 1 && pc_bonds[3]);
    /* P1 is reconnecting. It can't yet send a HID release, but the queued
     * deletion targets P3, which no longer owns the output. */
    unsigned releases_after_switch = ble_releases;
    ble_release_result = -EPERM;
    settle();
    assert(selected_profile == 1 && pc_bonds[1] && !pc_bonds[3]);
    assert(ble_releases == releases_after_switch);
    ble_release_result = 0;

    /* A queued release without transmission completion must not switch PCs. */
    connections[0].state = BT_CONN_STATE_CONNECTED;
    ble_release_result = -ETIMEDOUT;
    assert(!zmk_ble_prof_select(2));
    settle();
    assert(selected_profile == 1 && connections[0].state == BT_CONN_STATE_CONNECTED);
    assert(dongle_host_reports_allowed());
    ble_release_result = 0;

    assert(!zmk_endpoints_select_transport(ZMK_TRANSPORT_USB));
    settle();
    unsigned writes = selection_writes;
    fail_selection_save = true;
    assert(!zmk_ble_prof_select(2));
    settle();
    assert(selected_profile == 1 && preferred == ZMK_TRANSPORT_USB && selection_writes == writes);
    fail_selection_save = false;
    assert(!zmk_ble_prof_select(2));
    settle();
    assert(selection_writes == writes + 1 && saved_selection[0] == 2 && saved_selection[1] == ZMK_TRANSPORT_BLE);
    selected_profile = 0;
    preferred = ZMK_TRANSPORT_USB;
    dongle_host_restore_selection(&selected_profile);
    assert(selected_profile == 0 && preferred == ZMK_TRANSPORT_USB); /* Legacy fallback. */
    assert(!settings_set("selection", 2, read_selection, saved_selection));
    dongle_host_restore_selection(&selected_profile);
    assert(selected_profile == 2 && preferred == ZMK_TRANSPORT_BLE);
    assert(last_log_event == DONGLE_LOG_HOST_RESTORED &&
           last_log_slot == 2 && last_log_code == ZMK_TRANSPORT_BLE);
    dongle_host_connected();
    assert(last_log_event == DONGLE_LOG_HOST_COMMAND_QUEUED && last_log_slot == HOST_CONNECTED);
    tick();
    assert(last_log_event == DONGLE_LOG_HOST_GATE_WAIT && last_log_code == WAIT_INPUT_BARRIER);
    drain_input(input_callback);
    settle();
    assert(last_log_event == DONGLE_LOG_HOST_GATE_OPEN && last_log_slot == 2 && !last_log_code);
    /* USB is the entry after P3 on the top-level output menu. */
    assert(!dongle_menu_request());
    settle();
    menu_tap(19); /* P2 -> P3 */
    menu_tap(19); /* P3 -> USB */
    connections[0].state = BT_CONN_STATE_CONNECTED;
    ble_release_result = -ETIMEDOUT;
    assert(dongle_host_position(0, 58, true));
    tick();
    assert(preferred == ZMK_TRANSPORT_BLE); /* Wait for confirming key release. */
    assert(dongle_host_position(0, 58, false));
    settle();
    assert(preferred == ZMK_TRANSPORT_USB && selected_profile == 2);
    assert(connections[0].state == BT_CONN_STATE_DISCONNECTED);
    assert(connections[1].state == BT_CONN_STATE_CONNECTED);
    assert(saved_selection[0] == 2 && saved_selection[1] == ZMK_TRANSPORT_USB);
    assert(dongle_menu_snapshot().page == MENU_CLOSED && dongle_host_reports_allowed());
    usb_release_result = -EAGAIN;
    assert(!zmk_ble_prof_select(1));
    settle();
    assert(preferred == ZMK_TRANSPORT_USB && selected_profile == 2);
    assert(dongle_host_reports_allowed());
    usb_release_result = 0;

    assert(!zmk_endpoints_select_transport(ZMK_TRANSPORT_BLE));
    settle();
    connections[0].state = BT_CONN_STATE_CONNECTED;
    disconnect_result = -ENOMEM;
    assert(!dongle_menu_request()); /* BLE release is already timing out. */
    settle();
    assert(!paused && dongle_host_reports_allowed());
    disconnect_result = 0;
    hold_pc_connection = true;
    assert(!dongle_menu_request());
    tick();
    if (have_queued_input) drain_input(input_callback);
    now += 60000;
    settle();
    assert(!paused && dongle_host_reports_allowed());
    hold_pc_connection = false;
    assert(!dongle_menu_request());
    settle();
    assert(dongle_menu_snapshot().page == MENU_HOSTS);
    assert(connections[0].state == BT_CONN_STATE_DISCONNECTED);
    menu_tap(19);
    menu_tap(19);
    menu_tap(58);
    settle();
    assert(preferred == ZMK_TRANSPORT_USB && selected_profile == 2);
    puts("PASS: deletion targets the original slot; release timeout aborts a switch; selection saves/restores as one value");
    puts("PASS: menu waits for release, consumes navigation, defaults to Cancel and drops held releases after timeout");
    puts("PASS: controller selects BLE and disconnects only PC; toggle keys do not deadlock; background BLE preserves USB hold");
}
