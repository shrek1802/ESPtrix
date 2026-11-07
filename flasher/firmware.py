"""Safe selection and validation of ESPTRIX merged T-Embed CC1101 firmware.

A filename and image-header check cannot authenticate firmware or identify the
connected PCB. A matching SHA-256 file only detects accidental corruption.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
from pathlib import Path, PurePosixPath
import re
import tempfile
import zipfile

TARGET = "ESPTRIX-lilygo-t-embed-cc1101.bin"
SUMS = "ESPTRIX-SHA256SUMS.txt"
MAX_IMAGE_BYTES = 16 * 1024 * 1024
MIN_IMAGE_BYTES = 256 * 1024
APP_OFFSET = 0x10000


class FirmwareError(ValueError):
    pass


@dataclass
class FirmwareFile:
    path: Path
    size: int
    sha256: str
    checksum_verified: bool
    origin: str
    temp_folder: tempfile.TemporaryDirectory | None = None

    def cleanup(self) -> None:
        if self.temp_folder is not None:
            self.temp_folder.cleanup()
            self.temp_folder = None


def _digest(path: Path) -> str:
    hash_obj = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            hash_obj.update(chunk)
    return hash_obj.hexdigest()


def _verify_image(path: Path) -> int:
    size = path.stat().st_size
    if not MIN_IMAGE_BYTES <= size <= MAX_IMAGE_BYTES:
        raise FirmwareError("Image size is incompatible with a 16 MB merged factory image.")
    with path.open("rb") as image:
        if image.read(1) != b"\xe9":
            raise FirmwareError("Missing ESP bootloader header at offset 0x0.")
        image.seek(APP_OFFSET)
        if image.read(1) != b"\xe9":
            raise FirmwareError("Missing ESP application header at offset 0x10000.")
    return size


def _expected_checksum(text: str) -> str:
    # GitHub build writes standard sha256sum output. Don't accept a checksum
    # for an arbitrary filename, or accept multiple conflicting entries.
    pattern = re.compile(r"^([a-fA-F0-9]{64})\s+\*?" + re.escape(TARGET) + r"$")
    matches = [m.group(1).lower() for line in text.splitlines()
               if (m := pattern.fullmatch(line.strip()))]
    if len(matches) != 1:
        raise FirmwareError("Checksum file lacks exactly one ESPTRIX factory-image entry.")
    return matches[0]


def load_firmware(selected: str | Path) -> FirmwareFile:
    source = Path(selected).expanduser()
    if not source.is_file():
        raise FirmwareError("Choose a downloaded ESPTRIX .zip or merged .bin file.")
    folder = None
    try:
        if source.suffix.lower() == ".zip":
            folder = tempfile.TemporaryDirectory(prefix="esptrix-flasher-")
            with zipfile.ZipFile(source) as archive:
                matches = [m for m in archive.infolist()
                           if not m.is_dir() and PurePosixPath(m.filename).name == TARGET]
                if len(matches) != 1:
                    raise FirmwareError("ZIP must contain exactly one " + TARGET)
                member = matches[0]
                if not MIN_IMAGE_BYTES <= member.file_size <= MAX_IMAGE_BYTES:
                    raise FirmwareError("ZIP firmware size is invalid or unsafe to extract.")
                output = Path(folder.name) / TARGET
                # Extract only the chosen image, never archive paths.
                with archive.open(member) as src, output.open("wb") as dst:
                    while block := src.read(1024 * 1024):
                        dst.write(block)
                hashes = [m for m in archive.infolist()
                          if not m.is_dir() and PurePosixPath(m.filename).name == SUMS]
                if len(hashes) > 1:
                    raise FirmwareError("ZIP contains conflicting checksum files.")
                sums_text = archive.read(hashes[0]).decode("utf-8") if hashes else None
                if sums_text is not None and len(sums_text) > 4096:
                    raise FirmwareError("Checksum manifest is unexpectedly large.")
        else:
            if source.name != TARGET:
                raise FirmwareError("Only the merged " + TARGET +
                                    " is accepted; do not select firmware.bin.")
            output = source
            manifest = source.with_name(SUMS)
            sums_text = manifest.read_text(encoding="utf-8") if manifest.exists() else None

        size = _verify_image(output)
        sha256 = _digest(output)
        if sums_text is not None:
            if sha256 != _expected_checksum(sums_text):
                raise FirmwareError("SHA-256 mismatch — firmware may be corrupted.")
        result = FirmwareFile(output, size, sha256, sums_text is not None, str(source), folder)
        folder = None
        return result
    except (OSError, zipfile.BadZipFile, UnicodeDecodeError) as exc:
        raise FirmwareError(str(exc)) from exc
    finally:
        if folder is not None:
            folder.cleanup()
