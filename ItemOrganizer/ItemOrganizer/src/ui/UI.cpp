#include "ui/UI.h"

#include "item/ItemOrganizerManager.h"
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
				const auto name = MITHRAS::ITEM_ORGANIZER::Manager::GetKeyboardKeyName(code);
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

		void DrawGeneralTab(MITHRAS::ITEM_ORGANIZER::Manager* a_manager)
		{
			auto cfg = a_manager->GetConfig();
			const auto before = cfg;
			const auto keyOptions = BuildKeyboardOptions();

			ImGui::Checkbox("Enable", &cfg.enabled);

			const auto hotkeyName = MITHRAS::ITEM_ORGANIZER::Manager::GetKeyboardKeyName(cfg.hotkey);
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
			ImGui::TextDisabled("Hidden items are stored in ItemOrganizer.json and shared across all playthroughs.");
			ImGui::TextColored(
				ImVec4(0.90f, 0.75f, 1.00f, 1.00f),
				"Tip: Open any supported item menu, highlight an entry, then press your hotkey to hide it. Hiding is UI-only.");
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
			LOG_WARN("ItemOrganizer: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Item Organizer");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("ItemOrganizer: Registered SKSE Menu Framework section");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::ITEM_ORGANIZER::Manager::GetSingleton();
			if (ImGui::BeginTabBar("ItemOrganizerTabs")) {
				if (ImGui::BeginTabItem("General")) {
					DrawGeneralTab(manager);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Items")) {
					ImGui::TextColored(ImVec4(0.90f, 0.75f, 1.00f, 1.00f), "%s", "Hidden Items");
					ImGui::Separator();

					const auto entries = manager->GetHiddenItems();
					if (entries.empty()) {
						ImGui::TextDisabled("No hidden entries yet.");
					} else {
						std::vector<RE::FormID> toUnhide;
						if (ImGui::BeginTable("HiddenItemsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
							ImGui::TableSetupColumn("Entry", ImGuiTableColumnFlags_WidthStretch);
							ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 34.0f);

							for (const auto& entry : entries) {
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
							manager->UnhideItem(formID);
						}
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
	}
}
