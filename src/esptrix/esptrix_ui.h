// ESPTRIX additional RF diagnostics, based on Bruce 1.16.1.
// Copyright (C) 2026 ESPTRIX contributors.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

namespace esptrix {
void rfLabMenu();
void keyFobMenu();
void tpmsMenu();
void keyFobMonitor(float frequencyMHz);
void tpmsMonitor(float frequencyMHz);
}
