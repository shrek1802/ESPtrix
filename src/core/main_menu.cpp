// ESPTRIX V7 UNIFIED MENU - nine portable categories
// ESPTRIX V6.1 FRONT MENU — Access Control Lab + grouped new tools
// ESPTRIX V4 TEST LAB integration
// ESPTRIX BRUCE-RENDERER RESET v1
#include "main_menu.h"
#if defined(T_EMBED_1101)
#include "esptrix/esptrix_bruce_icons.h"
#include "esptrix/esptrix_access_control_icon.h"
#include "esptrix/esptrix_unified_icons.h"
#include "esptrix/esptrix_test_lab.h"
#endif
#include "display.h"
#include "utils.h"
#include <globals.h>

MainMenu::MainMenu() {
    _menuItems = {
        &wifiMenu,
        &bleMenu,
        &rfMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
        &loraMenu,
#endif
#if defined(FM_SI4713) && !defined(LITE_VERSION)
        &fmMenu,
#endif
        &irMenu,
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &gpsMenu,
        &rfidMenu,
        &fileMenu,
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
#endif
        &clockMenu,
        &othersMenu,
        &configMenu,
    };

    _totalItems = _menuItems.size();
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
#if defined(T_EMBED_1101)
    // BRUCE RECOVERY: hold the physical BACK key during the first menu entry.
    static bool bootChoiceMade = false;
    static bool useBruceRecoveryMenu = false;
    if (!bootChoiceMade) {
        bootChoiceMade = true;
        if (digitalRead(BK_BTN) == BTN_ACT) {
            useBruceRecoveryMenu = true;
            Serial.println("[ESPTRIX] BACK held: original Bruce menu recovery");
        }
    }
#endif
    returnToMenu = false;
    options = {};

#if defined(T_EMBED_1101)
    // All categories use BRUCE'S icon renderer and navigation. We supply only
    // the original icon bitmaps, names and action pointers, never a new layout.
    // V7 categories are data: board-specific availability lives in the router.
    static esptrix::UnifiedCategoryItem smartItem(esptrix::kUnifiedCategories[0]);
    static esptrix::UnifiedCategoryItem automotiveItem(esptrix::kUnifiedCategories[1]);
    static esptrix::UnifiedCategoryItem accessItem(esptrix::kUnifiedCategories[2]);
    static esptrix::UnifiedCategoryItem rfItem(esptrix::kUnifiedCategories[3]);
    static esptrix::UnifiedCategoryItem wifiItem(esptrix::kUnifiedCategories[4]);
    static esptrix::UnifiedCategoryItem bluetoothItem(esptrix::kUnifiedCategories[5]);
    static esptrix::UnifiedCategoryItem irUsbItem(esptrix::kUnifiedCategories[6]);
    static esptrix::UnifiedCategoryItem filesItem(esptrix::kUnifiedCategories[7]);
    static esptrix::UnifiedCategoryItem systemItem(esptrix::kUnifiedCategories[8]);
    const std::vector<MenuItemInterface *> esptrixItems = {
        &smartItem, &automotiveItem, &accessItem, &rfItem, &wifiItem,
        &bluetoothItem, &irUsbItem, &filesItem, &systemItem
    };
    // Holding BACK during boot still selects the original Bruce menu.
    const std::vector<MenuItemInterface *> &activeItems =
        useBruceRecoveryMenu ? _menuItems : esptrixItems;
#else
    const std::vector<MenuItemInterface *> &activeItems = _menuItems;
#endif

    const std::vector<String> disabled = bruceConfig.disabledMenus;
    for (MenuItemInterface *item : activeItems) {
        String itemName = item->getName();
        if (find(disabled.begin(), disabled.end(), itemName) != disabled.end()) continue;
        options.push_back({
            itemName,
            [item]() { item->optionsMenu(); },
            false,
            [](void *pointer, bool shouldRender) {
                if (!shouldRender) return false;
                drawMainBorder(false);
                MenuItemInterface *obj = static_cast<MenuItemInterface *>(pointer);
                float scale = float((float)tftWidth / (float)240);
                if (bruceConfigPins.rotation & 0b01)
                    scale = float((float)tftHeight / (float)135);
                obj->draw(scale);  // Bruce's native layout, title, arrows and status bar
#if defined(HAS_TOUCH)
                TouchFooter();
#endif
                return true;
            },
            item
        });
    }
    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "ESPTRIX", _currentIndex);
}

/*********************************************************************
**  Function: hideAppsMenu
**  Menu to Hide or show menus
**********************************************************************/

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
    int index = 0;
RESTART: // using gotos to avoid stackoverflow after many choices
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back(
            {label,
             [this, label, enabled]() {
                 if (enabled) bruceConfig.addDisabledMenu(label);
                 else bruceConfig.removeDisabledMenu(label);
             },
             enabled}
        );
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    index = loopOptions(options, index);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
