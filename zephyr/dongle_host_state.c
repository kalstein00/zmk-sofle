#include "dongle_host_state.h"

void dongle_host_state_begin(struct dongle_host_state *state) {
    state->switching = true;
    state->generation++;
}

bool dongle_host_state_position(struct dongle_host_state *state, uint8_t source,
                                uint32_t position, bool pressed) {
    if (source >= DONGLE_HOST_SOURCES || position >= DONGLE_HOST_POSITIONS) {
        return true;
    }
    uint64_t bit = UINT64_C(1) << position;
    if (pressed) {
        if (state->switching && !(state->held[source] & bit)) {
            state->swallowed[source] |= bit;
        }
        state->held[source] |= bit;
        return (state->swallowed[source] & bit) != 0;
    }
    bool swallow = (state->swallowed[source] & bit) != 0;
    state->held[source] &= ~bit;
    state->swallowed[source] &= ~bit;
    return swallow;
}

bool dongle_host_state_released(const struct dongle_host_state *state) {
    for (unsigned i = 0; i < DONGLE_HOST_SOURCES; i++) {
        if (state->held[i]) {
            return false;
        }
    }
    return true;
}
