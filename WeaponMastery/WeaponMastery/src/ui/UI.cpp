#include "ui/UI.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <format>
#include <functional>

namespace UI
{
	namespace
	{
		using BonusTuning = MITHRAS::MASTERY::MasteryConfig::BonusTuning;

		void DrawResetButton(const char* a_id, const char* a_message, const std::function<void()>& a_action)
		{
			const float buttonWidth = 140.0f;
			auto avail = ImGui::ImVec2Manager::Create();
			ImGui::GetContentRegionAvail(avail);
			const float offset = std::max(0.0f, avail->x - buttonWidth);
			ImGui::ImVec2Manager::Destroy(avail);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
			if (ImGui::Button(a_id, ImVec2(buttonWidth, 0.0f))) {
				a_action();
				RE::DebugNotification(a_message);
			}
		}

		void DrawBonusTuningEditor(BonusTuning& a_tuning, const std::string& a_prefix)
		{
			ImGui::SliderFloat(("Damage / level##" + a_prefix).c_str(), &a_tuning.damagePerLevel, 0.0f, 0.10f, "%.3f");
			ImGui::SliderFloat(("Speed / level##" + a_prefix).c_str(), &a_tuning.attackSpeedPerLevel, 0.0f, 0.10f, "%.3f");
			ImGui::SliderFloat(("Crit / level##" + a_prefix).c_str(), &a_tuning.critChancePerLevel, 0.0f, 0.10f, "%.3f");
			ImGui::SliderFloat(("Power stamina reduction / level##" + a_prefix).c_str(), &a_tuning.powerAttackStaminaReductionPerLevel, 0.0f, 0.10f, "%.3f");
			ImGui::SliderFloat(("Block stamina reduction / level##" + a_prefix).c_str(), &a_tuning.blockStaminaReductionPerLevel, 0.0f, 0.10f, "%.3f");

			ImGui::Separator();
			ImGui::SliderFloat(("Damage cap##" + a_prefix).c_str(), &a_tuning.damageCap, 0.0f, 1.5f, "%.2f");
			ImGui::SliderFloat(("Speed cap##" + a_prefix).c_str(), &a_tuning.attackSpeedCap, 0.0f, 1.5f, "%.2f");
			ImGui::SliderFloat(("Crit cap##" + a_prefix).c_str(), &a_tuning.critChanceCap, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat(("Power stamina floor##" + a_prefix).c_str(), &a_tuning.powerAttackStaminaFloor, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat(("Block stamina floor##" + a_prefix).c_str(), &a_tuning.blockStaminaFloor, 0.0f, 1.0f, "%.2f");
		}

		void ApplyIfChanged(const MITHRAS::MASTERY::MasteryConfig& a_before, const MITHRAS::MASTERY::MasteryConfig& a_after)
		{
			auto tuningEqual = [](const BonusTuning& a_lhs, const BonusTuning& a_rhs) {
				return a_lhs.damagePerLevel == a_rhs.damagePerLevel &&
				       a_lhs.attackSpeedPerLevel == a_rhs.attackSpeedPerLevel &&
				       a_lhs.critChancePerLevel == a_rhs.critChancePerLevel &&
				       a_lhs.powerAttackStaminaReductionPerLevel == a_rhs.powerAttackStaminaReductionPerLevel &&
				       a_lhs.blockStaminaReductionPerLevel == a_rhs.blockStaminaReductionPerLevel &&
				       a_lhs.damageCap == a_rhs.damageCap &&
				       a_lhs.attackSpeedCap == a_rhs.attackSpeedCap &&
				       a_lhs.critChanceCap == a_rhs.critChanceCap &&
				       a_lhs.powerAttackStaminaFloor == a_rhs.powerAttackStaminaFloor &&
				       a_lhs.blockStaminaFloor == a_rhs.blockStaminaFloor;
			};

			bool weaponTypeEqual = true;
			for (std::size_t i = 0; i < a_after.weaponTypeBonuses.size(); ++i) {
				if (a_before.weaponTypeBonuses[i].enabled != a_after.weaponTypeBonuses[i].enabled ||
				    !tuningEqual(a_before.weaponTypeBonuses[i].tuning, a_after.weaponTypeBonuses[i].tuning)) {
					weaponTypeEqual = false;
					break;
				}
			}

			if (a_before.enabled != a_after.enabled ||
				a_before.gainMultiplier != a_after.gainMultiplier ||
				a_before.thresholds != a_after.thresholds ||
				a_before.gainFromKills != a_after.gainFromKills ||
				a_before.gainFromHits != a_after.gainFromHits ||
				a_before.gainFromPowerHits != a_after.gainFromPowerHits ||
				a_before.gainFromBlocks != a_after.gainFromBlocks ||
				a_before.timeBasedLeveling != a_after.timeBasedLeveling ||
				!tuningEqual(a_before.generalBonuses, a_after.generalBonuses) ||
				!weaponTypeEqual) {
			MITHRAS::MASTERY::Manager::GetSingleton()->SetConfig(a_after, true);
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SKSE Menu Framework not installed - Mithras mastery pages unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Mithras Weapon Mastery");
		SKSEMenuFramework::AddSectionItem("Overview", MasteryPanel::Render);
		SKSEMenuFramework::AddSectionItem("Progression", ProgressionPanel::Render);
		SKSEMenuFramework::AddSectionItem("Bonuses", BonusesPanel::Render);
		SKSEMenuFramework::AddSectionItem("Debug", DebugPanel::Render);
		LOG_INFO("Registered SKSE Menu Framework section: Mithras Weapon Mastery");
	}

	namespace MasteryPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Reset To Defaults##overview", "Mithras: Reset overview defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			ImGui::SeparatorText("Core");
			ImGui::Checkbox("Enable Weapon Mastery", &config.enabled);
			ImGui::SliderFloat("Global gain multiplier", &config.gainMultiplier, 0.1f, 10.0f, "%.2f");

			ImGui::SeparatorText("Current Weapon");
			if (const auto key = manager->GetCurrentWeaponKey(); key.has_value()) {
				const auto stats = manager->GetStats(*key);
				ImGui::Text("Name: %s", manager->GetItemName(*key).c_str());
				ImGui::Text("Type: %s", manager->GetCurrentWeaponTypeName().c_str());
				ImGui::Text("Key: %s", MITHRAS::MASTERY::ToString(*key).c_str());
				ImGui::Text("Mastery level: %u", stats.level);
				ImGui::Text("Kills: %u", stats.kills);
				ImGui::Text("Equipped time: %.1fs", stats.secondsEquipped);
			} else {
				ImGui::Text("No weapon equipped.");
			}

			if (ImGui::Button("Reset Equipped Item Mastery")) {
				manager->ResetEquippedItemMastery();
				RE::DebugNotification("Mithras: Equipped item mastery reset");
			}

			ApplyIfChanged(before, config);
		}
	}

	namespace ProgressionPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Reset To Defaults##progression", "Mithras: Reset progression defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			ImGui::SeparatorText("Experience Sources");
			if (ImGui::BeginTable("gainSources", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Checkbox("Gain from kills", &config.gainFromKills);
				ImGui::TableSetColumnIndex(1); ImGui::Checkbox("Gain from hits", &config.gainFromHits);
				ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Checkbox("Gain from power hits", &config.gainFromPowerHits);
				ImGui::TableSetColumnIndex(1); ImGui::Checkbox("Gain from blocks", &config.gainFromBlocks);
				ImGui::EndTable();
			}
			ImGui::Checkbox("Time-based leveling", &config.timeBasedLeveling);

			ImGui::SeparatorText("Kill Thresholds");
			if (config.thresholds.size() < 5) {
				config.thresholds.resize(5, 200);
			}
			for (std::size_t i = 0; i < config.thresholds.size(); ++i) {
				int value = static_cast<int>(config.thresholds[i]);
				if (ImGui::InputInt(std::format("Level {} requirement", i + 1).c_str(), &value)) {
					config.thresholds[i] = static_cast<std::uint32_t>(std::max(1, value));
				}
			}

			ApplyIfChanged(before, config);
		}
	}

	namespace BonusesPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Reset To Defaults##bonuses", "Mithras: Reset bonus defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			if (ImGui::CollapsingHeader("General Bonuses", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawBonusTuningEditor(config.generalBonuses, "general");
			}

			for (std::size_t i = 0; i < config.weaponTypeBonuses.size(); ++i) {
				const auto type = static_cast<RE::WEAPON_TYPE>(i);
				auto& typeCfg = config.weaponTypeBonuses[i];
				const auto title = std::format("{} Bonuses", MITHRAS::MASTERY::WeaponTypeName(type));
				if (ImGui::CollapsingHeader(title.c_str())) {
					ImGui::Checkbox(std::format("Enable override##{}", i).c_str(), &typeCfg.enabled);
					if (typeCfg.enabled) {
						DrawBonusTuningEditor(typeCfg.tuning, std::format("type_{}", i));
					} else {
						ImGui::TextDisabled("Using General Bonuses");
					}
				}
			}

			ApplyIfChanged(before, config);
		}
	}

	namespace DebugPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MASTERY::Manager::GetSingleton();
			DrawResetButton("Reset To Defaults##debug", "Mithras: Reset config defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			ImGui::SeparatorText("Database");
			ImGui::Text("Tracked items: %u", static_cast<unsigned>(manager->GetDatabaseSize()));

			if (ImGui::Button("Clear Mastery Database")) {
				manager->ClearDatabase();
				RE::DebugNotification("Mithras: Mastery database cleared");
			}
		}
	}
}
