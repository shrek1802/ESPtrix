# Bruce upstream provenance and obligations

ESPTRIX is based on **Bruce Firmware**, developed by BruceDevices and contributors.

- Upstream repository: https://github.com/BruceDevices/firmware
- Release used as baseline: https://github.com/BruceDevices/firmware/releases/tag/1.16.1
- Tag: `1.16.1`
- Upstream copyright and author notices remain with the imported source.
- Upstream licence: **GNU Affero General Public License, version 3 or later (AGPL-3.0-or-later)**.
- Upstream `THIRD_PARTY.md` and `LICENSE` are retained at repository root.

This is an **independent derivative**, not an official BruceDevices product.

## How source import works

The checked-in bootstrap workflow retrieves the exact Bruce `1.16.1` tag and imports its source into this repository, keeping our project docs and ESPTRIX overlay. The script records the exact upstream commit SHA in `docs/UPSTREAM_BASELINE.md` on completion, then creates `development`.

We keep an explicitly documented base tag so improvements can be compared with the original Bruce release. Future merges should respect both upstream and third-party authorship, source availability and attribution. **Do not remove `LICENSE`, `THIRD_PARTY.md` or in-file notices.**

Source-file changes introduced by ESPTRIX are labelled in their own module source. The key-fob and TPMS starter screens are receive-only research/diagnostic tools and are not intended for signal replay or credential extraction.

## Making a build

```bash
pio run -e lilygo-t-embed-cc1101
```

Verify the exact board variant, pinouts and flash layout before uploading.
