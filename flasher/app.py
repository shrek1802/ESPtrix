"""ESPTRIX Flash Tool: GUI for Windows and Linux.
Independent ESPTRIX project, a Bruce Firmware derivative (AGPL-3.0-or-later).
"""
from __future__ import annotations

from datetime import datetime
from pathlib import Path
import queue
import shutil
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import webbrowser

from serial.tools import list_ports
from engine import FLASH_SIZE, back_up, esptool_child, flash, probe
from firmware import FirmwareError, FirmwareFile, load_firmware

BG, CARD, BLUE = "#09131f", "#17293c", "#3ab9ff"
WHITE, MUTED, GREEN = "#f6f9ff", "#a3b9ce", "#54e0ad"
BUILDS_URL = "https://github.com/shrek1802/ESPtrix/actions/workflows/build-t-embed.yml"


class ESPTRIXFlasher(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("ESPTRIX Flash Tool")
        self.geometry("820x725")
        self.minsize(730, 660)
        self.configure(bg=BG)
        self.events = queue.Queue()
        self.firmware: FirmwareFile | None = None
        self.busy = False
        self.port_names: dict[str, str] = {}
        self.backup_port = None
        self.port_value = tk.StringVar()
        self.baud_value = tk.StringVar(value="460800")
        self.firmware_value = tk.StringVar(value="No firmware selected")
        self.firmware_details = tk.StringVar(value="Choose a build ZIP or ESPTRIX merged factory image")
        self.device_details = tk.StringVar(value="Connect your LILYGO T-Embed CC1101 via USB")
        self.status = tk.StringVar(value="Ready")
        self.have_backup = tk.BooleanVar(value=False)
        self._styles()
        self._layout()
        self.refresh_ports()
        self.after(90, self.process_events)
        self.protocol("WM_DELETE_WINDOW", self.close)

    def _styles(self):
        s = ttk.Style(self)
        s.theme_use("clam")
        s.configure("Bg.TFrame", background=BG)
        s.configure("Card.TFrame", background=CARD)
        s.configure("Title.TLabel", background=CARD, foreground=WHITE,
                    font=("Segoe UI", 11, "bold"))
        s.configure("Body.TLabel", background=CARD, foreground=WHITE,
                    font=("Segoe UI", 10))
        s.configure("Note.TLabel", background=CARD, foreground=MUTED,
                    font=("Segoe UI", 9))
        s.configure("Footer.TLabel", background=BG, foreground=MUTED,
                    font=("Segoe UI", 9))
        s.configure("Button.TButton", background="#265072", foreground=WHITE,
                    borderwidth=0, padding=(12, 9), font=("Segoe UI", 10, "bold"))
        s.map("Button.TButton", background=[("active", "#3676a5"),
                                           ("disabled", "#253849")])
        s.configure("Flash.TButton", background="#087dae", foreground=WHITE,
                    borderwidth=0, padding=(19, 10), font=("Segoe UI", 11, "bold"))
        s.map("Flash.TButton", background=[("active", "#13a4dd"),
                                          ("disabled", "#253849")])
        s.configure("Combo.TCombobox", fieldbackground="#233f55",
                    background="#233f55", foreground=WHITE, padding=7)
        s.map("Combo.TCombobox", fieldbackground=[("readonly", "#233f55")],
              foreground=[("readonly", WHITE)])
        s.configure("Check.TCheckbutton", background=CARD, foreground=WHITE)
        s.map("Check.TCheckbutton", background=[("active", CARD)])
        s.configure("Bar.Horizontal.TProgressbar", background=BLUE,
                    troughcolor=CARD, bordercolor=BG, thickness=8)

    def _layout(self):
        logo = tk.Canvas(self, height=125, bg=BG, highlightthickness=0)
        logo.pack(fill="x", padx=18, pady=(6, 0))
        # ESPTRIX logo rendered as vector shapes; works without image files.
        logo.create_rectangle(21, 18, 106, 103, fill=CARD, outline=BLUE, width=2)
        for x in (31, 52, 73, 94):
            logo.create_line(x, 8, x, 18, fill=BLUE, width=2)
            logo.create_line(x, 103, x, 114, fill=BLUE, width=2)
        for y in (29, 50, 71, 92):
            logo.create_line(10, y, 21, y, fill=BLUE, width=2)
            logo.create_line(106, y, 117, y, fill=BLUE, width=2)
        logo.create_line(39, 44, 63, 44, 76, 31, 93, 31, fill=BLUE, width=3)
        logo.create_line(39, 78, 63, 78, 76, 92, 93, 92, fill=BLUE, width=3)
        logo.create_oval(89, 27, 97, 35, fill=BLUE, outline=BLUE)
        logo.create_oval(89, 88, 97, 96, fill=BLUE, outline=BLUE)
        logo.create_text(145, 23, text="ESP", anchor="nw", fill=WHITE,
                         font=("DejaVu Sans", 40, "bold"))
        logo.create_text(261, 23, text="TRIX", anchor="nw", fill=BLUE,
                         font=("DejaVu Sans", 40, "bold"))
        logo.create_text(147, 86, text="FLASH TOOL  /  LINUX + WINDOWS",
                         anchor="nw", fill=MUTED, font=("Segoe UI", 10, "bold"))
        logo.create_text(788, 39, text="ESP32-S3", anchor="ne",
                         fill=GREEN, font=("Segoe UI", 11, "bold"))
        logo.create_text(788, 62, text="T-EMBED CC1101  •  16 MB",
                         anchor="ne", fill=MUTED, font=("Segoe UI", 9))

        body = ttk.Frame(self, style="Bg.TFrame")
        body.pack(fill="both", expand=True, padx=20, pady=(0, 10))
        device = self._card(body, "01   CONNECT DEVICE")
        row = ttk.Frame(device, style="Card.TFrame")
        row.pack(fill="x", padx=13, pady=4)
        self.port_box = ttk.Combobox(row, textvariable=self.port_value,
                                     style="Combo.TCombobox", width=48)
        self.port_box.pack(side="left", fill="x", expand=True, padx=(0, 8))
        self.port_box.bind("<<ComboboxSelected>>", lambda _: self._port_changed())
        self.scan_button = self._button(row, "Rescan", self.refresh_ports)
        self.detect_button = self._button(row, "Detect", self.detect)
        ttk.Label(device, textvariable=self.device_details, style="Note.TLabel",
                  wraplength=750).pack(anchor="w", padx=13, pady=(2, 11))

        firmware = self._card(body, "02   CHOOSE ESPTRIX FIRMWARE")
        row = ttk.Frame(firmware, style="Card.TFrame")
        row.pack(fill="x", padx=13, pady=4)
        ttk.Label(row, textvariable=self.firmware_value,
                  style="Body.TLabel").pack(side="left", fill="x", expand=True)
        self.browse_button = self._button(row, "Browse", self.choose)
        self.build_button = self._button(
            row, "Get build", lambda: webbrowser.open(BUILDS_URL))
        ttk.Label(firmware, textvariable=self.firmware_details,
                  style="Note.TLabel").pack(anchor="w", padx=13, pady=(2, 11))

        actions = self._card(body, "03   BACKUP & FLASH")
        ttk.Label(actions, text="Make a full 16 MB backup before flashing your development firmware.",
                  style="Note.TLabel").pack(anchor="w", padx=13, pady=(2, 5))
        row = ttk.Frame(actions, style="Card.TFrame")
        row.pack(fill="x", padx=13, pady=(0, 7))
        self.backup_button = self._button(row, "Backup original firmware", self.backup)
        self.flash_button = ttk.Button(row, text="FLASH ESPTRIX", command=self.flash,
                                       style="Flash.TButton")
        self.flash_button.pack(side="left", padx=8)
        ttk.Checkbutton(actions, text="I already have a full backup of this device",
                        variable=self.have_backup, style="Check.TCheckbutton").pack(
                            anchor="w", padx=13, pady=(0, 10))

        status_bar = ttk.Frame(body, style="Bg.TFrame")
        status_bar.pack(fill="x", pady=(4, 5))
        ttk.Label(status_bar, textvariable=self.status,
                  style="Footer.TLabel").pack(side="left")
        self.progress = ttk.Progressbar(status_bar, mode="indeterminate",
                                        style="Bar.Horizontal.TProgressbar")
        self.progress.pack(side="right", fill="x", expand=True, padx=(12, 0))

        logs = ttk.Frame(body, style="Bg.TFrame")
        logs.pack(fill="both", expand=True)
        self.log_text = tk.Text(logs, height=9, bg="#06101b", fg="#d3e2ed",
                                font=("Consolas", 9), relief="flat",
                                padx=12, pady=8, state="disabled", wrap="word")
        self.log_text.pack(side="left", fill="both", expand=True)
        scroll = ttk.Scrollbar(logs, command=self.log_text.yview)
        scroll.pack(side="right", fill="y")
        self.log_text.configure(yscrollcommand=scroll.set)
        self.log("Ready. Select the USB device and ESPTRIX firmware file.")
        self.log("This tool does not erase the complete flash automatically.")

        footer = ttk.Frame(self, style="Bg.TFrame")
        footer.pack(fill="x", padx=20, pady=(0, 10))
        ttk.Label(footer, text="ESPTRIX • independent Bruce Firmware derivative • AGPL",
                  style="Footer.TLabel").pack(side="left")
        ttk.Label(footer, text="No automatic flashing",
                  style="Footer.TLabel").pack(side="right")

    def _card(self, parent, title):
        frame = ttk.Frame(parent, style="Card.TFrame")
        frame.pack(fill="x", pady=(0, 9))
        ttk.Label(frame, text=title, style="Title.TLabel").pack(
            anchor="w", padx=13, pady=(10, 2))
        return frame

    def _button(self, parent, text, action):
        button = ttk.Button(parent, text=text, command=action, style="Button.TButton")
        button.pack(side="left", padx=(0, 7))
        return button

    def log(self, line):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", str(line).rstrip() + "\n")
        if int(self.log_text.index("end-1c").split(".")[0]) > 1400:
            self.log_text.delete("1.0", "350.0")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _port(self):
        entry = self.port_value.get().strip()
        return self.port_names.get(entry, entry.split("  —  ")[0].strip())

    def _port_changed(self):
        self.device_details.set("Detect verifies ESP32-S3 and 16 MB flash. Confirm the PCB model manually.")

    def refresh_ports(self):
        previous = self._port()
        ports = sorted(list_ports.comports(), key=lambda item: item.device)
        self.port_names = {f"{p.device}  —  {p.description}": p.device for p in ports}
        self.port_box["values"] = list(self.port_names)
        match = next((name for name, port in self.port_names.items()
                      if port == previous), "")
        if not match and len(self.port_names) == 1:
            match = next(iter(self.port_names))
        self.port_value.set(match)
        self._port_changed()
        if not ports:
            self.device_details.set("No USB serial port found. Check USB cable, driver and permissions.")

    def choose(self):
        file = filedialog.askopenfilename(title="Choose ESPTRIX firmware",
                 filetypes=[("ESPTRIX firmware ZIP/BIN", "*.zip *.bin"),
                            ("All files", "*.*")])
        if not file:
            return
        try:
            chosen = load_firmware(file)
        except (OSError, FirmwareError) as exc:
            self.log("Rejected: " + str(exc))
            messagebox.showerror("Firmware rejected", str(exc))
            return
        if self.firmware:
            self.firmware.cleanup()
        self.firmware = chosen
        self.firmware_value.set(Path(file).name)
        checksum = "SHA-256 checksum OK" if chosen.checksum_verified else "No checksum supplied"
        self.firmware_details.set(f"{chosen.size / 1048576:.2f} MiB  •  {checksum}")
        self.log("Selected: " + file)
        self.log("SHA-256: " + chosen.sha256)

    def _busy(self, busy, status="Ready"):
        self.busy = busy
        self.status.set(status)
        state = "disabled" if busy else "normal"
        for widget in (self.scan_button, self.detect_button, self.browse_button,
                       self.build_button, self.backup_button, self.flash_button):
            widget.configure(state=state)
        self.port_box.configure(state=state)
        if busy:
            self.progress.start(12)
        else:
            self.progress.stop()

    def _work(self, title, operation):
        if self.busy:
            return
        self._busy(True, title)
        def worker():
            try:
                result = operation()
                self.events.put(("done", (title, result)))
            except Exception as error:
                self.events.put(("error", (title, str(error))))
        threading.Thread(target=worker, daemon=True).start()

    def _thread_log(self, message):
        self.events.put(("log", message))

    def detect(self):
        port = self._port()
        if not port:
            messagebox.showwarning("No device", "Select a USB port first.")
            return
        self._work("Detecting device", lambda: (probe(port, self._thread_log), port)[1])

    def backup(self):
        port = self._port()
        if not port:
            messagebox.showwarning("No device", "Select a USB port first.")
            return
        filename = "ESPTRIX-backup-" + datetime.now().strftime("%Y%m%d-%H%M%S") + ".bin"
        selected = filedialog.asksaveasfilename(title="Save a 16 MB flash backup",
                         initialfile=filename, defaultextension=".bin",
                         filetypes=[("Binary", "*.bin")])
        if not selected:
            return
        target = Path(selected)
        if target.exists():
            messagebox.showerror("File exists", "Choose another name to protect the old backup.")
            return
        if shutil.disk_usage(target.parent).free < 20 * 1024 * 1024:
            messagebox.showerror("Disk full", "At least 20 MB free space is required.")
            return
        baud = self.baud_value.get()
        self._work("Backing up 16 MB flash",
                   lambda: (port, back_up(port, baud, target, self._thread_log)))

    def flash(self):
        port = self._port()
        fw = self.firmware
        if not port or fw is None:
            messagebox.showwarning("Device / firmware missing",
                   "Select a serial port and ESPTRIX image first.")
            return
        if self.backup_port != port and not self.have_backup.get():
            messagebox.showwarning("Backup required",
                   "Please back up this device or tick that you already have a complete backup.")
            return
        if not fw.checksum_verified and not messagebox.askyesno("No build checksum",
             "The image format is valid, but no checksum manifest was supplied. Continue?"):
            return
        if not messagebox.askyesno("Confirm flashing",
             "Confirm this is a LILYGO T-Embed CC1101 with ESP32-S3 and 16 MB flash.\n\n"
             "The merged image will be written at 0x0. Existing firmware/settings may be lost.\n\n"
             "Proceed?"):
            return
        baud = self.baud_value.get()
        self._work("Flashing ESPTRIX",
                   lambda: (flash(port, baud, fw.path, self._thread_log), port)[1])

    def process_events(self):
        try:
            for _ in range(80):
                event, value = self.events.get_nowait()
                if event == "log":
                    self.log(value)
                    continue
                name, result = value
                self._busy(False)
                if event == "error":
                    self.log("FAILED — " + name + ": " + result)
                    self.status.set("FAILED: " + name)
                    messagebox.showerror(name, result)
                else:
                    self.log(name + " completed successfully.")
                    self.status.set("Completed: " + name)
                    if name == "Detecting device":
                        self.device_details.set(
                            "ESP32-S3, 16 MB flash verified. Confirm T-Embed CC1101 on the casing.")
                    elif name == "Backing up 16 MB flash":
                        self.backup_port, filename = result
                        messagebox.showinfo("Backup complete",
                              "Full flash image and .sha256 saved:\n" + filename)
                    elif name == "Flashing ESPTRIX":
                        messagebox.showinfo("Flash complete",
                            "ESPTRIX was written. Reconnect the T-Embed and check the screen, rotary controls and RF tools.")
        except queue.Empty:
            pass
        self.after(90, self.process_events)

    def close(self):
        if self.busy:
            messagebox.showwarning("Busy", "Finish backup or flashing before closing the tool.")
            return
        if self.firmware:
            self.firmware.cleanup()
        self.destroy()


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--esptool-run":
        raise SystemExit(esptool_child(sys.argv[2:]))
    ESPTRIXFlasher().mainloop()
