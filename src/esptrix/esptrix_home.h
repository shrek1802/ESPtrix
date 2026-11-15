// ESPTRIX graphical launcher. ESPTRIX modification to Bruce Firmware, 2026-11-07.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
namespace esptrix {
// Runtime-only choice; power cycle returns to ESPTRIX unless BACK is held.
bool nativeHomeEnabled();
void setNativeHomeEnabled(bool enabled);
void showHome();
}
