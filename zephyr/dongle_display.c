#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include "dongle_battery_snapshot.h"

LV_IMG_DECLARE(bongo_cat_none);
LV_IMG_DECLARE(bongo_cat_left1);
LV_IMG_DECLARE(bongo_cat_right1);
LV_IMG_DECLARE(bongo_cat_both1_open);

static lv_obj_t *labels[2], *bars[2], *warnings[2], *usb_label, *layer_label, *cat;
static unsigned ticks;
static uint32_t previous_activity;
static unsigned cat_ticks;

static void update_screen(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    struct dongle_slot_state sides[2] = {0};
    for (unsigned i = 0; i < 2; ++i) {
        struct dongle_slot_state s = dongle_battery_get(i);
        if (s.connected && s.side >= DONGLE_SIDE_LEFT && s.side <= DONGLE_SIDE_RIGHT)
            sides[s.side - DONGLE_SIDE_LEFT] = s;
    }
    for (unsigned i = 0; i < 2; ++i) {
        struct dongle_slot_state s = sides[i];
        if (!s.connected || !s.received) {
            lv_label_set_text_fmt(labels[i], "%s --", i ? "RIGHT" : "LEFT ");
            lv_obj_add_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text_fmt(labels[i], "%s %u%%", i ? "RIGHT" : "LEFT ", s.level);
            lv_bar_set_value(bars[i], s.level, LV_ANIM_OFF);
            lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            if (s.level <= 20 && (ticks % 4) < 2) lv_obj_clear_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_label_set_text(usb_label, zmk_usb_is_hid_ready() ? "USB: OK" : "USB: --");
    lv_label_set_text_fmt(layer_label, "Layer: %u", (unsigned)zmk_keymap_highest_layer_active());
    uint32_t activity = dongle_key_activity();
    if (activity != previous_activity) { previous_activity = activity; cat_ticks = 4; }
    if (cat_ticks) {
        lv_img_set_src(cat, (ticks & 1) ? &bongo_cat_left1 : &bongo_cat_right1);
        --cat_ticks;
    } else {
        lv_img_set_src(cat, (ticks % 32 < 2) ? &bongo_cat_both1_open : &bongo_cat_none);
    }
    ++ticks;
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_text_font(screen, &lv_font_unscii_8, LV_PART_MAIN);
    for (unsigned i = 0; i < 2; ++i) {
        labels[i] = lv_label_create(screen);
        lv_obj_set_pos(labels[i], 0, i * 14);
        bars[i] = lv_bar_create(screen);
        lv_obj_set_pos(bars[i], 82, i * 14 + 1);
        lv_obj_set_size(bars[i], 39, 7);
        lv_bar_set_range(bars[i], 0, 100);
        warnings[i] = lv_label_create(screen);
        lv_obj_set_pos(warnings[i], 121, i * 14);
        lv_label_set_text(warnings[i], "!");
    }
    usb_label = lv_label_create(screen);
    lv_obj_set_pos(usb_label, 0, 28);
    layer_label = lv_label_create(screen);
    lv_obj_set_pos(layer_label, 0, 40);
    cat = lv_img_create(screen);
    lv_img_set_src(cat, &bongo_cat_none);
    lv_obj_align(cat, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    update_screen(NULL);
    lv_timer_create(update_screen, 250, NULL);
    return screen;
}
