#pragma once
#include <stdint.h>
#include <zephyr/bluetooth/conn.h>
#include "dongle_slot_state.h"
void dongle_battery_connect(int source, struct bt_conn *conn);
void dongle_battery_disconnect(int source, struct bt_conn *conn);
void dongle_battery_sample(int source, struct bt_conn *conn, uint8_t level);
struct dongle_slot_state dongle_battery_get(unsigned source);
