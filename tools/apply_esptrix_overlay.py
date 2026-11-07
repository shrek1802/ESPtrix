#!/usr/bin/env python3
"""Integrate ESPTRIX RF Lab into the exact Bruce 1.16.1 source tree.

Idempotent and deliberately narrow: retains Bruce's original features.
"""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parent.parent
upstream_menu = root / "src/core/menu_items/RFMenu.cpp"
overlay_src = root / "esptrix/overlay/src/esptrix"

if not upstream_menu.is_file():
    raise SystemExit("Bruce upstream source not present; run bootstrap first.")

target = root / "src/esptrix"
target.mkdir(parents=True, exist_ok=True)
for name in ("esptrix_ui.h", "esptrix_ui.cpp"):
    shutil.copy2(overlay_src / name, target / name)

source = upstream_menu.read_text(encoding="utf-8")
include = '#include "esptrix/esptrix_ui.h"\n'
if include not in source:
    anchor = '#include "RFMenu.h"\n'
    if anchor not in source:
        raise SystemExit("Bruce RFMenu.cpp changed: missing include anchor")
    source = source.replace(anchor, anchor + include, 1)

entry = '        {"ESPtrix RF Lab", []() { esptrix::rfLabMenu(); }},\n'
if entry not in source:
    anchor = '    options = {\n'
    if anchor not in source:
        raise SystemExit("Bruce RFMenu.cpp changed: missing menu anchor")
    source = source.replace(anchor, anchor + entry, 1)

upstream_menu.write_text(source, encoding="utf-8")
print("ESPtrix RF Lab integrated into Bruce RF menu (receive-only milestone).")
