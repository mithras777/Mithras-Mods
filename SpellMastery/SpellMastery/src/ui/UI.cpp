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
		using BonusTuning = MITHRAS::SPELL_MASTERY::MasteryConfig::BonusTuning;
		using SpellSchool = MITHRAS::SPELL_MASTERY::SpellSchool;

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
			ImGui::SliderFloat(("Skill bonus / level##" + a_prefix).c_str(), &a_tuning.skillBonusPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Skill bonus cap##" + a_prefix).c_str(), &a_tuning.skillBonusCap, 0.0f, 100.0f, "%.1f");
			ImGui::SliderFloat(("Magicka rate / level##" + a_prefix).c_str(), &a_tuning.magickaRatePerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Magicka rate cap##" + a_prefix).c_str(), &a_tuning.magickaRateCap, 0.0f, 100.0f, "%.1f");
		}

		void ApplyIfChanged(
			const MITHRAS::SPELL_MASTERY::MasteryConfig& a_before,
			const MITHRAS::SPELL_MASTERY::MasteryConfig& a_after)
		{
			auto tuningEqual = [](const BonusTuning& a_lhs, const BonusTuning& a_rhs) {
				return a_lhs.skillBonusPerLevel == a_rhs.skillBonusPerLevel &&
				       a_lhs.skillBonusCap == a_rhs.skillBonusCap &&
				       a_lhs.magickaRatePerLevel == a_rhs.magickaRatePerLevel &&
				       a_lhs.magickaRateCap == a_rhs.magickaRateCap;
			};

			bool schoolEqual = true;
			for (std::size_t i = 0; i < a_after.schoolBonuses.size(); ++i) {
				if (a_before.schoolBonuses[i].enabled != a_after.schoolBonuses[i].enabled ||
				    !tuningEqual(a_before.schoolBonuses[i].tuning, a_after.schoolBonuses[i].tuning)) {
					schoolEqual = false;
					break;
				}
			}

			if (a_before.enabled != a_after.enabled ||
				a_before.gainMultiplier != a_after.gainMultiplier ||
				a_before.thresholds != a_after.thresholds ||
				a_before.gainFromKills != a_after.gainFromKills ||
				!tuningEqual(a_before.generalBonuses, a_after.generalBonuses) ||
				!schoolEqual) {
				MITHRAS::SPELL_MASTERY::Manager::GetSingleton()->SetConfig(a_after, true);
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SKSE Menu Framework not installed - Spell Mastery pages unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Spell Mastery");
		SKSEMenuFramework::AddSectionItem("Overview", MasteryPanel::Render);
		SKSEMenuFramework::AddSectionItem("Progression", ProgressionPanel::Render);
		SKSEMenuFramework::AddSectionItem("Bonuses", BonusesPanel::Render);
		SKSEMenuFramework::AddSectionItem("Debug", DebugPanel::Render);
		LOG_INFO("Registered SKSE Menu Framework section: Spell Mastery");
	}

	namespace MasteryPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SPELL_MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Defaults##spell_overview", "Mithras: Reset spell mastery defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			if (ImGui::CollapsingHeader("Core", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Enable Spell Mastery", &config.enabled);
				ImGui::SliderFloat("Global gain multiplier", &config.gainMultiplier, 0.1f, 10.0f, "%.2f");
			}

			ImGui::Separator();
			ImGui::Text("Active schools:");
			bool hasActive = false;
			for (std::size_t i = 0; i < MITHRAS::SPELL_MASTERY::kSchoolCount; ++i) {
				const auto school = static_cast<SpellSchool>(i);
				if (!manager->IsSchoolActive(school)) {
					continue;
				}
				hasActive = true;
				ImGui::BulletText("%s", MITHRAS::SPELL_MASTERY::SchoolName(school).data());
			}
			if (!hasActive) {
				ImGui::TextDisabled("None");
			}

			ApplyIfChanged(before, config);
		}
	}

	namespace ProgressionPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SPELL_MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Defaults##spell_progression", "Mithras: Reset spell progression defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			if (ImGui::CollapsingHeader("Experience Sources", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Gain from kills", &config.gainFromKills);
			}

			if (ImGui::CollapsingHeader("Kill Thresholds", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (config.thresholds.size() < 5) {
					config.thresholds.resize(5, 200);
				}
				for (std::size_t i = 0; i < config.thresholds.size(); ++i) {
					int value = static_cast<int>(config.thresholds[i]);
					if (ImGui::InputInt(std::format("Level {} requirement", i + 1).c_str(), &value)) {
						config.thresholds[i] = static_cast<std::uint32_t>(std::max(1, value));
					}
				}
			}

			ApplyIfChanged(before, config);
		}
	}

	namespace BonusesPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SPELL_MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			DrawResetButton("Defaults##spell_bonuses", "Mithras: Reset spell bonus defaults", [manager]() { manager->ResetAllConfigToDefault(true); });

			if (ImGui::CollapsingHeader("General Bonuses", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawBonusTuningEditor(config.generalBonuses, "spell_general");
			}

			for (std::size_t i = 0; i < MITHRAS::SPELL_MASTERY::kSchoolCount; ++i) {
				auto& schoolCfg = config.schoolBonuses[i];
				const auto school = static_cast<SpellSchool>(i);
				const auto title = std::format("{} Bonuses", MITHRAS::SPELL_MASTERY::SchoolName(school));
				if (ImGui::CollapsingHeader(title.c_str())) {
					ImGui::Checkbox(std::format("Enable override##spell_{}", i).c_str(), &schoolCfg.enabled);
					if (schoolCfg.enabled) {
						DrawBonusTuningEditor(schoolCfg.tuning, std::format("spell_school_{}", i));
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
			auto* manager = MITHRAS::SPELL_MASTERY::Manager::GetSingleton();
			const auto& masteryData = manager->GetMasteryData();

			ImGui::Text("Tracked schools: %u", static_cast<unsigned>(MITHRAS::SPELL_MASTERY::kSchoolCount));
			ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
			if (ImGui::Button("Reset")) {
				manager->ClearDatabase();
				RE::DebugNotification("Spell Mastery: Database reset");
			}

			ImGui::Separator();

			if (ImGui::BeginTable("SpellMasteryTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Kills", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableHeadersRow();

				for (std::size_t i = 0; i < masteryData.size(); ++i) {
					const auto school = static_cast<SpellSchool>(i);
					const auto& stats = masteryData[i];

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(MITHRAS::SPELL_MASTERY::SchoolName(school).data());

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(manager->IsSchoolActive(school) ? "Yes" : "No");

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%u", stats.level);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%u", stats.kills);
				}

				ImGui::EndTable();
			}
		}
	}
}
