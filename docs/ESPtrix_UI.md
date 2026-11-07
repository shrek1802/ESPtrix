# ESPTRIX Interface — Flipper-style carousel (development)

ESPTRIX is an independent AGPL-licensed derivative of Bruce Firmware.

## Main screen (320×170 T-Embed CC1101)

ESPTRIX now starts on a **horizontal icon carousel**, not 3×2 square tiles. Three original vector icons are visible, with the selected icon highlighted by a blue ring. The selected feature's name and description appear beneath it.

The device keeps a simple electric-blue, navy, white colour palette; original Bruce tools remain accessible.

- Rotate encoder: previous/next icon.
- Press encoder: open a vertical, scrollable list for that category.
- Rotate: move down/up a list; press to launch selected item.
- Back: return to carousel; Back from carousel enters Bruce mode.
- Select BRUCE icon: enter Bruce's original carousel. Bruce has ESPTRIX HOME to switch back.
- Hold physical Back during the startup menu transition: boot directly to Bruce.

## Categories

KEY FOB, TPMS, RF TOOLS, WI-FI, BLUETOOTH, NFC/RFID, INFRARED, FILES, MORE, SETTINGS, BRUCE, ABOUT.

Key Fob and TPMS use **native ESPTRIX frequency-selection list menus** and real receive-only RSSI tools. Other categories initially show a native entry that opens the inherited Bruce tools; migrating each submenu to the new list renderer is future work.

Submenu rows are 38 pixels tall: top-level heading uses TFT font 2 and details use TFT font 1. Three rows fit at once and long lists scroll without tile clutter.

**Important:** the RF activity graph is RSSI history, not a demodulated pulse waveform. TPMS payload decoding and fob modulation identification are not yet implemented.

## Status

Source implemented on `development`. Hardware validation remains necessary; earlier 3×2 grid has been replaced. Build success alone is not a hardware UI test.
