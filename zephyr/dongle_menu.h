#pragma once
#include <stdint.h>
#include <stdbool.h>

enum dongle_menu_page { MENU_CLOSED, MENU_HOSTS, MENU_ACTIONS, MENU_CONFIRM };
enum dongle_menu_action { MENU_USE, MENU_USE_USB, MENU_DELETE, MENU_DELETE_ALL, MENU_NO_ACTION };
enum { DONGLE_MENU_USB_ENTRY = 4, DONGLE_MENU_OUTPUT_COUNT };
enum { MENU_PROFILE_CONNECT, MENU_PROFILE_DELETE, MENU_PROFILE_CLEAR_ALL, MENU_PROFILE_COUNT };
enum { MENU_CONFIRM_CANCEL, MENU_CONFIRM_DELETE };
struct dongle_menu {
    enum dongle_menu_page page;
    uint8_t profile;
    uint8_t cursor;
    enum dongle_menu_action confirmation;
    int64_t deadline;
};
struct dongle_menu dongle_menu_snapshot(void);
int dongle_menu_request(void);
void dongle_menu_open(struct dongle_menu *menu, uint8_t profile, int64_t now);
void dongle_menu_expire(struct dongle_menu *menu, int64_t now);
enum dongle_menu_action dongle_menu_key(struct dongle_menu *menu, uint32_t position, int64_t now);
