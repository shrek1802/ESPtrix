"""Cross-platform esptool backend for the ESPTRIX desktop flasher.

The packaged executable calls itself with --esptool-run so no separate Python
installation, terminal or copy of esptool is needed on Windows or Linux.
"""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Callable

FLASH_SIZE = 0x1000000  # LILYGO T-Embed CC1101: 16 MB
Log = Callable[[str], None]


def esptool_child(args: list[str]) -> int:
    try:
        import esptool
        esptool.main(args)
        return 0
    except SystemExit as exc:
        return int(exc.code or 0) if isinstance(exc.code, int) else 1
    except Exception as exc:
        print("ESPTRIX esptool error: " + str(exc), flush=True)
        return 1


def run_esptool(args: list[str], log: Log) -> str:
    if getattr(sys, "frozen", False):
        command = [sys.executable, "--esptool-run", *args]
    else:
        command = [sys.executable, str(Path(__file__).resolve().parent / "app.py"),
                   "--esptool-run", *args]
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          encoding="utf-8", errors="replace", universal_newlines=True,
                          bufsize=1, startupinfo=startup) as process:
        assert process.stdout is not None
        output = []
        total = 0
        for line in process.stdout:
            for piece in line.replace("\r", "\n").splitlines():
                if piece.strip():
                    log(piece.strip())
            if total < 150000:
                output.append(line)
                total += len(line)
        exit_code = process.wait()
    if exit_code != 0:
        raise RuntimeError(f"esptool failed (code {exit_code}). See the progress log.")
    return "".join(output)


def common_args(port: str, baud: str = "115200") -> list[str]:
    return ["--chip", "esp32s3", "--port", port, "--baud", baud]


def probe(port: str, log: Log) -> None:
    """Check chip family and flash capacity; cannot identify the exact PCB."""
    result = run_esptool(common_args(port) + ["flash-id"], log)
    match = re.search(r"Detected flash size:\s*(\d+)\s*M[Bb]", result, re.I)
    if match is None or int(match.group(1)) != 16:
        raise RuntimeError("Expected ESP32-S3 with 16 MB flash. Device rejected.")
    log("ESP32-S3 and 16 MB flash detected; confirm T-Embed CC1101 on its label.")


def back_up(port: str, baud: str, output: Path, log: Log) -> str:
    probe(port, log)
    run_esptool(common_args(port, baud) +
                ["read-flash", "0x0", hex(FLASH_SIZE), str(output)], log)
    if output.stat().st_size != FLASH_SIZE:
        raise RuntimeError("Backup was incomplete. Do not rely on the file.")
    h = hashlib.sha256()
    with output.open("rb") as file:
        while block := file.read(1024 * 1024):
            h.update(block)
    (output.with_suffix(output.suffix + ".sha256")).write_text(
        h.hexdigest() + "  " + output.name + "\n", encoding="utf-8")
    return str(output)


def flash(port: str, baud: str, image: Path, log: Log) -> None:
    probe(port, log)
    run_esptool(common_args(port, baud) + ["write-flash", "0x0", str(image)], log)
