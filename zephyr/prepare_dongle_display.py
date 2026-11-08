"""Guard unblanking in the pinned display implementation during deep sleep."""
import hashlib
import pathlib
import sys

src, out = map(pathlib.Path, sys.argv[1:])
s = src.read_text(encoding="utf-8")
if hashlib.sha256(s.encode()).hexdigest() != "842ef56bb4815031efb0b40d122df589c231e3d29f9162d0d8d4685adf5b9715":
    raise SystemExit("Unexpected ZMK display source; review the sleep hook")
old = 'void unblank_display_cb(struct k_work *work) {'
if s.count(old) != 1:
    raise SystemExit("Unexpected display hook count")
s = s.replace(old, '#include "dongle_sleep.h"\n' + old + '\n    if (dongle_sleep_blanked()) return;')
out.write_text(s, encoding="utf-8", newline="\n")
