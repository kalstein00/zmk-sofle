#include <stdio.h>
#include "../../zephyr/dongle_slot_state.h"
#define CHECK(c, text) do { if (!(c)) { puts("FAIL: " text); return 1; } } while (0)
int main(void) {
    struct dongle_slot_state slot = {0};
    CHECK(!slot.connected && !slot.received, "initial unavailable");
    dongle_slot_connect(&slot, 1);
    CHECK(slot.connected && !slot.received, "connected without battery stays unknown");
    dongle_slot_battery(&slot, 1, 0);
    CHECK(slot.connected && slot.received && slot.level == 0, "valid zero is received");
    dongle_slot_disconnect(&slot, 1);
    CHECK(!slot.connected && !slot.received, "disconnect invalidates old battery");
    dongle_slot_connect(&slot, 2);
    CHECK(slot.connected && !slot.received, "reconnect awaits new battery");
    CHECK(slot.side == DONGLE_SIDE_UNKNOWN, "side is unknown before input");
    dongle_slot_position(&slot, 1, 0);
    CHECK(slot.side == DONGLE_SIDE_UNKNOWN, "stale input cannot identify side");
    dongle_slot_position(&slot, 2, 14);
    CHECK(slot.side == DONGLE_SIDE_LEFT, "left Q identifies left side");
    dongle_slot_position(&slot, 2, 20);
    CHECK(slot.side == DONGLE_SIDE_RIGHT, "right Y identifies right side");
    dongle_slot_position(&slot, 2, 64);
    CHECK(slot.side == DONGLE_SIDE_RIGHT, "invalid position cannot change side");
    dongle_slot_battery(&slot, 1, 90);
    dongle_slot_disconnect(&slot, 1);
    CHECK(slot.connected && !slot.received, "stale connection cannot alter new session");
    dongle_slot_battery(&slot, 2, 73);
    CHECK(slot.connected && slot.received && slot.level == 73, "new session accepts battery");
    puts("PASS: split battery lifecycle");
    return 0;
}
