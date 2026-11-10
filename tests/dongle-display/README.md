# Dongle display state checks

These host tests exercise the same state reducer and snapshot adapter linked into the dongle firmware. They cover two independent sources, never-received data, valid 0%, disconnect, reconnect, stale connection identity, and invalid source/percentage. The stubs remove Zephyr locking and Bluetooth runtime, so these checks do not establish concurrency, BLE reception, or screen appearance; the actual device journey remains necessary.

From a Unix environment with a C compiler, at the repository root:

```sh
cc -Wall -Wextra -Werror tests/dongle-display/state_test.c zephyr/dongle_slot_state.c -o /tmp/dongle-state-test && /tmp/dongle-state-test
cc -Wall -Wextra -Werror -Itests/dongle-display/stubs tests/dongle-display/snapshot_test.c zephyr/dongle_battery_snapshot.c zephyr/dongle_slot_state.c -o /tmp/dongle-snapshot-test && /tmp/dongle-snapshot-test
```

To exercise the actual generated ZMK BAS callbacks, first build the dongle candidate, then pass its generated source path to the extractor. The generated C file is temporary and must be removed after the run:

```sh
python tests/dongle-display/extract_callback_test.py /path/to/build/dongle_split_central.c tests/dongle-display/callback_test.generated.c
cc -Wall -Wextra -Wno-unused-parameter -Werror -Itests/dongle-display/stubs tests/dongle-display/callback_test.generated.c zephyr/dongle_battery_snapshot.c zephyr/dongle_slot_state.c -o /tmp/dongle-callback-test && /tmp/dongle-callback-test
rm tests/dongle-display/callback_test.generated.c
```

This checks zero through the actual callback bodies and the snapshot adapter with mocked GATT and queue boundaries. It does not validate radio delivery, scheduling, or peripheral identity.

## OLED simulator

With a C compiler and the pinned LVGL source, run `LVGL_DIR=/path/to/modules/lib/gui/lvgl bash tests/dongle-display/simulator/run.sh /path/to/output.ppm`. The harness renders the actual `dongle_display.c` with a 128×64, one-bit LVGL display, deterministic fake battery and USB data, and emits additional 20%, 21%, blinking and typing frames. It does not exercise Zephyr scheduling, BLE, I²C, or the physical display.
