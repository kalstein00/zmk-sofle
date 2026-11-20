#pragma once

#include <stdbool.h>
#include <stdint.h>

enum dongle_pc_status {
    DONGLE_PC_EMPTY,
    DONGLE_PC_PAIRING,
    DONGLE_PC_PAIRED,
    DONGLE_PC_CONNECTED,
    DONGLE_PC_FAILED,
    DONGLE_PC_TIMEOUT,
};

struct dongle_pc_snapshot {
    uint8_t profile;
    enum dongle_pc_status status;
};

struct dongle_pc_snapshot dongle_pc_snapshot(void);
enum dongle_pc_status dongle_pc_profile_status(uint8_t profile);
bool dongle_output_is_ble(void);
bool dongle_output_switching(void);
int dongle_local_battery_percent(void);
