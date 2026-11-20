#pragma once
#include <stdbool.h>
#include <stdint.h>

#define DONGLE_LOG_CAPACITY 256

static inline unsigned dongle_log_slot(uint32_t sequence) {
    return (sequence - 1u) % DONGLE_LOG_CAPACITY;
}

static inline bool dongle_log_is_newer(uint32_t sequence, uint32_t latest) {
    uint32_t distance = sequence - latest;
    return distance != 0 && distance < (UINT32_C(1) << 31);
}
