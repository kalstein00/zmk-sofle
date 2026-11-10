#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/../../.." && pwd)
lvgl=${LVGL_DIR:?Set LVGL_DIR to the pinned LVGL source}
out=${1:?Provide an output PPM path}
shift
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
find "$lvgl/src" -name '*.c' -print0 | xargs -0 gcc -std=c11 -O0 \
  -I"$root/tests/dongle-display/simulator" -I"$root/tests/dongle-display/simulator/include" \
  -I"$root/zephyr" -I"$lvgl" -DLV_CONF_INCLUDE_SIMPLE \
  "$root/tests/dongle-display/simulator/main.c" "$root/zephyr/dongle_display.c" \
  "$root/zephyr/bongo_cat_images.c" -lm -o "$build/simulator"
"$build/simulator" "$out" "$@"
"$build/simulator" "${out%.ppm}-low-on.ppm" 85 20 1 0
"$build/simulator" "${out%.ppm}-low-off.ppm" 85 20 2 0
"$build/simulator" "${out%.ppm}-above-threshold.ppm" 85 21 1 0
"$build/simulator" "${out%.ppm}-typing.ppm" 85 18 1 1
