// ESPTRIX V7 category icon adapter. Uses native Bruce carousel renderer.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <MenuItemInterface.h>
#include "esptrix_unified_menu.h"
#include "esptrix_unified_artwork.h"
#include "esptrix/esptrix_icons_v3.h"
#include "esptrix/esptrix_access_control_icon.h"

namespace esptrix {
class UnifiedCategoryItem final : public MenuItemInterface {
public:
    explicit UnifiedCategoryItem(const CategoryDefinition &category)
        : MenuItemInterface(category.title), id_(category.id) {}
    void optionsMenu(void) override { openUnifiedCategory(id_); }
    void drawIcon(float scale = 1) override {
        (void)scale;
        clearIconArea();
        const uint16_t *bmp = nullptr;
        switch (id_) {
            case CategoryId::SmartTest: bmp = unifiedIcons::SMART_TEST; break;
            case CategoryId::Automotive: bmp = icons::key_fob; break;
            case CategoryId::AccessControl: bmp = ACCESS_CONTROL_ICON; break;
            case CategoryId::SubGhz: bmp = icons::rf; break;
            case CategoryId::WifiNetwork: bmp = icons::wifi; break;
            case CategoryId::Bluetooth: bmp = icons::bluetooth; break;
            case CategoryId::IrUsb: bmp = unifiedIcons::IR_USB; break;
            case CategoryId::FilesReports: bmp = icons::files; break;
            case CategoryId::System: bmp = icons::settings; break;
        }
        if (!bmp) return;
        const bool previousSwap = tft.getSwapBytes();
        tft.setSwapBytes(true); // Preserve cyan instead of yellow/green.
        tft.pushImage(iconCenterX - 44, iconCenterY - 44, 88, 88, bmp);
        tft.setSwapBytes(previousSwap);
    }
    bool hasTheme() override { return false; }
    const String &themePath() override {
        static const String empty;
        return empty;
    }
private:
    CategoryId id_;
};
}
