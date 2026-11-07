# ESPTRIX first hardware test — T-Embed CC1101

**Development firmware.** Verified to compile for `lilygo-t-embed-cc1101`; this does **not** mean it has been tested on real hardware yet.

## Obtain the image

1. Visit the latest **successful** [ESPTRIX T-Embed GitHub Actions build](https://github.com/shrek1802/ESPtrix/actions/workflows/build-t-embed.yml).
2. Open the successful run and download the `ESPTRIX-t-embed-cc1101-development` artifact ZIP. You may need to sign into GitHub.
3. Extract `ESPTRIX-lilygo-t-embed-cc1101.bin` and `ESPTRIX-SHA256SUMS.txt`.
4. On Ubuntu in the extracted folder run `sha256sum -c ESPTRIX-SHA256SUMS.txt` and check for `OK`.

The large **merged factory** image is for flashing at offset `0x0`. The separate `firmware.bin` must **not** be flashed at `0x0` because it does not contain the bootloader and partition table.

## Before flashing

- Confirm that your device is specifically the **LILYGO T-Embed CC1101 (ESP32-S3, 16 MB flash)**. Do not use this binary with T-Embed non-CC1101 or other ESP32 variants.
- Back up anything important, including files and Bruce settings. A partition-table change can affect stored data.
- Connect with a data-capable USB cable and use a stable USB port.
- Never use the `erase-flash` command unless you intend to wipe the flash.

## Ubuntu tool installation

```bash
sudo apt update
sudo apt install -y pipx
pipx install esptool
```

Open a new terminal if `esptool` is not on your PATH, or use `~/.local/bin/esptool`.

Find your USB port:

```bash
ls -l /dev/serial/by-id/
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

For the commands below, replace `/dev/ttyACM0` with your actual port. If the device is not detected, consult the LILYGO manual for bootloader mode. Do not guess the port.

## Strongly recommended: read a complete flash backup

A full backup can take time and needs sufficient disk space:

```bash
esptool --chip esp32s3 --port /dev/ttyACM0 read-flash 0x0 0x1000000 ESPTRIX-before-flash-backup.bin
```

Store the backup away from the test folder.

## Flash the merged image

From the folder containing `ESPTRIX-lilygo-t-embed-cc1101.bin`:

```bash
esptool --chip esp32s3 --port /dev/ttyACM0 --baud 460800 write-flash 0x0 ESPTRIX-lilygo-t-embed-cc1101.bin
```

Disconnect and reconnect USB if the board does not reset automatically.

## Test checklist

- Boot and display: does ESPTRIX start without freezing?
- Bruce main menu: `ESPTRIX HOME` item should appear on the T-Embed.
- ESPTRIX launcher: the 3x2 tiles display clearly on both pages.
- Rotary encoder: highlights move one tile per step, without jumping.
- BACK and BRUCE tile: return to the inherited Bruce interface.
- Key Fob Tester: opens 315 / 433.92 / 868.35 MHz receive-only monitors.
- TPMS Monitor: opens 315 / 433.92 MHz activity monitoring without claiming pressure or temperature.
- Compare passive RSSI response to pressing **your own** compatible test transmitter.
- Check Wi-Fi, BLE, NFC, SD, power and battery functions for regressions.

**Do not expect actual TPMS pressure, valid key decoding, rolling-code authentication checks or remote transmission to be available in this first diagnostic milestone.**

Please report any boot-loop, unresponsive input or screen corruption, preferably with a photo and serial log.
