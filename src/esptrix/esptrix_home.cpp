// ESPTRIX icon-carousel launcher for LILYGO T-Embed CC1101, 320x170.
// Native ESPTRIX work derived from Bruce Firmware, 2026-11-07.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "esptrix_home.h"
#include "esptrix_ui.h"
#include "core/main_menu.h"
#include "core/utils.h"
#include <globals.h>
#include <stdint.h>

namespace esptrix {
namespace {
bool launchNativeHome = true;
constexpr uint16_t BG = 0x0862;     // midnight navy
constexpr uint16_t PANEL = 0x1926;  // subtle selection
constexpr uint16_t BLUE = 0x45FF;   // electric blue
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t MUTED = 0xA554;
constexpr uint16_t LINE = 0x29A9;
constexpr uint16_t GREEN = 0x3F52;
constexpr int kItemCount = 12;

struct HomeCategory {
    const char *title;
    const char *subtitle;
};
const HomeCategory categories[kItemCount] = {
    {"KEY FOB",  "Signal and burst testing"},
    {"TPMS",     "Tyre sensor monitoring"},
    {"RF TOOLS", "Sub-GHz diagnostics"},
    {"WI-FI",    "Wireless network tools"},
    {"BLUETOOTH","BLE discovery and tools"},
    {"NFC/RFID", "Contactless tools"},
    {"INFRARED", "IR capture and tools"},
    {"FILES",    "Storage and saved data"},
    {"MORE",     "Additional utilities"},
    {"SETTINGS", "Device configuration"},
    {"BRUCE",    "Original firmware menu"},
    {"ABOUT",    "ESPTRIX and licensing"}
};

struct MenuRow {
    const char *title;
    const char *description;
    void (*action)();
};

void drawTopBar(const char *title, int index, int total) {
    tft.fillRect(0, 0, tftWidth, 27, PANEL);
    tft.fillRect(0, 0, 3, 27, BLUE);
    tft.setTextColor(WHITE, PANEL);
    tft.drawString(title, 11, 5, 2);
    tft.setTextColor(BLUE, PANEL);
    if (total > 0) {
        tft.drawRightString(String(index + 1) + "/" + String(total), tftWidth - 11, 9, 1);
    }
    tft.drawFastHLine(9, 27, tftWidth - 18, LINE);
}

// Each icon is a simple ESPTRIX vector drawing: no Flipper artwork is used.
void drawSymbol(int id, int x, int y, uint16_t colour) {
    switch (id) {
        case 0:  // Key / remote
            tft.drawCircle(x - 9, y - 5, 7, colour);
            tft.fillCircle(x - 9, y - 5, 2, colour);
            tft.drawLine(x - 2, y, x + 12, y + 14, colour);
            tft.drawLine(x + 5, y + 7, x + 10, y + 2, colour);
            tft.drawLine(x + 9, y + 11, x + 14, y + 6, colour);
            break;
        case 1:  // Wheel / TPMS
            tft.drawCircle(x, y, 17, colour);
            tft.drawCircle(x, y, 11, colour);
            tft.fillCircle(x, y, 4, colour);
            tft.drawLine(x, y - 10, x, y - 5, colour);
            tft.drawLine(x, y + 5, x, y + 10, colour);
            tft.drawLine(x - 10, y, x - 5, y, colour);
            tft.drawLine(x + 5, y, x + 10, y, colour);
            break;
        case 2:  // RF aerial
            tft.drawLine(x, y + 16, x, y - 10, colour);
            tft.fillCircle(x, y - 11, 3, colour);
            tft.drawLine(x - 7, y + 16, x + 7, y + 16, colour);
            tft.drawArc(x, y - 8, 13, 11, 115, 245, colour, BG);
            tft.drawArc(x, y - 8, 20, 18, 120, 240, colour, BG);
            break;
        case 3:  // Wi-Fi bars
            tft.fillCircle(x, y + 13, 3, colour);
            tft.drawArc(x, y + 12, 13, 11, 210, 330, colour, BG);
            tft.drawArc(x, y + 12, 21, 19, 210, 330, colour, BG);
            break;
        case 4:  // Bluetooth rune
            tft.drawLine(x, y - 18, x, y + 18, colour);
            tft.drawLine(x, y - 18, x + 12, y - 8, colour);
            tft.drawLine(x + 12, y - 8, x - 9, y + 10, colour);
            tft.drawLine(x - 9, y - 10, x + 12, y + 8, colour);
            tft.drawLine(x + 12, y + 8, x, y + 18, colour);
            break;
        case 5:  // Contactless
            tft.drawRoundRect(x - 18, y - 14, 21, 28, 4, colour);
            tft.drawArc(x + 2, y, 10, 8, 290, 70, colour, BG);
            tft.drawArc(x + 2, y, 18, 16, 290, 70, colour, BG);
            break;
        case 6:  // IR LED
            tft.drawCircle(x - 8, y, 9, colour);
            tft.drawLine(x + 3, y - 8, x + 15, y - 16, colour);
            tft.drawLine(x + 4, y, x + 20, y, colour);
            tft.drawLine(x + 3, y + 8, x + 15, y + 16, colour);
            break;
        case 7:  // Folder
            tft.drawRoundRect(x - 19, y - 10, 38, 27, 3, colour);
            tft.drawLine(x - 17, y - 15, x - 2, y - 15, colour);
            tft.drawLine(x - 2, y - 15, x + 3, y - 10, colour);
            break;
        case 8:  // More
            tft.fillCircle(x - 12, y, 4, colour);
            tft.fillCircle(x, y, 4, colour);
            tft.fillCircle(x + 12, y, 4, colour);
            break;
        case 9:  // Settings
            tft.drawCircle(x, y, 15, colour);
            tft.drawCircle(x, y, 6, colour);
            tft.drawLine(x, y - 22, x, y - 15, colour);
            tft.drawLine(x, y + 15, x, y + 22, colour);
            tft.drawLine(x - 22, y, x - 15, y, colour);
            tft.drawLine(x + 15, y, x + 22, y, colour);
            break;
        case 10: // Bruce shortcut
            tft.drawCircle(x, y, 19, colour);
            tft.setTextColor(colour, BG);
            tft.drawCentreString("B", x, y - 12, 4);
            break;
        default: // Info
            tft.drawCircle(x, y, 19, colour);
            tft.fillCircle(x, y - 10, 2, colour);
            tft.drawLine(x, y - 2, x, y + 11, colour);
            break;
    }
}

void drawCarousel(int selected) {
    tft.fillScreen(BG);
    drawTopBar("ESPTRIX", selected, kItemCount);
    constexpr int centre = 160;
    constexpr int cy = 71;
    const int before = (selected + kItemCount - 1) % kItemCount;
    const int after = (selected + 1) % kItemCount;

    // Circular selection and faded neighbours, not a grid of boxes.
    tft.drawCircle(centre, cy, 32, BLUE);
    tft.drawCircle(centre, cy, 31, BLUE);
    tft.drawCircle(centre - 105, cy, 23, LINE);
    tft.drawCircle(centre + 105, cy, 23, LINE);
    drawSymbol(before, centre - 105, cy, MUTED);
    drawSymbol(selected, centre, cy, WHITE);
    drawSymbol(after, centre + 105, cy, MUTED);

    tft.setTextColor(WHITE, BG);
    tft.drawCentreString(categories[selected].title, centre, 110, 2);
    tft.setTextColor(MUTED, BG);
    tft.drawCentreString(categories[selected].subtitle, centre, 131, 1);
    tft.drawFastHLine(9, 148, tftWidth - 18, LINE);
    tft.drawString("< ROTATE >", 12, 155, 1);
    tft.drawCentreString("PRESS: MENU", centre, 155, 1);
    tft.drawRightString("BACK", tftWidth - 10, 155, 1);
}

int inputDelta() {
#ifdef HAS_ENCODER
    const int32_t steps = drainRotarySteps();
    if (steps != 0) {
        check(NextPress);
        check(PrevPress);
        // Keep Bruce's encoder sense: positive steps are Previous.
        int delta = static_cast<int>(steps > 20 ? 20 : steps < -20 ? -20 : steps);
        return -delta;
    }
#endif
    if (check(NextPress)) return 1;
    if (check(PrevPress)) return -1;
    return 0;
}

void clearInput() {
    check(SelPress);
    check(EscPress);
    check(NextPress);
    check(PrevPress);
#ifdef HAS_ENCODER
    drainRotarySteps();
#endif
}

void drawList(const char *title, const MenuRow *rows, int count, int selected) {
    tft.fillScreen(BG);
    drawTopBar(title, selected, count);
    const int top = selected >= 3 ? selected - 2 : 0;
    const int first = (count > 3 && top > count - 3) ? count - 3 : top;
    for (int slot = 0; slot < 3; ++slot) {
        const int idx = first + slot;
        if (idx >= count) break;
        const int y = 33 + slot * 40;
        const bool active = idx == selected;
        if (active) {
            tft.fillRoundRect(7, y, tftWidth - 14, 38, 5, PANEL);
            tft.fillRoundRect(8, y + 4, 3, 30, 1, BLUE);
        } else {
            tft.drawFastHLine(9, y + 38, tftWidth - 18, LINE);
        }
        tft.setTextColor(active ? BLUE : MUTED, active ? PANEL : BG);
        tft.drawCentreString(">", 22, y + 9, 2);
        tft.setTextColor(WHITE, active ? PANEL : BG);
        tft.drawString(rows[idx].title, 38, y + 3, 2);
        tft.setTextColor(MUTED, active ? PANEL : BG);
        tft.drawString(rows[idx].description, 38, y + 23, 1);
        tft.drawRightString(">", tftWidth - 18, y + 11, 1);
    }
    tft.setTextColor(MUTED, BG);
    tft.drawString("ROTATE: MOVE", 10, tftHeight - 13, 1);
    tft.drawRightString("PRESS: OPEN    BACK", tftWidth - 9, tftHeight - 13, 1);
}

void showList(const char *title, const MenuRow *rows, int count) {
    int selected = 0;
    clearInput();
    drawList(title, rows, count, selected);
    uint32_t ignoreSelectionUntil = millis() + 280;
    while (true) {
        if (check(EscPress)) break;
        const int delta = inputDelta();
        if (delta != 0) {
            selected = (selected + delta % count + count) % count;
            drawList(title, rows, count, selected);
        }
        if (check(SelPress) && static_cast<int32_t>(millis() - ignoreSelectionUntil) >= 0) {
            rows[selected].action();
            clearInput();
            ignoreSelectionUntil = millis() + 280;
            drawList(title, rows, count, selected);
        }
        delay(12);
    }
}

void about() {
    tft.fillScreen(BG);
    drawTopBar("ABOUT ESPTRIX", 0, 0);
    tft.setTextColor(WHITE, BG);
    tft.drawString("Independent Bruce Firmware fork", 10, 39, 1);
    tft.drawString("Upstream: BruceDevices / 1.16.1", 10, 56, 1);
    tft.setTextColor(BLUE, BG);
    tft.drawString("GNU AGPL-3.0-or-later", 10, 77, 2);
    tft.setTextColor(MUTED, BG);
    tft.drawString("github.com/shrek1802/ESPtrix", 10, 108, 1);
    tft.drawString("BACK / PRESS TO RETURN", 10, 148, 1);
    clearInput();
    uint32_t guard = millis() + 250;
    while (true) {
        if (check(EscPress)) break;
        if (check(SelPress) && static_cast<int32_t>(millis() - guard) >= 0) break;
        delay(12);
    }
}

void key433() { keyFobMonitor(433.92f); }
void key315() { keyFobMonitor(315.0f); }
void key868() { keyFobMonitor(868.35f); }
void tpms433() { tpmsMonitor(433.92f); }
void tpms315() { tpmsMonitor(315.0f); }

void openCategory(int index) {
    switch (index) {
        case 0: {
            const MenuRow rows[] = {
                {"433.92 MHz", "UK/EU key remote signal monitor", key433},
                {"315.00 MHz", "315 MHz remote signal monitor", key315},
                {"868.35 MHz", "868 MHz remote signal monitor", key868}
            };
            showList("KEY FOB TESTER", rows, 3);
            break;
        }
        case 1: {
            const MenuRow rows[] = {
                {"433.92 MHz", "Passive TPMS activity, no decoding", tpms433},
                {"315.00 MHz", "Passive TPMS activity, no decoding", tpms315}
            };
            showList("TPMS MONITOR", rows, 2);
            break;
        }
        case 2: {
            const MenuRow rows[] = {
                {"ESPTRIX RF LAB", "Signal strength and burst diagnostics", rfLabMenu},
                {"BRUCE RF TOOLS", "Original full Sub-GHz tool menu", []() { mainMenu.rfMenu.optionsMenu(); }}
            };
            showList("RF TOOLS", rows, 2);
            break;
        }
        case 3: {
            const MenuRow rows[] = {
                {"WI-FI TOOLS", "Open the original Wi-Fi utilities", []() { mainMenu.wifiMenu.optionsMenu(); }}
            };
            showList("WI-FI", rows, 1);
            break;
        }
        case 4: {
            const MenuRow rows[] = {
                {"BLE TOOLS", "Open the original Bluetooth utilities", []() { mainMenu.bleMenu.optionsMenu(); }}
            };
            showList("BLUETOOTH", rows, 1);
            break;
        }
        case 5: {
            const MenuRow rows[] = {
                {"NFC/RFID TOOLS", "Open the original contactless tools", []() { mainMenu.rfidMenu.optionsMenu(); }}
            };
            showList("NFC/RFID", rows, 1);
            break;
        }
        case 6: {
            const MenuRow rows[] = {
                {"INFRARED TOOLS", "Open the original IR utilities", []() { mainMenu.irMenu.optionsMenu(); }}
            };
            showList("INFRARED", rows, 1);
            break;
        }
        case 7: {
            const MenuRow rows[] = {
                {"FILE MANAGER", "Browse the SD card and saved data", []() { mainMenu.fileMenu.optionsMenu(); }}
            };
            showList("FILES", rows, 1);
            break;
        }
        case 8: {
            const MenuRow rows[] = {
                {"OTHER UTILITIES", "Open the original additional tools", []() { mainMenu.othersMenu.optionsMenu(); }}
            };
            showList("MORE", rows, 1);
            break;
        }
        case 9: {
            const MenuRow rows[] = {
                {"DEVICE SETTINGS", "Display, radio and device options", []() { mainMenu.configMenu.optionsMenu(); }}
            };
            showList("SETTINGS", rows, 1);
            break;
        }
        case 11: about(); break;
        default: break;  // BRUCE is handled by showHome().
    }
}
} // namespace

bool nativeHomeEnabled() { return launchNativeHome; }
void setNativeHomeEnabled(bool enabled) { launchNativeHome = enabled; }

void showHome() {
    // The original Bruce main menu remains a recovery path.
    static int selected = 0;
    clearInput();
    drawCarousel(selected);
    uint32_t ignoreSelectionUntil = millis() + 300;
    while (true) {
        if (check(EscPress)) {
            launchNativeHome = false;
            return;
        }
        const int delta = inputDelta();
        if (delta != 0) {
            selected = (selected + delta % kItemCount + kItemCount) % kItemCount;
            drawCarousel(selected);
        }
        if (check(SelPress) && static_cast<int32_t>(millis() - ignoreSelectionUntil) >= 0) {
            if (selected == 10) {
                launchNativeHome = false;
                return;
            }
            openCategory(selected);
            clearInput();
            ignoreSelectionUntil = millis() + 300;
            drawCarousel(selected);
        }
        delay(12);
    }
}
} // namespace esptrix
