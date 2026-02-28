#include "Utils.h"

#include <algorithm>
#include <cctype>

namespace DYNAMIC_SPAWNS::UTILS
{
	std::string ToLower(std::string a_value)
	{
		std::transform(a_value.begin(), a_value.end(), a_value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return a_value;
	}

	bool StringContainsInsensitive(const std::string& a_value, const std::string& a_needle)
	{
		return ToLower(a_value).contains(ToLower(a_needle));
	}

	bool IsDayTime()
	{
		const auto* calendar = RE::Calendar::GetSingleton();
		if (!calendar) {
			return true;
		}
		const auto hour = calendar->GetHour();
		return hour >= 6.0F && hour < 20.0F;
	}

	TimeOfDay GetTimeOfDay()
	{
		return IsDayTime() ? TimeOfDay::kDay : TimeOfDay::kNight;
	}

	bool IsCellNameBlacklisted(const std::string& a_cellName, const std::vector<std::string>& a_blacklistLower)
	{
		const auto cellLower = ToLower(a_cellName);
		for (const auto& blocked : a_blacklistLower) {
			if (!blocked.empty() && cellLower.contains(blocked)) {
				return true;
			}
		}
		return false;
	}

	std::string GetWorldspaceEditorID(RE::TESWorldSpace* a_worldspace)
	{
		if (!a_worldspace) {
			return {};
		}
		const auto editorID = a_worldspace->GetFormEditorID();
		return editorID ? std::string(editorID) : std::string{};
	}

	std::string GetCellName(RE::TESObjectCELL* a_cell)
	{
		if (!a_cell) {
			return {};
		}
		const auto* name = a_cell->GetName();
		return name ? std::string(name) : std::string{};
	}

	bool IsGameplayBlocked()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			return true;
		}
		if (ui->GameIsPaused()) {
			return true;
		}
		return ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::MainMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::FaderMenu::MENU_NAME);
	}

	RE::BGSLocation* GetCurrentLocation(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return nullptr;
		}
		if (auto* location = a_player->GetCurrentLocation()) {
			return location;
		}
		if (auto* cell = a_player->GetParentCell()) {
			return cell->GetLocation();
		}
		return nullptr;
	}
}
