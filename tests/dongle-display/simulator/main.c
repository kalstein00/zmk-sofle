#include <stdio.h>
#include <stdlib.h>
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
    FILE *out = fopen(argv[1], "wb");
    if (!out) return 3;
    fprintf(out, "P6\n128 64\n255\n");
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 128; ++x) {
        unsigned char p[3] = {pixels[y][x] * 255, pixels[y][x] * 255, pixels[y][x] * 255};
        fwrite(p, 1, 3, out);
    }
    return fclose(out) != 0;
}
