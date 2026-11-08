// ESPTRIX native list-menu renderer, local development milestone.
// Independent enhancements to Bruce Firmware. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <vector>
struct Option;
namespace esptrix {
int nativeMenu(std::vector<Option>& items, uint8_t menuType,
               const char *subText, int selected, bool interpreter);
void brandedBoot();
}
