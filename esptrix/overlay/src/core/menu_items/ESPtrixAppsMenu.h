// ESPTRIX apps drawer: all original Bruce menus remain untouched.
// Copyright (C) 2026 ESPTRIX contributors. AGPL-3.0-or-later
#pragma once
#include <MenuItemInterface.h>

class ESPtrixAppsMenu : public MenuItemInterface {
public:
    ESPtrixAppsMenu() : MenuItemInterface("ESPTRIX Apps") {}
    void optionsMenu(void) override;
    void drawIcon(float scale = 1) override;
    bool hasTheme() override { return false; } // Preserve standard menu rendering and ESPTRIX colours.
    const String& themePath() override { return bruceConfig.theme.paths.others; }
};
