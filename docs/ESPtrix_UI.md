# ESPTRIX launcher — first native UI milestone

**Target:** LILYGO T-Embed CC1101, 320 × 170 in landscape.

This milestone adds an ESPTRIX HOME entry to the existing Bruce main menu on the T-Embed CC1101 only. Bruce's menu remains intact, including every existing application and settings screen.

## Controls

- Turn rotary encoder: highlight the next/previous tile.
- Press encoder: launch the highlighted tool.
- Back button: leave ESPTRIX HOME for Bruce.
- Select BRUCE: return to the original Bruce menu.

## Tiles

Page 1: Key Fob, TPMS, RF Tools, Wi-Fi, BLE, NFC/RFID.

Page 2: Infrared, Files, More, Settings, Bruce, About.

Key Fob and TPMS tiles open frequency selectors, then the receive-only signal-strength monitors. **They do not yet perform packet decoding.** The About screen acknowledges Bruce Firmware, AGPL licensing and repository details.

## UI specifications

Dark navy background, electric-blue highlight, white labels, two-page 3×2 icon grid, battery indicator, clear encoder focus. This stage intentionally avoids replacing Bruce's core menu renderer, allowing safe incremental migration.

## Status

Implemented in development source; pending a successful target-board build and real hardware validation.
