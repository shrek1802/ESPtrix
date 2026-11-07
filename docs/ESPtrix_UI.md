# ESPTRIX launcher — T-Embed CC1101

Development milestone: ESPTRIX is now the default **main menu** on the T-Embed CC1101, while all of Bruce's original tools remain reachable.

## Boot and recovery

- On normal startup, after the existing Bruce boot animation, the ESPTRIX navy/electric-blue two-page launcher appears.
- **Hold the physical BACK button when the device finishes booting and opens the main menu** to start with the original Bruce interface. Release it once the Bruce menu appears.
- In ESPTRIX HOME, press BACK or select the **BRUCE** tile (page two) to switch to the original Bruce interface for the remainder of the session.
- In Bruce's carousel menu, open **ESPTRIX HOME** (last menu item) to switch back.
- Changing the menu is a runtime-only choice. On reboot it defaults back to ESPTRIX unless BACK is held at startup.

## Navigation

- Rotate the encoder: move between tiles.
- Press the encoder: open a tool.
- BACK: switch to the original Bruce UI.

Page 1: Key Fob, TPMS, RF Tools, Wi-Fi, BLE, NFC/RFID.
Page 2: Infrared, Files, More, Settings, Bruce, About.

RF diagnostic screens remain **receive-only** and currently show RSSI signal trends, not real key code or TPMS packet decoding.

## Testing status

The first ESPTRIX build (#20) showed a blank screen on physical hardware. Build #22 booted successfully, displaying the original Bruce UI, which is why the native launcher became the next milestone. This default-launcher change requires a **fresh firmware build and hardware validation**. If the home screen misbehaves, use BACK to enter Bruce or reflash the known-working Bruce firmware.
