// ESPTRIX V3 icons, using Bruce's native MenuItemInterface renderer.
// User-approved artwork embedded as full-colour 88x88 RGB565 assets.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <MenuItemInterface.h>
#include "esptrix_icons_v3.h"
#include "esptrix/esptrix_ui.h"

namespace esptrix {

class IconMenuItem final : public MenuItemInterface {
public:
    using Action = void (*)();
    IconMenuItem(const char *name, uint8_t icon, Action action)
        : MenuItemInterface(name), iconIndex_(icon), action_(action) {}

    void optionsMenu(void) override {
        if (action_) action_();
    }

    void drawIcon(float scale = 1) override {
        (void)scale;
        clearIconArea();
        if (iconIndex_ >= icons::COUNT) return;
        // Bruce still draws the status bar, title, arrows, and handles rotation.
        // Native RGB565 assets preserve white/cyan anti-aliasing; no hand-drawn shapes.
        // ESPTRIX V3 RGB565 byte-order correction.
        // Assets are native uint16_t RGB565. TFT_eSPI otherwise interprets
        // their little-endian memory bytes in reverse order (cyan -> yellow).
        // Preserve Bruce's previous swap state for every other drawing call.
        const bool previousSwap = tft.getSwapBytes();
        tft.setSwapBytes(true);
        tft.pushImage(iconCenterX - icons::WIDTH / 2,
                      iconCenterY - icons::HEIGHT / 2,
                      icons::WIDTH, icons::HEIGHT,
                      icons::IMAGES[iconIndex_]);
        tft.setSwapBytes(previousSwap);
    }

    bool hasTheme() override { return false; }
    const String &themePath() override {
        static const String empty;
        return empty;
    }

private:
    uint8_t iconIndex_;
    Action action_;
};

inline void showAbout() {
    std::vector<Option> rows = {
        {"ESPTRIX Development", []() { displayInfo("Independent ESPTRIX firmware", true); }},
        {"Bruce 1.16.1 base", []() { displayInfo("Based on BruceDevices firmware", true); }},
        {"AGPL-3.0-or-later", []() { displayInfo("Open-source license: AGPL-3.0+", true); }},
        {"Project on GitHub", []() { displayInfo("github.com/shrek1802/ESPtrix", true); }},
    };
    loopOptions(rows, MENU_TYPE_SUBMENU, "About ESPTRIX");
}

} // namespace esptrix
