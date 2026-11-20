#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include "dongle_log.h"

#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#define BT_LE_SCAN_PASSIVE 0
#define split_central_device_found NULL
#define LOG_DBG(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define ARG_UNUSED(x) (void)(x)
#define K_MSEC(ms) (ms)
struct k_work { int unused; };
struct k_work_delayable { void (*handler)(struct k_work *); };
#define K_WORK_DELAYABLE_DEFINE(name, fn) static struct k_work_delayable name = {fn}
static struct k_work_delayable *pending_scan;
static void k_work_reschedule(struct k_work_delayable *work, int ms) {
    assert(ms == 500);
    pending_scan = work;
}
static bool is_enabled = true, is_scanning;
static struct { void *conn; } peripherals[2];
static int scan_error = -EBUSY;
static unsigned attempts;
static int bt_le_scan_start(int type, void *callback) {
    attempts++;
    return scan_error;
}

static int start_scanning(void);
#include "scan.generated.c"

int main(void) {
    assert(start_scanning() == -EBUSY);
    assert(!is_scanning && pending_scan);
    scan_error = 0;
    struct k_work_delayable *retry = pending_scan;
    pending_scan = NULL;
    retry->handler(NULL);
    assert(attempts == 2 && is_scanning && !pending_scan);
    assert(!start_scanning() && attempts == 2);

    is_scanning = false;
    scan_error = -EBUSY;
    assert(start_scanning() == -EBUSY && pending_scan);
    is_enabled = false;
    retry = pending_scan;
    pending_scan = NULL;
    retry->handler(NULL);
    assert(attempts == 3 && !is_scanning && !pending_scan);
    puts("PASS: failed split scan schedules retry and disabled transport suppresses it");
}
