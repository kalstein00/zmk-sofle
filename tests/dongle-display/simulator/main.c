#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <lvgl.h>
#include "dongle_battery_snapshot.h"

static unsigned char pixels[64][128];
static struct dongle_slot_state slots[2];
static uint32_t activity;
static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *colors) {
    for (int y = area->y1; y <= area->y2; ++y)
        for (int x = area->x1; x <= area->x2; ++x) {
            if (x >= 0 && x < 128 && y >= 0 && y < 64)
                pixels[y][x] = lv_color_brightness(*colors) > 127;
            ++colors;
        }
    lv_disp_flush_ready(driver);
}
struct dongle_slot_state dongle_battery_get(unsigned n) { return slots[n]; }
uint32_t dongle_key_activity(void) { return activity; }
bool zmk_usb_is_hid_ready(void) { return true; }
unsigned zmk_keymap_highest_layer_active(void) { return 1; }
lv_obj_t *zmk_display_status_screen(void);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
#include "dongle_status.h"
#include "dongle_menu.h"
static struct dongle_pc_snapshot pc = {2, DONGLE_PC_CONNECTED};
static bool ble = true;
static int local_battery = 73;
static struct dongle_menu menu;
struct dongle_pc_snapshot dongle_pc_snapshot(void) { return pc; }
struct dongle_menu dongle_menu_snapshot(void) { return menu; }
enum dongle_pc_status dongle_pc_profile_status(uint8_t profile) {
    return profile == 2 ? pc.status : profile == 0 ? DONGLE_PC_PAIRED : DONGLE_PC_EMPTY;
}
bool dongle_output_is_ble(void) { return ble; }
bool dongle_output_switching(void) { return false; }
int dongle_local_battery_percent(void) { return local_battery; }

static bool has_label(lv_obj_t *screen, const char *text) {
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(screen); i++) {
        lv_obj_t *child = lv_obj_get_child(screen, i);
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        if (lv_obj_check_type(child, &lv_label_class) && strstr(lv_label_get_text(child), text)) {
            return true;
        }
        if (has_label(child, text)) return true;
    }
    return false;
}
#endif
int main(int argc, char **argv) {
    if (argc != 2 && argc != 6) return 2;
    slots[0] = (struct dongle_slot_state){.connected = true, .received = true, .level = 85, .side = DONGLE_SIDE_LEFT};
    slots[1] = (struct dongle_slot_state){.connected = true, .received = true, .level = 18, .side = DONGLE_SIDE_RIGHT};
    lv_init();
    static lv_color_t buffer[128 * 64];
    static lv_disp_draw_buf_t draw;
    static lv_disp_drv_t driver;
    lv_disp_draw_buf_init(&draw, buffer, NULL, 128 * 64);
    lv_disp_drv_init(&driver);
    driver.hor_res = 128; driver.ver_res = 64;
    driver.flush_cb = flush; driver.draw_buf = &draw;
    lv_disp_drv_register(&driver);
    lv_disp_set_theme(NULL, lv_theme_mono_init(NULL, false, &lv_font_unscii_8));
    lv_scr_load(zmk_display_status_screen());
    lv_timer_handler();
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    assert(has_label(lv_scr_act(), "BLE P2"));
    assert(has_label(lv_scr_act(), "Connected"));
    assert(has_label(lv_scr_act(), "D 73%"));
    assert(has_label(lv_scr_act(), "L 85%"));
    assert(has_label(lv_scr_act(), "R 18%"));
    assert(has_label(lv_scr_act(), "Lyr 1"));
    pc.status = DONGLE_PC_PAIRING;
    lv_tick_inc(250);
    lv_timer_handler();
    assert(has_label(lv_scr_act(), "Pairing"));
    pc.status = DONGLE_PC_FAILED;
    lv_tick_inc(250);
    lv_timer_handler();
    assert(has_label(lv_scr_act(), "Pair failed"));
    pc.status = DONGLE_PC_TIMEOUT;
    ble = false;
    local_battery = 0;
    lv_tick_inc(250);
    lv_timer_handler();
    assert(has_label(lv_scr_act(), "USB P2"));
    assert(has_label(lv_scr_act(), "Timeout"));
    assert(has_label(lv_scr_act(), "D 0%"));
    pc.status = DONGLE_PC_CONNECTED;
    ble = true;
    local_battery = 73;
    lv_tick_inc(250);
    lv_timer_handler();
#endif
    if (argc == 6) {
        slots[0].level = atoi(argv[2]); slots[1].level = atoi(argv[3]);
        activity = atoi(argv[5]);
        for (int i = 0; i < atoi(argv[4]); ++i) {
            lv_tick_inc(250);
            lv_timer_handler();
        }
    }
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    const char *menu_page = getenv("SOFLE_MENU_PAGE");
    if (menu_page) {
        const char *usb_entry = getenv("SOFLE_MENU_USB");
        menu = (struct dongle_menu){.page = atoi(menu_page),
                                    .profile = usb_entry ? DONGLE_MENU_USB_ENTRY : 2,
                                    .confirmation = MENU_DELETE};
        lv_tick_inc(250);
        lv_timer_handler();
        lv_refr_now(NULL);
        assert(!has_label(lv_scr_act(), "D 73%"));
        if (menu.page == MENU_HOSTS) {
            assert(has_label(lv_scr_act(), usb_entry ? ">USB output" : ">2 Connected"));
            assert(has_label(lv_scr_act(), "Center U/D/OK"));
        }
        if (menu.page == MENU_CONFIRM) assert(has_label(lv_scr_act(), ">Cancel"));
    }
#endif
    FILE *out = fopen(argv[1], "wb");
    if (!out) return 3;
    fprintf(out, "P6\n128 64\n255\n");
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 128; ++x) {
        unsigned char p[3] = {pixels[y][x] * 255, pixels[y][x] * 255, pixels[y][x] * 255};
        fwrite(p, 1, 3, out);
    }
    return fclose(out) != 0;
}
