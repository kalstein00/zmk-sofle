#pragma once
#include <stdbool.h>
#include <stdint.h>
struct dongle_slot_state { bool connected; bool received; uint8_t level; uintptr_t connection; };
void dongle_slot_connect(struct dongle_slot_state *state, uintptr_t connection);
void dongle_slot_disconnect(struct dongle_slot_state *state, uintptr_t connection);
void dongle_slot_battery(struct dongle_slot_state *state, uintptr_t connection, uint8_t level);
