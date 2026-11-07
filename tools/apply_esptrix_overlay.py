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
for name in ("esptrix_ui.h", "esptrix_ui.cpp", "esptrix_signal_meter.h", "esptrix_home.h", "esptrix_home.cpp"):
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

# ESPTRIX native launcher remains a one-entry opt-in menu for T-Embed CC1101.
main_menu = root / "src/core/main_menu.cpp"
text = main_menu.read_text(encoding="utf-8")
home_include = '#include "esptrix/esptrix_home.h"\\n'.replace("\\n", "\n")
if home_include not in text:
    anchor = '#include "main_menu.h"\\n'.replace("\\n", "\n")
    if anchor not in text:
        raise SystemExit("Bruce main_menu.cpp changed: missing include")
    text = text.replace(anchor, anchor + home_include, 1)
home_entry = '''    // ESPTRIX is opt-in during early hardware validation. Retain Bruce's
    // verified default first item and icon renderer at initial startup.
#if defined(T_EMBED_1101)
    options.push_back({"ESPTRIX HOME", []() { esptrix::showHome(); }});
#endif
'''
if home_entry not in text:
    anchor = '    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);'
    if anchor not in text:
        raise SystemExit("Bruce main_menu.cpp changed: missing menu entry point")
    text = text.replace(anchor, home_entry + anchor, 1)
main_menu.write_text(text, encoding="utf-8")
