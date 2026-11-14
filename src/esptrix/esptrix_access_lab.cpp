// ESPTRIX V6 Access Control Lab - Chameleon Ultra, read-only inspection.
// Built on the Bruce 1.16.1 ESPTRIX derivative. SPDX-License-Identifier: AGPL-3.0-or-later
// Only BLE connection, reader mode, HF/LF tag scans and local reporting are used.
// No authentication guessing, nonce harvesting, key recovery, writing or emulation.

#include "esptrix_access_lab.h"
#include "core/display.h"
#include "core/main_menu.h"
#include <chameleonUltra.h>
#include <SD.h>
#include <globals.h>
#include <cstdio>
#include <vector>

#if defined(T_EMBED_1101)
namespace esptrix {
namespace {
constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t CYAN  = 0x15DF; // T-Embed electric-cyan RGB565
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t MUTED = 0xAD55;
constexpr uint16_t LINE  = 0x31A6;
constexpr char REPORT_PATH[] = "/esptrix_access_lab.csv";

// Upstream ESP-ChameleonUltra's public API is used; firmware/model commands
// exist in its wire protocol but are not public methods of this dependency.
static ChameleonUltra ultra(false);
static bool connected = false;

enum class CardBand : uint8_t { None, HF13, LF125 };
struct CardRecord {
    CardBand band = CardBand::None;
    String uid;
    String kind;
    uint8_t sak = 0;
    uint8_t atqa0 = 0;
    uint8_t atqa1 = 0;
    uint32_t atMillis = 0;
};
static CardRecord last;
static CardRecord reference;

void screen(const char *heading, const char *status = "") {
    tft.setTextSize(1);
    tft.setTextFont(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextWrap(false);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(heading, 9, 5, 2);
    tft.setTextColor(CYAN, BLACK);
    if (*status) tft.drawRightString(status, tftWidth - 9, 10, 1);
    tft.drawFastHLine(9, 26, tftWidth - 18, LINE);
    tft.setTextColor(MUTED, BLACK);
    tft.drawString("PRESS: CONTINUE   BACK: EXIT", 10, tftHeight - 13, 1);
}

void label(const char *name, const String &value, int y, uint16_t colour = WHITE) {
    tft.setTextColor(MUTED, BLACK);
    tft.drawString(name, 11, y, 1);
    tft.setTextColor(colour, BLACK);
    tft.drawRightString(value, tftWidth - 11, y - 1, 1);
    if (y < tftHeight - 28) tft.drawFastHLine(11, y + 15, tftWidth - 22, LINE);
}

void flushButtons() {
    (void)check(SelPress);
    (void)check(EscPress);
}

// Do not consume the selection press that opened a screen as confirmation.
// Every interaction can be cancelled with BACK.
bool awaitPress() {
    flushButtons();
    const uint32_t start = millis();
    while (true) {
        if (check(EscPress)) return false;
        if (check(SelPress) && static_cast<uint32_t>(millis() - start) >= 280) return true;
        delay(20);
    }
}

void awaitExit() {
    flushButtons();
    delay(180);
    while (!check(EscPress) && !check(SelPress)) delay(25);
}

String hexByte(uint8_t b) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned>(b));
    return String(buf);
}

String uidHex(const uint8_t *bytes, uint8_t len) {
    String result;
    if (len > 10) len = 10;
    for (uint8_t i = 0; i < len; ++i) result += hexByte(bytes[i]);
    return result;
}

String tail4(const String &uid) {
    if (uid.length() <= 4) return String("****");
    return String("****") + uid.substring(uid.length() - 4, uid.length());
}

bool classicCandidate(uint8_t sak) {
    // A SAK hint is not cryptographic proof of a Classic card or that a
    // specific access reader uses Crypto1. No keys/blocks are accessed.
    switch (sak) {
        case 0x08: case 0x09: case 0x10: case 0x11:
        case 0x18: case 0x19: return true;
        default: return false;
    }
}

void simpleMessage(const char *title, const char *a, const char *b = "") {
    screen(title);
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(a, 11, 54, 1);
    tft.setTextColor(MUTED, BLACK);
    if (*b) tft.drawString(b, 11, 78, 1);
    awaitExit();
}

// The library discovers BLE peripherals advertising the *exact* name
// "ChameleonUltra". Connection and the battery command are live checks.
bool openConnection() {
    screen("CHAMELEON ULTRA", "BLE");
    label("Status", "Searching...", 52, CYAN);
    if (connected && ultra.cmdBatteryInfo()) return true;
    connected = false;
    if (!ultra.searchChameleonDevice() || !ultra.connectToChamelon()) {
        simpleMessage("CHAMELEON ULTRA", "No connection found", "Power on Ultra; enable BLE");
        return false;
    }
    // Positive response verifies device control channel, not firmware version.
    if (!ultra.cmdBatteryInfo()) {
        simpleMessage("CHAMELEON ULTRA", "BLE link, no reply", "Try again after reboot");
        return false;
    }
    connected = true;
    return true;
}

void deviceInfo() {
    if (!openConnection()) return;
    screen("CHAMELEON ULTRA", "CONNECTED");
    label("BLE device", "ChameleonUltra", 37, CYAN);
    if (ultra.cmdResponse.dataSize >= 3) {
        const uint8_t percent = ultra.cmdResponse.data[2];
        label("Battery", percent <= 100 ? String(static_cast<unsigned>(percent)) + "%" : "Unknown", 60);
    } else {
        label("Battery", "Not reported", 60);
    }
    label("HF 13.56 MHz", "Reader command", 83);
    label("LF 125 kHz", "Reader command", 106);
    label("FW / model", "Not queried", 129, MUTED);
    awaitExit();
}

bool prepareRead(const char *title, const char *instruction) {
    if (!openConnection()) return false;
    screen(title, "READ ONLY");
    tft.setTextColor(WHITE, BLACK);
    tft.drawString(instruction, 10, 49, 2);
    tft.setTextColor(MUTED, BLACK);
    tft.drawString("Hold your test tag at the Ultra", 10, 82, 1);
    tft.drawString("This switches Ultra to reader mode", 10, 102, 1);
    if (!awaitPress()) return false;
    screen(title, "SCANNING");
    return ultra.cmdChangeMode(ChameleonUltra::HW_MODE_READER);
}

bool scanHF() {
    if (!prepareRead("NFC CARD INSPECTOR", "Press wheel to scan")) return false;
    if (!ultra.cmd14aScan()) {
        simpleMessage("NFC CARD INSPECTOR", "No supported tag detected", "Try moving closer and rescan");
        return false;
    }
    const auto &tag = ultra.hfTagData;
    if (tag.size == 0 || tag.size > 10) {
        simpleMessage("NFC CARD INSPECTOR", "Unexpected tag response");
        return false;
    }
    last.band = CardBand::HF13;
    last.uid = uidHex(tag.uidByte, tag.size);
    last.sak = tag.sak;
    last.atqa0 = tag.atqaByte[0];
    last.atqa1 = tag.atqaByte[1];
    last.kind = ultra.getTagTypeStr(tag.sak);
    last.atMillis = millis();
    return true;
}

bool scanLF() {
    if (!prepareRead("RFID TAG INSPECTOR", "Press wheel to scan")) return false;
    if (!ultra.cmdLFRead()) {
        simpleMessage("RFID TAG INSPECTOR", "No supported LF tag found", "This path reads EM410x tags");
        return false;
    }
    const auto &tag = ultra.lfTagData;
    if (tag.size == 0 || tag.size > 10) {
        simpleMessage("RFID TAG INSPECTOR", "Unexpected LF tag data");
        return false;
    }
    last.band = CardBand::LF125;
    last.uid = uidHex(tag.uidByte, tag.size);
    last.kind = "EM410x-compatible";
    last.sak = 0;
    last.atqa0 = 0;
    last.atqa1 = 0;
    last.atMillis = millis();
    return true;
}

void showCard(const CardRecord &card) {
    screen(card.band == CardBand::HF13 ? "NFC CARD INSPECTOR" : "RFID TAG INSPECTOR", "READ ONLY");
    label("Technology", card.band == CardBand::HF13 ? "13.56 MHz / 14443A" : "125 kHz / EM410x", 34, CYAN);
    // Show exact UID on device; SD reports redact it by default.
    label("Card UID", card.uid, 57);
    label("Reported type", card.kind, 80);
    if (card.band == CardBand::HF13) {
        label("SAK", "0x" + hexByte(card.sak), 103);
        label("ATQA", hexByte(card.atqa0) + hexByte(card.atqa1), 126);
    } else {
        label("RFID length", String(static_cast<unsigned>(card.uid.length() / 2)) + " bytes", 103);
        label("Security", "UID is not authentication", 126);
    }
    awaitExit();
}

void hfInspector() { if (scanHF()) showCard(last); }
void lfInspector() { if (scanLF()) showCard(last); }

void mifareDiagnostics() {
    if (last.band != CardBand::HF13) {
        simpleMessage("MIFARE CLASSIC", "Read an NFC test card first", "NFC Card Inspector -> scan");
        return;
    }
    const bool likelyClassic = classicCandidate(last.sak);
    screen("MIFARE CLASSIC", "DIAGNOSTICS");
    label("Last card SAK", "0x" + hexByte(last.sak), 37, CYAN);
    label("Classic candidate", likelyClassic ? "YES (SAK hint)" : "NOT INDICATED", 63);
    label("Crypto1 status", "Not tested", 89);
    label("Reader response", "Not tested", 115);
    tft.setTextColor(MUTED, BLACK);
    tft.drawString("No keys or nonces are collected", 10, 141, 1);
    awaitExit();
}

bool sameCard(const CardRecord &a, const CardRecord &b) {
    return a.band != CardBand::None && a.band == b.band && a.uid == b.uid;
}

void saveReference() {
    if (last.band == CardBand::None) {
        simpleMessage("COMPARE CREDENTIALS", "Scan a test tag first");
        return;
    }
    reference = last;
    simpleMessage("COMPARE CREDENTIALS", "Reference saved in memory", "Not written to SD");
}

void compareCard(bool hf) {
    if (reference.band == CardBand::None) {
        simpleMessage("COMPARE CREDENTIALS", "Set a reference tag first");
        return;
    }
    if (reference.band != (hf ? CardBand::HF13 : CardBand::LF125)) {
        simpleMessage("COMPARE CREDENTIALS", "Reference uses other band", "Choose matching tag technology");
        return;
    }
    if (!(hf ? scanHF() : scanLF())) return;
    screen("COMPARE CREDENTIALS", "UID ONLY");
    label("Reference UID", tail4(reference.uid), 35);
    label("Scanned UID", tail4(last.uid), 62);
    label("Same UID", sameCard(reference, last) ? "YES" : "NO", 92, CYAN);
    tft.setTextColor(MUTED, BLACK);
    tft.drawString("UID match does not prove access", 10, 127, 1);
    awaitExit();
}

void compareHF() { compareCard(true); }
void compareLF() { compareCard(false); }

void referenceMenu() {
    std::vector<Option> rows = {
        {"Use Last Scan as Reference", saveReference},
        {"Scan / Compare NFC", compareHF},
        {"Scan / Compare 125 kHz", compareLF},
        {"Back", []() {}}
    };
    while (true) {
        const int choice = loopOptions(rows, MENU_TYPE_SUBMENU, "CREDENTIAL COMPARISON");
        if (choice < 0 || choice == static_cast<int>(rows.size()) - 1) break;
    }
}

void saveReport() {
    if (last.band == CardBand::None) {
        simpleMessage("ACCESS LAB REPORT", "Nothing scanned yet");
        return;
    }
    if (!sdcardMounted) {
        simpleMessage("ACCESS LAB REPORT", "SD card not mounted");
        return;
    }
    const bool exists = SD.exists(REPORT_PATH);
    File report = SD.open(REPORT_PATH, FILE_APPEND);
    if (!report) {
        simpleMessage("ACCESS LAB REPORT", "Could not open SD file");
        return;
    }
    if (!exists) report.println("uptime_ms,band,type,uid_suffix,uid_bytes,sak,atqa");
    // A short UID suffix is retained for correlation without writing an
    // access credential or complete tag identifier onto removable storage.
    const String band = last.band == CardBand::HF13 ? "HF13.56" : "LF125";
    const String row = String(static_cast<unsigned long>(last.atMillis)) + "," + band + "," +
         (last.band == CardBand::HF13 ? "ISO14443A" : "EM410x") + "," +
         tail4(last.uid) + "," + String(static_cast<unsigned>(last.uid.length()/2)) + "," +
         hexByte(last.sak) + "," + hexByte(last.atqa0) + hexByte(last.atqa1);
    report.println(row);
    report.close();
    screen("ACCESS LAB REPORT", "SAVED");
    tft.setTextColor(CYAN, BLACK);
    tft.drawString("Report saved to SD card", 10, 58, 2);
    tft.setTextColor(MUTED, BLACK);
    tft.drawString(REPORT_PATH, 10, 88, 1);
    tft.drawString("UID shortened; time is uptime", 10, 111, 1);
    awaitExit();
}

} // namespace

void accessControlLabMenu() {
    std::vector<Option> rows = {
        {"Connect / Device Info", deviceInfo},
        {"NFC 13.56 MHz Inspector", hfInspector},
        {"RFID 125 kHz Inspector", lfInspector},
        {"MIFARE Classic Diagnostics", mifareDiagnostics},
        {"Compare Test Credentials", referenceMenu},
        {"Save Last Scan to SD", saveReport},
        {"Back", []() {}}
    };
    while (true) {
        const int choice = loopOptions(rows, MENU_TYPE_SUBMENU, "ACCESS CONTROL LAB");
        if (choice < 0 || choice == static_cast<int>(rows.size()) - 1) break;
    }
}
} // namespace esptrix
#else
namespace esptrix { void accessControlLabMenu() {} }
#endif
