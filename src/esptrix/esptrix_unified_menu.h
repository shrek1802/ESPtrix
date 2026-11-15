// ESPTRIX V7 unified, portable category definitions.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stdint.h>
namespace esptrix {
enum class CategoryId : uint8_t {
    SmartTest = 0, Automotive, AccessControl, SubGhz,
    WifiNetwork, Bluetooth, IrUsb, FilesReports, System
};
struct CategoryDefinition { CategoryId id; const char *title; };
// Category labels and ordering are board-independent. Tool availability is not.
static constexpr CategoryDefinition kUnifiedCategories[] = {
    {CategoryId::SmartTest,     "Smart Test"},
    {CategoryId::Automotive,    "Automotive"},
    {CategoryId::AccessControl, "Access Control"},
    {CategoryId::SubGhz,       "Sub-GHz & RF"},
    {CategoryId::WifiNetwork,  "Wi-Fi & Network"},
    {CategoryId::Bluetooth,    "Bluetooth"},
    {CategoryId::IrUsb,        "Infrared & USB"},
    {CategoryId::FilesReports, "Files & Reports"},
    {CategoryId::System,       "System"},
};
constexpr unsigned kUnifiedCategoryCount = sizeof(kUnifiedCategories) / sizeof(kUnifiedCategories[0]);
void openUnifiedCategory(CategoryId category);
}
