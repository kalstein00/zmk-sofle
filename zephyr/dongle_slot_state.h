#pragma once
#include <stdbool.h>
#include <stdint.h>
enum dongle_side { DONGLE_SIDE_UNKNOWN, DONGLE_SIDE_LEFT, DONGLE_SIDE_RIGHT };
struct dongle_slot_state { bool connected; bool received; uint8_t level; uintptr_t connection; enum dongle_side side; };
void dongle_slot_connect(struct dongle_slot_state *state, uintptr_t connection);
void dongle_slot_disconnect(struct dongle_slot_state *state, uintptr_t connection);
void dongle_slot_battery(struct dongle_slot_state *state, uintptr_t connection, uint8_t level);
void dongle_slot_position(struct dongle_slot_state *state, uintptr_t connection, uint32_t position);
