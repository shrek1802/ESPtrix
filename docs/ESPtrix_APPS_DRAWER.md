# ESPTRIX Apps drawer

**Target:** Bruce original main menu + a single ESPTRIX Apps launcher, using the current ESPTRIX colours/icons.

This branch preserves all Bruce main-menu categories, including Wi-Fi, BLE, Radio, RFID, Files and settings, and registers one additional `ESPTRIX Apps` icon. The existing Bruce tools are not replaced. The menu renderer, theme, and rotary controls are inherited from the current firmware.

## Sources

The public repo currently contains only early RF Lab code. Consequently this branch shows `ESPTRIX Apps > RF Lab` when built from this checkout alone.

If the user's separate ESPTRIX V8.2.2 / Bruce 1.17 migration source contains the header `src/esptrix/esptrix_unified_menu.h` with `openUnifiedCategory(CategoryId)`, the drawer instead exposes these nine existing application groups:

- Smart Test
- Automotive Tools
- Access Control Lab
- RF & Sub-GHz Tools
- Wi-Fi Tools
- BLE Tools
- Infrared & USB
- Files & Reports
- ESPTRIX System

This condition is compile-time; none of the V8 functions are invented in this branch. The application groups reuse existing code via the unified dispatcher.

## Integration caution

The Bruce 1.17 candidate is generated locally by the non-destructive migration kit. To preserve local V8 source and theme changes, apply the standalone Apps Menu Patch to the candidate after a clean migration. It validates the native Bruce root-menu interface and V8 category names, refuses incompatible source, and never edits the stable project.

**Not yet built or verified on hardware.** Keep the published V8.2.2 stable release unchanged. Before flashing, verify Bruce 1.17 boot, menu, rotary input, SD, RF, Wi-Fi and BLE and all ESPTRIX apps. Do not merge this PR as a tested release.
