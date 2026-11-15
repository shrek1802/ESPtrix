// ESPTRIX Apps: a single independent drawer alongside Bruce 1.17 icons.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <MenuItemInterface.h>

class ESPtrixAppsMenu : public MenuItemInterface {
public:
    ESPtrixAppsMenu() : MenuItemInterface("ESPTRIX Apps") {}
    void optionsMenu(void) override;
    void drawIcon(float scale = 1) override;
    bool hasTheme() override { return false; }
    const String& themePath() override { return bruceConfig.theme.paths.others; }
};
