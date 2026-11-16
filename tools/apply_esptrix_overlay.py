#!/usr/bin/env python3
"""Apply ESPTRIX UI/RF Lab as one top-level Apps drawer to Bruce source.

Idempotent: preserves Bruce's original root categories and their contents.
Intended for a clean or previously ESPTRIX-patched Bruce 1.16.x / 1.17 source
tree; validates known insertion points and stops if upstream changes them.
"""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parent.parent
menu_cpp = root / "src/core/main_menu.cpp"
menu_h = root / "src/core/main_menu.h"
rf_cpp = root / "src/core/menu_items/RFMenu.cpp"
overlay_root = root / "esptrix/overlay/src"
if not all(p.is_file() for p in (menu_cpp, menu_h, rf_cpp)):
    raise SystemExit("Bruce source not found; bootstrap or migrate Bruce first.")

for subdir, names in (
    ("esptrix", ("esptrix_ui.h", "esptrix_ui.cpp")),
    ("core/menu_items", ("ESPtrixAppsMenu.h", "ESPtrixAppsMenu.cpp")),
):
    source_dir = overlay_root / subdir
    destination_dir = root / "src" / subdir
    destination_dir.mkdir(parents=True, exist_ok=True)
    for name in names:
        shutil.copy2(source_dir / name, destination_dir / name)

def add_once(path, entry, anchor, replacement):
    contents = path.read_text(encoding="utf-8")
    if entry not in contents:
        if anchor not in contents:
            raise SystemExit(f"[STOP] Bruce changed menu anchor in {path}: {anchor!r}")
        contents = contents.replace(anchor, replacement, 1)
    path.write_text(contents, encoding="utf-8")

add_once(
    menu_h, '#include "menu_items/ESPtrixAppsMenu.h"',
    '#include "menu_items/EthernetMenu.h"\n',
    '#include "menu_items/EthernetMenu.h"\n#include "menu_items/ESPtrixAppsMenu.h"\n',
)
add_once(
    menu_h, 'ESPtrixAppsMenu esptrixAppsMenu;',
    '    WifiMenu wifiMenu;\n',
    '    WifiMenu wifiMenu;\n    ESPtrixAppsMenu esptrixAppsMenu;\n',
)
add_once(
    menu_cpp, '&esptrixAppsMenu,',
    '        &configMenu,\n',
    '        &esptrixAppsMenu,\n        &configMenu,\n',
)

# Older ESPTRIX overlays put RF Lab in Bruce's RF menu. Undo only that entry.
rf_source = rf_cpp.read_text(encoding="utf-8")
rf_source = rf_source.replace('#include "esptrix/esptrix_ui.h"\n', '')
rf_source = rf_source.replace('        {"ESPtrix RF Lab", []() { esptrix::rfLabMenu(); }},\n', '')
rf_cpp.write_text(rf_source, encoding="utf-8")
print("ESPTRIX Apps drawer integrated; all original Bruce categories retained.")
