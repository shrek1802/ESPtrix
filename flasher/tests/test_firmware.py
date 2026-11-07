"""Run with: python -m unittest discover -s flasher/tests -v."""
import hashlib
from pathlib import Path
import tempfile
import unittest
import zipfile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from firmware import (TARGET, SUMS, APP_OFFSET, MIN_IMAGE_BYTES,
                      FirmwareError, load_firmware)


class FirmwareSelectionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        blob = bytearray(MIN_IMAGE_BYTES + 128)
        blob[0] = 0xE9
        blob[APP_OFFSET] = 0xE9
        self.data = bytes(blob)

    def tearDown(self):
        self.tmp.cleanup()

    def _file(self):
        p = self.dir / TARGET
        p.write_bytes(self.data)
        return p

    def test_good_merged_image_and_checksum(self):
        p = self._file()
        (self.dir / SUMS).write_text(hashlib.sha256(self.data).hexdigest() +
                                    "  " + TARGET + "\n")
        fw = load_firmware(p)
        self.assertTrue(fw.checksum_verified)
        self.assertEqual(fw.size, len(self.data))
        fw.cleanup()

    def test_corrupt_checksum_rejected(self):
        p = self._file()
        (self.dir / SUMS).write_text("0" * 64 + "  " + TARGET + "\n")
        with self.assertRaisesRegex(FirmwareError, "SHA-256 mismatch"):
            load_firmware(p)

    def test_invalid_bootloader_header_rejected(self):
        p = self._file()
        data = bytearray(self.data)
        data[0] = 0
        p.write_bytes(data)
        with self.assertRaisesRegex(FirmwareError, "bootloader"):
            load_firmware(p)

    def test_application_only_image_rejected(self):
        p = self.dir / "firmware.bin"
        p.write_bytes(self.data)
        with self.assertRaisesRegex(FirmwareError, "Only the merged"):
            load_firmware(p)

    def test_extracts_only_matching_image_from_zip(self):
        z = self.dir / "download.zip"
        sums = hashlib.sha256(self.data).hexdigest() + "  " + TARGET + "\n"
        with zipfile.ZipFile(z, "w") as f:
            f.writestr("nested/" + TARGET, self.data)
            f.writestr("../wrong.txt", "never extracted")
            f.writestr(SUMS, sums)
        fw = load_firmware(z)
        self.assertTrue(fw.checksum_verified)
        self.assertTrue(fw.path.is_file())
        self.assertFalse((Path(fw.path).parent / "wrong.txt").exists())
        folder = fw.path.parent
        fw.cleanup()
        self.assertFalse(folder.exists())

    def test_zip_bomb_claimed_size_rejected(self):
        z = self.dir / "download.zip"
        with zipfile.ZipFile(z, "w") as f:
            f.writestr(TARGET, b"x" * 2048)
        with self.assertRaises(FirmwareError):
            load_firmware(z)

    def test_duplicate_image_zip_rejected(self):
        z = self.dir / "download.zip"
        with zipfile.ZipFile(z, "w") as f:
            f.writestr("one/" + TARGET, self.data)
            f.writestr("two/" + TARGET, self.data)
        with self.assertRaisesRegex(FirmwareError, "exactly one"):
            load_firmware(z)


if __name__ == "__main__":
    unittest.main()
