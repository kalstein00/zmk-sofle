#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include "dongle_battery_snapshot.h"

LV_IMG_DECLARE(bongo_cat_both1);
static lv_obj_t *slot_labels[2];
static lv_obj_t *usb_label;
static lv_obj_t *layer_label;

static void update_screen(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    for (unsigned i = 0; i < 2; ++i) {
        struct dongle_slot_state s = dongle_battery_get(i);
        if (!s.connected) lv_label_set_text_fmt(slot_labels[i], "S%u: OFF", i + 1);
        else if (!s.received) lv_label_set_text_fmt(slot_labels[i], "S%u: --", i + 1);
        else lv_label_set_text_fmt(slot_labels[i], "S%u: %u%%", i + 1, s.level);
    }
    lv_label_set_text(usb_label, zmk_usb_is_hid_ready() ? "USB: OK" : "USB: --");
    lv_label_set_text_fmt(layer_label, "Layer: %u", (unsigned)zmk_keymap_highest_layer_active());
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_text_font(screen, &lv_font_unscii_8, LV_PART_MAIN);
    for (unsigned i = 0; i < 2; ++i) {
        slot_labels[i] = lv_label_create(screen);
        lv_obj_set_pos(slot_labels[i], 0, i * 12);
    }
    usb_label = lv_label_create(screen);
    lv_obj_set_pos(usb_label, 0, 24);
    layer_label = lv_label_create(screen);
    lv_obj_set_pos(layer_label, 0, 36);
    lv_obj_t *cat = lv_img_create(screen);
    lv_img_set_src(cat, &bongo_cat_both1);
    lv_obj_align(cat, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    update_screen(NULL);
    lv_timer_create(update_screen, 500, NULL);
    return screen;
}
