// Independent ESPTRIX drawer: never alter or replace Bruce app implementations.
// Copyright (C) 2026 ESPTRIX contributors. AGPL-3.0-or-later
#include "ESPtrixAppsMenu.h"
#include "core/display.h"
// The public repository has the early RF Lab only. Local ESPTRIX V8 builds
// additionally expose the unified category dispatcher.
#if __has_include("esptrix/esptrix_unified_menu.h")
#include "esptrix/esptrix_unified_menu.h"
#define ESPTRIX_HAS_UNIFIED_APPS 1
#else
#include "esptrix/esptrix_ui.h"
#define ESPTRIX_HAS_UNIFIED_APPS 0
#endif

void ESPtrixAppsMenu::optionsMenu() {
    options = {
#if ESPTRIX_HAS_UNIFIED_APPS
        {"Smart Test", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::SmartTest); }},
        {"Automotive Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::Automotive); }},
        {"Access Control Lab", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::AccessControl); }},
        {"RF & Sub-GHz Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::SubGhz); }},
        {"Wi-Fi Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::WifiNetwork); }},
        {"BLE Tools", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::Bluetooth); }},
        {"Infrared & USB", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::IrUsb); }},
        {"Files & Reports", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::FilesReports); }},
        {"ESPTRIX System", []() { esptrix::openUnifiedCategory(esptrix::CategoryId::System); }},
#else
        // Early public-source fallback until local V8 modules are migrated.
        {"RF Lab", []() { esptrix::rfLabMenu(); }},
#endif
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
