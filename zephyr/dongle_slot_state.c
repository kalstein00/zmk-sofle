#include "dongle_slot_state.h"
void dongle_slot_connect(struct dongle_slot_state *s, uintptr_t c) {
    s->connected = true; s->received = false; s->level = 0; s->connection = c; s->side = DONGLE_SIDE_UNKNOWN;
}
void dongle_slot_disconnect(struct dongle_slot_state *s, uintptr_t c) {
    if (s->connected && s->connection == c) {
        s->connected = false; s->received = false; s->connection = 0; s->level = 0; s->side = DONGLE_SIDE_UNKNOWN;
    }
}
void dongle_slot_position(struct dongle_slot_state *s, uintptr_t c, uint32_t position) {
    if (!s->connected || s->connection != c || position >= 64) return;
    unsigned col = position < 52 ? position % 13 : position - 52;
    s->side = col < 6 ? DONGLE_SIDE_LEFT : DONGLE_SIDE_RIGHT;
}
void dongle_slot_battery(struct dongle_slot_state *s, uintptr_t c, uint8_t n) {
    if (s->connected && s->connection == c && n <= 100) { s->received = true; s->level = n; }
}
