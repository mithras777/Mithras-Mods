#pragma once

#include "Settings.h"
#include <string>
#include <vector>

namespace DYNAMIC_SPAWNS::UTILS
{
	[[nodiscard]] std::string ToLower(std::string a_value);
	[[nodiscard]] bool StringContainsInsensitive(const std::string& a_value, const std::string& a_needle);
	[[nodiscard]] bool IsDayTime();
	[[nodiscard]] TimeOfDay GetTimeOfDay();
	[[nodiscard]] bool IsCellNameBlacklisted(const std::string& a_cellName, const std::vector<std::string>& a_blacklistLower);
	[[nodiscard]] std::string GetWorldspaceEditorID(RE::TESWorldSpace* a_worldspace);
	[[nodiscard]] std::string GetCellName(RE::TESObjectCELL* a_cell);
	[[nodiscard]] bool IsGameplayBlocked();
	[[nodiscard]] RE::BGSLocation* GetCurrentLocation(RE::PlayerCharacter* a_player);
}
