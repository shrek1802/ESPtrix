// Host-side checks for passive ESPTRIX RSSI activity counter.
// Run: g++ -std=c++17 -Wall -Wextra -Werror -I src tests/signal_meter_test.cpp -o /tmp/esptrix-meter-test && /tmp/esptrix-meter-test
#include "esptrix/esptrix_signal_meter.h"
#include <assert.h>
#include <stdint.h>

int main() {
    esptrix::SignalMeter meter;
    uint32_t ms = 0;
    for (int i = 0; i < 40; ++i, ms += 20) {
        auto snapshot = meter.sample(-96, ms);
        assert(snapshot.peaks == 0);
    }
    assert(meter.state().calibrated);
    assert(!meter.state().active);

    auto first = meter.sample(-48, ms);
    assert(first.peaks == 1 && first.active);
    ms += 20;
    for (int i = 0; i < 10; ++i, ms += 20) {
        auto snapshot = meter.sample(-49, ms);
        assert(snapshot.peaks == 1);
    }
    auto low = meter.sample(-95, ms);
    assert(!low.active);
    ms += 240;
    assert(meter.sample(-52, ms).peaks == 2);

    esptrix::SignalMeter quiet;
    for (int i = 0; i < 100; ++i) {
        auto snapshot = quiet.sample(-94 + (i % 3), i * 40);
        assert(snapshot.peaks == 0);
    }
    return 0;
}
