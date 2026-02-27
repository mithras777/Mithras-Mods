#include "ui/MCM_Menu.h"

#include "movement/MovementPatcher.h"
#include "movement/Settings.h"
#include "util/LogUtil.h"

#include <array>
#include <string>

namespace UI::MCM
{
	namespace
	{
		MOVEMENT::SettingsData g_config{};
		MOVEMENT::SettingsData g_before{};
		bool g_dirty{ false };

		constexpr std::array<const char*, 5> kRows{
			"Walk",
			"Run",
			"Sprint",
			"Sneak Walk",
			"Sneak Run"
		};

		bool ConfigChanged(const MOVEMENT::SettingsData& a_lhs, const MOVEMENT::SettingsData& a_rhs)
		{
			if (a_lhs.version != a_rhs.version) {
				return true;
			}
			if (a_lhs.general.enabled != a_rhs.general.enabled ||
			    a_lhs.general.restoreOnDisable != a_rhs.general.restoreOnDisable) {
				return true;
			}
			if (a_lhs.entries.size() != a_rhs.entries.size()) {
				return true;
			}
			for (std::size_t i = 0; i < a_lhs.entries.size(); ++i) {
				const auto& l = a_lhs.entries[i];
				const auto& r = a_rhs.entries[i];
				if (l.name != r.name || l.form != r.form || l.group != r.group || l.enabled != r.enabled) {
					return true;
				}
				for (std::size_t row = 0; row < 5; ++row) {
					if (l.speeds[row][0] != r.speeds[row][0] || l.speeds[row][1] != r.speeds[row][1]) {
						return true;
					}
				}
			}
			return false;
		}

		void QueueApply()
		{
			g_dirty = true;
		}

		void CommitIfDirty()
		{
			if (!g_dirty) {
				return;
			}

			auto* settings = MOVEMENT::Settings::GetSingleton();
			auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();
			const auto previous = g_before;

			settings->Set(g_config, true);
			patcher->OnSettingsChanged(previous, g_config);

			g_before = g_config;
			g_dirty = false;
		}

		void RenderGroup(const char* a_group)
		{
			for (std::size_t i = 0; i < g_config.entries.size(); ++i) {
				auto& entry = g_config.entries[i];
				if (entry.group != a_group) {
					continue;
				}

				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();
				ImGui::Checkbox("Enabled", &entry.enabled);
				ImGui::SameLine();
				ImGui::TextUnformatted(entry.name.c_str());
				ImGui::TextDisabled("%s", entry.form.c_str());

				if (ImGui::TreeNode("Speeds")) {
					for (std::size_t row = 0; row < kRows.size(); ++row) {
						ImGui::SliderFloat((std::string(kRows[row]) + " Left").c_str(), &entry.speeds[row][0], 0.0f, 2000.0f, "%.1f");
						ImGui::SliderFloat((std::string(kRows[row]) + " Right").c_str(), &entry.speeds[row][1], 0.0f, 2000.0f, "%.1f");
					}
					ImGui::TreePop();
				}

				ImGui::PopID();
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("[Movement] SKSE Menu Framework not installed - MCM unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Configurable Movement Speed");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* settings = MOVEMENT::Settings::GetSingleton();
			g_config = settings->Get();
			g_before = g_config;
			g_dirty = false;

			if (ImGui::BeginTabBar("ConfigMovementSpeedTabs")) {
				if (ImGui::BeginTabItem("General")) {
					GeneralTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Combat")) {
					CombatTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Default")) {
					DefaultTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Misc")) {
					MiscTab::Render();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			if (ConfigChanged(g_before, g_config)) {
				QueueApply();
			}

			CommitIfDirty();
		}
	}

	namespace GeneralTab
	{
		void Render()
		{
			ImGui::Checkbox("Enabled", &g_config.general.enabled);
			ImGui::Checkbox("Restore on disable", &g_config.general.restoreOnDisable);
			ImGui::Spacing();

			if (ImGui::Button("Apply Now")) {
				MOVEMENT::MovementPatcher::GetSingleton()->ApplyNow(g_config);
			}

			ImGui::SameLine();
			if (ImGui::Button("Restore Originals")) {
				auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();
				patcher->Restore();
				g_config.general.enabled = false;
				QueueApply();
			}

			ImGui::TextDisabled("Each enabled entry writes its Left/Right speeds directly to the mapped movement type.");
		}
	}

	namespace CombatTab
	{
		void Render()
		{
			RenderGroup("Combat");
		}
	}

	namespace DefaultTab
	{
		void Render()
		{
			RenderGroup("Default");
		}
	}

	namespace MiscTab
	{
		void Render()
		{
			RenderGroup("Misc");
		}
	}
}
