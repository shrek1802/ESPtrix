// ESPTRIX V7.2 TOOL REPAIR: honest labels, Wi-Fi reports, corrected BLE inspector; Bruce recovery in System
// ESPTRIX V7 - one navigation tree around EXISTING tools.
// SPDX-License-Identifier: AGPL-3.0-or-later
// The first V7 release is a unified menu, NOT new RF decoding, cloning, or a
// completed guided Smart Test wizard. Every listed action invokes real code.
#include "esptrix_unified_menu.h"
#if defined(T_EMBED_1101)
#include "esptrix/esptrix_ui.h"
#include "esptrix/esptrix_access_lab.h"
#include "esptrix/esptrix_test_lab.h"
#include "esptrix/esptrix_v8_tools.h" // ESPTRIX V8 DIAGNOSTIC SUITE
#include "esptrix/esptrix_bruce_icons.h"
#include "core/main_menu.h"
#include "core/display.h"
#include "core/utils.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "core/wifi/webInterface.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/record.h"
#include "modules/rf/rf_spectrum.h"
#include "modules/rf/rf_waterfall.h"
#include "modules/wifi/channel_analyzer.h"
#include "modules/ble/ble_common.h"
#include "modules/ble/ble_sniffer.h"
#ifndef LITE_VERSION
#include "modules/ble/BLE_Suite.h"
#endif
#include "modules/ir/ir_read.h"
#include "modules/rfid/tag_o_matic.h"
#include "modules/ir/custom_ir.h"
#include "modules/badusb_ble/ducky_typer.h"
#include <WiFi.h>
#include <SD.h>
#include <LittleFS.h>
#include <globals.h>
#include <vector>

namespace esptrix {
// Thin adapters expose selected ALREADY-IMPLEMENTED V5 diagnostic actions.
// They are defined beside the existing functions in esptrix_test_lab.cpp.
void v7WifiSurvey();
void v7WifiAudit();
void v7WifiDuplicateSsid();
void v7WifiChannelSurvey();
void v7WifiSaveReport();
void v7RouterServiceCheck();
void v7RfSurvey();
void v7RfNoise433();
void v7DeviceHealth();
void v7EncoderTest();
void v7SdCardCheck();

namespace {
// Bruce's scrollable native list is retained; submenu routing is independent
// of the device's display resolution, encoder or touch hardware.
void runMenu(const char *title, std::vector<Option> choices) {
    choices.push_back({"Back", []() {}});
    // Existing ESPTRIX V5 uses this proven pattern: Bruce exits loopOptions
    // after an action; return to same group until Back is selected.
    while (true) {
        int selected = loopOptions(choices, MENU_TYPE_SUBMENU, title);
        if (selected < 0 || selected == static_cast<int>(choices.size()) - 1) break;
    }
}

bool hasCc1101() {
    return bruceConfigPins.rfModule == CC1101_SPI_MODULE;
}
void noRadio() { displayInfo("Select/attach CC1101 RF hardware", true); }
void runRfScan() { if (hasCc1101()) { RFScan scanner; } else noRadio(); }
void runRfRaw() { if (hasCc1101()) rf_raw_record(); else noRadio(); }
void runRfSpectrum() { if (hasCc1101()) rf_spectrum(); else noRadio(); }
void runRfWaterfall() { if (hasCc1101()) rf_waterfall(); else noRadio(); }
void runRfRssi() { if (hasCc1101()) rf_CC1101_rssi(); else noRadio(); }
void runWifiConnect() { (void)wifiConnectMenu(WIFI_STA); }
void runIrRead() { IrRead reader; }
void runSd() { if (setupSdCard()) loopSD(SD); else displayInfo("SD card not mounted", true); }
void runLittleFs() { loopSD(LittleFS); }
void runUsbKeyboard() { ducky_keyboard(hid_usb, false); }
void runUsbHidScripts() { ducky_setup(hid_usb, false); }

void showCapabilities() {
    // Probe SD when the user opens capabilities, not just its stale boot flag.
    const bool sdReady = setupSdCard();
    std::vector<Option> rows = {
        {"Board: T-Embed CC1101", []() { displayInfo("T-Embed ESP32-S3", true); }},
        {String("Sub-GHz CC1101: ") + (hasCc1101() ? "SELECTED" : "NOT SELECTED"), []() { displayInfo("Check RF module configuration", true); }},
        {"Wi-Fi: 2.4 GHz", []() { displayInfo("ESP32-S3 2.4 GHz Wi-Fi", true); }},
        {"BLE: Supported", []() { displayInfo("Bluetooth Low Energy available", true); }},
        {String("SD card: ") + (sdReady ? "READY" : "NOT DETECTED"), []() { displayInfo("SD availability depends on media", true); }},
        {"HF/LF: Chameleon Ultra BLE", []() { displayInfo("Requires powered and connected Ultra", true); }},
    };
    runMenu("BOARD CAPABILITIES", rows);
}

// Full legacy menu is a recovery/advanced route; it is no longer a duplicate
// second row of home icons. We do not remove upstream functions or files.
void legacyTools() {
    std::vector<Option> rows = {
        {"Wi-Fi (Bruce)", []() { mainMenu.wifiMenu.optionsMenu(); }},
        {"Bluetooth (Bruce)", []() { mainMenu.bleMenu.optionsMenu(); }},
        {"Sub-GHz (Bruce)", []() { mainMenu.rfMenu.optionsMenu(); }},
        {"NFC / RFID (Bruce)", []() { mainMenu.rfidMenu.optionsMenu(); }},
        {"Infrared (Bruce)", []() { mainMenu.irMenu.optionsMenu(); }},
        {"Files (Bruce)", []() { mainMenu.fileMenu.optionsMenu(); }},
        {"Utilities (Bruce)", []() { mainMenu.othersMenu.optionsMenu(); }},
        {"GPS (Bruce)", []() { mainMenu.gpsMenu.optionsMenu(); }},
        {"NRF24 (Bruce)", []() { mainMenu.nrf24Menu.optionsMenu(); }},
#ifndef LITE_VERSION
        {"LoRa (Bruce)", []() { mainMenu.loraMenu.optionsMenu(); }},
        {"Ethernet (Bruce)", []() { mainMenu.ethernetMenu.optionsMenu(); }},
#endif
        {"Clock (Bruce)", []() { mainMenu.clockMenu.optionsMenu(); }},
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        {"Scripts (Bruce)", []() { mainMenu.scriptsMenu.optionsMenu(); }},
#endif
    };
    runMenu("ADVANCED / ORIGINAL", rows);
}

// ESPTRIX V7.1 MENU CLEANUP - one primary home for every app.
// Smart Test is unique diagnostics/guidance, not duplicate launch buttons.
void testGuide() {
    runMenu("FIND THE RIGHT TEST", {
        {"Vehicle keys / tyres", []() { displayInfo("Automotive: Key Fob or TPMS", true); }},
        {"RF remote / sensor", []() { displayInfo("Sub-GHz: Capture or Spectrum", true); }},
        {"NFC / access tags", []() { displayInfo("Access Control: Chameleon or NFC", true); }},
        {"Wi-Fi networks", []() { displayInfo("Wi-Fi: Site Survey or Audit", true); }},
        {"Bluetooth devices", []() { displayInfo("Bluetooth: Scan or Inspect", true); }},
        {"IR remote / USB", []() { displayInfo("Infrared & USB: IR receive or manual USB", true); }},
    });
}
void smartTest() {
    runMenu("SMART TEST", {
        {"Device Preflight / Health", v7DeviceHealth},
        {"Input & Wheel Check", v7EncoderTest},
        {"SD Card Check", v7SdCardCheck},
        {"Acoustic Frequency Analyser", v8AcousticSpectrum},
        {"Hardware Capabilities", showCapabilities},
        {"Test Selection Guide", testGuide},
    });
}
void automotive() {
    runMenu("AUTOMOTIVE LAB", {
        {"Key Fob Tester", keyFobMenu},
        {"TPMS Signal Monitor", tpmsMenu},
        {"Advanced Key Fob Analyser", v8KeyFobAnalyzer},
        {"TPMS Protocol Inspector", v8TpmsInspector},
    });
}
void accessControl() {
    runMenu("ACCESS CONTROL LAB", {
        {"Chameleon Ultra Lab", accessControlLabMenu},
        {"Onboard NFC Reader", []() { TagOMatic(); }},
        {"NFC Card Inspector", v8NfcInspector},
    });
}
void subGhz() {
    if (!hasCc1101()) { noRadio(); return; }
    runMenu("SUB-GHZ & RF", {
        {"Signal Scan / Capture", runRfScan},
#ifndef LITE_VERSION
        {"RAW Recording", runRfRaw},
#endif
        {"RF Spectrum", runRfSpectrum},
        {"RSSI Spectrum", runRfRssi},
        {"RF Waterfall", runRfWaterfall},
        {"RF Band Survey", v7RfSurvey},
        {"RF Pulse Scope", v8PulseScope},
        {"RF Interference Finder", v8InterferenceFinder},
        {"433 MHz Noise Floor", v7RfNoise433},
    });
}
void wifiNetwork() {
    runMenu("WI-FI & NETWORK", {
        {"Wi-Fi Site Survey", v7WifiSurvey},
        {"Security Configuration Audit", v7WifiAudit},
        {"SSID Configuration Review", v7WifiDuplicateSsid},
        {"Channel Congestion", v7WifiChannelSurvey},
        {"Save Survey to SD", v7WifiSaveReport},
        {"Wi-Fi Signal Walk Test", v8WifiWalkSurvey},
        {"Connect to Wi-Fi", runWifiConnect},
#ifndef LITE_VERSION
        {"Channel Analyzer", channel_analyzer_setup},
#endif
        {"Router Service Check", v7RouterServiceCheck},
    });
}
void bluetooth() {
    runMenu("BLUETOOTH", {
        {"BLE Device Scanner", ble_scan},
        {"BLE Device Locator", v8BleLocator},
        {"Original Bruce BLE Tools", []() { mainMenu.bleMenu.optionsMenu(); }},
#if defined(LITE_VERSION)
        {"BLE Adverts (Snapshot)", BLE_SnifferMenu},
#else
        {"BLE Adverts (Snapshot)", BLE_Sniffer},
#endif
    });
}
void infraredUsb() {
    std::vector<Option> rows = {
        {"Read / Record IR", runIrRead},
        {"Custom IR Transmit", otherIRcodes},
        // TV-B-Gone sends power codes; it is not an IR receiver test.
        // Accessible via System > Original Bruce / Legacy > Infrared.
#ifndef LITE_VERSION
        {"USB Keyboard (Manual)", runUsbKeyboard},
        {"USB Script Runner", runUsbHidScripts},
#endif
    };
    runMenu("INFRARED & USB", rows);
}
void filesReports() {
    std::vector<Option> rows = {
        {"SD Card Files / Reports", runSd},
        {"Internal LittleFS Files", runLittleFs},
        {"Web File Manager", loopOptionsWebUi},
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        {"JavaScript Applications", []() { mainMenu.scriptsMenu.optionsMenu(); }},
#endif
    };
    runMenu("FILES & REPORTS", rows);
}
void systemMenu() {
    runMenu("SYSTEM", {
        {"Device Settings", []() { mainMenu.configMenu.optionsMenu(); }},
        {"ESPTRIX Information", showAbout},
        // Compatibility-only route for Bruce apps not yet migrated to V7.
        // Not duplicated within the normal eight functional categories.
        {"Original Bruce / Legacy", legacyTools},
    });
}
} // namespace

void openUnifiedCategory(CategoryId category) {
    switch (category) {
        case CategoryId::SmartTest: smartTest(); break;
        case CategoryId::Automotive: automotive(); break;
        case CategoryId::AccessControl: accessControl(); break;
        case CategoryId::SubGhz: subGhz(); break;
        case CategoryId::WifiNetwork: wifiNetwork(); break;
        case CategoryId::Bluetooth: bluetooth(); break;
        case CategoryId::IrUsb: infraredUsb(); break;
        case CategoryId::FilesReports: filesReports(); break;
        case CategoryId::System: systemMenu(); break;
    }
}
} // namespace esptrix
#else
namespace esptrix { void openUnifiedCategory(CategoryId) {} }
#endif
