#include "ui/UI.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <format>

namespace UI
{
	namespace
	{
		using Config = MITHRAS::SPELL_MASTERY::MasteryConfig;
		using BonusTuning = Config::BonusTuning;
		using SpellSchool = MITHRAS::SPELL_MASTERY::SpellSchool;

		void DrawBonusTuningEditor(BonusTuning& a_tuning, const std::string& a_prefix)
		{
			ImGui::SliderFloat(("Skill bonus / level##" + a_prefix).c_str(), &a_tuning.skillBonusPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Skill bonus cap##" + a_prefix).c_str(), &a_tuning.skillBonusCap, 0.0f, 200.0f, "%.1f");
			ImGui::SliderFloat(("Skill XP / level##" + a_prefix).c_str(), &a_tuning.skillAdvancePerLevel, 0.0f, 20.0f, "%.2f");
			ImGui::SliderFloat(("Skill XP cap##" + a_prefix).c_str(), &a_tuning.skillAdvanceCap, 0.0f, 300.0f, "%.1f");
			ImGui::SliderFloat(("Power bonus / level##" + a_prefix).c_str(), &a_tuning.powerBonusPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Power bonus cap##" + a_prefix).c_str(), &a_tuning.powerBonusCap, 0.0f, 200.0f, "%.1f");
			ImGui::SliderFloat(("Cost reduction / level##" + a_prefix).c_str(), &a_tuning.costReductionPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Cost reduction cap##" + a_prefix).c_str(), &a_tuning.costReductionCap, 0.0f, 100.0f, "%.1f");
			ImGui::SliderFloat(("Magicka rate / level##" + a_prefix).c_str(), &a_tuning.magickaRatePerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat(("Magicka rate cap##" + a_prefix).c_str(), &a_tuning.magickaRateCap, 0.0f, 100.0f, "%.1f");
			ImGui::SliderFloat(("Max magicka / level##" + a_prefix).c_str(), &a_tuning.magickaFlatPerLevel, 0.0f, 30.0f, "%.2f");
			ImGui::SliderFloat(("Max magicka cap##" + a_prefix).c_str(), &a_tuning.magickaFlatCap, 0.0f, 500.0f, "%.1f");
		}

		void DrawSchoolProgressionEditor(SpellSchool a_school, Config::SchoolConfig& a_cfg, std::size_t a_index)
		{
			auto& p = a_cfg.progression;
			const auto header = std::format("{} Progression", MITHRAS::SPELL_MASTERY::SchoolName(a_school));
			if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				return;
			}

			switch (a_school) {
				case SpellSchool::kDestruction:
					ImGui::Checkbox(std::format("Gain from kills##{}", a_index).c_str(), &p.gainFromKills);
					ImGui::Checkbox(std::format("Gain from hits##{}", a_index).c_str(), &p.gainFromHits);
					ImGui::Checkbox(std::format("Gain from uses##{}", a_index).c_str(), &p.gainFromUses);
					break;
				case SpellSchool::kConjuration:
					ImGui::Checkbox(std::format("Gain from summons##{}", a_index).c_str(), &p.gainFromSummons);
					ImGui::Checkbox(std::format("Gain from uses##{}", a_index).c_str(), &p.gainFromUses);
					break;
				case SpellSchool::kAlteration:
					ImGui::Checkbox(std::format("Gain from hits##{}", a_index).c_str(), &p.gainFromHits);
					ImGui::Checkbox(std::format("Gain from uses##{}", a_index).c_str(), &p.gainFromUses);
					break;
				case SpellSchool::kIllusion:
					ImGui::Checkbox(std::format("Gain from hits##{}", a_index).c_str(), &p.gainFromHits);
					ImGui::Checkbox(std::format("Gain from uses##{}", a_index).c_str(), &p.gainFromUses);
					break;
				case SpellSchool::kRestoration:
					ImGui::Checkbox(std::format("Gain from equip time##{}", a_index).c_str(), &p.gainFromEquipTime);
					ImGui::SliderFloat(std::format("Seconds per point##{}", a_index).c_str(), &p.equipSecondsPerPoint, 0.5f, 30.0f, "%.1f");
					break;
				default:
					break;
			}
		}

		void ApplyIfChanged(const Config& a_before, const Config& a_after)
		{
			auto bonusEqual = [](const BonusTuning& a_lhs, const BonusTuning& a_rhs) {
				return a_lhs.skillBonusPerLevel == a_rhs.skillBonusPerLevel &&
				       a_lhs.skillBonusCap == a_rhs.skillBonusCap &&
				       a_lhs.skillAdvancePerLevel == a_rhs.skillAdvancePerLevel &&
				       a_lhs.skillAdvanceCap == a_rhs.skillAdvanceCap &&
				       a_lhs.powerBonusPerLevel == a_rhs.powerBonusPerLevel &&
				       a_lhs.powerBonusCap == a_rhs.powerBonusCap &&
				       a_lhs.costReductionPerLevel == a_rhs.costReductionPerLevel &&
				       a_lhs.costReductionCap == a_rhs.costReductionCap &&
				       a_lhs.magickaRatePerLevel == a_rhs.magickaRatePerLevel &&
				       a_lhs.magickaRateCap == a_rhs.magickaRateCap &&
				       a_lhs.magickaFlatPerLevel == a_rhs.magickaFlatPerLevel &&
				       a_lhs.magickaFlatCap == a_rhs.magickaFlatCap;
			};

			bool schoolEqual = true;
			for (std::size_t i = 0; i < a_after.schools.size(); ++i) {
				const auto& b = a_before.schools[i];
				const auto& a = a_after.schools[i];
				if (b.useBonusOverride != a.useBonusOverride ||
					b.progression.gainFromKills != a.progression.gainFromKills ||
					b.progression.gainFromUses != a.progression.gainFromUses ||
					b.progression.gainFromSummons != a.progression.gainFromSummons ||
					b.progression.gainFromHits != a.progression.gainFromHits ||
					b.progression.gainFromEquipTime != a.progression.gainFromEquipTime ||
					b.progression.equipSecondsPerPoint != a.progression.equipSecondsPerPoint ||
					!bonusEqual(b.bonus, a.bonus)) {
					schoolEqual = false;
					break;
				}
			}

			if (a_before.enabled != a_after.enabled ||
				a_before.gainMultiplier != a_after.gainMultiplier ||
				a_before.thresholds != a_after.thresholds ||
				!bonusEqual(a_before.generalBonuses, a_after.generalBonuses) ||
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
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("Registered SKSE Menu Framework section: Spell Mastery");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			if (ImGui::BeginTabBar("SpellMasteryTabs")) {
				if (ImGui::BeginTabItem("Overview")) {
					MasteryPanel::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Progression")) {
					ProgressionPanel::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Bonuses")) {
					BonusesPanel::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Debug")) {
					DebugPanel::Render();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
	}

	namespace MasteryPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SPELL_MASTERY::Manager::GetSingleton();
			auto config = manager->GetConfig();
			const auto before = config;

			if (ImGui::CollapsingHeader("Core", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Enable Spell Mastery", &config.enabled);
				ImGui::SliderFloat("Global gain multiplier", &config.gainMultiplier, 0.1f, 10.0f, "%.2f");
			}

			ImGui::Separator();
			ImGui::Text("Equipped spells:");
			const auto equipped = manager->GetEquippedSpellKeys();
			if (equipped.empty()) {
				ImGui::TextDisabled("None");
			} else {
				for (const auto& key : equipped) {
					ImGui::BulletText("%s (%s)", key.name.c_str(), MITHRAS::SPELL_MASTERY::SchoolName(key.school).data());
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Defaults##spell_overview")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset spell mastery defaults");
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

			if (ImGui::CollapsingHeader("Level Thresholds", ImGuiTreeNodeFlags_DefaultOpen)) {
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

			for (std::size_t i = 0; i < MITHRAS::SPELL_MASTERY::kSchoolCount; ++i) {
				DrawSchoolProgressionEditor(static_cast<SpellSchool>(i), config.schools[i], i);
			}
			ImGui::Separator();
			if (ImGui::Button("Defaults##spell_progression")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset spell progression defaults");
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

			if (ImGui::CollapsingHeader("General Bonuses", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawBonusTuningEditor(config.generalBonuses, "spell_general");
			}

			for (std::size_t i = 0; i < MITHRAS::SPELL_MASTERY::kSchoolCount; ++i) {
				auto& schoolCfg = config.schools[i];
				const auto school = static_cast<SpellSchool>(i);
				const auto title = std::format("{} Bonuses", MITHRAS::SPELL_MASTERY::SchoolName(school));
				if (ImGui::CollapsingHeader(title.c_str())) {
					ImGui::Checkbox(std::format("Use school override##spell_{}", i).c_str(), &schoolCfg.useBonusOverride);
					if (schoolCfg.useBonusOverride) {
						DrawBonusTuningEditor(schoolCfg.bonus, std::format("spell_school_{}", i));
					} else {
						ImGui::TextDisabled("Using General Bonuses");
					}
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Defaults##spell_bonuses")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset spell bonus defaults");
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

			ImGui::Text("Tracked spells: %u", static_cast<unsigned>(masteryData.size()));
			ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
			if (ImGui::Button("Reset")) {
				manager->ClearDatabase();
				RE::DebugNotification("Spell Mastery: Database reset");
			}

			ImGui::Separator();

			if (masteryData.empty()) {
				ImGui::TextDisabled("No spells tracked yet.");
				return;
			}

			if (ImGui::BeginTable("SpellMasteryTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Kills", ImGuiTableColumnFlags_WidthFixed, 45.0f);
				ImGui::TableSetupColumn("Hits", ImGuiTableColumnFlags_WidthFixed, 45.0f);
				ImGui::TableSetupColumn("Uses", ImGuiTableColumnFlags_WidthFixed, 45.0f);
				ImGui::TableSetupColumn("Summons", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Equip(s)", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableHeadersRow();

				for (const auto& [key, _] : masteryData) {
					const auto stats = manager->GetStats(key);
					const auto count = manager->GetProgressCount(key);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(key.name.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(MITHRAS::SPELL_MASTERY::SchoolName(key.school).data());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%u", stats.level);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%u", count);

					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%u", stats.kills);

					ImGui::TableSetColumnIndex(5);
					ImGui::Text("%u", stats.hits);

					ImGui::TableSetColumnIndex(6);
					ImGui::Text("%u", stats.uses);

					ImGui::TableSetColumnIndex(7);
					ImGui::Text("%u", stats.summons);

					ImGui::TableSetColumnIndex(8);
					ImGui::Text("%.0f", stats.equippedSeconds);
				}

				ImGui::EndTable();
			}
		}
	}
}
