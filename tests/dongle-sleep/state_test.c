#include <assert.h>
#include "../../zephyr/dongle_sleep_state.h"

int main(void) {
    struct dongle_sleep_state s = {0};
    dongle_sleep_connected(&s, 0);
    dongle_sleep_connected(&s, 1);
    assert(dongle_sleep_begin(&s) == 3);
    uint32_t old_generation = s.generation[0];
    assert(s.blanked);
    assert(!dongle_sleep_key(&s, 0, false));
    assert(!dongle_sleep_key(&s, 0, true));
    dongle_sleep_disconnected(&s, 0);
    assert(!dongle_sleep_key(&s, 0, true));
    dongle_sleep_connected(&s, 0);
    assert(!dongle_sleep_target_current_state(&s, 0, old_generation));
    assert(!dongle_sleep_key(&s, 0, false));
    assert(dongle_sleep_key(&s, 0, true));
    assert(!s.blanked);
    dongle_sleep_disconnected(&s, 1);
    assert(dongle_sleep_begin(&s) == 1);
    dongle_sleep_disconnected(&s, 0);
    dongle_sleep_connected(&s, 0);
    assert(dongle_sleep_key(&s, 0, true));
    dongle_sleep_begin(&s);
    assert(dongle_sleep_send_failed(&s, 0));
    assert(!s.blanked);
    return 0;
}
