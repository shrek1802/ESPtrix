# ESPTRIX

**Advanced wireless and security toolkit for ESP32 devices**

> **Status: early development.** The first target is the **LILYGO T-Embed CC1101**. Do not flash an unverified build to hardware.

## Built from Bruce

**ESPTRIX is an independent fork/derivative of [Bruce Firmware](https://github.com/BruceDevices/firmware), based on the [Bruce 1.16.1 release](https://github.com/BruceDevices/firmware/releases/tag/1.16.1).** Bruce and its contributors created the underlying firmware. ESPTRIX is **not** an official BruceDevices release or endorsed by BruceDevices.

We intend to retain Bruce's existing features while adding a more polished black/electric-blue UI and focused tools for signal analysis, device diagnostics and authorised security testing.

## Planned ESPTRIX features

- New home menu, status bar, rotary navigation, favourites and consistent application layouts
- **Key Fob Tester:** passive, multi-band RF activity indication; later real pulse decoding, signal comparisons and saved measurements
- **TPMS Monitor:** initial passive 315/433 MHz activity monitor; pressure, temperature, IDs and fault flags only once compatible sensor decoders are implemented
- RF history, signal comparison, exports and device profiles
- Wi-Fi, BLE, NFC, IR and USB inspection; controlled security-assessment modules
- Optional sensor modules and desktop companion

**No TPMS values, modulation types or decoded data should be shown unless genuinely measured or decoded.**

## Development

The repository is bootstrapped from the **Bruce 1.16.1 source tag**, not its latest moving branch. See [docs/UPSTREAM.md](docs/UPSTREAM.md) for origin, attribution and the import procedure.

- `main`: stable integration baseline
- `development`: ongoing firmware and UI work
- PlatformIO build environment: `lilygo-t-embed-cc1101`

Once the source import completes, build on Ubuntu with:

```bash
python3 -m pip install --user platformio
pio run -e lilygo-t-embed-cc1101
```

Never distribute a firmware binary before checking hardware compatibility and testing the build.

## Licensing and acknowledgements

Bruce is licensed under **AGPL-3.0-or-later**. ESPTRIX changes derived from Bruce are distributed on the same basis. The upstream `LICENSE` and `THIRD_PARTY.md` files are preserved during import. Individual third-party components can carry additional obligations; see their original notices.

Source: https://github.com/BruceDevices/firmware

Use only on devices and networks you own or have explicit authorisation to assess. Transmit-only features must comply with local radio regulations.

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md). Contributions and protocol documentation are welcome.
