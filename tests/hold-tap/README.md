# Home-row mod-tap behavior

These fixtures use the pinned ZMK native test runner and the same `config/hold_tap.dtsi` included in the firmware. `fast-roll` checks that nested and rolling D/M taps produce ordinary letters and a long D hold produces Ctrl+M. `quick-repeat` checks that tapping then holding D resolves the second press as Ctrl, and tapping then holding Space activates a layer. `layer-rapid-taps` checks that ten isolated Space layer-taps with 30ms presses and 30ms gaps produce ten taps. They do not exercise BLE timing, hardware debounce, OS composition, or the saved Studio keymap.

From the pinned ZMK app directory, after preparing its native POSIX build environment, run:

```sh
bash run-test.sh /absolute/path/to/zmk-sofle/tests/hold-tap/fast-roll
bash run-test.sh /absolute/path/to/zmk-sofle/tests/hold-tap/quick-repeat
bash run-test.sh /absolute/path/to/zmk-sofle/tests/hold-tap/layer-rapid-taps
```
