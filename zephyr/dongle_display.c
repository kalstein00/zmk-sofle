#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include "dongle_battery_snapshot.h"
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
#include "dongle_status.h"
#include "dongle_menu.h"
static lv_obj_t *profile_label, *local_battery_label;
static lv_obj_t *status_panel, *menu_label;

static const char *pc_status_text(enum dongle_pc_status status) {
    switch (status) {
    case DONGLE_PC_PAIRING: return "Pairing";
    case DONGLE_PC_PAIRED: return "Paired";
    case DONGLE_PC_CONNECTED: return "Connected";
    case DONGLE_PC_FAILED: return "Pair failed";
    case DONGLE_PC_TIMEOUT: return "Timeout";
    default: return "Empty";
    }
}

static void render_menu(struct dongle_menu menu) {
    if (menu.page == MENU_CLOSED) {
        lv_obj_add_flag(menu_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(status_panel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(status_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(menu_label, LV_OBJ_FLAG_HIDDEN);
    if (menu.page == MENU_HOSTS) {
        lv_label_set_text_fmt(menu_label, "OUTPUT\n%c0 %s\n%c1 %s\n%c2 %s\n%c3 %s\n%cUSB output\nCenter U/D/OK",
            menu.profile == 0 ? '>' : ' ', pc_status_text(dongle_pc_profile_status(0)),
            menu.profile == 1 ? '>' : ' ', pc_status_text(dongle_pc_profile_status(1)),
            menu.profile == 2 ? '>' : ' ', pc_status_text(dongle_pc_profile_status(2)),
            menu.profile == 3 ? '>' : ' ', pc_status_text(dongle_pc_profile_status(3)),
            menu.profile == DONGLE_MENU_USB_ENTRY ? '>' : ' ');
    } else if (menu.page == MENU_ACTIONS) {
        enum dongle_pc_status status = dongle_pc_profile_status(menu.profile);
        bool paired = status == DONGLE_PC_PAIRED || status == DONGLE_PC_CONNECTED;
        lv_label_set_text_fmt(menu_label, "PROFILE %u\n%c%s\n%cDelete bond\n%cClear all PCs\nCenter U/D/OK\nCenter L: Back",
            (unsigned)menu.profile, menu.cursor == MENU_PROFILE_CONNECT ? '>' : ' ', paired ? "Connect" : "Start pairing",
            menu.cursor == MENU_PROFILE_DELETE ? '>' : ' ',
            menu.cursor == MENU_PROFILE_CLEAR_ALL ? '>' : ' ');
    } else {
        if (menu.confirmation == MENU_DELETE_ALL) {
            lv_label_set_text_fmt(menu_label, "Clear all PCs?\n%cCancel\n%cDelete\nCenter U/D/OK",
                menu.cursor == MENU_CONFIRM_CANCEL ? '>' : ' ',
                menu.cursor == MENU_CONFIRM_DELETE ? '>' : ' ');
        } else {
            lv_label_set_text_fmt(menu_label, "Delete bond P%u?\n%cCancel\n%cDelete\nCenter U/D/OK",
                (unsigned)menu.profile, menu.cursor == MENU_CONFIRM_CANCEL ? '>' : ' ',
                menu.cursor == MENU_CONFIRM_DELETE ? '>' : ' ');
        }
    }
}
#endif

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
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    struct dongle_menu menu = dongle_menu_snapshot();
    render_menu(menu);
    if (menu.page != MENU_CLOSED) return;
#endif
    struct dongle_slot_state sides[2] = {0};
    for (unsigned i = 0; i < 2; ++i) {
        struct dongle_slot_state s = dongle_battery_get(i);
        if (s.connected && s.side >= DONGLE_SIDE_LEFT && s.side <= DONGLE_SIDE_RIGHT)
            sides[s.side - DONGLE_SIDE_LEFT] = s;
    }
    for (unsigned i = 0; i < 2; ++i) {
        struct dongle_slot_state s = sides[i];
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        const char *side = i ? "R" : "L";
#else
        const char *side = i ? "RIGHT" : "LEFT ";
#endif
        if (!s.connected || !s.received) {
            lv_label_set_text_fmt(labels[i], "%s --", side);
            lv_obj_add_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text_fmt(labels[i], "%s %u%%", side, s.level);
            lv_bar_set_value(bars[i], s.level, LV_ANIM_OFF);
            lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            if (s.level <= 20 && (ticks % 4) < 2) lv_obj_clear_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    struct dongle_pc_snapshot pc = dongle_pc_snapshot();
    bool ble = dongle_output_is_ble();
    lv_label_set_text_fmt(profile_label, "%s P%u", ble ? "BLE" : "USB", (unsigned)pc.profile);
    const char *status = dongle_output_switching() ? "Switching" : pc_status_text(pc.status);
    if (!ble && !zmk_usb_is_hid_ready() && !dongle_output_switching()) {
        lv_label_set_text_fmt(usb_label, "%s/U-", status);
    } else {
        lv_label_set_text(usb_label, status);
    }
    int battery = dongle_local_battery_percent();
    if (battery < 0) lv_label_set_text(local_battery_label, "D --");
    else lv_label_set_text_fmt(local_battery_label, "D %d%%", battery);
    lv_label_set_text_fmt(layer_label, "Lyr %u", (unsigned)zmk_keymap_highest_layer_active());
#else
    lv_label_set_text(usb_label, zmk_usb_is_hid_ready() ? "USB: OK" : "USB: --");
    lv_label_set_text_fmt(layer_label, "Layer: %u", (unsigned)zmk_keymap_highest_layer_active());
#endif
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
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    status_panel = lv_obj_create(screen);
    lv_obj_remove_style_all(status_panel);
    lv_obj_clear_flag(status_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(status_panel, 128, 64);
    lv_obj_t *parent = status_panel;
    menu_label = lv_label_create(screen);
    lv_obj_set_pos(menu_label, 0, 0);
    lv_obj_set_style_text_line_space(menu_label, 0, LV_PART_MAIN);
#else
    lv_obj_t *parent = screen;
#endif
    for (unsigned i = 0; i < 2; ++i) {
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
        int y = 27 + i * 14, bar_x = 49, bar_width = 20, warning_x = 69;
#else
        int y = i * 14, bar_x = 82, bar_width = 39, warning_x = 121;
#endif
        labels[i] = lv_label_create(parent);
        lv_obj_set_pos(labels[i], 0, y);
        bars[i] = lv_bar_create(parent);
        lv_obj_set_pos(bars[i], bar_x, y + 1);
        lv_obj_set_size(bars[i], bar_width, 7);
        lv_bar_set_range(bars[i], 0, 100);
        warnings[i] = lv_label_create(parent);
        lv_obj_set_pos(warnings[i], warning_x, y);
        lv_label_set_text(warnings[i], "!");
    }
    usb_label = lv_label_create(parent);
    layer_label = lv_label_create(parent);
#ifdef CONFIG_SHIELD_SOFLE_DONGLE_WIRELESS
    profile_label = lv_label_create(parent);
    local_battery_label = lv_label_create(parent);
    lv_obj_set_pos(profile_label, 0, 0);
    lv_obj_set_pos(local_battery_label, 72, 0);
    lv_obj_set_pos(usb_label, 0, 12);
    lv_obj_set_pos(layer_label, 0, 55);
#else
    lv_obj_set_pos(usb_label, 0, 28);
    lv_obj_set_pos(layer_label, 0, 40);
#endif
    cat = lv_img_create(parent);
    lv_img_set_src(cat, &bongo_cat_none);
    lv_obj_align(cat, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    update_screen(NULL);
    lv_timer_create(update_screen, 250, NULL);
    return screen;
}
