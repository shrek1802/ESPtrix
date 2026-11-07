// ESPTRIX RF Lab - receive-only diagnostic UI.
// ESPTRIX is derived from Bruce Firmware: https://github.com/BruceDevices/firmware
// Copyright (C) 2026 ESPTRIX contributors.
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "esptrix_ui.h"
#include "core/display.h"
#include "modules/rf/rf_utils.h"
#include <globals.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <algorithm>
#include <vector>

namespace esptrix {
namespace {
constexpr uint16_t BACKGROUND = 0x0862; // deep blue-black
constexpr uint16_t PANEL      = 0x1905;
constexpr uint16_t ACCENT     = 0x45FF; // electric blue
constexpr uint16_t WHITE      = 0xFFFF;
constexpr uint16_t SUBTLE     = 0xA5B4;
constexpr uint16_t GREEN      = 0x3F52;

// A first milestone only: CC1101 RSSI sampling, not packet decoding.
// A high RSSI means radio activity; it does NOT prove a valid key or TPMS frame.
void drawFrame(const char *name, float frequencyMHz, bool tpms) {
    const int w = tftWidth, h = tftHeight;
    tft.fillScreen(BACKGROUND);
    tft.fillRect(0, 0, w, 24, PANEL);
    tft.fillRect(0, 0, 4, 24, ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(WHITE, PANEL);
    tft.drawString("ESPTRIX", 9, 5, 2);
    tft.setTextColor(ACCENT, PANEL);
    tft.drawRightString(name, w - 8, 6, 1);

    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString("LISTENING  |  RX ONLY", 9, 31, 1);
    tft.setTextColor(WHITE, BACKGROUND);
    tft.drawString(String(frequencyMHz, 2) + " MHz", 9, 47, 4);

    tft.fillRoundRect(8, 80, (w - 24) / 2, 46, 6, PANEL);
    tft.fillRoundRect(16 + (w - 24) / 2, 80, (w - 24) / 2, 46, 6, PANEL);
    tft.setTextColor(SUBTLE, PANEL);
    tft.drawString("RSSI", 16, 85, 1);
    tft.drawString("ACTIVITY PEAKS", 24 + (w - 24) / 2, 85, 1);

    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString(tpms ? "TPMS: Decoder not yet implemented" : "Activity only - not a key decode",
                   9, h - 36, 1);
    tft.drawString("Press BACK / SELECT to exit", 9, h - 21, 1);
}

void showMonitor(const char *name, float frequencyMHz, bool tpms) {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("ESPtrix RF Lab requires CC1101", true);
        return;
    }
    if (!initRfModule("rx", frequencyMHz)) {
        displayError("Unable to start CC1101 receiver", true);
        return;
    }
    // All screens in this module are passive - never transmit.
    drawFrame(name, frequencyMHz, tpms);

    int activityPeaks = 0;
    bool wasHigh = false;
    int lastRssi = -110;
    uint32_t lastDraw = 0;

    while (true) {
        if (check(EscPress) || check(SelPress)) break;
        lastRssi = ELECHOUSE_cc1101.getRssi();
        // Fixed provisional threshold; calibration and sensitivity settings
        // will be added after hardware validation.
        const bool isHigh = (lastRssi >= -68);
        if (isHigh && !wasHigh) ++activityPeaks;
        wasHigh = isHigh;

        if (millis() - lastDraw >= 90) {
            const int w = tftWidth;
            const int cell = (w - 24) / 2;
            tft.fillRect(16, 101, cell - 12, 20, PANEL);
            tft.fillRect(24 + cell, 101, cell - 12, 20, PANEL);
            tft.setTextColor(isHigh ? GREEN : WHITE, PANEL);
            tft.drawString(String(lastRssi) + " dBm", 16, 102, 2);
            tft.setTextColor(WHITE, PANEL);
            tft.drawString(String(activityPeaks), 24 + cell, 102, 2);
            lastDraw = millis();
        }
        delay(20);
    }
    deinitRfModule();
}

} // namespace

void rfLabMenu() {
    std::vector<Option> items = {
        {"Key Tester | 433.92 MHz", []() { showMonitor("KEY FOB TESTER", 433.92f, false); }},
        {"Key Tester | 315.00 MHz", []() { showMonitor("KEY FOB TESTER", 315.00f, false); }},
        {"Key Tester | 868.35 MHz", []() { showMonitor("KEY FOB TESTER", 868.35f, false); }},
        {"TPMS Monitor | 433.92 MHz", []() { showMonitor("TPMS MONITOR", 433.92f, true); }},
        {"TPMS Monitor | 315.00 MHz", []() { showMonitor("TPMS MONITOR", 315.00f, true); }},
    };
    loopOptions(items, MENU_TYPE_SUBMENU, "ESPtrix RF Lab");
}
} // namespace esptrix
