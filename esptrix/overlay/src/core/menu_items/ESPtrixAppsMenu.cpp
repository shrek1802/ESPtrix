// Independent ESPTRIX drawer: never alter or replace Bruce app implementations.
// Copyright (C) 2026 ESPTRIX contributors. AGPL-3.0-or-later
#include "ESPtrixAppsMenu.h"
#include "core/display.h"
#include "esptrix/esptrix_ui.h"

void ESPtrixAppsMenu::optionsMenu() {
    options = {
        // Existing public ESPTRIX RF Lab; add V8/local tools here when their sources are integrated.
        {"RF Lab", []() { esptrix::rfLabMenu(); }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "ESPTRIX Apps");
}

void ESPtrixAppsMenu::drawIcon(float scale) {
    clearIconArea();
    // Compact four-tile apps icon; colour inherits the active ESPTRIX theme.
    const int cell = static_cast<int>(15 * scale);
    const int gap = static_cast<int>(5 * scale);
    const int radius = static_cast<int>(3 * scale);
    const int side = 2 * cell + gap;
    const int left = iconCenterX - side / 2;
    const int top = iconCenterY - side / 2;
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const int x = left + c * (cell + gap);
            const int y = top + r * (cell + gap);
            if (r == 0 && c == 0) {
                tft.fillRoundRect(x, y, cell, cell, radius, bruceConfig.priColor);
            } else {
                tft.drawRoundRect(x, y, cell, cell, radius, bruceConfig.priColor);
            }
        }
    }
}
