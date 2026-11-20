#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DONGLE_HOST_SOURCES 2
#define DONGLE_HOST_POSITIONS 64

/* Physical input state, independent of saved keymaps and HID modifier counts. */
struct dongle_host_state {
    uint64_t held[DONGLE_HOST_SOURCES];
    uint64_t swallowed[DONGLE_HOST_SOURCES];
    uint32_t generation;
    bool switching;
};

void dongle_host_state_begin(struct dongle_host_state *state);
bool dongle_host_state_position(struct dongle_host_state *state, uint8_t source,
                                uint32_t position, bool pressed);
bool dongle_host_state_released(const struct dongle_host_state *state);
