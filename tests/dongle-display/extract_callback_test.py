"""Extract production-generated callbacks into a host harness; no callback rewrite."""
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text(encoding="utf-8")
template = Path(__file__).with_name("callback_test.c.in").read_text(encoding="utf-8")
def extract(signature):
    begin = source.index(signature)
    start = source.index("{", begin)
    pos, depth = start + 1, 1
    while depth:
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    return source[begin:pos]
parts = [extract("static uint8_t " + name + "(") for name in (
    "split_central_battery_level_notify_func", "split_central_battery_level_read_func")]
Path(sys.argv[2]).write_text(template.replace("/* CALLBACKS */", "\n\n".join(parts)), encoding="utf-8")
