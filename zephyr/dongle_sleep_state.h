#pragma once
#include <stdbool.h>
#include <stdint.h>

struct dongle_sleep_state {
    bool blanked;
    uint8_t disconnected;
    uint8_t reconnected;
    uint8_t connected;
    uint32_t generation[2];
};

static inline uint8_t dongle_sleep_begin(struct dongle_sleep_state *s) {
    s->blanked = true;
    s->disconnected = (uint8_t)~s->connected;
    s->reconnected = 0;
    return s->connected;
}
static inline void dongle_sleep_disconnected(struct dongle_sleep_state *s, unsigned slot) {
    if (slot < 2) { s->connected &= ~(1u << slot); s->disconnected |= 1u << slot; s->reconnected &= ~(1u << slot); }
}
static inline void dongle_sleep_connected(struct dongle_sleep_state *s, unsigned slot) {
    if (slot >= 2) return;
    ++s->generation[slot];
    s->connected |= 1u << slot;
    if (s->blanked && slot < 2 && (s->disconnected & (1u << slot)))
        s->reconnected |= 1u << slot;
}
static inline bool dongle_sleep_send_failed(struct dongle_sleep_state *s, unsigned slot) {
    if (s->blanked && slot < 2 && (s->connected & (1u << slot))) {
        s->blanked = false;
        return true;
    }
    return false;
}
static inline bool dongle_sleep_target_current_state(const struct dongle_sleep_state *s, unsigned slot, uint32_t generation) {
    return slot < 2 && (s->connected & (1u << slot)) && s->generation[slot] == generation;
}
static inline bool dongle_sleep_key(struct dongle_sleep_state *s, unsigned slot, bool pressed) {
    if (s->blanked && pressed && slot < 2 && (s->reconnected & (1u << slot))) {
        s->blanked = false;
        return true;
    }
    return false;
}
