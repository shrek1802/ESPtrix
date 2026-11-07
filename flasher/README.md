# ESPTRIX Flash Tool

**A small branded desktop flashing application for Linux and Windows** with the ESPTRIX logo at the top. ESPTRIX is an independent derivative of [Bruce Firmware](https://github.com/BruceDevices/firmware); its firmware is based on Bruce 1.16.1 and remains subject to AGPL licensing. This utility uses Espressif's `esptool` and `pyserial`.

## Download

Open [GitHub Actions – Build ESPTRIX Flash Tool](https://github.com/shrek1802/ESPtrix/actions/workflows/build-flasher.yml).

Open a successful run, then download one of the ZIP artifacts:

- `ESPTRIX-Flasher-Windows`: contains `ESPTRIX-Flasher.exe`
- `ESPTRIX-Flasher-Linux`: contains `ESPTRIX-Flasher`

Extract the file before running. Windows Defender SmartScreen may prompt for an unsigned application; the executable has not been code-signed. Do not bypass warnings for files obtained from unknown sources; use the project's own GitHub build.

On Ubuntu, open a terminal in the extracted folder:

```bash
chmod +x ESPTRIX-Flasher
./ESPTRIX-Flasher
```

If your user does not have permission to access `/dev/ttyACM0` or `/dev/ttyUSB0`, add yourself to the `dialout` group and sign out then back in:

```bash
sudo usermod -aG dialout "$USER"
```

Do not run the flasher as root. On Windows, USB serial drivers may be needed depending on the USB interface. The application does not install drivers.

## Flashing workflow

1. Connect the **LILYGO T-Embed CC1101** using a data USB cable.
2. Click **Rescan** to see USB serial ports and **Detect** to check ESP32-S3 / 16 MB flash. **The PCB model itself cannot be identified electronically; visually check it.**
3. Click **Get build** to open the ESPTRIX firmware builds, and download the latest successful `ESPTRIX-t-embed-cc1101-development` ZIP.
4. Click **Browse** and select that entire build ZIP. The tool extracts only the correctly named merged image, validates ESP image headers and checks SHA-256 where the manifest is included. It rejects the standalone `firmware.bin`.
5. Click **Backup original firmware** and choose a filename; this saves a full 16 MB image and a SHA-256 file alongside it.
6. Click **FLASH ESPTRIX** and read the confirmation. The tool rechecks the connected device and flashes the merged image at `0x0`.
7. Reconnect/reset the board and test the screen, rotary wheel, and RF features.

Use only on the exact device variant stated above. The tool intentionally provides **no full erase button, no arbitrary flash offsets, and no unattended flashing**. A matching checksum protects against accidental file corruption but does not authenticate the software publisher.

## Running from source

Python 3.11+ and a graphical desktop session are required. This is an ordinary Tkinter application.

```bash
python -m pip install -r flasher/requirements.txt
python flasher/app.py
```

Run the tests:

```bash
python -m unittest discover -s flasher/tests -v
```

The standalone desktop packages bundle Python, esptool, pyserial and the GUI so end users do not need to install Python.

## Notes

The first version is an ESPTRIX/T-Embed-specific flasher, **not a general-purpose ESP32 flashing tool**. It does not support app-only OTA images, unrelated boards, secure boot/flash-encrypted firmware, or arbitrary partitions. ESPTRIX firmware is still in development. Back up before flashing. Follow the upstream software licences in the repository.
