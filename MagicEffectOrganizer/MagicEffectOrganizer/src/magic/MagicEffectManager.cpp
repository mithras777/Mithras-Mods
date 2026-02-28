#include "magic/MagicEffectManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <format>
#include <optional>
#include <set>
#include "json/single_include/nlohmann/json.hpp"

namespace MITHRAS::MAGIC_EFFECT_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;

		constexpr RE::EffectSetting::EffectSettingData::Flag kHideFlag = RE::EffectSetting::EffectSettingData::Flag::kHideInUI;

		std::vector<RE::EffectSetting*> BuildPlayerActiveEffectBases()
		{
			std::vector<RE::EffectSetting*> out;
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* target = player ? player->GetMagicTarget() : nullptr;
			auto* activeList = target ? target->GetActiveEffectList() : nullptr;
			if (!activeList) {
				return out;
			}

			std::set<RE::FormID> seen;
			for (auto* activeEffect : *activeList) {
				if (!activeEffect) {
					continue;
				}

				auto* base = activeEffect->GetBaseObject();
				if (!base || base->IsDeleted()) {
					continue;
				}

				const auto formID = base->GetFormID();
				if (formID == 0 || seen.contains(formID)) {
					continue;
				}

				seen.insert(formID);
				out.push_back(base);
			}
			return out;
		}

		bool IsActiveEffectFormID(const std::vector<RE::EffectSetting*>& a_activeBases, RE::FormID a_formID)
		{
			for (const auto* base : a_activeBases) {
				if (base && base->GetFormID() == a_formID) {
					return true;
				}
			}
			return false;
		}

		std::optional<RE::FormID> ResolveSelectedActiveEffectFormID(RE::MagicItemList::Item* a_selected)
		{
			if (!a_selected) {
				return std::nullopt;
			}

			const auto activeBases = BuildPlayerActiveEffectBases();
			if (activeBases.empty()) {
				return std::nullopt;
			}

			if (auto* selectedEffect = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::EffectSetting>() : nullptr) {
				const auto selectedFormID = selectedEffect->GetFormID();
				if (selectedFormID != 0 && IsActiveEffectFormID(activeBases, selectedFormID)) {
					return selectedFormID;
				}
			}

			if (const auto* selectedMagicItem = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::MagicItem>() : nullptr) {
				for (const auto* itemEffect : selectedMagicItem->effects) {
					if (!itemEffect || !itemEffect->baseEffect) {
						continue;
					}
					const auto formID = itemEffect->baseEffect->GetFormID();
					if (formID != 0 && IsActiveEffectFormID(activeBases, formID)) {
						return formID;
					}
				}
			}

			const char* selectedName = a_selected->data.GetName();
			if (!selectedName || selectedName[0] == '\0') {
				return std::nullopt;
			}

			for (const auto* base : activeBases) {
				if (!base) {
					continue;
				}
				const char* baseName = base->GetName();
				if (baseName && _stricmp(baseName, selectedName) == 0) {
					return base->GetFormID();
				}
			}

			return std::nullopt;
		}

		RE::MagicItemList::Item* ResolveSelectedItemFallback(RE::MagicItemList* a_itemList)
		{
			if (!a_itemList) {
				return nullptr;
			}

			const auto count = a_itemList->items.size();
			if (count == 0) {
				return nullptr;
			}

			constexpr std::array<std::string_view, 5> kIndexKeys{
				"selectedIndex",
				"_selectedIndex",
				"itemIndex",
				"index",
				"selectedEntry"
			};

			RE::GFxValue idxValue{};
			for (const auto key : kIndexKeys) {
				if (!a_itemList->entryList.GetMember(key.data(), &idxValue) || !idxValue.IsNumber()) {
					continue;
				}

				const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
				if (index >= 0 && static_cast<std::size_t>(index) < count) {
					return a_itemList->items[static_cast<std::size_t>(index)];
				}
			}

			for (const auto key : kIndexKeys) {
				if (!a_itemList->root.GetMember(key.data(), &idxValue) || !idxValue.IsNumber()) {
					continue;
				}

				const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
				if (index >= 0 && static_cast<std::size_t>(index) < count) {
					return a_itemList->items[static_cast<std::size_t>(index)];
				}
			}

			return nullptr;
		}
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			m_captureHotkey = false;
		}

		LoadConfigFromJson();
		ApplyTrackedFlags();
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

		ApplyTrackedFlags();
		if (a_save) {
			SaveConfigToJson();
		}
	}

	void Manager::SetEnabled(bool a_enabled)
	{
		OrganizerConfig cfg = GetConfig();
		cfg.enabled = a_enabled;
		SetConfig(cfg, true);
	}

	void Manager::SetHotkey(std::uint32_t a_hotkey)
	{
		OrganizerConfig cfg = GetConfig();
		cfg.hotkey = a_hotkey;
		SetConfig(cfg, true);
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
			LOG_INFO("MagicEffectOrganizer: Hotkey updated to {}", a_keyCode);
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

	std::vector<MagicEffectEntry> Manager::GetVisibleEffects() const
	{
		OrganizerConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		auto all = BuildPlayerActiveEffects();
		std::vector<MagicEffectEntry> visible;
		visible.reserve(all.size());
		for (const auto& entry : all) {
			if (!IsFormIDHidden(cfg.hiddenEffectFormIDs, entry.formID)) {
				visible.push_back(entry);
			}
		}
		return visible;
	}

	std::vector<MagicEffectEntry> Manager::GetHiddenEffects() const
	{
		OrganizerConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		std::vector<MagicEffectEntry> hidden;
		hidden.reserve(cfg.hiddenEffectFormIDs.size());

		for (const auto formID : cfg.hiddenEffectFormIDs) {
			auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID);
			if (effect) {
				hidden.push_back({ formID, BuildEffectDisplayName(effect) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}

		return hidden;
	}

	bool Manager::HideEffect(RE::FormID a_formID)
	{
		if (a_formID == 0) {
			return false;
		}

		bool changed = false;
		{
			std::scoped_lock lock(m_lock);
			if (!IsFormIDHidden(m_config.hiddenEffectFormIDs, a_formID)) {
				m_config.hiddenEffectFormIDs.push_back(a_formID);
				changed = true;
			}
		}

		if (!changed) {
			return false;
		}

		ApplyTrackedFlags();
		SaveConfigToJson();
		return true;
	}

	bool Manager::UnhideEffect(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_config.hiddenEffectFormIDs;
			auto it = std::remove(hidden.begin(), hidden.end(), a_formID);
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}

		if (!removed) {
			return false;
		}

		if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(a_formID)) {
			effect->data.flags.reset(kHideFlag);
		}

		RefreshMagicMenuIfOpen();
		SaveConfigToJson();
		return true;
	}

	bool Manager::HideCurrentlySelectedMagicMenuEffect()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			LOG_INFO("MagicEffectOrganizer: Hide request ignored (MagicMenu not open)");
			return false;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			LOG_INFO("MagicEffectOrganizer: Hide request failed (MagicMenu handle unavailable)");
			return false;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			LOG_INFO("MagicEffectOrganizer: Hide request failed (MagicMenu itemList unavailable)");
			return false;
		}

		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			runtime.itemList->Update(player);
		} else {
			runtime.itemList->Update();
		}

		auto* selected = ResolveSelectedItemFallback(runtime.itemList);
		if (!selected) {
			selected = runtime.itemList->GetSelectedItem();
		}
		if (!selected) {
			LOG_INFO("MagicEffectOrganizer: Hide request failed (no selected item in MagicMenu list; itemCount={})", runtime.itemList->items.size());
			return false;
		}

		const auto selectedFormID = ResolveSelectedActiveEffectFormID(selected);
		if (!selectedFormID.has_value()) {
			const char* selectedName = selected->data.GetName();
			const auto filter = selected->data.GetFilterFlag();
			const auto* baseForm = selected->data.baseForm;
			LOG_INFO(
				"MagicEffectOrganizer: Hotkey resolve miss (name='{}', filter={}, baseForm={:08X}, type={})",
				selectedName ? selectedName : "",
				filter,
				baseForm ? baseForm->GetFormID() : 0,
				baseForm ? static_cast<std::uint32_t>(baseForm->GetFormType()) : 0U);
			return false;
		}

		auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(*selectedFormID);
		if (!effect) {
			LOG_INFO("MagicEffectOrganizer: Hide request failed (resolved form {:08X} was not an EffectSetting)", *selectedFormID);
			return false;
		}

		if (HideEffect(*selectedFormID)) {
			LOG_INFO("MagicEffectOrganizer: Hidden magic effect {}", BuildEffectDisplayName(effect));
			return true;
		}
		LOG_INFO("MagicEffectOrganizer: Hide request ignored (effect {:08X} already hidden)", *selectedFormID);
		return false;
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		OrganizerConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		json root = {
			{ "enabled", cfg.enabled },
			{ "hotkey", cfg.hotkey },
			{ "hiddenEffectFormIDs", cfg.hiddenEffectFormIDs }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("MagicEffectOrganizer: Failed to write config {}", path.string());
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
			LOG_WARN("MagicEffectOrganizer: Failed to read config {}", path.string());
			return;
		}

		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("MagicEffectOrganizer: Config parse failed ({}), rewriting defaults", e.what());
			SaveConfigToJson();
			return;
		}

		OrganizerConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		cfg.enabled = parsed.value("enabled", cfg.enabled);
		cfg.hotkey = parsed.value("hotkey", cfg.hotkey);
		cfg.hiddenEffectFormIDs = parsed.value("hiddenEffectFormIDs", cfg.hiddenEffectFormIDs);

		{
			std::scoped_lock lock(m_lock);
			m_config = cfg;
			NormalizeConfig();
		}
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"MagicEffectOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "MagicEffectOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		auto& hidden = m_config.hiddenEffectFormIDs;
		hidden.erase(
			std::remove_if(hidden.begin(), hidden.end(), [](const RE::FormID id) { return id == 0; }),
			hidden.end());
		std::sort(hidden.begin(), hidden.end());
		hidden.erase(std::unique(hidden.begin(), hidden.end()), hidden.end());
	}

	void Manager::ApplyTrackedFlags()
	{
		OrganizerConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		for (const auto formID : cfg.hiddenEffectFormIDs) {
			auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID);
			if (!effect) {
				continue;
			}

			effect->data.flags.set(cfg.enabled, kHideFlag);
		}

		RefreshMagicMenuIfOpen();
	}

	void Manager::RefreshMagicMenuIfOpen() const
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			return;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		runtime.itemList->Update(player);
	}

	std::string Manager::BuildEffectDisplayName(const RE::EffectSetting* a_effect)
	{
		if (!a_effect) {
			return "<None>";
		}

		const char* name = a_effect->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_effect->GetFormID());
		}

		const char* editorID = a_effect->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_effect->GetFormID());
		}

		return std::format("<{:08X}>", a_effect->GetFormID());
	}

	std::vector<MagicEffectEntry> Manager::BuildPlayerActiveEffects()
	{
		std::vector<MagicEffectEntry> out;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return out;
		}

		auto* target = player->GetMagicTarget();
		if (!target) {
			return out;
		}

		auto* activeList = target->GetActiveEffectList();
		if (!activeList) {
			return out;
		}

		std::set<RE::FormID> uniqueIDs;
		for (auto* activeEffect : *activeList) {
			if (!activeEffect) {
				continue;
			}

			auto* effect = activeEffect->GetBaseObject();
			if (!effect || effect->IsDeleted()) {
				continue;
			}

			const auto formID = effect->GetFormID();
			if (formID == 0 || uniqueIDs.contains(formID)) {
				continue;
			}

			uniqueIDs.insert(formID);
			out.push_back({ formID, BuildEffectDisplayName(effect) });
		}

		std::sort(out.begin(), out.end(), [](const MagicEffectEntry& a_lhs, const MagicEffectEntry& a_rhs) {
			return _stricmp(a_lhs.displayName.c_str(), a_rhs.displayName.c_str()) < 0;
		});
		return out;
	}

	bool Manager::IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID)
	{
		return std::find(a_hidden.begin(), a_hidden.end(), a_formID) != a_hidden.end();
	}
}
