#include "ui/MCM_Menu.h"

#include "movement/MovementPatcher.h"
#include "movement/Settings.h"
#include "util/LogUtil.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace UI::MCM
{
	namespace
	{
		MOVEMENT::SettingsData g_config{};
		MOVEMENT::SettingsData g_before{};
		MOVEMENT::SettingsData g_defaults{};
		bool g_dirty{ false };

		constexpr std::array<const char*, 4> kDirections{
			"Left",
			"Right",
			"Forward",
			"Back"
		};

		std::string PrettyName(std::string_view a_name)
		{
			auto replaceAll = [](std::string& a_text, std::string_view a_from, std::string_view a_to) {
				std::size_t pos = 0;
				while ((pos = a_text.find(a_from, pos)) != std::string::npos) {
					a_text.replace(pos, a_from.size(), a_to);
					pos += a_to.size();
				}
			};

			std::string out(a_name);
			if (out.rfind("NPC_", 0) == 0) {
				out.erase(0, 4);
			} else if (out.rfind("Horse_", 0) == 0) {
				out.erase(0, 6);
			}

			if (out.size() > 3 && out.compare(out.size() - 3, 3, "_MT") == 0) {
				out.erase(out.size() - 3);
			}

			replaceAll(out, "1HM", "One Handed");
			replaceAll(out, "2HM", "Two Handed");
			replaceAll(out, "BowDrawn", "Bow Drawn");
			replaceAll(out, "PowerAttacking", "Power Attacking");
			replaceAll(out, "MagicCasting", "Magic Casting");
			replaceAll(out, "ShieldCharge", "Shield Charge");
			if (out == "Bow") {
				out = "Bow Not Drawn";
			}
			if (out == "Magic") {
				out = "Magic Not Casting";
			}

			std::replace(out.begin(), out.end(), '_', ' ');
			return out;
		}

		bool IsSprintEntry(std::string_view a_name)
		{
			return a_name.find("Sprinting") != std::string_view::npos ||
			       a_name.find("Sprint") != std::string_view::npos;
		}

		void EnforceLateralToForward(MOVEMENT::MovementEntry& a_entry)
		{
			a_entry.speeds[2][1] = std::max({ a_entry.speeds[2][1], a_entry.speeds[0][1], a_entry.speeds[1][1] });
		}

		std::string CanonicalForm(std::string_view a_form)
		{
			std::string out(a_form);
			std::transform(
				out.begin(),
				out.end(),
				out.begin(),
				[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
			return out;
		}

		const MOVEMENT::MovementEntry* FindDefaultEntry(std::string_view a_form)
		{
			const auto key = CanonicalForm(a_form);
			for (const auto& entry : g_defaults.entries) {
				if (CanonicalForm(entry.form) == key) {
					return &entry;
				}
			}
			return nullptr;
		}

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
				for (std::size_t direction = 0; direction < 5; ++direction) {
					if (l.speeds[direction][0] != r.speeds[direction][0] || l.speeds[direction][1] != r.speeds[direction][1]) {
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
				const auto displayName = PrettyName(entry.name);
				if (ImGui::TreeNode(displayName.c_str())) {
					ImGui::Checkbox("Enabled", &entry.enabled);
					ImGui::SameLine();
					if (ImGui::Button("Reset")) {
						if (const auto* defaults = FindDefaultEntry(entry.form)) {
							for (std::size_t direction = 0; direction < 5; ++direction) {
								entry.speeds[direction][0] = defaults->speeds[direction][0];
								entry.speeds[direction][1] = defaults->speeds[direction][1];
							}
						}
					}
					if (IsSprintEntry(entry.name)) {
						ImGui::SliderFloat("Sprinting", &entry.speeds[2][1], 0.0f, 2000.0f, "%.1f");
					} else if (entry.group == "Horses") {
						ImGui::SliderFloat("Forward", &entry.speeds[2][1], 0.0f, 2000.0f, "%.1f");
					} else {
						bool lateralChanged = false;
						for (std::size_t direction = 0; direction < kDirections.size(); ++direction) {
							std::string label = std::string(kDirections[direction]) + "##Run" + kDirections[direction];
							if (ImGui::SliderFloat(label.c_str(), &entry.speeds[direction][1], 0.0f, 2000.0f, "%.1f")) {
								lateralChanged = lateralChanged || direction == 0 || direction == 1;
							}
						}
						if (lateralChanged) {
							EnforceLateralToForward(entry);
						}
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

		SKSEMenuFramework::SetSection("Configurable Speeds");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* settings = MOVEMENT::Settings::GetSingleton();
			g_config = settings->Get();
			g_defaults = settings->GetDefaults();
			g_before = g_config;
			g_dirty = false;

			if (ImGui::BeginTabBar("ConfigMovementSpeedTabs")) {
				if (ImGui::BeginTabItem("General")) {
					GeneralTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Default")) {
					DefaultTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Combat")) {
					CombatTab::Render();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Horses")) {
					HorsesTab::Render();
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
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextColored(
				ImVec4(0.90f, 0.75f, 1.00f, 1.00f),
				"Tip: Speed changes apply after your movement state updates. "
				"After changing a value, briefly sprint to refresh.");
			ImGui::PopTextWrapPos();
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

	namespace HorsesTab
	{
		void Render()
		{
			RenderGroup("Horses");
		}
	}
}
