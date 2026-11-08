# Dongle deep sleep state checks

Run from the repository root with a C compiler:

```sh
cc -Wall -Wextra -Werror tests/dongle-sleep/state_test.c -o /tmp/dongle-sleep-test && /tmp/dongle-sleep-test
```

This checks the slot lifecycle, stale connection generation, release filtering and detected-send-failure state. It does not exercise the BLE command queue, hardware power-off, the display work queue, HID reports or timing; those require the actual device journey.
