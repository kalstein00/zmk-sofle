#pragma once
#include <stdint.h>
#include <zephyr/bluetooth/conn.h>
#include "dongle_slot_state.h"
void dongle_battery_connect(int source, struct bt_conn *conn);
void dongle_battery_disconnect(int source, struct bt_conn *conn);
void dongle_battery_sample(int source, struct bt_conn *conn, uint8_t level);
void dongle_battery_position(int source, struct bt_conn *conn, uint32_t position);
uint32_t dongle_key_activity(void);
struct dongle_slot_state dongle_battery_get(unsigned source);
