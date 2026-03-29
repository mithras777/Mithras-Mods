#include "item/ItemOrganizerManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>

#include "json/single_include/nlohmann/json.hpp"

namespace MITHRAS::ITEM_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;
		constexpr auto kHiddenItemsKey = "hidden_items";

		std::vector<RE::FormID> ReadFormIDArray(const json& a_root, const char* a_key)
		{
			std::vector<RE::FormID> out;
			if (!a_root.contains(a_key) || !a_root[a_key].is_array()) {
				return out;
			}

			for (const auto& entry : a_root[a_key]) {
				if (!entry.is_number_unsigned()) {
					continue;
				}
				const auto raw = entry.get<std::uint64_t>();
				const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
				if (formID != 0) {
					out.push_back(formID);
				}
			}
			return out;
		}

		json WriteFormIDArray(const std::vector<RE::FormID>& a_formIDs)
		{
			json out = json::array();
			for (const auto formID : a_formIDs) {
				if (formID != 0) {
					out.push_back(formID);
				}
			}
			return out;
		}
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			m_captureHotkey = false;
			m_hiddenItemFormIDs.clear();
		}
		LoadConfigFromJson();
	}

	OrganizerConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetConfig(const OrganizerConfig& a_config, bool a_save)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			NormalizeConfig();
		}
		if (a_save) {
			SaveConfigToJson();
		}
		RefreshOpenMenus();
	}

	void Manager::SetEnabled(bool a_enabled)
	{
		auto cfg = GetConfig();
		cfg.enabled = a_enabled;
		SetConfig(cfg, true);
	}

	void Manager::SetHotkey(std::uint32_t a_hotkey)
	{
		auto cfg = GetConfig();
		cfg.hotkey = a_hotkey;
		SetConfig(cfg, true);
	}

	void Manager::Tick()
	{
		// Filter-only mode has no periodic reconciliation work.
	}

	void Manager::SetCaptureHotkey(bool a_capture)
	{
		std::scoped_lock lock(m_lock);
		m_captureHotkey = a_capture;
	}

	bool Manager::IsCapturingHotkey() const
	{
		std::scoped_lock lock(m_lock);
		return m_captureHotkey;
	}

	void Manager::OnHotkeyPressed(std::uint32_t a_keyCode)
	{
		bool update = false;
		{
			std::scoped_lock lock(m_lock);
			if (!m_captureHotkey) {
				return;
			}
			m_captureHotkey = false;
			m_config.hotkey = a_keyCode;
			update = true;
		}
		if (update) {
			LOG_INFO("ItemOrganizer: Hotkey updated to {}", a_keyCode);
			SaveConfigToJson();
		}
	}

	std::string Manager::GetKeyboardKeyName(std::uint32_t a_keyCode)
	{
		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		if (!inputMgr) {
			return "Unknown";
		}
		RE::BSFixedString name{};
		if (inputMgr->GetButtonNameFromID(RE::INPUT_DEVICE::kKeyboard, static_cast<std::int32_t>(a_keyCode), name)) {
			if (const auto* c = name.c_str(); c && c[0] != '\0') {
				return c;
			}
		}
		return std::format("Key {}", a_keyCode);
	}

	std::vector<ItemEntry> Manager::GetHiddenItems() const
	{
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenItemFormIDs;
		}
		std::vector<ItemEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto formID : hiddenFormIDs) {
			auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(formID);
			if (item) {
				hidden.push_back({ formID, BuildItemDisplayName(item) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}
		return hidden;
	}

	bool Manager::HideItem(RE::FormID a_formID)
	{
		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_formID);
		if (!item) {
			return false;
		}

		bool changed = false;
		{
			std::scoped_lock lock(m_lock);
			if (!IsFormIDHidden(m_hiddenItemFormIDs, a_formID)) {
				m_hiddenItemFormIDs.push_back(a_formID);
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}

		SaveConfigToJson();
		RefreshOpenMenus();
		return true;
	}

	bool Manager::UnhideItem(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenItemFormIDs;
			auto it = std::remove(hidden.begin(), hidden.end(), a_formID);
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}

		SaveConfigToJson();
		RefreshOpenMenus();
		return true;
	}

	bool Manager::HideCurrentlySelectedMenuEntry()
	{
		auto tryHideFrom = [&](RE::ItemList* a_itemList, const char* a_menuName) {
			if (!a_itemList) {
				return false;
			}

			auto* selected = a_itemList->GetSelectedItem();
			if (!selected) {
				selected = ResolveSelectedItemFallback(a_itemList);
			}
			const auto formID = ResolveItemFormID(selected);
			if (formID != 0) {
				return HideItem(formID);
			}

			LOG_INFO("ItemOrganizer: Hotkey pressed in {} but no hideable selected entry was resolved", a_menuName);
			return false;
		};

		auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			return false;
		}
		if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
			if (auto menu = ui->GetMenu<RE::InventoryMenu>()) {
				return tryHideFrom(menu->GetRuntimeData().itemList, RE::InventoryMenu::MENU_NAME.data());
			}
		}
		if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
			if (auto menu = ui->GetMenu<RE::ContainerMenu>()) {
				return tryHideFrom(menu->GetRuntimeData().itemList, RE::ContainerMenu::MENU_NAME.data());
			}
		}
		if (ui->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
			if (auto menu = ui->GetMenu<RE::BarterMenu>()) {
				return tryHideFrom(menu->GetRuntimeData().itemList, RE::BarterMenu::MENU_NAME.data());
			}
		}
		if (ui->IsMenuOpen(RE::GiftMenu::MENU_NAME)) {
			if (auto menu = ui->GetMenu<RE::GiftMenu>()) {
				return tryHideFrom(menu->GetRuntimeData().itemList, RE::GiftMenu::MENU_NAME.data());
			}
		}

		LOG_INFO("ItemOrganizer: Hide request ignored (no supported item menu open)");
		return false;
	}

	bool Manager::IsItemHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}
		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenItemFormIDs, a_formID);
	}

	void Manager::ApplyFilter(RE::ItemList* a_itemList) const
	{
		if (!a_itemList) {
			return;
		}

		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenItems;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenItems = m_hiddenItemFormIDs;
		}
		if (!cfg.enabled || hiddenItems.empty()) {
			return;
		}

		auto& items = a_itemList->items;
		for (std::uint32_t i = items.size(); i > 0; --i) {
			const auto idx = i - 1;
			const auto* item = items[idx];
			const auto formID = ResolveItemFormID(item);
			if (formID != 0 && IsFormIDHidden(hiddenItems, formID)) {
				items.erase(items.begin() + idx);
			}
		}

		auto& entryList = a_itemList->entryList;
		const auto arraySize = entryList.GetArraySize();
		for (std::uint32_t i = arraySize; i > 0; --i) {
			const auto idx = i - 1;
			RE::GFxValue entry{};
			if (!entryList.GetElement(idx, &entry) || !entry.IsObject()) {
				continue;
			}
			RE::GFxValue formIdVal{};
			if (!entry.GetMember("formId", &formIdVal) || !formIdVal.IsNumber()) {
				continue;
			}
			const auto raw = static_cast<std::uint64_t>(formIdVal.GetNumber());
			const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
			if (formID != 0 && IsFormIDHidden(hiddenItems, formID)) {
				entryList.RemoveElements(idx, 1);
			}
		}
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenItems;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenItems = m_hiddenItemFormIDs;
		}

		json root = {
			{ "enabled", cfg.enabled },
			{ "hotkey", cfg.hotkey },
			{ kHiddenItemsKey, WriteFormIDArray(hiddenItems) }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("ItemOrganizer: Failed to write config {}", path.string());
			return;
		}
		file << root.dump(2);
	}

	void Manager::LoadConfigFromJson()
	{
		const auto path = GetConfigPath();
		if (!std::filesystem::exists(path)) {
			SaveConfigToJson();
			return;
		}

		std::ifstream file(path);
		if (!file.is_open()) {
			LOG_WARN("ItemOrganizer: Failed to read config {}", path.string());
			return;
		}

		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("ItemOrganizer: Config parse failed ({}), rewriting defaults", e.what());
			SaveConfigToJson();
			return;
		}

		OrganizerConfig cfg{};
		std::vector<RE::FormID> loadedItems;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		cfg.enabled = parsed.value("enabled", cfg.enabled);
		cfg.hotkey = parsed.value("hotkey", cfg.hotkey);
		loadedItems = ReadFormIDArray(parsed, kHiddenItemsKey);

		{
			std::scoped_lock lock(m_lock);
			m_config = cfg;
			m_hiddenItemFormIDs = std::move(loadedItems);
			NormalizeConfig();
		}
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"ItemOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "ItemOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		m_hiddenItemFormIDs.erase(
			std::remove_if(m_hiddenItemFormIDs.begin(), m_hiddenItemFormIDs.end(), [](const RE::FormID id) { return id == 0; }),
			m_hiddenItemFormIDs.end());
		std::sort(m_hiddenItemFormIDs.begin(), m_hiddenItemFormIDs.end());
		m_hiddenItemFormIDs.erase(std::unique(m_hiddenItemFormIDs.begin(), m_hiddenItemFormIDs.end()), m_hiddenItemFormIDs.end());
	}

	void Manager::RefreshOpenMenus() const
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			return;
		}

		auto refreshMenu = [&](auto a_menu) {
			if (!a_menu) {
				return;
			}
			auto& runtime = a_menu->GetRuntimeData();
			if (!runtime.itemList) {
				return;
			}
			runtime.itemList->Update();
			ApplyFilter(runtime.itemList);
		};

		if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
			refreshMenu(ui->GetMenu<RE::InventoryMenu>());
		}
		if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
			refreshMenu(ui->GetMenu<RE::ContainerMenu>());
		}
		if (ui->IsMenuOpen(RE::BarterMenu::MENU_NAME)) {
			refreshMenu(ui->GetMenu<RE::BarterMenu>());
		}
		if (ui->IsMenuOpen(RE::GiftMenu::MENU_NAME)) {
			refreshMenu(ui->GetMenu<RE::GiftMenu>());
		}
	}

	bool Manager::IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID)
	{
		return std::find(a_hidden.begin(), a_hidden.end(), a_formID) != a_hidden.end();
	}

	std::string Manager::BuildItemDisplayName(const RE::TESBoundObject* a_item)
	{
		if (!a_item) {
			return "<None>";
		}
		const char* name = a_item->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_item->GetFormID());
		}
		const char* editorID = a_item->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_item->GetFormID());
		}
		return std::format("<{:08X}>", a_item->GetFormID());
	}

	RE::FormID Manager::ResolveItemFormID(const RE::ItemList::Item* a_item)
	{
		if (!a_item || !a_item->data.objDesc) {
			return 0;
		}
		const auto* obj = a_item->data.objDesc->object;
		return obj ? obj->GetFormID() : 0;
	}

	RE::ItemList::Item* Manager::ResolveSelectedItemFallback(RE::ItemList* a_itemList)
	{
		if (!a_itemList) {
			return nullptr;
		}
		const auto count = a_itemList->items.size();
		if (count == 0) {
			return nullptr;
		}

		constexpr std::array<std::string_view, 5> kIndexKeys{ "selectedIndex", "_selectedIndex", "itemIndex", "index", "selectedEntry" };
		RE::GFxValue idxValue{};
		for (const auto key : kIndexKeys) {
			if (a_itemList->entryList.GetMember(key.data(), &idxValue) && idxValue.IsNumber()) {
				const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
				if (index >= 0 && static_cast<std::size_t>(index) < count) {
					return a_itemList->items[static_cast<std::size_t>(index)];
				}
			}
		}
		for (const auto key : kIndexKeys) {
			if (a_itemList->root.GetMember(key.data(), &idxValue) && idxValue.IsNumber()) {
				const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
				if (index >= 0 && static_cast<std::size_t>(index) < count) {
					return a_itemList->items[static_cast<std::size_t>(index)];
				}
			}
		}
		return nullptr;
	}
}
