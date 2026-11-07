# ESPTRIX development roadmap

**Target hardware:** LILYGO T-Embed CC1101 (ESP32-S3).

## M0 — Bootstrap

- [x] Establish ESPTRIX project identity, Bruce 1.16.1 provenance and AGPL notice
- [ ] Import the exact upstream tag into `main`
- [ ] Create `development` from the imported baseline
- [ ] Confirm clean PlatformIO build and firmware image size

## M1 — RF Lab first screen

- [ ] Add **ESPtrix RF Lab** to the Bruce RF menu, without removing Bruce tools
- [ ] Add polished dark/blue layout at 320 × 170
- [ ] Key Tester: receiver-only RSSI and approximate activity-peaks monitor at selectable 315/433/868 MHz
- [ ] TPMS Monitor: honest receiver-only initial monitor at 315/433 MHz
- [ ] Validate menus, rotary controls and exit paths on hardware

## M2 — Usable diagnostic tools

- [ ] Implement pulse capture and burst timing (no transmit/replay in the tester)
- [ ] Automatic frequency presets with radio-band limitations noted
- [ ] Activity history and SD CSV export
- [ ] Add test signal simulator and unit tests

## M3 — TPMS decoding

- [ ] Select and review open-source, licence-compatible TPMS decoders
- [ ] Decode known protocol(s) with test vectors
- [ ] Display pressure, temperature, sensor ID and flags **only if valid data is decoded**
- [ ] Protocol and checksum validation, unknown-sensor handling
- [ ] Optional external 125 kHz activation board after electrical review

## M4 — Interface modernisation

- [ ] ESPTRIX splash screen, main categories, UI components
- [ ] Improved status bar, favourites, recent tools, settings
- [ ] Keep all inherited Bruce tools reachable during migration

## M5 — Security research

- [ ] Passive cross-protocol evidence/event logging
- [ ] BLE service and security configuration auditing
- [ ] IoT network misconfiguration checks (authorised scope)
- [ ] Bench-only protocol robustness tests, isolated from production devices

## Quality gates

Each firmware build must compile for the exact board environment. Avoid invented readings, disclose incomplete features, keep RF transmit functions out of passive diagnostics, preserve upstream notices, and publish source for redistributed builds under the applicable licences.
