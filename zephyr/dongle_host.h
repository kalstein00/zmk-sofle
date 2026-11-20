#pragma once

#include <zephyr/bluetooth/conn.h>
#include <zmk/endpoints_types.h>

/* Hooks used only by the separately generated wireless dongle sources. */
bool dongle_host_position(uint8_t source, uint32_t position, bool pressed);
bool dongle_host_reports_allowed(void);
bool dongle_host_transport_allowed(enum zmk_transport transport);
uint32_t dongle_host_generation(void);
void dongle_host_connected(void);
void dongle_host_input_lock(void);
void dongle_host_input_unlock(void);

/* Serialized BLE primitives implemented in the generated pinned ble.c. */
int dongle_ble_select_profile(uint8_t index);
int dongle_ble_clear_profiles(bool all);
int dongle_ble_clear_slot(uint8_t profile);
int dongle_host_clear_profile(uint8_t profile);
void dongle_host_restore_selection(uint8_t *profile);
int dongle_ble_apply_profile(uint8_t profile);
void dongle_ble_pause(void);
void dongle_ble_resume(void);
int dongle_endpoints_select_transport(enum zmk_transport transport);
bool dongle_behavior_queue_idle(void);
bool dongle_sticky_idle(void);
void dongle_sticky_reset(void);
bool dongle_mouse_idle(void);
void dongle_mouse_reset(void);
int dongle_hog_release(void);
int dongle_usb_release(void);
