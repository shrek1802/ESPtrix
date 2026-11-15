// ESPTRIX Apps drawer. Original Bruce 1.17 categories remain on main screen.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ESPtrixAppsMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "esptrix/esptrix_unified_menu.h"
#include <algorithm>

void ESPtrixAppsMenu::optionsMenu() {
    options = {
        {"Smart Test", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::SmartTest); }},
        {"Automotive Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::Automotive); }},
        {"Access Control Lab", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::AccessControl); }},
        {"RF & Sub-GHz Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::SubGhz); }},
        {"Wi-Fi Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::WifiNetwork); }},
        {"BLE Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::Bluetooth); }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "ESPTRIX Apps");
}

void ESPtrixAppsMenu::drawIcon(float scale) {
    clearIconArea();
    // Four rounded application tiles, using the existing electric-blue theme.
    const int tile = std::max(6, static_cast<int>(14 * scale));
    const int gap = std::max(2, static_cast<int>(4 * scale));
    const int side = tile * 2 + gap;
    const int x0 = iconCenterX - side / 2;
    const int y0 = iconCenterY - side / 2;
    const int rad = std::max(2, static_cast<int>(3 * scale));
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const int x = x0 + c * (tile + gap);
            const int y = y0 + r * (tile + gap);
            if (r == 0 && c == 0) tft.fillRoundRect(x, y, tile, tile, rad, bruceConfig.priColor);
            else tft.drawRoundRect(x, y, tile, tile, rad, bruceConfig.priColor);
        }
    }
}
