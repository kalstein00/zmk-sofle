#include "hog_stubs.h"
#define CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS 1
#include "dongle_log.h"

static unsigned notify_results[3];
void dongle_log_note(enum dongle_log_event event, int slot, int code) {
    assert(event == DONGLE_LOG_HOST_HID_NOTIFY_RESULT);
    assert(slot >= 0 && slot < 3 && code == 0);
    notify_results[slot]++;
}

#include "hog.generated.c"

int main(void) {
    hog_svc.attrs[5].id=5; hog_svc.attrs[9].id=9; hog_svc.attrs[13].id=13;
    struct zmk_hid_keyboard_report_body key={.keys={4}};
    struct zmk_hid_consumer_report_body consumer={.keys={0xE9}};
    struct zmk_hid_mouse_report_body mouse={.buttons=1};
    assert(!zmk_hog_send_keyboard_report(&key));
    assert(!zmk_hog_send_consumer_report(&consumer));
    assert(!zmk_hog_send_mouse_report(&mouse));
    active=&office; /* The worker runs after the active profile has changed. */
    flush_work();
    assert(sent_count==3);
    assert(notify_results[0] == 1 && notify_results[1] == 1 && notify_results[2] == 1);
    for (unsigned i=0;i<3;i++) assert(sent[i].host==0);
    assert(home.refs==1 && office.refs==1);

    sent_count=0;
    assert(!zmk_hog_send_keyboard_report(&key));
    generation++; /* A switch invalidates every pending old-generation report. */
    flush_work();
    assert(!sent_count && office.refs==1);
    office.connected=false;
    assert(zmk_hog_send_keyboard_report(&key)==-ENOTCONN);
    office.connected=true; flush_work();
    assert(!sent_count); /* Offline keystrokes are never replayed on reconnect. */

    selected=ZMK_TRANSPORT_USB;
    assert(!zmk_hog_send_keyboard_report(&key));
    flush_work(); assert(!sent_count);
    selected=ZMK_TRANSPORT_BLE;
    for (unsigned i=1;i<=12;i++) { key.keys[0]=i; assert(!zmk_hog_send_keyboard_report(&key)); }
    flush_work();
    assert(sent_count==4 && sent[3].data[2]==12 && office.refs==1);
    assert(notify_results[0] == 2); /* Four reports in one generation produce one record. */

    sent_count=0;
    assert(!zmk_hog_send_keyboard_report(&key));
    output_blocked=true; generation++;
    assert(!dongle_hog_release());
    assert(completed_notifications == 3);
    assert(sent_count==3); /* Only neutral keyboard, consumer, and mouse reports. */
    for (unsigned i=0;i<3;i++) {
        assert(sent[i].host==1);
        for (unsigned j=0;j<sent[i].len;j++) assert(sent[i].data[j]==0);
    }
    assert(home.refs==1 && office.refs==1);
    suppress_completion = true;
    assert(dongle_hog_release() == -ETIMEDOUT);
    assert(completed_notifications == 3);
    suppress_completion = false;
    assert(!dongle_hog_release());
    assert(completed_notifications == 6);
    puts("PASS: queued HID stays bound to its PC; stale/offline/inactive output is dropped; neutral release includes mouse");
}
