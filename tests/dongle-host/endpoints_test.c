#define TEST_REAL_ENDPOINTS
#include "controller_stubs.h"
#define CONFIG_SETTINGS 0
#define CONFIG_ZMK_USB 1
#define CONFIG_ZMK_BLE 1
#define CONFIG_ZMK_POINTING 1
#define IS_ENABLED(x) (x)
#define LOG_DBG(...) ((void)0)
#define HID_USAGE_KEY 7
#define HID_USAGE_CONSUMER 12
#define ZMK_ENDPOINT_USB_COUNT 1
#define ZMK_ENDPOINT_STR_LEN 10
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
typedef int zmk_event_t;
struct zmk_endpoint_changed { struct zmk_endpoint_instance endpoint; };
static void raise_zmk_endpoint_changed(struct zmk_endpoint_changed event) {}
static bool usb_ready=true, ble_ready;
static unsigned usb_packets, ble_packets;
static bool blocked;
static bool dongle_host_reports_allowed(void) { return !blocked; }
static bool zmk_usb_is_hid_ready(void) { return usb_ready; }
static bool zmk_ble_active_profile_is_connected(void) { return ble_ready; }
static struct zmk_hid_keyboard_report *zmk_hid_get_keyboard_report(void) { return &keyboard; }
static struct zmk_hid_consumer_report *zmk_hid_get_consumer_report(void) { return &consumer; }
static int zmk_usb_hid_send_keyboard_report(void) { usb_packets++; return 0; }
static int zmk_usb_hid_send_consumer_report(void) { usb_packets++; return 0; }
static int zmk_usb_hid_send_mouse_report(void) { usb_packets++; return 0; }
static int zmk_hog_send_keyboard_report(struct zmk_hid_keyboard_report_body *report) { ble_packets++; return 0; }
static int zmk_hog_send_consumer_report(struct zmk_hid_consumer_report_body *report) { ble_packets++; return 0; }
static int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report) { ble_packets++; return 0; }
enum zmk_transport zmk_endpoints_get_preferred_transport(void);
int zmk_endpoints_select_transport(enum zmk_transport transport);
#include "endpoints.generated.c"
int zmk_endpoints_select_transport(enum zmk_transport transport) {
    return dongle_endpoints_select_transport(transport);
}

int main(void) {
    assert(!startup());
    assert(zmk_endpoints_selected().transport==ZMK_TRANSPORT_USB);
    assert(!zmk_endpoints_select_transport(ZMK_TRANSPORT_BLE));
    assert(zmk_endpoints_selected().transport==ZMK_TRANSPORT_BLE);
    assert(!zmk_endpoints_send_report(HID_USAGE_KEY));
    assert(!usb_packets && ble_packets==1);
    ble_ready=true; endpoint_listener(NULL);
    assert(zmk_endpoints_selected().transport==ZMK_TRANSPORT_BLE);
    ble_ready=false; endpoint_listener(NULL);
    assert(zmk_endpoints_selected().transport==ZMK_TRANSPORT_BLE);
    assert(!zmk_endpoints_select_transport(ZMK_TRANSPORT_USB));
    usb_ready=false; ble_ready=true; endpoint_listener(NULL);
    assert(zmk_endpoints_selected().transport==ZMK_TRANSPORT_USB);
    blocked=true;
    unsigned before=usb_packets+ble_packets;
    zmk_endpoints_send_report(HID_USAGE_KEY);
    zmk_endpoints_send_report(HID_USAGE_CONSUMER);
    zmk_endpoints_send_mouse_report();
    assert(usb_packets+ble_packets==before);
    puts("PASS: actual endpoints keep explicit USB/BLE selection across readiness changes and suppress all report types during switching");
}
