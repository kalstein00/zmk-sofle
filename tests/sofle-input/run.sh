#!/usr/bin/env bash
set -eu

# Run from a disposable Linux ZMK workspace with BabbleSim already built.
: "${ZMK_APP:?Set ZMK_APP to the pinned ZMK app directory}"
: "${ZMK_TEST_MODULES:?Set ZMK_TEST_MODULES to the semicolon-separated extra module paths}"
: "${BSIM_OUT_PATH:?Set BSIM_OUT_PATH to the built BabbleSim directory}"
: "${BSIM_COMPONENTS_PATH:?Set BSIM_COMPONENTS_PATH to the BabbleSim components directory}"
repo=$(cd "$(dirname "$0")/../.." && pwd)
app=$(cd "$ZMK_APP" && pwd)
fixture=$(mktemp -d "$app/tests/ble/sofle-input.XXXXXX")
runner=$(mktemp "$app/run-sofle-input.XXXXXX")
trap 'rm -f "$runner"; rm -rf "$fixture"' EXIT
cp "$repo"/tests/sofle-input/{nrf52_bsim.conf,peripheral1.overlay,peripheral2.overlay,events.patterns,snapshot.log} "$fixture/"
sed "s|@REPO@|$repo|g" "$repo/tests/sofle-input/nrf52_bsim.keymap.in" > "$fixture/nrf52_bsim.keymap"
testcase="tests/ble/$(basename "$fixture")"
exe_name=${testcase//\//_}
cat > "$fixture/siblings.txt" <<EOF
./ble_test_central.exe -d=2
./${exe_name}_peripheral1.exe -d=3
./${exe_name}_peripheral2.exe -d=4
EOF
sed -e "s|-DZMK_CONFIG=|-DZMK_EXTRA_MODULES=\"$ZMK_TEST_MODULES\" -DZMK_CONFIG=|g" \
    -e "s|extra_cmake_args=\"-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y\"|extra_cmake_args=\"-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y -DEXTRA_CONF_FILE=$repo/config/sofle_dongle.conf\"|" \
    "$app/run-ble-test.sh" > "$runner"
cd "$app"
bash "$runner" "$testcase"
