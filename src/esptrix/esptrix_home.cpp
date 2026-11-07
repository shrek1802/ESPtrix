// ESPTRIX home launcher for LILYGO T-Embed CC1101.
// Independent enhancements to Bruce Firmware, 2026-11-07.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "esptrix_home.h"
#include "esptrix_ui.h"
#include "core/main_menu.h"
#include <globals.h>
#include <stdint.h>

namespace esptrix {
namespace {
constexpr uint16_t BG = 0x0862;
constexpr uint16_t CARD = 0x1905;
constexpr uint16_t SELECTED = 0x24F1;
constexpr uint16_t BLUE = 0x45FF;
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t MUTED = 0xAD55;
constexpr uint16_t GREEN = 0x3F52;
constexpr int kColumns = 3;
constexpr int kPerPage = 6;
constexpr int kItems = 12;

struct LauncherItem {
    const char *label;
    const char *symbol;
    void (*open)();
};

void about() {
    tft.fillScreen(BG);
    tft.fillRect(0, 0, tftWidth, 25, CARD);
    tft.setTextColor(BLUE, CARD);
    tft.drawString("ESPTRIX  |  ABOUT", 9, 5, 2);
    tft.setTextColor(WHITE, BG);
    tft.drawString("An independent Bruce Firmware fork", 8, 37, 1);
    tft.drawString("Based on Bruce 1.16.1 / BruceDevices", 8, 53, 1);
    tft.setTextColor(GREEN, BG);
    tft.drawString("GNU AGPL-3.0-or-later", 8, 73, 2);
    tft.setTextColor(MUTED, BG);
    tft.drawString("NO WARRANTY. Share under AGPL terms.", 8, 100, 1);
    tft.drawString("github.com/shrek1802/ESPtrix", 8, 117, 1);
    tft.drawString("BACK or SELECT to return", 8, tftHeight - 18, 1);
    // The open-source attribution remains accessible on the device.
    check(SelPress); check(EscPress);
    delay(250);
    while (!check(SelPress) && !check(EscPress)) delay(20);
}

void leaveHome() {} // Returning to Bruce is handled in the caller.

LauncherItem items[kItems] = {
    {"KEY FOB", "K",   keyFobMenu},
    {"TPMS",    "P",   tpmsMenu},
    {"RF TOOLS","RF",  []() { mainMenu.rfMenu.optionsMenu(); }},
    {"WI-FI",   "W",   []() { mainMenu.wifiMenu.optionsMenu(); }},
    {"BLE",     "BT",  []() { mainMenu.bleMenu.optionsMenu(); }},
    {"NFC/RFID","N",   []() { mainMenu.rfidMenu.optionsMenu(); }},
    {"INFRARED","IR",  []() { mainMenu.irMenu.optionsMenu(); }},
    {"FILES",   "F",   []() { mainMenu.fileMenu.optionsMenu(); }},
    {"MORE",    "+",   []() { mainMenu.othersMenu.optionsMenu(); }},
    {"SETTINGS","S",   []() { mainMenu.configMenu.optionsMenu(); }},
    {"BRUCE",   "B",   leaveHome},
    {"ABOUT",   "i",   about},
};

void drawHeader(int page) {
    const int w = tftWidth;
    tft.fillRect(0, 0, w, 26, CARD);
    tft.fillRect(0, 0, 4, 26, BLUE);
    tft.setTextColor(WHITE, CARD);
    tft.drawString("ESP", 10, 4, 2);
    tft.setTextColor(BLUE, CARD);
    tft.drawString("TRIX", 38, 4, 2);
    tft.setTextColor(MUTED, CARD);
    tft.drawString("DEV", 92, 7, 1);
    tft.drawRightString(String(page + 1) + "/2", w - 48, 8, 1);
    int battery = getBattery();
    if (battery >= 0 && battery <= 100) {
        tft.setTextColor(GREEN, CARD);
        tft.drawRightString(String(battery) + "%", w - 8, 8, 1);
    }
}

void drawTile(int slot, const LauncherItem &item, bool focused) {
    const int w = tftWidth;
    const int tileW = (w - 36) / kColumns;
    const int tileH = 54;
    const int x = 9 + (slot % kColumns) * (tileW + 9);
    const int y = 33 + (slot / kColumns) * 59;
    const uint16_t bg = focused ? SELECTED : CARD;
    tft.fillRoundRect(x, y, tileW, tileH, 7, bg);
    if (focused) {
        tft.drawRoundRect(x, y, tileW, tileH, 7, BLUE);
        tft.fillRoundRect(x + 5, y + 7, 3, tileH - 14, 1, BLUE);
    }
    tft.setTextColor(focused ? WHITE : BLUE, bg);
    tft.drawCentreString(item.symbol, x + tileW / 2, y + 4, 2);
    tft.setTextColor(WHITE, bg);
    tft.drawCentreString(item.label, x + tileW / 2, y + 30, 1);
}

void drawPage(int selected) {
    tft.fillScreen(BG);
    int page = selected / kPerPage;
    drawHeader(page);
    for (int slot = 0; slot < kPerPage; ++slot) {
        int idx = page * kPerPage + slot;
        drawTile(slot, items[idx], idx == selected);
    }
    tft.setTextColor(MUTED, BG);
    tft.drawString("ROTATE: MOVE  |  PRESS: OPEN", 9, tftHeight - 13, 1);
    tft.drawRightString("BACK", tftWidth - 8, tftHeight - 13, 1);
}
} // namespace

void showHome() {
    // Only installed on the T-Embed CC1101 menu, but keep the home screen
    // independent from radio settings and never start any transmit feature.
    static int selected = 0;
    drawPage(selected);
    // Clear stale menu events when entering from Bruce's own main menu.
    check(SelPress); check(EscPress);
    check(NextPress); check(PrevPress);
#ifdef HAS_ENCODER
    drainRotarySteps();
#endif
    uint32_t inputGuardUntil = millis() + 350;
    while (true) {
        if (check(EscPress)) return;
        int delta = 0;
#ifdef HAS_ENCODER
        int32_t steps = drainRotarySteps();
        if (steps != 0) {
            check(NextPress); check(PrevPress);
            if (steps > kItems) steps = kItems;
            if (steps < -kItems) steps = -kItems;
            delta = -static_cast<int>(steps);
        }
#endif
        if (delta == 0) {
            if (check(NextPress)) delta = 1;
            else if (check(PrevPress)) delta = -1;
        }
        if (delta) {
            selected = (selected + delta % kItems + kItems) % kItems;
            drawPage(selected);
        }
        if (check(SelPress) && static_cast<int32_t>(millis() - inputGuardUntil) >= 0) {
            if (selected == 10) return; // Always allow access to the original Bruce home.
            items[selected].open();
            check(SelPress); check(EscPress);
#ifdef HAS_ENCODER
            drainRotarySteps();
#endif
            inputGuardUntil = millis() + 350;
            drawPage(selected);
        }
        delay(12);
    }
}
} // namespace esptrix
