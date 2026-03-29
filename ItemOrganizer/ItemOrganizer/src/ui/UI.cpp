#include "ui/UI.h"

#include "item/ItemOrganizerManager.h"
#include "util/LogUtil.h"

#include "RE/A/AlchemyItem.h"
#include "RE/I/IngredientItem.h"
#include "RE/T/TESAmmo.h"
#include "RE/T/TESKey.h"
#include "RE/T/TESObjectARMO.h"
#include "RE/T/TESObjectBOOK.h"
#include "RE/T/TESObjectMISC.h"
#include "RE/T/TESObjectWEAP.h"

#include <array>
#include <string_view>
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

		enum class ItemCategory
		{
			Weapons,
			Armor,
			Books,
			Potions,
			Ingredients,
			Ammo,
			Keys,
			Misc
		};

		constexpr std::array<ItemCategory, 8> kCategoryOrder{
			ItemCategory::Weapons,
			ItemCategory::Armor,
			ItemCategory::Books,
			ItemCategory::Potions,
			ItemCategory::Ingredients,
			ItemCategory::Ammo,
			ItemCategory::Keys,
			ItemCategory::Misc
		};

		constexpr std::string_view GetCategoryLabel(ItemCategory a_category)
		{
			switch (a_category) {
			case ItemCategory::Weapons:
				return "Weapons";
			case ItemCategory::Armor:
				return "Armor";
			case ItemCategory::Books:
				return "Books";
			case ItemCategory::Potions:
				return "Potions";
			case ItemCategory::Ingredients:
				return "Ingredients";
			case ItemCategory::Ammo:
				return "Ammo";
			case ItemCategory::Keys:
				return "Keys";
			case ItemCategory::Misc:
			default:
				return "Misc";
			}
		}

		ItemCategory CategorizeHiddenEntry(RE::FormID a_formID)
		{
			auto* form = RE::TESForm::LookupByID(a_formID);
			if (!form) {
				return ItemCategory::Misc;
			}
			if (form->As<RE::TESObjectWEAP>()) {
				return ItemCategory::Weapons;
			}
			if (form->As<RE::TESObjectARMO>()) {
				return ItemCategory::Armor;
			}
			if (form->As<RE::TESObjectBOOK>()) {
				return ItemCategory::Books;
			}
			if (form->As<RE::AlchemyItem>()) {
				return ItemCategory::Potions;
			}
			if (form->As<RE::IngredientItem>()) {
				return ItemCategory::Ingredients;
			}
			if (form->As<RE::TESAmmo>()) {
				return ItemCategory::Ammo;
			}
			if (form->As<RE::TESKey>()) {
				return ItemCategory::Keys;
			}
			if (form->As<RE::TESObjectMISC>()) {
				return ItemCategory::Misc;
			}
			return ItemCategory::Misc;
		}

		void DrawHiddenItemsTable(MITHRAS::ITEM_ORGANIZER::Manager* a_manager, const std::vector<MITHRAS::ITEM_ORGANIZER::ItemEntry>& a_entries)
		{
			if (a_entries.empty()) {
				ImGui::TextDisabled("No hidden entries in this category.");
				return;
			}

			std::vector<RE::FormID> toUnhide;
			if (ImGui::BeginTable("HiddenItemsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
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
				a_manager->UnhideItem(formID);
			}
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
					const auto entries = manager->GetHiddenItems();
					std::array<std::vector<MITHRAS::ITEM_ORGANIZER::ItemEntry>, kCategoryOrder.size()> categorized{};
					for (const auto& entry : entries) {
						const auto category = CategorizeHiddenEntry(entry.formID);
						categorized[static_cast<std::size_t>(category)].push_back(entry);
					}

					if (ImGui::BeginTabBar("HiddenItemCategories")) {
						for (const auto category : kCategoryOrder) {
							const auto index = static_cast<std::size_t>(category);
							const auto label = GetCategoryLabel(category);
							if (ImGui::BeginTabItem(label.data())) {
								ImGui::TextColored(ImVec4(0.90f, 0.75f, 1.00f, 1.00f), "Hidden %s", label.data());
								ImGui::Separator();
								DrawHiddenItemsTable(manager, categorized[index]);
								ImGui::EndTabItem();
							}
						}
						ImGui::EndTabBar();
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
	}
}
