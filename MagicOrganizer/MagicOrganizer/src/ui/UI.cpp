#include "ui/UI.h"

#include "magic/MagicOrganizerManager.h"
#include "util/LogUtil.h"

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
				const auto name = MITHRAS::MAGIC_ORGANIZER::Manager::GetKeyboardKeyName(code);
				if (name == "Unknown") {
					continue;
				}
				out.push_back({ code, name });
			}
			return out;
		}

		std::string StripDisplayFormID(std::string a_name)
		{
			if (a_name.size() >= 11 && a_name[a_name.size() - 1] == ']' && a_name[a_name.size() - 10] == '[' && a_name[a_name.size() - 11] == ' ') {
				a_name.erase(a_name.size() - 11);
			}
			return a_name;
		}

		template <class T, class Fn>
		void DrawHiddenTable(const char* a_tableID, const char* a_label, const std::vector<T>& a_entries, Fn&& a_unhide)
		{
			ImGui::TextColored(ImVec4(0.90f, 0.75f, 1.00f, 1.00f), "%s", a_label);
			ImGui::Separator();
			if (a_entries.empty()) {
				ImGui::TextDisabled("No hidden entries yet.");
				return;
			}

			std::vector<RE::FormID> toUnhide;
			if (ImGui::BeginTable(a_tableID, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Entry", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 34.0f);

				for (const auto& entry : a_entries) {
					ImGui::PushID(static_cast<int>(entry.formID));
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					const auto cleaned = StripDisplayFormID(entry.displayName);
					ImGui::TextUnformatted(cleaned.c_str());
					ImGui::TableSetColumnIndex(1);
					if (ImGui::Button("X")) {
						toUnhide.push_back(entry.formID);
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			for (const auto formID : toUnhide) {
				a_unhide(formID);
			}
		}

		void DrawGeneralTab(MITHRAS::MAGIC_ORGANIZER::Manager* a_manager)
		{
			auto cfg = a_manager->GetConfig();
			const auto before = cfg;
			const auto keyOptions = BuildKeyboardOptions();

			ImGui::Checkbox("Enable", &cfg.enabled);

			const auto hotkeyName = MITHRAS::MAGIC_ORGANIZER::Manager::GetKeyboardKeyName(cfg.hotkey);
			if (ImGui::BeginCombo("Hotkey", hotkeyName.c_str())) {
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
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextColored(
				ImVec4(0.90f, 0.75f, 1.00f, 1.00f),
				"Tip: Open Magic Menu, highlight any spell/power/shout/effect, then press your hotkey to hide it.");
			ImGui::PopTextWrapPos();

			if (before.enabled != cfg.enabled) {
				a_manager->SetEnabled(cfg.enabled);
			}
			if (before.hotkey != cfg.hotkey) {
				a_manager->SetHotkey(cfg.hotkey);
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("MagicOrganizer: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Magic Organizer");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("MagicOrganizer: Registered SKSE Menu Framework section");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MAGIC_ORGANIZER::Manager::GetSingleton();
			if (ImGui::BeginTabBar("MagicOrganizerTabs")) {
				if (ImGui::BeginTabItem("General")) {
					DrawGeneralTab(manager);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Spells")) {
					DrawHiddenTable("HiddenSpellsTable", "Hidden Spells", manager->GetHiddenSpells(), [&](RE::FormID id) {
						manager->UnhideSpell(id);
					});
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Powers")) {
					DrawHiddenTable("HiddenPowersTable", "Hidden Powers/Shouts", manager->GetHiddenPowersAndShouts(), [&](RE::FormID id) {
						manager->UnhidePowerOrShout(id);
					});
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Magic Effects")) {
					DrawHiddenTable("HiddenEffectsTable", "Hidden Magic Effects", manager->GetHiddenEffects(), [&](RE::FormID id) {
						manager->UnhideEffect(id);
					});
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
	}
}
