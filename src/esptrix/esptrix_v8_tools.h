#pragma once
// ESPTRIX V8 diagnostics. The only user-facing entry points of this module.
// SPDX-License-Identifier: AGPL-3.0-or-later
namespace esptrix {
void v8KeyFobAnalyzer();
void v8PulseScope();
void v8InterferenceFinder();
void v8TpmsInspector();
void v8NfcInspector();
void v8BleLocator();
void v8WifiWalkSurvey();
void v8AcousticSpectrum();
}
