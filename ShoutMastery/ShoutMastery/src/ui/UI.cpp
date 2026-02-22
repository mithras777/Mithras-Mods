#include "ui/UI.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <format>

namespace UI
{
	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SKSE Menu Framework not installed - Shout Mastery pages unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Shout Mastery");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("Registered SKSE Menu Framework section: Shout Mastery");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			if (ImGui::BeginTabBar("ShoutMasteryTabs")) {
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
			auto* manager = MITHRAS::SHOUT_MASTERY::Manager::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;

			ImGui::Checkbox("Enable Shout Mastery", &cfg.enabled);
			ImGui::SliderFloat("Global gain multiplier", &cfg.gainMultiplier, 0.1f, 10.0f, "%.2f");

			if (const auto current = manager->GetCurrentShoutKey(); current.has_value()) {
				const auto stats = manager->GetStats(*current);
				ImGui::Text("Equipped shout: %s", current->name.c_str());
				ImGui::Text("Mastery level: %u", stats.level);
				ImGui::Text("Uses: %u", stats.uses);
			} else {
				ImGui::TextDisabled("No shout equipped");
			}
			ImGui::Separator();
			if (ImGui::Button("Defaults##shout_overview")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset shout mastery defaults");
			}

			if (before.enabled != cfg.enabled ||
				before.gainMultiplier != cfg.gainMultiplier) {
				manager->SetConfig(cfg, true);
			}
		}
	}

	namespace ProgressionPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SHOUT_MASTERY::Manager::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;

			ImGui::Checkbox("Gain from uses", &cfg.gainFromUses);

			if (cfg.thresholds.size() < 5) {
				cfg.thresholds.resize(5, 200);
			}
			for (std::size_t i = 0; i < cfg.thresholds.size(); ++i) {
				int value = static_cast<int>(cfg.thresholds[i]);
				if (ImGui::InputInt(std::format("Level {} requirement", i + 1).c_str(), &value)) {
					cfg.thresholds[i] = static_cast<std::uint32_t>(std::max(1, value));
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Defaults##shout_progression")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset shout progression defaults");
			}

			if (before.gainFromUses != cfg.gainFromUses ||
				before.thresholds != cfg.thresholds) {
				manager->SetConfig(cfg, true);
			}
		}
	}

	namespace BonusesPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SHOUT_MASTERY::Manager::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;
			auto& b = cfg.bonuses;

			ImGui::SliderFloat("Cooldown reduction / level", &b.shoutRecoveryPerLevel, -10.0f, 0.0f, "%.2f");
			ImGui::SliderFloat("Cooldown reduction cap", &b.shoutRecoveryCap, -100.0f, 0.0f, "%.1f");
			ImGui::SliderFloat("Shout damage / level", &b.shoutPowerPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("Shout damage cap", &b.shoutPowerCap, 0.0f, 150.0f, "%.1f");
			ImGui::SliderFloat("Voice rate / level", &b.voiceRatePerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("Voice rate cap", &b.voiceRateCap, 0.0f, 250.0f, "%.1f");
			ImGui::SliderFloat("Voice points / level", &b.voicePointsPerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("Voice points cap", &b.voicePointsCap, 0.0f, 250.0f, "%.1f");
			ImGui::SliderFloat("Stamina regen / level", &b.staminaRatePerLevel, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("Stamina regen cap", &b.staminaRateCap, 0.0f, 150.0f, "%.1f");
			ImGui::Separator();
			if (ImGui::Button("Defaults##shout_bonuses")) {
				manager->ResetAllConfigToDefault(true);
				RE::DebugNotification("Mithras: Reset shout bonus defaults");
			}

			if (before.bonuses.shoutRecoveryPerLevel != b.shoutRecoveryPerLevel ||
				before.bonuses.shoutRecoveryCap != b.shoutRecoveryCap ||
				before.bonuses.shoutPowerPerLevel != b.shoutPowerPerLevel ||
				before.bonuses.shoutPowerCap != b.shoutPowerCap ||
				before.bonuses.voiceRatePerLevel != b.voiceRatePerLevel ||
				before.bonuses.voiceRateCap != b.voiceRateCap ||
				before.bonuses.voicePointsPerLevel != b.voicePointsPerLevel ||
				before.bonuses.voicePointsCap != b.voicePointsCap ||
				before.bonuses.staminaRatePerLevel != b.staminaRatePerLevel ||
				before.bonuses.staminaRateCap != b.staminaRateCap) {
				manager->SetConfig(cfg, true);
			}
		}
	}

	namespace DebugPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SHOUT_MASTERY::Manager::GetSingleton();
			const auto& data = manager->GetMasteryData();

			ImGui::Text("Tracked shouts: %u", static_cast<unsigned>(data.size()));
			ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
			if (ImGui::Button("Reset")) {
				manager->ClearDatabase();
				RE::DebugNotification("Shout Mastery: Database reset");
			}
			ImGui::Separator();

			if (data.empty()) {
				ImGui::TextDisabled("No shouts tracked yet.");
				return;
			}

			if (ImGui::BeginTable("ShoutMasteryTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Shout", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Uses", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("FormID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableHeadersRow();

				for (const auto& [key, stats] : data) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(key.name.c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%u", stats.level);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%u", stats.uses);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%08X", key.formID);
				}

				ImGui::EndTable();
			}
		}
	}
}
