// ESPTRIX V8.2.2: embedded boot animation for LILYGO T-Embed CC1101.
// This splash never examines button inputs. Bruce recovery is implemented
// later by MainMenu::begin() and is therefore not disabled by this animation.
// Animated art / AGPL source attribution inherited from ESPTRIX/Bruce.
#include "esptrix_boot.h"
#include <Arduino.h>
#include <pins_arduino.h>

#if defined(T_EMBED_1101) && defined(HAS_SCREEN)
#include "core/display.h"
#include "esptrix_boot_frames.h"

namespace esptrix {
namespace {
// Decode one RGB565 raster line in stack memory. RLE runs may cross scanlines.
bool paintFrame(int frame) {
    using namespace boot_art;
    if (frame < 0 || frame >= kFrames) return false;
    if (tft.width() < kWidth || tft.height() < kHeight) return false;
    const int x0 = (tft.width() - kWidth) / 2;
    const int y0 = (tft.height() - kHeight) / 2;
    const uint32_t begin = kFrameOffsets[frame];
    const uint32_t end = begin + kFrameLengths[frame];
    uint32_t cursor = begin;
    uint16_t color = 0;
    uint16_t remaining = 0;
    uint16_t linePixels[kWidth];
    const bool oldSwap = tft.getSwapBytes();
    tft.setSwapBytes(true);
    bool success = true;
    for (int y = 0; y < kHeight && success; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            if (remaining == 0) {
                if (cursor + 1 >= end) { success = false; break; }
                remaining = pgm_read_byte(&kRuns[cursor++]);
                const uint8_t index = pgm_read_byte(&kRuns[cursor++]);
                if (remaining == 0 || index >= kPaletteSize) { success = false; break; }
                color = pgm_read_word(&kPalette[index]);
            }
            linePixels[x] = color;
            --remaining;
        }
        if (success) tft.pushImage(x0, y0 + y, kWidth, 1, linePixels);
    }
    tft.setSwapBytes(oldSwap);
    return success && cursor == end && remaining == 0;
}

// Visual fallback even if display rotation or image decoding differs locally.
void showFallback() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawCentreString("ESPTRIX", tft.width() / 2, tft.height() / 2 - 12, 1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("DIAGNOSTIC FIRMWARE", tft.width() / 2, tft.height() / 2 + 15, 1);
    delay(1500);
}
} // namespace

void playBootLogo() {
    static bool alreadyPlayed = false;
    if (alreadyPlayed) return; // Older startup code may call this a second time.
    alreadyPlayed = true;
    // Intentionally do not read BK_BTN here: the pin is not a reliable
    // startup-bypass test before MainMenu::begin(). The recovery check remains
    // in Bruce's existing main menu and continues to work normally.
    Serial.println("[ESPTRIX BOOT V8.2.2] animation:start");
    if (tft.width() < boot_art::kWidth || tft.height() < boot_art::kHeight) {
        Serial.println("[ESPTRIX BOOT V8.2.2] animation:fallback-orientation");
        showFallback();
        tft.fillScreen(TFT_BLACK);
        return;
    }
    tft.fillScreen(TFT_BLACK);
    bool ok = true;
    for (int frame = 0; frame < boot_art::kFrames; ++frame) {
        if (!paintFrame(frame)) {
            Serial.printf("[ESPTRIX BOOT V8.2.2] animation:frame-%d-error\n", frame);
            ok = false;
            break;
        }
        delay(frame == boot_art::kFrames - 1 ? 850 : 200);
    }
    if (!ok) showFallback();
    tft.fillScreen(TFT_BLACK);
    Serial.println(ok ? "[ESPTRIX BOOT V8.2.2] animation:complete"
                      : "[ESPTRIX BOOT V8.2.2] animation:fallback-complete");
}
} // namespace esptrix
#else
namespace esptrix { void playBootLogo() {} }
#endif
