#include <stdio.h>
#include "../../zephyr/dongle_battery_snapshot.h"
#define CHECK(c, text) do { if (!(c)) { puts("FAIL: " text); return 1; } } while (0)
int main(void) {
    struct bt_conn a={1}, b={2}, c={3};
    dongle_battery_connect(0, &a);
    dongle_battery_connect(1, &b);
    dongle_battery_sample(0, &a, 0);
    dongle_battery_sample(1, &b, 89);
    CHECK(dongle_battery_get(0).received && dongle_battery_get(0).level == 0, "zero is valid");
    CHECK(dongle_battery_get(1).received && dongle_battery_get(1).level == 89, "independent slot");
    dongle_battery_position(0, &a, 20);
    dongle_battery_position(1, &b, 14);
    CHECK(dongle_battery_get(0).side == DONGLE_SIDE_RIGHT && dongle_battery_get(1).side == DONGLE_SIDE_LEFT, "side follows key position, not slot order");
    dongle_battery_disconnect(0, &a);
    CHECK(!dongle_battery_get(0).connected && !dongle_battery_get(0).received, "disconnect clears reading");
    dongle_battery_connect(0, &c);
    dongle_battery_sample(0, &a, 9);
    dongle_battery_disconnect(0, &a);
    CHECK(dongle_battery_get(0).connected && !dongle_battery_get(0).received, "stale session ignored");
    dongle_battery_sample(0, &c, 101);
    CHECK(!dongle_battery_get(0).received, "invalid percentage ignored");
    dongle_battery_sample(-1, &c, 50);
    dongle_battery_connect(2, &c);
    CHECK(!dongle_battery_get(2).connected && dongle_battery_get(1).level == 89, "invalid slot ignored");
    puts("PASS: callback snapshot lifecycle");
    return 0;
}
