#include "dongle_menu.h"

/* Positions in the Sofle logical transform, independent of DYA bindings. */
enum { ESC = 0, UP = 6, DOWN = 19, LEFT = 32, RIGHT = 45,
       KEY_Q = 14, KEY_W = 15, KEY_E = 16, KEY_S = 28,
       ENTER_OUTER = 51, ENTER_CENTER = 58, ENTER_THUMB = 60, ESC_THUMB = 63 };

void dongle_menu_open(struct dongle_menu *menu, uint8_t profile, int64_t now) {
    *menu = (struct dongle_menu){.page = MENU_HOSTS, .profile = profile, .deadline = now + 30000};
}

void dongle_menu_expire(struct dongle_menu *menu, int64_t now) {
    if (now >= menu->deadline) menu->page = MENU_CLOSED;
}

enum dongle_menu_action dongle_menu_key(struct dongle_menu *menu, uint32_t pos, int64_t now) {
    menu->deadline = now + 30000;
    bool enter = pos == ENTER_OUTER || pos == ENTER_CENTER || pos == ENTER_THUMB || pos == KEY_E;
    bool up = pos == UP || pos == KEY_W;
    bool down = pos == DOWN || pos == KEY_S;
    if (pos == ESC || pos == ESC_THUMB || pos == LEFT || pos == KEY_Q) {
        if (menu->page == MENU_HOSTS) menu->page = MENU_CLOSED;
        else if (menu->page == MENU_ACTIONS) menu->page = MENU_HOSTS;
        else if (menu->page == MENU_CONFIRM) menu->page = MENU_ACTIONS;
    } else if (menu->page == MENU_HOSTS) {
        if (up) menu->profile = (menu->profile + DONGLE_MENU_OUTPUT_COUNT - 1) % DONGLE_MENU_OUTPUT_COUNT;
        else if (down) menu->profile = (menu->profile + 1) % DONGLE_MENU_OUTPUT_COUNT;
        else if (enter || pos == RIGHT) {
            if (menu->profile == DONGLE_MENU_USB_ENTRY) {
                menu->page = MENU_CLOSED;
                return MENU_USE_USB;
            }
            menu->page = MENU_ACTIONS;
            menu->cursor = MENU_PROFILE_CONNECT;
        }
    } else if (menu->page == MENU_ACTIONS) {
        if (up) menu->cursor = (menu->cursor + MENU_PROFILE_COUNT - 1) % MENU_PROFILE_COUNT;
        else if (down) menu->cursor = (menu->cursor + 1) % MENU_PROFILE_COUNT;
        else if (enter || pos == RIGHT) {
            if (menu->cursor == MENU_PROFILE_CONNECT) {
                menu->page = MENU_CLOSED;
                return MENU_USE;
            }
            menu->confirmation = menu->cursor == MENU_PROFILE_DELETE ? MENU_DELETE : MENU_DELETE_ALL;
            menu->page = MENU_CONFIRM;
            menu->cursor = MENU_CONFIRM_CANCEL;
        }
    } else if (menu->page == MENU_CONFIRM) {
        if (up || down || pos == RIGHT) {
            menu->cursor = menu->cursor == MENU_CONFIRM_CANCEL ? MENU_CONFIRM_DELETE : MENU_CONFIRM_CANCEL;
        }
        else if (enter) {
            if (menu->cursor == MENU_CONFIRM_DELETE) {
                menu->page = MENU_CLOSED;
                return menu->confirmation;
            }
            menu->cursor = MENU_PROFILE_CONNECT;
            menu->page = MENU_ACTIONS;
        }
    }
    return MENU_NO_ACTION;
}
