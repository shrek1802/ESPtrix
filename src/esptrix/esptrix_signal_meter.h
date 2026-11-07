// ESPTRIX passive RF activity meter.
// ESPTRIX additions to Bruce Firmware (AGPL-3.0-or-later).
// Measures RSSI changes only: never treats activity as a decoded packet.
#pragma once
#include <stdint.h>

namespace esptrix {

struct SignalSnapshot {
    int rssiDbm;
    int floorDbm;
    uint32_t peaks;
    bool active;
    bool calibrated;
};

class SignalMeter {
public:
    SignalMeter() = default;

    SignalSnapshot sample(int rssiDbm, uint32_t nowMs) {
        if (rssiDbm < -125) rssiDbm = -125;
        if (rssiDbm > 0) rssiDbm = 0;
        rssi_ = rssiDbm;

        if (warmup_ == 0) {
            floor_ = rssiDbm;
        }
        if (warmup_ < kWarmupSamples) {
            // Establish a local baseline before claiming any burst.
            floor_ = (floor_ * 7 + rssiDbm) / 8;
            ++warmup_;
            return state();
        }

        // Hysteresis prevents one long transmission from being counted
        // repeatedly. A gap between events avoids rapid noise-edge counts.
        if (active_) {
            if (rssiDbm <= floor_ + kReleaseDelta) active_ = false;
        } else if (rssiDbm >= floor_ + kTriggerDelta &&
                   (peaks_ == 0 || static_cast<uint32_t>(nowMs - lastPeakMs_) >= kDeadTimeMs)) {
            active_ = true;
            ++peaks_;
            lastPeakMs_ = nowMs;
        }

        // Track slow environmental drift only when outside a burst.
        // Do not let a strong signal raise the noise floor.
        if (!active_ && rssiDbm <= floor_ + kReleaseDelta) {
            floor_ = (floor_ * 31 + rssiDbm) / 32;
        }
        return state();
    }

    SignalSnapshot state() const {
        return {rssi_, floor_, peaks_, active_, warmup_ >= kWarmupSamples};
    }

private:
    static constexpr uint8_t kWarmupSamples = 20;
    static constexpr int kTriggerDelta = 12;
    static constexpr int kReleaseDelta = 6;
    static constexpr uint32_t kDeadTimeMs = 220;

    uint8_t warmup_ = 0;
    int rssi_ = -110;
    int floor_ = -110;
    uint32_t peaks_ = 0;
    uint32_t lastPeakMs_ = 0;
    bool active_ = false;
};

} // namespace esptrix
