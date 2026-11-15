// ESPTRIX V3 minimal RF monitor for T-Embed CC1101; receive only.
// Based on ESPTRIX/Bruce 1.16.1; SPDX-License-Identifier: AGPL-3.0-or-later
#include "esptrix_ui.h"
#include "esptrix_signal_meter.h"
#include "core/display.h"
#include "modules/rf/rf_utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <globals.h>
#include <vector>

namespace esptrix {
namespace {
constexpr uint16_t BG    = 0x0000;
constexpr uint16_t CYAN  = 0x15DF;
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t SOFT  = 0xB596;
constexpr uint16_t LINE  = 0x2987;
constexpr uint16_t GREEN = 0x2F51;
constexpr uint32_t REFRESH_MS = 160;

void beginTypography() {
    // Bruce tools sometimes leave the display at text size 2 or 3;
    // reset it before every redraw to prevent clipped/wonky labels.
    tft.setTextSize(1);
    tft.setTextFont(1);
    tft.setTextDatum(TL_DATUM);
}

void drawFrame(const char *title, float freq, bool tpms) {
    beginTypography();
    const int w = tftWidth;
    tft.fillScreen(BG);
    tft.setTextColor(WHITE, BG);
    tft.setTextSize(2);
    tft.drawString(title, 9, 4, 1);
    tft.setTextSize(1);
    tft.setTextColor(CYAN, BG);
    tft.drawRightString("RX ONLY", w - 10, 9, 1);
    tft.drawFastHLine(9, 26, w - 18, LINE);

    tft.setTextColor(SOFT, BG);
    tft.drawString("FREQUENCY", 10, 34, 1);
    tft.setTextColor(WHITE, BG);
    // Built-in font 1 at size 4 works even on boards without optional font 4.
    tft.setTextSize(4);
    tft.drawString(String(freq, 2), 10, 45, 1);
    tft.setTextSize(2);
    tft.setTextColor(CYAN, BG);
    tft.drawString("MHz", 178, 58, 1);
    tft.setTextSize(1);
    tft.drawFastHLine(9, 80, w - 18, LINE);

    tft.setTextColor(SOFT, BG);
    tft.drawString("RSSI", 11, 89, 1);
    tft.drawString("BURSTS", 172, 89, 1);
    tft.drawFastVLine(157, 88, 30, LINE);
    // Dynamic values are placed at y=101 in drawReadings().
    tft.drawFastHLine(9, 123, w - 18, LINE);
    tft.setTextColor(SOFT, BG);
    tft.drawString(tpms ? "TPMS RF ACTIVITY ONLY" : "REMOTE RF ACTIVITY ONLY", 10, 156, 1);
    tft.drawRightString("BACK", w - 10, 156, 1);
}

void drawReadings(const SignalSnapshot &state, bool tpms) {
    beginTypography();
    const int w = tftWidth;
    // Update only the changing values, not the whole screen.
    tft.fillRect(10, 100, 143, 21, BG);
    tft.fillRect(170, 100, w - 180, 21, BG);
    tft.setTextColor(WHITE, BG);
    tft.setTextSize(2);
    tft.drawString(String(state.rssiDbm) + " dBm", 10, 101, 1);
    tft.setTextColor(CYAN, BG);
    tft.drawString(String(state.peaks), 172, 101, 1);
    tft.setTextSize(1);

    tft.fillRect(9, 128, w - 18, 24, BG);
    if (!state.calibrated) {
        tft.setTextColor(SOFT, BG);
        tft.setTextSize(2);
        tft.drawString("CALIBRATING...", 10, 132, 1);
        tft.setTextSize(1);
    } else if (state.active) {
        tft.setTextColor(GREEN, BG);
        tft.fillCircle(16, 140, 4, GREEN);
        tft.setTextSize(2);
        tft.drawString("SIGNAL DETECTED", 28, 132, 1);
        tft.setTextSize(1);
    } else {
        tft.setTextColor(CYAN, BG);
        tft.fillCircle(16, 140, 3, CYAN);
        tft.setTextSize(2);
        tft.drawString(tpms ? "LISTENING TPMS" : "LISTENING REMOTE", 28, 132, 1);
        tft.setTextSize(1);
    }
}

void showMonitor(const char *title, float frequencyMHz, bool tpms) {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("ESPTRIX requires the CC1101 receiver", true);
        return;
    }
    if (!initRfModule("rx", frequencyMHz)) {
        displayError("Unable to start CC1101 receiver", true);
        return;
    }

    drawFrame(title, frequencyMHz, tpms);
    SignalMeter meter;
    // Prevent a menu-selection click from immediately exiting the monitor.
    check(SelPress);
    check(EscPress);
    uint32_t lastRender = millis() - REFRESH_MS;
    while (true) {
        if (check(EscPress) || check(SelPress)) break;
        const uint32_t now = millis();
        const SignalSnapshot state = meter.sample(ELECHOUSE_cc1101.getRssi(), now);
        if (static_cast<uint32_t>(now - lastRender) >= REFRESH_MS) {
            drawReadings(state, tpms);
            lastRender = now;
        }
        delay(20);
    }
    deinitRfModule();
}
} // namespace

void keyFobMonitor(float frequencyMHz) { showMonitor("KEY FOB TESTER", frequencyMHz, false); }
void tpmsMonitor(float frequencyMHz) { showMonitor("TPMS MONITOR", frequencyMHz, true); }

void keyFobMenu() {
    std::vector<Option> options = {
        {"433.92 MHz", []() { keyFobMonitor(433.92f); }},
        {"315.00 MHz", []() { keyFobMonitor(315.00f); }},
        {"868.35 MHz", []() { keyFobMonitor(868.35f); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Key Fob Tester");
}
void tpmsMenu() {
    std::vector<Option> options = {
        {"433.92 MHz", []() { tpmsMonitor(433.92f); }},
        {"315.00 MHz", []() { tpmsMonitor(315.00f); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "TPMS Monitor");
}
void rfLabMenu() {
    std::vector<Option> options = {
        {"Key Fob - 433.92 MHz", []() { keyFobMonitor(433.92f); }},
        {"Key Fob - 315 MHz", []() { keyFobMonitor(315.00f); }},
        {"Key Fob - 868.35 MHz", []() { keyFobMonitor(868.35f); }},
        {"TPMS - 433.92 MHz", []() { tpmsMonitor(433.92f); }},
        {"TPMS - 315 MHz", []() { tpmsMonitor(315.00f); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "ESPTRIX RF LAB");
}
} // namespace esptrix
