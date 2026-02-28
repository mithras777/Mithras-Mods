#include "ui/UI.h"

#include "magic/MagicEffectManager.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <format>
#include <vector>

namespace UI
{
	namespace
	{
		struct KeyOption
		{
			std::uint32_t code{ 0 };
			std::string name{};
		};

		std::vector<KeyOption> BuildKeyboardOptions()
		{
			std::vector<KeyOption> out;
			out.reserve(256);
			for (std::uint32_t code = 1; code < 256; ++code) {
				const auto name = MITHRAS::MAGIC_EFFECT_ORGANIZER::Manager::GetKeyboardKeyName(code);
				if (name == "Unknown") {
					continue;
				}
				out.push_back({ code, name });
			}
			return out;
		}

		void DrawGeneralTab(MITHRAS::MAGIC_EFFECT_ORGANIZER::Manager* a_manager)
		{
			auto cfg = a_manager->GetConfig();
			const auto before = cfg;
			const auto keyOptions = BuildKeyboardOptions();

			ImGui::Checkbox("Enable hidden effect organizer", &cfg.enabled);

			const auto hotkeyName = MITHRAS::MAGIC_EFFECT_ORGANIZER::Manager::GetKeyboardKeyName(cfg.hotkey);
			if (ImGui::BeginCombo("Magic menu hide hotkey", hotkeyName.c_str())) {
				for (const auto& option : keyOptions) {
					const bool selected = (option.code == cfg.hotkey);
					if (ImGui::Selectable(option.name.c_str(), selected)) {
						cfg.hotkey = option.code;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();
			ImGui::TextDisabled("Tip: Open Magic Menu, highlight an effect, then press your hotkey to hide it.");

			if (before.enabled != cfg.enabled) {
				a_manager->SetEnabled(cfg.enabled);
			}
			if (before.hotkey != cfg.hotkey) {
				a_manager->SetHotkey(cfg.hotkey);
			}
		}

		void DrawOrganizerTab(MITHRAS::MAGIC_EFFECT_ORGANIZER::Manager* a_manager)
		{
			const auto visibleEffects = a_manager->GetVisibleEffects();
			const auto hiddenEffects = a_manager->GetHiddenEffects();

			static RE::FormID selectedVisibleFormID = 0;
			if (selectedVisibleFormID == 0 ||
				std::none_of(visibleEffects.begin(), visibleEffects.end(), [&](const auto& e) { return e.formID == selectedVisibleFormID; })) {
				selectedVisibleFormID = visibleEffects.empty() ? 0 : visibleEffects.front().formID;
			}

			const auto selectedIt = std::find_if(
				visibleEffects.begin(),
				visibleEffects.end(),
				[&](const auto& e) { return e.formID == selectedVisibleFormID; });
			const char* selectedLabel = (selectedIt != visibleEffects.end()) ? selectedIt->displayName.c_str() : "<None>";

			if (ImGui::BeginCombo("Magic Effects", selectedLabel)) {
				for (const auto& entry : visibleEffects) {
					const bool selected = (entry.formID == selectedVisibleFormID);
					if (ImGui::Selectable(entry.displayName.c_str(), selected)) {
						selectedVisibleFormID = entry.formID;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			const bool canHide = selectedVisibleFormID != 0;
			if (!canHide) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Hide selected effect") && canHide) {
				a_manager->HideEffect(selectedVisibleFormID);
			}
			if (!canHide) {
				ImGui::EndDisabled();
			}

			ImGui::SeparatorText("Hidden Effects");
			if (hiddenEffects.empty()) {
				ImGui::TextDisabled("No hidden effects yet.");
				return;
			}

			if (ImGui::BeginTable("HiddenMagicEffectsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Effect", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 48.0f);
				ImGui::TableHeadersRow();

				std::vector<RE::FormID> toUnhide;
				for (const auto& entry : hiddenEffects) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(entry.displayName.c_str());

					ImGui::TableSetColumnIndex(1);
					const auto buttonId = std::format("X##{:08X}", entry.formID);
					if (ImGui::Button(buttonId.c_str())) {
						toUnhide.push_back(entry.formID);
					}
				}

				ImGui::EndTable();

				for (const auto formID : toUnhide) {
					a_manager->UnhideEffect(formID);
				}
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("MagicEffectOrganizer: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Magic Effect Organizer");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("MagicEffectOrganizer: Registered SKSE Menu Framework section");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MAGIC_EFFECT_ORGANIZER::Manager::GetSingleton();
			if (ImGui::BeginTabBar("MagicEffectOrganizerTabs")) {
				if (ImGui::BeginTabItem("General")) {
					DrawGeneralTab(manager);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Organizer")) {
					DrawOrganizerTab(manager);
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
	}
}
