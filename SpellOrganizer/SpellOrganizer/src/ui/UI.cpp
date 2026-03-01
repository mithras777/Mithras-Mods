#include "ui/UI.h"

#include "spell/SpellManager.h"
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
				const auto name = MITHRAS::SPELL_ORGANIZER::Manager::GetKeyboardKeyName(code);
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

		void DrawGeneralTab(MITHRAS::SPELL_ORGANIZER::Manager* a_manager)
		{
			auto cfg = a_manager->GetConfig();
			const auto before = cfg;
			const auto keyOptions = BuildKeyboardOptions();

			ImGui::Checkbox("Enable", &cfg.enabled);

			const auto hotkeyName = MITHRAS::SPELL_ORGANIZER::Manager::GetKeyboardKeyName(cfg.hotkey);
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
				"Tip: Open Magic Menu, highlight a spell, then press your hotkey to hide it.");
			ImGui::PopTextWrapPos();

			if (before.enabled != cfg.enabled) {
				a_manager->SetEnabled(cfg.enabled);
			}
			if (before.hotkey != cfg.hotkey) {
				a_manager->SetHotkey(cfg.hotkey);
			}
		}

		void DrawOrganizerTab(MITHRAS::SPELL_ORGANIZER::Manager* a_manager)
		{
			const auto hiddenSpells = a_manager->GetHiddenSpells();
			ImGui::TextColored(ImVec4(0.90f, 0.75f, 1.00f, 1.00f), "Hidden Spells List");
			ImGui::Separator();
			if (hiddenSpells.empty()) {
				ImGui::TextDisabled("No hidden spells yet.");
				return;
			}

			std::vector<RE::FormID> toUnhide;
			if (ImGui::BeginTable("HiddenSpellsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 34.0f);

				for (const auto& entry : hiddenSpells) {
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
				a_manager->UnhideSpell(formID);
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SpellOrganizer: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Spell Organizer");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("SpellOrganizer: Registered SKSE Menu Framework section");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::SPELL_ORGANIZER::Manager::GetSingleton();
			if (ImGui::BeginTabBar("SpellOrganizerTabs")) {
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
