#include <assert.h>
#include <stdio.h>
#include "dongle_host_state.h"

int main(void) {
    struct dongle_host_state state = {0};
    assert(dongle_host_state_released(&state));
    assert(!dongle_host_state_position(&state, 0, 53, true)); /* Existing Ctrl. */
    assert(!dongle_host_state_position(&state, 1, 61, true)); /* Profile layer. */
    dongle_host_state_begin(&state);
    assert(state.switching && state.generation == 1);
    assert(!dongle_host_state_released(&state));
    /* Old releases must still reach hold-tap/layer/HID cleanup. */
    assert(!dongle_host_state_position(&state, 0, 53, false));
    /* New presses and their releases are consumed, never replayed. */
    assert(dongle_host_state_position(&state, 0, 14, true));
    assert(dongle_host_state_position(&state, 0, 14, false));
    assert(!dongle_host_state_released(&state));
    assert(!dongle_host_state_position(&state, 1, 61, false));
    assert(dongle_host_state_released(&state));
    state.switching = false;
    assert(!dongle_host_state_position(&state, 0, 14, true));
    assert(!dongle_host_state_position(&state, 0, 14, false));
    dongle_host_state_begin(&state);
    assert(state.generation == 2);
    assert(dongle_host_state_position(&state, 2, 0, true));
    assert(dongle_host_state_position(&state, 0, 64, true));
    assert(dongle_host_state_released(&state));
    puts("PASS: old held-key releases pass; transition presses are consumed; next fresh press works");
}
