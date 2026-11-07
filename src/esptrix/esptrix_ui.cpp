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

void drawFrame(const char *title, float frequencyMHz, bool tpms) {
    const int w = tftWidth;
    const int h = tftHeight;
    const int cardWidth = (w - 24) / 2;
    tft.fillScreen(BACKGROUND);
    tft.fillRect(0, 0, w, 23, PANEL);
    tft.fillRect(0, 0, 4, 23, ACCENT);
    tft.setTextColor(WHITE, PANEL);
    tft.drawString("ESPTRIX", 10, 4, 2);
    tft.setTextColor(ACCENT, PANEL);
    tft.drawRightString(title, w - 9, 6, 1);

    tft.setTextColor(WHITE, BACKGROUND);
    tft.drawString(String(frequencyMHz, 2) + " MHz", 10, 31, 4);
    tft.fillRoundRect(w - 98, 30, 90, 26, 6, PANEL);
    tft.setTextColor(GREEN, PANEL);
    tft.drawString("RX ONLY", w - 88, 35, 2);

    tft.fillRoundRect(8, 64, cardWidth, 45, 6, PANEL);
    tft.fillRoundRect(16 + cardWidth, 64, cardWidth, 45, 6, PANEL);
    tft.setTextColor(SUBTLE, PANEL);
    tft.drawString("SIGNAL STRENGTH", 16, 69, 1);
    tft.drawString("ACTIVITY PEAKS", 24 + cardWidth, 69, 1);

    tft.fillRoundRect(8, 112, w - 16, 39, 6, PANEL);
    tft.setTextColor(SUBTLE, PANEL);
    tft.drawString("RSSI HISTORY  /  NOT PACKET DATA", 15, 114, 1);
    tft.drawFastHLine(15, 137, w - 30, GRID);

    tft.setTextColor(SUBTLE, BACKGROUND);
    tft.drawString(tpms ? "TPMS: NO SENSOR DECODER YET" : "KEY: ACTIVITY, NOT VALIDATION", 9, h - 15, 1);
    tft.drawRightString("BACK/OK: EXIT", w - 8, h - 15, 1);
}

void drawReadings(const SignalSnapshot &state, const int history[kHistorySize]) {
    const int w = tftWidth;
    const int cardWidth = (w - 24) / 2;
    tft.fillRect(15, 82, cardWidth - 12, 23, PANEL);
    tft.fillRect(23 + cardWidth, 82, cardWidth - 12, 23, PANEL);

    tft.setTextColor(state.active ? GREEN : WHITE, PANEL);
    tft.drawString(String(state.rssiDbm) + " dBm", 16, 84, 2);
    tft.setTextColor(WHITE, PANEL);
    tft.drawString(String(state.peaks), 24 + cardWidth, 84, 2);

    // Redraw only the history region; keep header and cards stationary.
    tft.fillRect(15, 125, w - 30, 23, PANEL);
    tft.drawFastHLine(15, 137, w - 30, GRID);
    for (unsigned i = 1; i < kHistorySize; ++i) {
        const int x0 = 16 + ((w - 34) * (i - 1)) / (kHistorySize - 1);
        const int x1 = 16 + ((w - 34) * i) / (kHistorySize - 1);
        tft.drawLine(
            x0, rssiToY(history[i - 1], 126, 146),
            x1, rssiToY(history[i], 126, 146), ACCENT
        );
    }
    tft.setTextColor(state.calibrated ? SUBTLE : ACCENT, PANEL);
    tft.drawRightString(state.calibrated ? "LIVE" : "CAL", w - 17, 114, 1);
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
