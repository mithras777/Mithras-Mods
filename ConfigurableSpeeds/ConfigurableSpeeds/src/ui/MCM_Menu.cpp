#include "ui/MCM_Menu.h"

#include "movement/MovementPatcher.h"
#include "movement/Settings.h"
#include "plugin.h"
#include "util/LogUtil.h"

#include <array>
#include <format>
#include <string>

namespace UI::MCM
{
	namespace
	{
		MOVEMENT::SettingsData g_config{};
		MOVEMENT::SettingsData g_before{};
		bool g_dirty{ false };

		char g_formInput[128]{};
		char g_nameInput[128]{};

		bool ConfigChanged(const MOVEMENT::SettingsData& a_lhs, const MOVEMENT::SettingsData& a_rhs)
		{
			if (a_lhs.version != a_rhs.version) {
				return true;
			}

			if (a_lhs.general.enabled != a_rhs.general.enabled ||
				a_lhs.general.restoreOnDisable != a_rhs.general.restoreOnDisable ||
				a_lhs.general.dumpOnStartup != a_rhs.general.dumpOnStartup ||
				a_lhs.general.policy.useMultipliers != a_rhs.general.policy.useMultipliers ||
				a_lhs.general.policy.useOverrides != a_rhs.general.policy.useOverrides) {
				return true;
			}

			const auto& lm = a_lhs.multipliers;
			const auto& rm = a_rhs.multipliers;
			if (lm.walk != rm.walk || lm.run != rm.run || lm.sprint != rm.sprint ||
				lm.sneakWalk != rm.sneakWalk || lm.sneakRun != rm.sneakRun) {
				return true;
			}

			const auto& lo = a_lhs.overrides;
			const auto& ro = a_rhs.overrides;
			if (lo.walk.enabled != ro.walk.enabled || lo.walk.value != ro.walk.value ||
				lo.run.enabled != ro.run.enabled || lo.run.value != ro.run.value ||
				lo.sprint.enabled != ro.sprint.enabled || lo.sprint.value != ro.sprint.value ||
				lo.sneakWalk.enabled != ro.sneakWalk.enabled || lo.sneakWalk.value != ro.sneakWalk.value ||
				lo.sneakRun.enabled != ro.sneakRun.enabled || lo.sneakRun.value != ro.sneakRun.value) {
				return true;
			}

			if (a_lhs.targets.size() != a_rhs.targets.size()) {
				return true;
			}

			for (std::size_t i = 0; i < a_lhs.targets.size(); ++i) {
				const auto& l = a_lhs.targets[i];
				const auto& r = a_rhs.targets[i];
				if (l.name != r.name || l.form != r.form || l.enabled != r.enabled) {
					return true;
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
				if (ImGui::BeginTabItem("Multipliers")) {
					MultipliersTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Overrides")) {
					OverridesTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Targets")) {
					TargetsTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Tools")) {
					ToolsTab::Render();
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
			ImGui::Checkbox("Dump on startup", &g_config.general.dumpOnStartup);
			ImGui::Separator();
			ImGui::Checkbox("Use multipliers", &g_config.general.policy.useMultipliers);
			ImGui::Checkbox("Use overrides", &g_config.general.policy.useOverrides);
			ImGui::Spacing();

			if (ImGui::Button("Apply Now")) {
				auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();
				patcher->ApplyNow(g_config);
			}

			ImGui::SameLine();
			if (ImGui::Button("Restore Originals")) {
				auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();
				patcher->Restore();
				g_config.general.enabled = false;
				QueueApply();
			}
		}
	}

	namespace MultipliersTab
	{
		void Render()
		{
			ImGui::SliderFloat("Walk", &g_config.multipliers.walk, 0.1f, 3.0f, "%.2f");
			ImGui::SliderFloat("Run", &g_config.multipliers.run, 0.1f, 3.0f, "%.2f");
			ImGui::SliderFloat("Sprint", &g_config.multipliers.sprint, 0.1f, 3.0f, "%.2f");
			ImGui::SliderFloat("Sneak Walk", &g_config.multipliers.sneakWalk, 0.1f, 3.0f, "%.2f");
			ImGui::SliderFloat("Sneak Run", &g_config.multipliers.sneakRun, 0.1f, 3.0f, "%.2f");
			ImGui::TextDisabled("v1: same category controls both [i][0] and [i][1] variants.");
		}
	}

	namespace OverridesTab
	{
		void Render()
		{
			ImGui::Checkbox("Enable Walk Override", &g_config.overrides.walk.enabled);
			ImGui::SliderFloat("Walk Value", &g_config.overrides.walk.value, 0.0f, 2000.0f, "%.1f");
			ImGui::Checkbox("Enable Run Override", &g_config.overrides.run.enabled);
			ImGui::SliderFloat("Run Value", &g_config.overrides.run.value, 0.0f, 2000.0f, "%.1f");
			ImGui::Checkbox("Enable Sprint Override", &g_config.overrides.sprint.enabled);
			ImGui::SliderFloat("Sprint Value", &g_config.overrides.sprint.value, 0.0f, 2000.0f, "%.1f");
			ImGui::Checkbox("Enable Sneak Walk Override", &g_config.overrides.sneakWalk.enabled);
			ImGui::SliderFloat("Sneak Walk Value", &g_config.overrides.sneakWalk.value, 0.0f, 2000.0f, "%.1f");
			ImGui::Checkbox("Enable Sneak Run Override", &g_config.overrides.sneakRun.enabled);
			ImGui::SliderFloat("Sneak Run Value", &g_config.overrides.sneakRun.value, 0.0f, 2000.0f, "%.1f");
			ImGui::TextDisabled("Enabled overrides win when 'Use overrides' is enabled in General.");
		}
	}

	namespace TargetsTab
	{
		void Render()
		{
			auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();

			std::size_t removeIndex = static_cast<std::size_t>(-1);
			for (std::size_t i = 0; i < g_config.targets.size(); ++i) {
				auto& target = g_config.targets[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();
				ImGui::Checkbox("##enabled", &target.enabled);
				ImGui::SameLine();

				const auto preview = patcher->PreviewTarget(target);
				const std::string label = preview.resolved ?
					std::format("{} [{}]", preview.displayName, preview.pluginAndForm) :
					std::format("{} [unresolved]", target.form);
				ImGui::TextUnformatted(label.c_str());
				if (ImGui::Button("Remove")) {
					removeIndex = i;
				}
				ImGui::PopID();
			}

			if (removeIndex != static_cast<std::size_t>(-1) && removeIndex < g_config.targets.size()) {
				g_config.targets.erase(g_config.targets.begin() + static_cast<std::ptrdiff_t>(removeIndex));
			}

			ImGui::Separator();
			ImGui::InputText("Plugin|0xFormID", g_formInput, sizeof(g_formInput));
			ImGui::InputText("Display Name (Optional)", g_nameInput, sizeof(g_nameInput));
			if (ImGui::Button("Add Target")) {
				MOVEMENT::TargetEntry target{};
				target.form = g_formInput;
				target.name = g_nameInput;
				target.enabled = true;
				if (!target.form.empty()) {
					g_config.targets.push_back(std::move(target));
					g_formInput[0] = '\0';
					g_nameInput[0] = '\0';
				}
			}
		}
	}

	namespace ToolsTab
	{
		void Render()
		{
			auto* patcher = MOVEMENT::MovementPatcher::GetSingleton();
			if (ImGui::Button("Dump Movement Types Now")) {
				patcher->DumpMovementTypesNow();
			}

			const std::string hint = std::format(
				"Output is written to plugin log: {}",
				patcher->GetLogPathHint());
			ImGui::TextWrapped("%s", hint.c_str());

			const auto configPath = MOVEMENT::Settings::GetSingleton()->GetConfigPathString();
			ImGui::TextWrapped("JSON: %s", configPath.c_str());
		}
	}
}
