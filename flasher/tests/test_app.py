"""Headless smoke checks for the ESPTRIX desktop application."""
from pathlib import Path
import ast
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from engine import FLASH_SIZE, common_args


class AppTests(unittest.TestCase):
    def test_gui_python_syntax(self):
        ast.parse((ROOT / "app.py").read_text(encoding="utf-8"))

    def test_esp32s3_chip_guard(self):
        self.assertEqual(common_args("COM3")[:4],
                         ["--chip", "esp32s3", "--port", "COM3"])
        self.assertEqual(FLASH_SIZE, 0x1000000)

    def test_no_erase_all_command(self):
        src = (ROOT / "engine.py").read_text(encoding="utf-8")
        self.assertNotIn('"erase-flash"', src)
        self.assertIn('"read-flash"', src)
        self.assertIn('"write-flash", "0x0"', src)

    def test_vector_wordmark_in_gui(self):
        src = (ROOT / "app.py").read_text(encoding="utf-8")
        self.assertIn('text="ESP"', src)
        self.assertIn('text="TRIX"', src)


if __name__ == "__main__":
    unittest.main()
