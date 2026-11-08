"""Apply #5 snapshot hooks to the pinned ZMK central without editing west sources."""
import hashlib
import pathlib
import sys
src, out = map(pathlib.Path, sys.argv[1:])
s = src.read_text(encoding="utf-8")
if hashlib.sha256(s.encode()).hexdigest() != "535fa1e0c7e5795a8fb271df8cf79a59f68916e8c247d241fef97bc22f4fbd2a":
    raise SystemExit("Unexpected ZMK central source; review the snapshot hooks")
def replace(old, new, count=1):
    global s
    if s.count(old) != count:
        raise SystemExit("Unexpected ZMK central hook count")
    s = s.replace(old, new)
replace('#include <zmk/physical_layouts.h>', '#include <zmk/physical_layouts.h>\n#include "dongle_battery_snapshot.h"')
replace('    uint8_t battery_level = ((uint8_t *)data)[0];', '    uint8_t battery_level = ((uint8_t *)data)[0];\n    dongle_battery_sample(peripheral_slot_index_for_conn(conn), conn, battery_level);', 2)
replace('    confirm_peripheral_slot_conn(conn);\n    split_central_process_connection(conn);', '    confirm_peripheral_slot_conn(conn);\n    dongle_battery_connect(peripheral_slot_index_for_conn(conn), conn);\n    split_central_process_connection(conn);')
replace('    err = release_peripheral_slot_for_conn(conn);', '    dongle_battery_disconnect(peripheral_slot_index_for_conn(conn), conn);\n    err = release_peripheral_slot_for_conn(conn);')
out.write_text(s, encoding="utf-8", newline="\n")
