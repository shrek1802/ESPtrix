// ESPTRIX Test Lab for LILYGO T-Embed CC1101 (ESP32-S3, 320x170).
// Passive RF diagnostics and device diagnostics; never transmits RF.
// Based on the independently modified Bruce Firmware 1.16.1 codebase.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "esptrix_test_lab.h"
#include "esptrix/esptrix_ui.h"
#include "esptrix/esptrix_access_lab.h"
#include "core/main_menu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/rf/rf_utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SD.h>
#include <globals.h>
#include <vector>
#include <stdint.h>
#include <cstring>

#if defined(T_EMBED_1101)
namespace esptrix {
namespace {
constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t CYAN = 0x15DF;
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t GREY = 0xAD55;
constexpr uint16_t LINE = 0x31A6;
constexpr uint16_t BLUE = 0x04DD;
constexpr float BANDS[] = {315.00f, 433.92f, 868.35f};
constexpr unsigned BAND_COUNT = 3;
void rfDiagnosticMenu();
void hardwareMenu();


// T-Embed's firmware build cannot be assumed to include optional TFT font 2.
// Use the always-available built-in font 1 and its integer scaling instead.
// This fixes blank readings while keeping the clean black / cyan layout.
void largeText(const String &value, int x, int y, uint16_t color = WHITE) {
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(color, BLACK);
    tft.drawString(value, x, y, 1);
    tft.setTextSize(1);
}

void largeTextRight(const String &value, int x, int y, uint16_t color = WHITE) {
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(color, BLACK);
    tft.drawRightString(value, x, y, 1);
    tft.setTextSize(1);
}

void prepareScreen(const char *title, const char *right = "") {
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextWrap(false);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE, BLACK);
    // A 2x built-in font is 12px per character; long titles remain at 1x.
    const size_t titleLen = strlen(title);
    const size_t rightLen = strlen(right);
    if (titleLen * 12 + rightLen * 6 + 28 < static_cast<size_t>(tftWidth)) {
        largeText(String(title), 10, 6);
    } else {
        tft.drawString(title, 10, 9, 1);
    }
    tft.setTextColor(CYAN, BLACK);
    if (*right) tft.drawRightString(right, tftWidth - 9, 10, 1);
    tft.drawFastHLine(9, 26, tftWidth - 18, LINE);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("BACK: EXIT", 10, tftHeight - 14, 1);
}

void labelValue(const char *label, const String &value, int y, uint16_t color = WHITE) {
    tft.setTextFont(1);
    tft.setTextSize(1);
    // Clear only the affected row, never the whole screen during a live update.
    tft.fillRect(10, y - 3, tftWidth - 20, 21, BLACK);
    tft.setTextColor(GREY, BLACK);
    tft.drawString(label, 12, y + 2, 1);
    largeTextRight(value, tftWidth - 12, y - 2, color);
}

void clearButtonEvents() {
    check(SelPress);
    check(EscPress);
    check(NextPress);
    check(PrevPress);
#ifdef HAS_ENCODER
    drainRotarySteps();
#endif
}

// Bruce's native loopOptions() is deliberately used for every submenu.
// It exits after executing one choice. Reopen it until the user selects BACK.
void stayInMenu(const char *name, std::vector<Option> &rows) {
    if (rows.empty()) return;
    while (true) {
        const int selected = loopOptions(rows, MENU_TYPE_SUBMENU, name);
        if (selected < 0 || selected == static_cast<int>(rows.size()) - 1) break;
    }
}

struct BandResult {
    bool valid = false;
    int minDbm = 0;
    int maxDbm = 0;
    int avgDbm = 0;
};

// Samples raw RSSI only. A quick band survey can miss short transmissions;
// it is NOT packet decoding, frequency identification or a spectrum analyser.
BandResult measureBand(float mhz, unsigned samples) {
    BandResult result;
    if (!initRfModule("rx", mhz)) return result;
    int lo = 0, hi = -125;
    int64_t sum = 0;
    unsigned read = 0;
    for (unsigned n = 0; n < samples; ++n) {
        if (check(EscPress)) break;
        const int rssi = ELECHOUSE_cc1101.getRssi();
        if (read == 0 || rssi < lo) lo = rssi;
        if (read == 0 || rssi > hi) hi = rssi;
        sum += rssi;
        ++read;
        delay(22);
    }
    deinitRfModule();
    if (read) {
        result.valid = true;
        result.minDbm = lo;
        result.maxDbm = hi;
        result.avgDbm = static_cast<int>(sum / read);
    }
    return result;
}

void rfBandSurvey() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("CC1101 is not selected", true);
        return;
    }
    clearButtonEvents();
    prepareScreen("RF BAND SURVEY", "RX ONLY");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Quick RSSI measurement", 12, 33, 1);
    BandResult results[BAND_COUNT];
    for (unsigned i = 0; i < BAND_COUNT; ++i) {
        tft.fillRect(10, 52, tftWidth - 20, 18, BLACK);
        tft.setTextColor(CYAN, BLACK);
        largeText("MEASURING " + String(BANDS[i], 2) + " MHz", 12, 52, CYAN);
        results[i] = measureBand(BANDS[i], 24);
        // A short measurement is valid only if it completed with samples.
        if (!results[i].valid) break;
    }
    prepareScreen("RF BAND SURVEY", "RX ONLY");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("FREQUENCY", 12, 33, 1);
    tft.drawRightString("AVG / PEAK RSSI", tftWidth - 10, 33, 1);
    for (unsigned i = 0; i < BAND_COUNT; ++i) {
        const int y = 56 + static_cast<int>(i) * 29;
        tft.setTextColor(CYAN, BLACK);
        largeText(String(BANDS[i], 2), 12, y, CYAN);
        tft.setTextColor(WHITE, BLACK);
        const String reading = results[i].valid
            ? String(results[i].avgDbm) + " / " + String(results[i].maxDbm) + " dBm"
            : String("--");
        tft.drawRightString(reading, tftWidth - 10, y + 3, 1);
        tft.drawFastHLine(11, y + 24, tftWidth - 22, LINE);
    }
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Sample only; short bursts may be missed", 10, 147, 1);
    clearButtonEvents();
    while (!check(EscPress) && !check(SelPress)) delay(25);
}

void noiseFloor(float mhz) {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("CC1101 is not selected", true);
        return;
    }
    clearButtonEvents();
    prepareScreen("RF NOISE BASELINE", "RX ONLY");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("MEASURING FREQUENCY", 11, 38, 1);
    tft.setTextSize(3);
    tft.setTextFont(1);
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(String(mhz, 2), 10, 48, 1);
    tft.setTextSize(1);
    tft.setTextColor(CYAN, BLACK);
    largeText("MHz", 154, 65, CYAN);
    BandResult result = measureBand(mhz, 95);  // ~2 seconds, passive
    prepareScreen("RF NOISE BASELINE", "RX ONLY");
    labelValue("FREQUENCY", String(mhz, 2) + " MHz", 39, CYAN);
    labelValue("AVERAGE", result.valid ? String(result.avgDbm) + " dBm" : "--", 69);
    labelValue("LOWEST", result.valid ? String(result.minDbm) + " dBm" : "--", 94);
    labelValue("PEAK", result.valid ? String(result.maxDbm) + " dBm" : "--", 119);
    clearButtonEvents();
    while (!check(EscPress) && !check(SelPress)) delay(25);
}
void noise315() { noiseFloor(315.00f); }
void noise433() { noiseFloor(433.92f); }
void noise868() { noiseFloor(868.35f); }

void deviceHealth() {
    clearButtonEvents();
    prepareScreen("DEVICE HEALTH", "LIVE");
    // The SD flag alone is not proof the card is missing. Ask Bruce to mount it.
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Checking storage...", 12, 132, 1);
    const bool sdReady = setupSdCard();
    String prev[6];
    uint32_t last = 0;
    while (true) {
        if (check(EscPress) || check(SelPress)) break;
        if (last == 0 || static_cast<uint32_t>(millis() - last) >= 1000) {
            const String values[] = {
                String(ESP.getFreeHeap() / 1024) + " KB",
                String(ESP.getFreePsram() / 1024) + " KB",
                String(ESP.getPsramSize() / 1024) + " KB",
                String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB",
                String(getBattery()) + "%",
                sdReady ? "SD: READY" : "SD: NOT FOUND"
            };
            const char *labels[] = {"Free heap", "Free PSRAM", "PSRAM total", "Flash total"};
            const int ys[] = {35, 59, 83, 107};
            for (int i = 0; i < 4; ++i) {
                if (last == 0 || values[i] != prev[i]) labelValue(labels[i], values[i], ys[i]);
            }
            if (last == 0 || values[4] != prev[4] || values[5] != prev[5]) {
                tft.fillRect(10, 131, tftWidth - 20, 18, BLACK);
                tft.setTextFont(1);
                tft.setTextSize(1);
                tft.setTextColor(GREY, BLACK);
                tft.drawString("Battery", 12, 135, 1);
                tft.setTextColor(CYAN, BLACK);
                tft.drawString(values[4], 66, 135, 1);
                tft.setTextColor(sdReady ? CYAN : GREY, BLACK);
                tft.drawRightString(values[5], tftWidth - 12, 135, 1);
            }
            for (int i = 0; i < 6; ++i) prev[i] = values[i];
            last = millis();
        }
        delay(30);
    }
}

void encoderTest() {
    clearButtonEvents();
    prepareScreen("INPUT CHECK", "LIVE");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Rotate wheel / press wheel", 12, 37, 1);
    tft.drawString("BACK exits this screen", 12, 135, 1);
    int32_t rotary = 0;
    uint32_t presses = 0;
    labelValue("Wheel steps", String(rotary), 66, CYAN);
    labelValue("Wheel presses", String(presses), 100, WHITE);
    while (true) {
        if (check(EscPress)) break;
        int32_t step = 0;
#ifdef HAS_ENCODER
        step = drainRotarySteps();
        // Also consume Bruce's event flags. If the independent step count is
        // unavailable, fall back to its normal menu-navigation events.
        const bool prev = check(PrevPress);
        const bool next = check(NextPress);
        if (step == 0) step = (prev ? 1 : 0) - (next ? 1 : 0);
#else
        if (check(PrevPress)) ++step;
        if (check(NextPress)) --step;
#endif
        const bool pressed = check(SelPress);
        if (step != 0) {
            rotary += step;
            labelValue("Wheel steps", String(rotary), 66, CYAN);
        }
        if (pressed) {
            ++presses;
            labelValue("Wheel presses", String(presses), 100, WHITE);
        }
        // No repeated fillScreen(): redraw only changed values.
        delay(15);
    }
}

void sdCardCheck() {
    clearButtonEvents();
    prepareScreen("SD CARD CHECK", "READ ONLY");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Checking Bruce SD interface...", 12, 32, 1);
    const bool mounted = setupSdCard();
    labelValue("Chip select", String(bruceConfigPins.SDCARD_bus.cs), 55, GREY);
    labelValue("Mount status", mounted ? String("READY") : String("FAILED"), 83,
               mounted ? CYAN : 0xFBC0);
    if (mounted) {
        const uint64_t size = SD.cardSize();
        labelValue("Card size", String(static_cast<unsigned long>(size / 1048576ULL)) + " MB", 111);
        // Don't write to the card: test that the root directory is readable.
        File root = SD.open("/");
        const bool canRead = root && root.isDirectory();
        if (root) root.close();
        tft.setTextColor(canRead ? CYAN : GREY, BLACK);
        tft.drawString(canRead ? "Root directory readable" : "Cannot read card root", 12, 143, 1);
    } else {
        tft.setTextColor(GREY, BLACK);
        tft.drawString("Check card / CS pin / SPI bus", 12, 143, 1);
    }
    while (!check(EscPress) && !check(SelPress)) delay(25);
    clearButtonEvents();
}

// A scan is a snapshot. Copy AP data before scanDelete(), then show details
// from the snapshot rather than relying on invalidated Arduino WiFi indexes.
struct WifiRecord {
    String ssid;
    String bssid;
    int rssi;
    int channel;
    int auth;
};
static std::vector<WifiRecord> lastScan;
static bool hasScan = false;

const char *authName(int a) {
    // Arduino-ESP32 3.x wifi_auth_mode_t values; numeric mapping avoids
    // depending on optional WPA3 enum names in alternate core releases.
    switch (a) {
        case 0: return "OPEN";
        case 1: return "WEP";
        case 2: return "WPA";
        case 3: return "WPA2-Personal";
        case 4: return "WPA/WPA2";
        case 5: return "WPA2-Enterprise";
        case 6: return "WPA3-Personal";
        case 7: return "WPA2/WPA3";
        case 8: return "WAPI";
        case 9: return "OWE";
        case 10: return "WPA3-192bit";
        case 13: return "WPA3-Enterprise";
        default: return "Other / Unknown";
    }
}

bool weakAuth(int a) { return a == 0 || a == 1 || a == 2 || a == 4; }
bool openAuth(int a) { return a == 0; }

// Do not attempt to associate with scanned networks. When radio was off,
// return it to the original off state after gathering the snapshot.
bool collectWifiScan() {
    clearButtonEvents();
    prepareScreen("WI-FI SITE SURVEY", "2.4 GHz");
    tft.setTextColor(CYAN, BLACK);
    largeText("SCANNING NETWORKS...", 12, 63, CYAN);
    const bool radioWasOff = (WiFi.getMode() == WIFI_OFF);
    if (radioWasOff) WiFi.mode(WIFI_STA);
    const int found = WiFi.scanNetworks(false, true);
    if (found < 0) {
        WiFi.scanDelete();
        if (radioWasOff) WiFi.mode(WIFI_OFF);
        displayError("Wi-Fi scan failed", true);
        return false;
    }
    std::vector<WifiRecord> newScan;
    const int limit = found < 40 ? found : 40;
    newScan.reserve(limit);
    for (int i = 0; i < limit; ++i) {
        WifiRecord r;
        r.ssid = WiFi.SSID(i);
        r.bssid = WiFi.BSSIDstr(i);
        r.rssi = WiFi.RSSI(i);
        r.channel = WiFi.channel(i);
        r.auth = static_cast<int>(WiFi.encryptionType(i));
        newScan.push_back(r);
    }
    WiFi.scanDelete();
    if (radioWasOff) WiFi.mode(WIFI_OFF);
    lastScan.swap(newScan);
    hasScan = true;
    return true;
}

// A persistent full-information page, not a brief generic status popup.
void wifiDetails(const WifiRecord &r) {
    clearButtonEvents();
    prepareScreen("NETWORK DETAILS", "2.4 GHz");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("SSID", 12, 34, 1);
    tft.setTextColor(WHITE, BLACK);
    const String ssid = r.ssid.length() ? r.ssid : String("(Hidden SSID)");
    // Long SSIDs are shown in the built-in 6-px font to avoid truncation.
    if (ssid.length() <= 24) largeText(ssid, 12, 46, WHITE);
    else tft.drawString(ssid, 12, 49, 1);
    tft.drawFastHLine(10, 69, tftWidth - 20, LINE);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("BSSID", 12, 76, 1);
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(r.bssid, 91, 75, 1);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("CHANNEL", 12, 94, 1);
    tft.setTextColor(WHITE, BLACK);
    largeText(String(r.channel), 91, 92, WHITE);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("SIGNAL", 12, 113, 1);
    tft.setTextColor(CYAN, BLACK);
    largeText(String(r.rssi) + " dBm", 91, 111, CYAN);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("SECURITY", 12, 136, 1);
    tft.setTextColor(weakAuth(r.auth) ? 0xFBC0 : CYAN, BLACK);
    tft.drawString(authName(r.auth), 91, 132, 1);
    while (!check(EscPress) && !check(SelPress)) delay(25);
    clearButtonEvents();
}

void wifiResults() {
    if (!hasScan && !collectWifiScan()) return;
    // Rows retain the full RSSI and channel in the details page; the carousel
    // renderer only needs a readable SSID and concise security indicator.
    for (;;) {
        std::vector<Option> rows;
        const size_t show = lastScan.size();  // Display all entries retained in the 40-AP snapshot.
        if (show == 0) {
            rows.push_back({"No networks detected", []() { displayInfo("No APs detected", true); }});
        }
        for (size_t i = 0; i < show; ++i) {
            const WifiRecord record = lastScan[i];
            String label = record.ssid.length() ? record.ssid : String("(hidden)");
            if (label.length() > 18) label = label.substring(0, 18);
            label += "  CH" + String(record.channel);
            rows.push_back({label, [record]() { wifiDetails(record); }});
        }
        const int rescanIdx = static_cast<int>(rows.size());
        rows.push_back({"Rescan networks", []() {}});
        rows.push_back({"Back", []() {}});
        const int selected = loopOptions(rows, MENU_TYPE_SUBMENU, "WI-FI SITE SURVEY");
        if (selected < 0 || selected == static_cast<int>(rows.size()) - 1) return;
        if (selected == rescanIdx) {
            if (!collectWifiScan()) return;
        }
        // After an AP details screen, loopOptions returns and reopens the
        // same survey. Never recursively re-enter wifiResults().
    }
}

void wifiSurvey() {
    if (!collectWifiScan()) return;
    wifiResults();
}

void wifiAuditSummary() {
    // A security audit must use a fresh scan, not a previous visit to the survey.
    if (!collectWifiScan()) return;
    int open = 0, old = 0, modern = 0, mixed = 0;
    for (const auto &r : lastScan) {
        if (openAuth(r.auth)) ++open;
        else if (weakAuth(r.auth)) ++old;
        else ++modern;
    }
    for (size_t i = 0; i < lastScan.size(); ++i) {
        if (lastScan[i].ssid.length() == 0) continue;
        for (size_t j = i + 1; j < lastScan.size(); ++j) {
            if (lastScan[i].ssid == lastScan[j].ssid &&
                openAuth(lastScan[i].auth) != openAuth(lastScan[j].auth)) {
                ++mixed;
                break;
            }
        }
    }
    clearButtonEvents();
    prepareScreen("WI-FI SECURITY AUDIT", "PASSIVE");
    labelValue("Access points", String(static_cast<int>(lastScan.size())), 35);
    labelValue("Unencrypted", String(open), 59, open ? 0xFBC0 : CYAN);
    labelValue("Legacy settings", String(old), 83, old ? 0xFBC0 : CYAN);
    labelValue("Modern/other", String(modern), 107, CYAN);
    labelValue("SSID security conflicts", String(mixed), 130, mixed ? 0xFBC0 : CYAN);
    while (!check(EscPress) && !check(SelPress)) delay(25);
    clearButtonEvents();
    // Different BSSIDs on the same SSID are NORMAL for mesh networks.
    // An open/secured mix deserves review, but is NOT proof of an Evil Twin.
}

String csvQuote(const String &s) {
    String safe = s;
    // SSIDs are untrusted: prevent spreadsheet programs treating them as formulas.
    if (safe.length() && (safe[0] == '=' || safe[0] == '+' || safe[0] == '-' || safe[0] == '@')) {
        safe = String("'") + safe;
    }
    String result = "\"";
    for (size_t i = 0; i < safe.length(); ++i) {
        if (safe[i] == '"') result += "\"\"";
        else if (safe[i] == '\n' || safe[i] == '\r') result += ' ';
        else result += safe[i];
    }
    return result + "\"";
}

void wifiSaveReport() {
    // A report is a measurement, so acquire a new snapshot for each export.
    if (!collectWifiScan()) return;
    if (!setupSdCard()) { displayError("SD card unavailable", true); return; }
    File out = SD.open("/esptrix_wifi_audit.csv", FILE_APPEND);
    if (!out) { displayError("Cannot write SD report", true); return; }
    // Appending to an existing CSV must not insert an extra header row.
    if (out.size() == 0) out.println("SSID,BSSID,Channel,RSSI_dBm,Authentication");
    for (const auto &r : lastScan) {
        out.println(csvQuote(r.ssid) + "," + csvQuote(r.bssid) + "," + String(r.channel) + "," + String(r.rssi) + "," + csvQuote(authName(r.auth)));
    }
    out.close();
    displayInfo("Saved to /esptrix_wifi_audit.csv", true);
}

void wifiDuplicateNameCheck() {
    if (!collectWifiScan()) return;
    clearButtonEvents();
    std::vector<Option> rows;
    for (size_t i = 0; i < lastScan.size(); ++i) {
        if (lastScan[i].ssid.length() == 0) continue;
        bool difference = false;
        for (size_t j = 0; j < lastScan.size(); ++j) {
            if (i != j && lastScan[i].ssid == lastScan[j].ssid &&
                openAuth(lastScan[i].auth) != openAuth(lastScan[j].auth)) {
                difference = true; break;
            }
        }
        if (difference) {
            const WifiRecord rec = lastScan[i];
            String label = rec.ssid;
            if (label.length() > 20) label = label.substring(0, 20);
            label += openAuth(rec.auth) ? " OPEN" : " SECURED";
            rows.push_back({label, [rec]() { wifiDetails(rec); }});
        }
    }
    if (rows.empty()) {
        prepareScreen("SSID CONFIGURATION", "SCAN");
        tft.setTextColor(CYAN, BLACK);
        tft.drawString("No open/secured SSID conflicts", 10, 54, 1);
        tft.setTextColor(GREY, BLACK);
        tft.drawString("Matching SSIDs alone are normal", 10, 76, 1);
        while (!check(EscPress) && !check(SelPress)) delay(25);
        return;
    }
    rows.push_back({"Back", []() {}});
    stayInMenu("REVIEW SSID SETTINGS", rows);
    // SSID differences are NOT conclusive evidence of an Evil Twin.
}

void wifiChannelAudit() {
    if (!collectWifiScan()) return;
    int scores[3] = {0, 0, 0};
    constexpr int channels[3] = {1, 6, 11};
    for (const auto &ap : lastScan) {
        for (int i = 0; i < 3; ++i) {
            const int d = ap.channel > channels[i] ? ap.channel - channels[i] : channels[i] - ap.channel;
            if (d <= 2) scores[i] += (ap.rssi > -70 ? 2 : 1);
        }
    }
    int best = 0;
    for (int i = 1; i < 3; ++i) if (scores[i] < scores[best]) best = i;
    clearButtonEvents();
    prepareScreen("CHANNEL CONGESTION", "ESTIMATE");
    labelValue("Channel 1 score", String(scores[0]), 36);
    labelValue("Channel 6 score", String(scores[1]), 65);
    labelValue("Channel 11 score", String(scores[2]), 94);
    labelValue("Lowest detected", "CH" + String(channels[best]), 121, CYAN);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Only detected APs; not live traffic", 10, 147, 1);
    while (!check(EscPress) && !check(SelPress)) delay(25);
}

void wifiSecurityMenu() {
    std::vector<Option> rows = {
        {"Wi-Fi Site Survey", wifiSurvey},
        {"Wi-Fi Encryption Audit", wifiAuditSummary},
        {"SSID Conflict Review", wifiDuplicateNameCheck},
        {"Channel Congestion", wifiChannelAudit},
        {"Review Scanned Networks", wifiResults},
        {"Save Survey to SD", wifiSaveReport},
        {"Original Wi-Fi Tools", []() { mainMenu.wifiMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("WI-FI SECURITY", rows);
}

// Explicit user-initiated gateway-only TCP service inspection. No subnet
// scanning, brute force, credentials, or exploitation. Open != vulnerable.
void gatewayAudit() {
    if (WiFi.status() != WL_CONNECTED) {
        displayInfo("Connect to your own Wi-Fi first", true);
        return;
    }
    clearButtonEvents();
    prepareScreen("ROUTER SERVICE CHECK", "AUTHORISE");
    tft.setTextColor(WHITE, BLACK);
    tft.drawString("Only test a router you own or", 12, 40, 1);
    tft.drawString("have permission to assess.", 12, 55, 1);
    tft.setTextColor(CYAN, BLACK);
    tft.drawString("PRESS: CHECK    BACK: CANCEL", 12, 100, 1);
    bool approved = false;
    uint32_t inputGuard = millis() + 450;
    while (true) {
        if (check(EscPress)) return;
        if (check(SelPress) && static_cast<int32_t>(millis() - inputGuard) >= 0) {
            approved = true; break;
        }
        delay(20);
    }
    if (!approved) return;
    const IPAddress gateway = WiFi.gatewayIP();
    if (gateway == IPAddress(0, 0, 0, 0)) {
        displayError("No router gateway found", true);
        return;
    }
    constexpr uint16_t ports[] = {21, 22, 23, 53, 80, 443, 8080, 8443};
    int open = 0;
    String found;
    prepareScreen("ROUTER SERVICE CHECK", "TCP");
    tft.setTextColor(GREY, BLACK);
    tft.drawString("GATEWAY", 10, 36, 1);
    tft.setTextColor(WHITE, BLACK);
    largeText(gateway.toString(), 10, 49, WHITE);
    for (unsigned i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i) {
        if (check(EscPress)) break;
        tft.fillRect(10, 83, tftWidth - 20, 16, BLACK);
        tft.setTextColor(CYAN, BLACK);
        largeText("Checking TCP " + String(ports[i]), 10, 83, CYAN);
        WiFiClient client;
        if (client.connect(gateway, ports[i], 170)) {
            ++open;
            found += String(ports[i]) + " ";
        }
        client.stop();
        delay(10);
    }
    prepareScreen("ROUTER SERVICE CHECK", "DONE");
    labelValue("Gateway", gateway.toString(), 35);
    labelValue("TCP ports open", String(open), 65, CYAN);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("RESPONDING PORTS:", 10, 98, 1);
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(found.length() ? found : String("None of 8 tested"), 10, 113, 1);
    tft.setTextColor(GREY, BLACK);
    tft.drawString("Open ports do not prove a flaw", 10, 137, 1);
    clearButtonEvents();
    while (!check(EscPress) && !check(SelPress)) delay(25);
}

void bleSecurityMenu() {
    std::vector<Option> rows = {
        {"BLE Device Scan (Bruce)", []() { mainMenu.bleMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("BLUETOOTH SECURITY", rows);
}

void automotiveMenu() {
    std::vector<Option> rows = {
        {"Key Fob RF Monitor", keyFobMenu},
        {"TPMS RF Monitor", tpmsMenu},
        {"RF Band Survey", rfBandSurvey},
        {"RF Noise Baseline", noise433},
        {"Back", []() {}}
    };
    stayInMenu("AUTOMOTIVE TESTS", rows);
}

void securityLabMenu() {
    std::vector<Option> rows = {
        {"Wi-Fi Security", wifiSecurityMenu},
        {"Router Port Check", gatewayAudit},
        {"Bluetooth Security", bleSecurityMenu},
        {"Sub-GHz RF Security", rfDiagnosticMenu},
        {"Automotive RF Tests", automotiveMenu},
        {"Access Control Lab", accessControlLabMenu},
        {"Infrared Tools", []() { mainMenu.irMenu.optionsMenu(); }},
        {"Device Diagnostics", hardwareMenu},
        {"Back", []() {}}
    };
    stayInMenu("ESPTRIX SECURITY LAB", rows);
}

void rfDiagnosticMenu() {
    std::vector<Option> rows = {
        {"RF Band Survey", rfBandSurvey},
        {"Noise Floor - 433 MHz", noise433},
        {"Noise Floor - 315 MHz", noise315},
        {"Noise Floor - 868 MHz", noise868},
        {"Key Fob Detector", keyFobMenu},
        {"TPMS Activity", tpmsMenu},
        {"Original RF Tools", []() { mainMenu.rfMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("RF DIAGNOSTICS", rows);
}

void wirelessMenu() {
    std::vector<Option> rows = {
        {"Wi-Fi Site Survey", wifiSurvey},
        {"Wi-Fi Security Audit", wifiAuditSummary},
        {"Original Wi-Fi Tools", []() { mainMenu.wifiMenu.optionsMenu(); }},
        {"Bluetooth Tools", []() { mainMenu.bleMenu.optionsMenu(); }},
        {"NFC / RFID Tools", []() { mainMenu.rfidMenu.optionsMenu(); }},
        {"Infrared Tools", []() { mainMenu.irMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("WIRELESS TESTS", rows);
}

void hardwareMenu() {
    std::vector<Option> rows = {
        {"Live Device Health", deviceHealth},
        {"Wheel / Button Check", encoderTest},
        {"Original Utilities", []() { mainMenu.othersMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("DEVICE TESTS", rows);
}
} // namespace

// ESPTRIX V7 public wrappers for existing verified V5 diagnostics
// Keep the V5 implementation in place and avoid copy/paste duplication.
void v7WifiSurvey()         { wifiSurvey(); }
void v7WifiAudit()          { wifiAuditSummary(); }
void v7WifiDuplicateSsid()  { wifiDuplicateNameCheck(); }
void v7WifiChannelSurvey()  { wifiChannelAudit(); }
void v7WifiSaveReport()      { wifiSaveReport(); }
void v7RouterServiceCheck() { gatewayAudit(); }
void v7RfSurvey()           { rfBandSurvey(); }
void v7RfNoise433()         { noise433(); }
void v7DeviceHealth()       { deviceHealth(); }
void v7EncoderTest()        { encoderTest(); }
void v7SdCardCheck()       { sdCardCheck(); }

void testLabMenu() {
    std::vector<Option> rows = {
        {"Security Lab", securityLabMenu},
        {"RF Diagnostics", rfDiagnosticMenu},
        {"Wireless Tests", wirelessMenu},
        {"Device Tests", hardwareMenu},
        {"Original Utilities", []() { mainMenu.othersMenu.optionsMenu(); }},
        {"Back", []() {}}
    };
    stayInMenu("ESPTRIX TEST LAB", rows);
}
} // namespace esptrix
#else
namespace esptrix { void testLabMenu() {} }
#endif
