// ESPTRIX RF Lab - polished, receive-only radio diagnostics.
// Original firmware: BruceDevices/firmware (Bruce 1.16.1).
// ESPTRIX modifications: 2026-11-07; SPDX-License-Identifier: AGPL-3.0-or-later

#include "esptrix_ui.h"
#include "esptrix_signal_meter.h"
#include "core/display.h"
#include "modules/rf/rf_utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <globals.h>
#include <vector>

namespace esptrix {
namespace {
constexpr uint16_t BACKGROUND = 0x0862;  // near-black midnight blue
constexpr uint16_t PANEL = 0x1905;       // card blue-grey
constexpr uint16_t ACCENT = 0x45FF;      // electric blue
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t SUBTLE = 0xA5B4;
constexpr uint16_t GREEN = 0x3F52;
constexpr uint16_t GRID = 0x3187;
constexpr unsigned kHistorySize = 48;
constexpr uint32_t kRefreshMs = 120;

// A signal-strength trend is NOT a demodulated pulse waveform.
// Packets, key correctness and TPMS fields cannot be inferred from RSSI alone.
int rssiToY(int rssi, int top, int bottom) {
    if (rssi < -110) rssi = -110;
    if (rssi > -30) rssi = -30;
    return bottom - ((rssi + 110) * (bottom - top)) / 80;
}

// Dashboard layout tuned for the actual 320x170 T-Embed display.
// Unlike the original concept artwork, the graph is measured RSSI history,
// not a demodulated OOK bitstream. Modulation is intentionally not guessed.
void drawFrame(const char *title, float frequencyMHz, bool tpms) {
    const int w = tftWidth;
    const int h = tftHeight;
    tft.fillScreen(BACKGROUND);

    tft.fillRect(0, 0, w, 25, PANEL);
    tft.fillRect(0, 0, 3, 25, ACCENT);
    tft.setTextColor(WHITE, PANEL);
    tft.drawString(title, 9, 4, 2);
    tft.setTextColor(GREEN, PANEL);
    tft.drawRightString("RX ONLY", w - 9, 8, 1);

    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString("TUNED FREQUENCY", 10, 30, 1);
    tft.setTextColor(WHITE, BACKGROUND);
    tft.drawString(String(frequencyMHz, 2) + " MHz", 10, 40, 4);

    tft.drawFastHLine(9, 70, w - 18, GRID);
    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString("RSSI", 11, 74, 1);
    tft.drawString("MODULATION", 116, 74, 1);
    tft.drawString("BURSTS", 244, 74, 1);
    tft.drawFastVLine(104, 74, 19, GRID);
    tft.drawFastVLine(232, 74, 19, GRID);

    // Values are updated independently; labels don't move or flicker.
    tft.fillRoundRect(8, 98, w - 16, 53, 5, PANEL);
    tft.setTextColor(SUBTLE, PANEL);
    tft.drawString("LIVE SIGNAL ACTIVITY", 16, 101, 1);
    tft.drawRightString("RSSI TREND", w - 16, 101, 1);
    tft.drawFastHLine(15, 127, w - 30, GRID);
    tft.drawFastHLine(15, 142, w - 30, GRID);

    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString(tpms ? "TPMS: ACTIVITY ONLY" : "REMOTE: ACTIVITY ONLY", 9, h - 13, 1);
    tft.drawRightString("BACK: EXIT", w - 8, h - 13, 1);
}

void drawReadings(const SignalSnapshot &state, const int history[kHistorySize]) {
    const int w = tftWidth;
    // Keep the static headers intact, redrawing only values and plot.
    tft.fillRect(10, 84, 89, 14, BACKGROUND);
    tft.fillRect(114, 84, 114, 14, BACKGROUND);
    tft.fillRect(244, 84, w - 253, 14, BACKGROUND);

    tft.setTextColor(state.active ? GREEN : WHITE, BACKGROUND);
    tft.drawString(String(state.rssiDbm) + " dBm", 11, 83, 2);
    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString("--", 117, 83, 2);  // not decoded
    tft.setTextColor(WHITE, BACKGROUND);
    tft.drawString(String(state.peaks), 245, 83, 2);

    tft.fillRect(15, 113, w - 30, 35, PANEL);
    tft.drawFastHLine(15, 127, w - 30, GRID);
    tft.drawFastHLine(15, 142, w - 30, GRID);
    for (unsigned i = 1; i < kHistorySize; ++i) {
        const int x0 = 16 + ((w - 34) * (i - 1)) / (kHistorySize - 1);
        const int x1 = 16 + ((w - 34) * i) / (kHistorySize - 1);
        tft.drawLine(x0, rssiToY(history[i - 1], 115, 145),
                     x1, rssiToY(history[i], 115, 145), ACCENT);
    }
    tft.setTextColor(state.active ? GREEN : SUBTLE, PANEL);
    tft.drawRightString(state.calibrated
        ? (state.active ? "SIGNAL" : "LISTENING") : "CALIBRATING",
        w - 17, 101, 1);
}

void showMonitor(const char *title, float frequencyMHz, bool tpms) {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("ESPTRIX RF Lab requires CC1101", true);
        return;
    }

    if (!initRfModule("rx", frequencyMHz)) {
        displayError("Unable to start CC1101 receiver", true);
        return;
    }

    drawFrame(title, frequencyMHz, tpms);
    SignalMeter meter;
    int history[kHistorySize];
    for (unsigned i = 0; i < kHistorySize; ++i) history[i] = -110;
    uint32_t lastDraw = millis() - kRefreshMs;

    // This monitor never calls SetTx, RF send, or any replay path.
    while (true) {
        if (check(EscPress) || check(SelPress)) break;
        const uint32_t now = millis();
        const SignalSnapshot state = meter.sample(ELECHOUSE_cc1101.getRssi(), now);
        if (static_cast<uint32_t>(now - lastDraw) >= kRefreshMs) {
            for (unsigned i = 1; i < kHistorySize; ++i) history[i - 1] = history[i];
            history[kHistorySize - 1] = state.rssiDbm;
            drawReadings(state, history);
            lastDraw = now;
        }
        delay(20);
    }
    deinitRfModule();
}
}  // namespace

void keyFobMonitor(float frequencyMHz) {
    showMonitor("KEY FOB TESTER", frequencyMHz, false);
}

void tpmsMonitor(float frequencyMHz) {
    showMonitor("TPMS MONITOR", frequencyMHz, true);
}

void keyFobMenu() {
    std::vector<Option> options = {
        {"433.92 MHz", []() { showMonitor("KEY FOB TESTER", 433.92f, false); }},
        {"315.00 MHz", []() { showMonitor("KEY FOB TESTER", 315.00f, false); }},
        {"868.35 MHz", []() { showMonitor("KEY FOB TESTER", 868.35f, false); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Key Fob Tester");
}

void tpmsMenu() {
    std::vector<Option> options = {
        {"433.92 MHz", []() { showMonitor("TPMS MONITOR", 433.92f, true); }},
        {"315.00 MHz", []() { showMonitor("TPMS MONITOR", 315.00f, true); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "TPMS Monitor");
}

void rfLabMenu() {
    std::vector<Option> items = {
        {"Key Fob Tester | 433.92 MHz", []() { showMonitor("KEY FOB TESTER", 433.92f, false); }},
        {"Key Fob Tester | 315 MHz", []() { showMonitor("KEY FOB TESTER", 315.00f, false); }},
        {"Key Fob Tester | 868.35 MHz", []() { showMonitor("KEY FOB TESTER", 868.35f, false); }},
        {"TPMS Monitor | 433.92 MHz", []() { showMonitor("TPMS MONITOR", 433.92f, true); }},
        {"TPMS Monitor | 315 MHz", []() { showMonitor("TPMS MONITOR", 315.00f, true); }},
    };
    loopOptions(items, MENU_TYPE_SUBMENU, "ESPtrix RF Lab");
}
}  // namespace esptrix
