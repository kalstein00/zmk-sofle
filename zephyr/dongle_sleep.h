#pragma once
#include <stdbool.h>
#include <stdint.h>
bool dongle_sleep_blanked(void);
struct bt_conn;
void dongle_sleep_connection(int slot, struct bt_conn *conn, bool connected);
struct bt_conn *dongle_sleep_target_ref(unsigned slot, uint32_t generation);
void dongle_sleep_send_result(unsigned slot, uint32_t generation, int result);
void dongle_sleep_input_received(int slot, struct bt_conn *conn);
