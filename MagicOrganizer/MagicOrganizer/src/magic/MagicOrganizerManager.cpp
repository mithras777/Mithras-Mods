#include "magic/MagicOrganizerManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <fstream>
#include <optional>
#include <set>

#include "json/single_include/nlohmann/json.hpp"

namespace MITHRAS::MAGIC_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kHiddenAllRecord = 'MORG';
		constexpr std::uint32_t kSerializationID = 'MGOG';
		constexpr RE::EffectSetting::EffectSettingData::Flag kHideFlag = RE::EffectSetting::EffectSettingData::Flag::kHideInUI;
		constexpr auto kGlobalHiddenListsKey = "global_hidden_lists";
		constexpr auto kHiddenSpellsKey = "hidden_spells";
		constexpr auto kHiddenPowersKey = "hidden_powers";
		constexpr auto kHiddenEffectsKey = "hidden_effects";
		constexpr auto kReconcileInterval = std::chrono::seconds(3);

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

		std::optional<RE::FormID> ResolveSelectedFormIDFromMovieView(const RE::MagicItemList* a_itemList)
		{
			if (!a_itemList || !a_itemList->view) {
				return std::nullopt;
			}
			RE::GFxValue val{};
			if (!a_itemList->view->GetVariable(&val, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId") || !val.IsNumber()) {
				return std::nullopt;
			}
			const auto raw = static_cast<std::uint64_t>(val.GetNumber());
			const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
			return formID ? std::optional<RE::FormID>(formID) : std::nullopt;
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

		RE::TESSpellList::SpellData* GetPlayerSpellList(RE::PlayerCharacter* a_player)
		{
			if (!a_player) {
				return nullptr;
			}
			auto* actorBase = a_player->GetActorBase();
			auto* npc = actorBase ? actorBase->As<RE::TESNPC>() : nullptr;
			return npc ? npc->GetSpellList() : nullptr;
		}

		bool RemovePlayerSpell(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell)
		{
			if (!a_player || !a_spell) {
				return false;
			}
			bool removed = false;
			if (a_player->HasSpell(a_spell)) {
				removed = a_player->RemoveSpell(a_spell) || removed;
			}
			if (auto* spellList = GetPlayerSpellList(a_player)) {
				removed = spellList->RemoveSpell(a_spell) || removed;
			}
			return removed;
		}

		bool AddPlayerSpell(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell)
		{
			if (!a_player || !a_spell) {
				return false;
			}
			if (a_player->HasSpell(a_spell)) {
				return true;
			}
			return a_player->AddSpell(a_spell);
		}

		bool RemovePlayerShout(RE::PlayerCharacter* a_player, RE::TESShout* a_shout)
		{
			if (!a_player || !a_shout) {
				return false;
			}
			bool removed = false;
			if (auto* spellList = GetPlayerSpellList(a_player)) {
				removed = spellList->RemoveShout(a_shout) || removed;
			}
			return removed;
		}

		bool AddPlayerShout(RE::PlayerCharacter* a_player, RE::TESShout* a_shout)
		{
			if (!a_player || !a_shout) {
				return false;
			}
			if (a_player->HasShout(a_shout)) {
				return true;
			}
			return a_player->AddShout(a_shout);
		}

		std::optional<RE::FormID> ResolveSelectedActiveEffectFormID(RE::MagicItemList::Item* a_selected)
		{
			if (!a_selected) {
				return std::nullopt;
			}

			std::vector<RE::FormID> activeBaseFormIDs;
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* target = player ? player->GetMagicTarget() : nullptr;
			auto* activeList = target ? target->GetActiveEffectList() : nullptr;
			if (!activeList) {
				return std::nullopt;
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
				activeBaseFormIDs.push_back(formID);
			}

			if (activeBaseFormIDs.empty()) {
				return std::nullopt;
			}

			auto isActive = [&](RE::FormID a_formID) {
				for (const auto formID : activeBaseFormIDs) {
					if (formID == a_formID) {
						return true;
					}
				}
				return false;
			};
			if (auto* selectedEffect = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::EffectSetting>() : nullptr) {
				const auto selectedFormID = selectedEffect->GetFormID();
				if (selectedFormID != 0 && isActive(selectedFormID)) {
					return selectedFormID;
				}
			}
			if (const auto* selectedMagicItem = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::MagicItem>() : nullptr) {
				for (const auto* itemEffect : selectedMagicItem->effects) {
					if (!itemEffect || !itemEffect->baseEffect) {
						continue;
					}
					const auto formID = itemEffect->baseEffect->GetFormID();
					if (formID != 0 && isActive(formID)) {
						return formID;
					}
				}
			}
			const char* selectedName = a_selected->data.GetName();
			if (!selectedName || selectedName[0] == '\0') {
				return std::nullopt;
			}
			for (auto* activeEffect : *activeList) {
				if (!activeEffect) {
					continue;
				}
				auto* base = activeEffect->GetBaseObject();
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

		void SaveCallback(SKSE::SerializationInterface* a_serialization)
		{
			Manager::GetSingleton()->SaveHiddenToCosave(a_serialization);
		}
		void LoadCallback(SKSE::SerializationInterface* a_serialization)
		{
			Manager::GetSingleton()->LoadHiddenFromCosave(a_serialization);
		}
		void RevertCallback(SKSE::SerializationInterface*)
		{
			Manager::GetSingleton()->RevertHiddenFromCosave();
		}
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			m_captureHotkey = false;
			m_removedByOrganizerSpells.clear();
			m_removedByOrganizerPowers.clear();
			m_removedByOrganizerShouts.clear();
			m_nextReconcileAt = std::chrono::steady_clock::now();
		}
		LoadConfigFromJson();
		ApplyTrackedFlags();
		MaybeRunReconcile(true);
	}

	void Manager::RegisterSerialization()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("MagicOrganizer: Serialization interface unavailable");
			return;
		}
		serialization->SetUniqueID(kSerializationID);
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);
		serialization->SetRevertCallback(RevertCallback);
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
		auto cfg = GetConfig();
		cfg.enabled = a_enabled;
		SetConfig(cfg, true);
		if (a_enabled) {
			MaybeRunReconcile(true);
		}
	}

	void Manager::SetHotkey(std::uint32_t a_hotkey)
	{
		auto cfg = GetConfig();
		cfg.hotkey = a_hotkey;
		SetConfig(cfg, true);
	}

	void Manager::SetGlobalHiddenLists(bool a_enabled)
	{
		OrganizerConfig cfg{};
		std::vector<RE::FormID> mergedSpells;
		std::vector<RE::FormID> mergedPowers;
		std::vector<RE::FormID> mergedEffects;
		std::size_t fromConfigSpells = 0;
		std::size_t fromConfigPowers = 0;
		std::size_t fromConfigEffects = 0;

		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			if (cfg.globalHiddenLists == a_enabled) {
				return;
			}
			mergedSpells = m_hiddenSpellFormIDs;
			mergedPowers = m_hiddenPowerShoutFormIDs;
			mergedEffects = m_hiddenEffectFormIDs;
		}

		if (a_enabled) {
			const auto path = GetConfigPath();
			if (std::ifstream file(path); file.is_open()) {
				json parsed{};
				try {
					file >> parsed;
					auto cfgSpells = ReadFormIDArray(parsed, kHiddenSpellsKey);
					auto cfgPowers = ReadFormIDArray(parsed, kHiddenPowersKey);
					auto cfgEffects = ReadFormIDArray(parsed, kHiddenEffectsKey);
					fromConfigSpells = cfgSpells.size();
					fromConfigPowers = cfgPowers.size();
					fromConfigEffects = cfgEffects.size();
					mergedSpells.insert(mergedSpells.end(), cfgSpells.begin(), cfgSpells.end());
					mergedPowers.insert(mergedPowers.end(), cfgPowers.begin(), cfgPowers.end());
					mergedEffects.insert(mergedEffects.end(), cfgEffects.begin(), cfgEffects.end());
				} catch (const std::exception& e) {
					LOG_WARN("MagicOrganizer: Failed reading existing global hidden lists ({}), using current save lists", e.what());
				}
			}
		}

		{
			std::scoped_lock lock(m_lock);
			m_config.globalHiddenLists = a_enabled;
			if (a_enabled) {
				m_hiddenSpellFormIDs = std::move(mergedSpells);
				m_hiddenPowerShoutFormIDs = std::move(mergedPowers);
				m_hiddenEffectFormIDs = std::move(mergedEffects);
				NormalizeConfig();
				LOG_INFO(
					"MagicOrganizer: Global hidden lists enabled (merged config lists: spells={}, powers={}, effects={})",
					fromConfigSpells,
					fromConfigPowers,
					fromConfigEffects);
			} else {
				LOG_INFO("MagicOrganizer: Global hidden lists disabled (cosave mode resumed)");
			}
		}

		SaveConfigToJson();
		ApplyTrackedFlags();
		if (a_enabled) {
			MaybeRunReconcile(true);
		}
	}

	void Manager::Tick()
	{
		MaybeRunReconcile(false);
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
			LOG_INFO("MagicOrganizer: Hotkey updated to {}", a_keyCode);
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

	std::vector<SpellEntry> Manager::GetHiddenSpells() const
	{
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenSpellFormIDs;
		}
		std::vector<SpellEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto formID : hiddenFormIDs) {
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
			if (spell && IsOrganizerSpell(spell)) {
				hidden.push_back({ formID, BuildSpellDisplayName(spell) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}
		return hidden;
	}

	std::vector<PowerShoutEntry> Manager::GetHiddenPowersAndShouts() const
	{
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenPowerShoutFormIDs;
		}
		std::vector<PowerShoutEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto formID : hiddenFormIDs) {
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
			if (spell && IsOrganizerPower(spell)) {
				hidden.push_back({ formID, BuildPowerShoutDisplayName(spell) });
				continue;
			}
			auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
			if (shout && IsOrganizerShout(shout)) {
				hidden.push_back({ formID, BuildPowerShoutDisplayName(shout) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}
		return hidden;
	}

	std::vector<MagicEffectEntry> Manager::GetHiddenEffects() const
	{
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenEffectFormIDs;
		}
		std::vector<MagicEffectEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto formID : hiddenFormIDs) {
			auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID);
			if (effect) {
				hidden.push_back({ formID, BuildEffectDisplayName(effect) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}
		return hidden;
	}

	bool Manager::HideSpell(RE::FormID a_formID)
	{
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		if (!spell || !IsOrganizerSpell(spell)) {
			return false;
		}
		bool changed = false;
		bool enabled = false;
		bool persistGlobal = false;
		{
			std::scoped_lock lock(m_lock);
			enabled = m_config.enabled;
			persistGlobal = m_config.globalHiddenLists;
			if (!IsFormIDHidden(m_hiddenSpellFormIDs, a_formID)) {
				m_hiddenSpellFormIDs.push_back(a_formID);
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		if (enabled) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				if (RemovePlayerSpell(player, spell)) {
					std::scoped_lock lock(m_lock);
					m_removedByOrganizerSpells.insert(a_formID);
				}
			}
		}
		if (persistGlobal) {
			SaveConfigToJson();
		}
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideSpell(RE::FormID a_formID)
	{
		bool removed = false;
		bool persistGlobal = false;
		bool restoreAllowed = false;
		{
			std::scoped_lock lock(m_lock);
			persistGlobal = m_config.globalHiddenLists;
			auto& hidden = m_hiddenSpellFormIDs;
			auto it = std::remove(hidden.begin(), hidden.end(), a_formID);
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
			restoreAllowed = m_removedByOrganizerSpells.erase(a_formID) > 0;
		}
		if (!removed) {
			return false;
		}
		if (restoreAllowed) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
			if (player && spell && IsOrganizerSpell(spell)) {
				AddPlayerSpell(player, spell);
			}
		} else {
			LOG_INFO("MagicOrganizer: Skipped restoring spell {:08X} (not removed by organizer in this playthrough)", a_formID);
		}
		if (persistGlobal) {
			SaveConfigToJson();
		}
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HidePowerOrShout(RE::FormID a_formID)
	{
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		auto* shout = RE::TESForm::LookupByID<RE::TESShout>(a_formID);
		const bool isPower = spell && IsOrganizerPower(spell);
		const bool isShout = shout && IsOrganizerShout(shout);
		if (!isPower && !isShout) {
			return false;
		}
		bool changed = false;
		bool enabled = false;
		bool persistGlobal = false;
		{
			std::scoped_lock lock(m_lock);
			enabled = m_config.enabled;
			persistGlobal = m_config.globalHiddenLists;
			if (!IsFormIDHidden(m_hiddenPowerShoutFormIDs, a_formID)) {
				m_hiddenPowerShoutFormIDs.push_back(a_formID);
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		if (enabled) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player && isPower) {
				if (RemovePlayerSpell(player, spell)) {
					std::scoped_lock lock(m_lock);
					m_removedByOrganizerPowers.insert(a_formID);
				}
			}
			if (player && isShout) {
				if (RemovePlayerShout(player, shout)) {
					std::scoped_lock lock(m_lock);
					m_removedByOrganizerShouts.insert(a_formID);
				}
			}
		}
		if (persistGlobal) {
			SaveConfigToJson();
		}
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhidePowerOrShout(RE::FormID a_formID)
	{
		bool removed = false;
		bool persistGlobal = false;
		bool restorePower = false;
		bool restoreShout = false;
		{
			std::scoped_lock lock(m_lock);
			persistGlobal = m_config.globalHiddenLists;
			auto& hidden = m_hiddenPowerShoutFormIDs;
			auto it = std::remove(hidden.begin(), hidden.end(), a_formID);
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
			restorePower = m_removedByOrganizerPowers.erase(a_formID) > 0;
			restoreShout = m_removedByOrganizerShouts.erase(a_formID) > 0;
		}
		if (!removed) {
			return false;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		auto* shout = RE::TESForm::LookupByID<RE::TESShout>(a_formID);
		if (restorePower && player && spell && IsOrganizerPower(spell)) {
			AddPlayerSpell(player, spell);
		}
		if (restoreShout && player && shout && IsOrganizerShout(shout)) {
			AddPlayerShout(player, shout);
		}
		if ((spell && IsOrganizerPower(spell) && !restorePower) || (shout && IsOrganizerShout(shout) && !restoreShout)) {
			LOG_INFO("MagicOrganizer: Skipped restoring power/shout {:08X} (not removed by organizer in this playthrough)", a_formID);
		}
		if (persistGlobal) {
			SaveConfigToJson();
		}
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideEffect(RE::FormID a_formID)
	{
		if (a_formID == 0) {
			return false;
		}
		bool changed = false;
		bool persistGlobal = false;
		{
			std::scoped_lock lock(m_lock);
			persistGlobal = m_config.globalHiddenLists;
			if (!IsFormIDHidden(m_hiddenEffectFormIDs, a_formID)) {
				m_hiddenEffectFormIDs.push_back(a_formID);
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		if (persistGlobal) {
			SaveConfigToJson();
		}
		ApplyTrackedFlags();
		return true;
	}

	bool Manager::UnhideEffect(RE::FormID a_formID)
	{
		bool removed = false;
		bool persistGlobal = false;
		{
			std::scoped_lock lock(m_lock);
			persistGlobal = m_config.globalHiddenLists;
			auto& hidden = m_hiddenEffectFormIDs;
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
		if (persistGlobal) {
			SaveConfigToJson();
		}
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideCurrentlySelectedMagicMenuEntry()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			LOG_INFO("MagicOrganizer: Hide request ignored (MagicMenu not open)");
			return false;
		}
		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return false;
		}
		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return false;
		}

		auto* selected = runtime.itemList->GetSelectedItem();
		if (!selected) {
			selected = ResolveSelectedItemFallback(runtime.itemList);
		}

		if (selected) {
			if (auto* shout = selected->data.baseForm ? selected->data.baseForm->As<RE::TESShout>() : nullptr) {
				return HidePowerOrShout(shout->GetFormID());
			}
			if (auto* spell = selected->data.baseForm ? selected->data.baseForm->As<RE::SpellItem>() : nullptr) {
				if (IsOrganizerPower(spell)) {
					return HidePowerOrShout(spell->GetFormID());
				}
				if (IsOrganizerSpell(spell)) {
					return HideSpell(spell->GetFormID());
				}
			}
			if (const auto effectFormID = ResolveSelectedActiveEffectFormID(selected); effectFormID.has_value()) {
				return HideEffect(*effectFormID);
			}
		}

		if (const auto fallbackFormID = ResolveSelectedFormIDFromMovieView(runtime.itemList); fallbackFormID.has_value()) {
			if (auto* shout = RE::TESForm::LookupByID<RE::TESShout>(*fallbackFormID)) {
				(void)shout;
				return HidePowerOrShout(*fallbackFormID);
			}
			if (auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*fallbackFormID)) {
				if (IsOrganizerPower(spell)) {
					return HidePowerOrShout(*fallbackFormID);
				}
				if (IsOrganizerSpell(spell)) {
					return HideSpell(*fallbackFormID);
				}
			}
		}

		LOG_INFO("MagicOrganizer: Hotkey pressed but no hideable selected entry was resolved");
		return false;
	}

	bool Manager::IsPowerShoutHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}
		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenPowerShoutFormIDs, a_formID);
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenSpells;
		std::vector<RE::FormID> hiddenPowers;
		std::vector<RE::FormID> hiddenEffects;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenSpells = m_hiddenSpellFormIDs;
			hiddenPowers = m_hiddenPowerShoutFormIDs;
			hiddenEffects = m_hiddenEffectFormIDs;
		}
		json root = {
			{ "enabled", cfg.enabled },
			{ "hotkey", cfg.hotkey },
			{ kGlobalHiddenListsKey, cfg.globalHiddenLists }
		};
		if (cfg.globalHiddenLists) {
			root[kHiddenSpellsKey] = WriteFormIDArray(hiddenSpells);
			root[kHiddenPowersKey] = WriteFormIDArray(hiddenPowers);
			root[kHiddenEffectsKey] = WriteFormIDArray(hiddenEffects);
		} else {
			if (std::ifstream previous(path); previous.is_open()) {
				json previousRoot{};
				try {
					previous >> previousRoot;
					if (previousRoot.contains(kHiddenSpellsKey)) {
						root[kHiddenSpellsKey] = previousRoot[kHiddenSpellsKey];
					}
					if (previousRoot.contains(kHiddenPowersKey)) {
						root[kHiddenPowersKey] = previousRoot[kHiddenPowersKey];
					}
					if (previousRoot.contains(kHiddenEffectsKey)) {
						root[kHiddenEffectsKey] = previousRoot[kHiddenEffectsKey];
					}
				} catch (const std::exception&) {
					// Preserve forward progress: ignore legacy/invalid prior JSON payloads.
				}
			}
		}
		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("MagicOrganizer: Failed to write config {}", path.string());
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
			LOG_WARN("MagicOrganizer: Failed to read config {}", path.string());
			return;
		}
		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("MagicOrganizer: Config parse failed ({}), rewriting defaults", e.what());
			SaveConfigToJson();
			return;
		}
		OrganizerConfig cfg{};
		std::vector<RE::FormID> loadedSpells;
		std::vector<RE::FormID> loadedPowers;
		std::vector<RE::FormID> loadedEffects;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}
		cfg.enabled = parsed.value("enabled", cfg.enabled);
		cfg.hotkey = parsed.value("hotkey", cfg.hotkey);
		cfg.globalHiddenLists = parsed.value(kGlobalHiddenListsKey, cfg.globalHiddenLists);
		if (cfg.globalHiddenLists) {
			loadedSpells = ReadFormIDArray(parsed, kHiddenSpellsKey);
			loadedPowers = ReadFormIDArray(parsed, kHiddenPowersKey);
			loadedEffects = ReadFormIDArray(parsed, kHiddenEffectsKey);
		}
		{
			std::scoped_lock lock(m_lock);
			m_config = cfg;
			if (cfg.globalHiddenLists) {
				m_hiddenSpellFormIDs = std::move(loadedSpells);
				m_hiddenPowerShoutFormIDs = std::move(loadedPowers);
				m_hiddenEffectFormIDs = std::move(loadedEffects);
			}
			NormalizeConfig();
		}
	}

	void Manager::SaveHiddenToCosave(SKSE::SerializationInterface* a_serialization) const
	{
		if (!a_serialization) {
			return;
		}
		{
			std::scoped_lock lock(m_lock);
			if (m_config.globalHiddenLists) {
				return;
			}
		}
		std::vector<RE::FormID> spells;
		std::vector<RE::FormID> powers;
		std::vector<RE::FormID> effects;
		{
			std::scoped_lock lock(m_lock);
			spells = m_hiddenSpellFormIDs;
			powers = m_hiddenPowerShoutFormIDs;
			effects = m_hiddenEffectFormIDs;
		}
		if (!a_serialization->OpenRecord(kHiddenAllRecord, kSerializationVersion)) {
			LOG_WARN("MagicOrganizer: Failed to open cosave record");
			return;
		}
		auto writeList = [&](const std::vector<RE::FormID>& a_list) {
			const std::uint32_t count = static_cast<std::uint32_t>(a_list.size());
			if (!a_serialization->WriteRecordData(count)) {
				return false;
			}
			for (const auto formID : a_list) {
				if (!a_serialization->WriteRecordData(formID)) {
					return false;
				}
			}
			return true;
		};
		if (!writeList(spells) || !writeList(powers)) {
			return;
		}
		writeList(effects);
	}

	void Manager::LoadHiddenFromCosave(SKSE::SerializationInterface* a_serialization)
	{
		if (!a_serialization) {
			return;
		}
		bool globalHiddenLists = false;
		{
			std::scoped_lock lock(m_lock);
			globalHiddenLists = m_config.globalHiddenLists;
		}
		if (globalHiddenLists) {
			ApplyTrackedFlags();
			MaybeRunReconcile(true);
			return;
		}
		std::vector<RE::FormID> loadedSpells;
		std::vector<RE::FormID> loadedPowers;
		std::vector<RE::FormID> loadedEffects;
		auto readList = [&](std::vector<RE::FormID>& a_out) {
			std::uint32_t count = 0;
			if (!a_serialization->ReadRecordData(count)) {
				return false;
			}
			a_out.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i) {
				RE::FormID stored = 0;
				if (!a_serialization->ReadRecordData(stored)) {
					return false;
				}
				RE::FormID resolved = 0;
				if (a_serialization->ResolveFormID(stored, resolved)) {
					a_out.push_back(resolved);
				} else if ((stored & 0xFF000000) == 0) {
					a_out.push_back(stored);
				}
			}
			return true;
		};

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_serialization->GetNextRecordInfo(type, version, length)) {
			if (type != kHiddenAllRecord || version != kSerializationVersion) {
				continue;
			}
			if (!readList(loadedSpells) || !readList(loadedPowers)) {
				continue;
			}
			readList(loadedEffects);
		}

		std::vector<RE::FormID> previousSpells;
		std::vector<RE::FormID> previousPowers;
		std::vector<RE::FormID> previousEffects;
		std::unordered_set<RE::FormID> removedSpells;
		std::unordered_set<RE::FormID> removedPowers;
		std::unordered_set<RE::FormID> removedShouts;
		bool enabled = false;
		{
			std::scoped_lock lock(m_lock);
			previousSpells = m_hiddenSpellFormIDs;
			previousPowers = m_hiddenPowerShoutFormIDs;
			previousEffects = m_hiddenEffectFormIDs;
			removedSpells = m_removedByOrganizerSpells;
			removedPowers = m_removedByOrganizerPowers;
			removedShouts = m_removedByOrganizerShouts;
			enabled = m_config.enabled;
		}

		if (enabled) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			for (const auto formID : previousSpells) {
				if (removedSpells.contains(formID) && player) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerSpell(spell)) {
					AddPlayerSpell(player, spell);
					}
				}
			}
			for (const auto formID : previousPowers) {
				if (removedPowers.contains(formID) && player) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerPower(spell)) {
						AddPlayerSpell(player, spell);
					}
				}
				if (removedShouts.contains(formID) && player) {
					auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
					if (shout && IsOrganizerShout(shout)) {
						AddPlayerShout(player, shout);
					}
				}
			}
			for (const auto formID : previousEffects) {
				if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID)) {
					effect->data.flags.reset(kHideFlag);
				}
			}
		}

		{
			std::scoped_lock lock(m_lock);
			m_hiddenSpellFormIDs = std::move(loadedSpells);
			m_hiddenPowerShoutFormIDs = std::move(loadedPowers);
			m_hiddenEffectFormIDs = std::move(loadedEffects);
			m_removedByOrganizerSpells.clear();
			m_removedByOrganizerPowers.clear();
			m_removedByOrganizerShouts.clear();
			NormalizeConfig();
		}
		ApplyTrackedFlags();
		MaybeRunReconcile(true);
	}

	void Manager::RevertHiddenFromCosave()
	{
		{
			std::scoped_lock lock(m_lock);
			if (m_config.globalHiddenLists) {
				return;
			}
		}
		std::vector<RE::FormID> previousSpells;
		std::vector<RE::FormID> previousPowers;
		std::vector<RE::FormID> previousEffects;
		std::unordered_set<RE::FormID> removedSpells;
		std::unordered_set<RE::FormID> removedPowers;
		std::unordered_set<RE::FormID> removedShouts;
		bool enabled = false;
		{
			std::scoped_lock lock(m_lock);
			previousSpells = m_hiddenSpellFormIDs;
			previousPowers = m_hiddenPowerShoutFormIDs;
			previousEffects = m_hiddenEffectFormIDs;
			removedSpells = m_removedByOrganizerSpells;
			removedPowers = m_removedByOrganizerPowers;
			removedShouts = m_removedByOrganizerShouts;
			enabled = m_config.enabled;
			m_hiddenSpellFormIDs.clear();
			m_hiddenPowerShoutFormIDs.clear();
			m_hiddenEffectFormIDs.clear();
			m_removedByOrganizerSpells.clear();
			m_removedByOrganizerPowers.clear();
			m_removedByOrganizerShouts.clear();
		}

		if (enabled) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			for (const auto formID : previousSpells) {
				if (removedSpells.contains(formID) && player) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerSpell(spell)) {
						AddPlayerSpell(player, spell);
					}
				}
			}
			for (const auto formID : previousPowers) {
				if (removedPowers.contains(formID) && player) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerPower(spell)) {
						AddPlayerSpell(player, spell);
					}
				}
				if (removedShouts.contains(formID) && player) {
					auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
					if (shout && IsOrganizerShout(shout)) {
						AddPlayerShout(player, shout);
					}
				}
			}
			for (const auto formID : previousEffects) {
				if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID)) {
					effect->data.flags.reset(kHideFlag);
				}
			}
		}

		RefreshMagicMenuIfOpen();
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"MagicOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "MagicOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		auto normalize = [](std::vector<RE::FormID>& a_list) {
			a_list.erase(std::remove_if(a_list.begin(), a_list.end(), [](const RE::FormID id) { return id == 0; }), a_list.end());
			std::sort(a_list.begin(), a_list.end());
			a_list.erase(std::unique(a_list.begin(), a_list.end()), a_list.end());
		};
		normalize(m_hiddenSpellFormIDs);
		normalize(m_hiddenPowerShoutFormIDs);
		normalize(m_hiddenEffectFormIDs);
	}

	void Manager::ApplyTrackedFlags()
	{
		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenSpells;
		std::vector<RE::FormID> hiddenPowers;
		std::vector<RE::FormID> hiddenEffects;
		std::unordered_set<RE::FormID> removedSpells;
		std::unordered_set<RE::FormID> removedPowers;
		std::unordered_set<RE::FormID> removedShouts;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenSpells = m_hiddenSpellFormIDs;
			hiddenPowers = m_hiddenPowerShoutFormIDs;
			hiddenEffects = m_hiddenEffectFormIDs;
			removedSpells = m_removedByOrganizerSpells;
			removedPowers = m_removedByOrganizerPowers;
			removedShouts = m_removedByOrganizerShouts;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			for (const auto formID : hiddenSpells) {
				auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
				if (!spell || !IsOrganizerSpell(spell)) {
					continue;
				}
				if (cfg.enabled) {
					if (RemovePlayerSpell(player, spell)) {
						removedSpells.insert(formID);
					}
				} else {
					if (removedSpells.erase(formID) > 0) {
						AddPlayerSpell(player, spell);
					}
				}
			}
			for (const auto formID : hiddenPowers) {
				auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
				if (spell && IsOrganizerPower(spell)) {
					if (cfg.enabled) {
						if (RemovePlayerSpell(player, spell)) {
							removedPowers.insert(formID);
						}
					} else {
						if (removedPowers.erase(formID) > 0) {
							AddPlayerSpell(player, spell);
						}
					}
					continue;
				}
				auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
				if (shout && IsOrganizerShout(shout)) {
					if (cfg.enabled) {
						if (RemovePlayerShout(player, shout)) {
							removedShouts.insert(formID);
						}
					} else {
						if (removedShouts.erase(formID) > 0) {
							AddPlayerShout(player, shout);
						}
					}
				}
			}
		}

		{
			std::scoped_lock lock(m_lock);
			m_removedByOrganizerSpells = std::move(removedSpells);
			m_removedByOrganizerPowers = std::move(removedPowers);
			m_removedByOrganizerShouts = std::move(removedShouts);
		}

		for (const auto formID : hiddenEffects) {
			auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID);
			if (effect) {
				effect->data.flags.set(cfg.enabled, kHideFlag);
			}
		}
		RefreshMagicMenuIfOpen();
	}

	void Manager::MaybeRunReconcile(bool a_force)
	{
		OrganizerConfig cfg{};
		const auto now = std::chrono::steady_clock::now();
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			if (!cfg.enabled) {
				return;
			}
			if (!a_force && now < m_nextReconcileAt) {
				return;
			}
			m_nextReconcileAt = now + kReconcileInterval;
		}
		ReconcileOwnedHiddenEntries();
	}

	void Manager::ReconcileOwnedHiddenEntries()
	{
		std::vector<RE::FormID> hiddenSpells;
		std::vector<RE::FormID> hiddenPowers;
		{
			std::scoped_lock lock(m_lock);
			if (!m_config.enabled) {
				return;
			}
			hiddenSpells = m_hiddenSpellFormIDs;
			hiddenPowers = m_hiddenPowerShoutFormIDs;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		std::vector<RE::FormID> removedSpells;
		std::vector<RE::FormID> removedPowers;
		std::vector<RE::FormID> removedShouts;

		for (const auto formID : hiddenSpells) {
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
			if (!spell || !IsOrganizerSpell(spell) || !player->HasSpell(spell)) {
				continue;
			}
			if (RemovePlayerSpell(player, spell)) {
				removedSpells.push_back(formID);
			}
		}

		for (const auto formID : hiddenPowers) {
			if (auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID); spell && IsOrganizerPower(spell) && player->HasSpell(spell)) {
				if (RemovePlayerSpell(player, spell)) {
					removedPowers.push_back(formID);
				}
				continue;
			}
			if (auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID); shout && IsOrganizerShout(shout) && player->HasShout(shout)) {
				if (RemovePlayerShout(player, shout)) {
					removedShouts.push_back(formID);
				}
			}
		}

		if (removedSpells.empty() && removedPowers.empty() && removedShouts.empty()) {
			return;
		}

		{
			std::scoped_lock lock(m_lock);
			for (const auto formID : removedSpells) {
				m_removedByOrganizerSpells.insert(formID);
			}
			for (const auto formID : removedPowers) {
				m_removedByOrganizerPowers.insert(formID);
			}
			for (const auto formID : removedShouts) {
				m_removedByOrganizerShouts.insert(formID);
			}
		}

		LOG_INFO(
			"MagicOrganizer: Reconciled owned hidden entries (spells={}, powers={}, shouts={})",
			removedSpells.size(),
			removedPowers.size(),
			removedShouts.size());
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

		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenFormIDs = m_hiddenPowerShoutFormIDs;
		}
		if (!cfg.enabled || hiddenFormIDs.empty()) {
			return;
		}

		auto& items = runtime.itemList->items;
		for (std::uint32_t i = items.size(); i > 0; --i) {
			const auto idx = i - 1;
			const auto* item = items[idx];
			if (!item || !item->data.baseForm) {
				continue;
			}
			const auto* baseForm = item->data.baseForm;
			const auto formID = baseForm->GetFormID();
			if (formID == 0 || !IsFormIDHidden(hiddenFormIDs, formID)) {
				continue;
			}
			bool shouldHide = false;
			if (const auto* spell = baseForm->As<RE::SpellItem>()) {
				shouldHide = IsOrganizerPower(spell);
			} else if (baseForm->As<RE::TESShout>()) {
				shouldHide = true;
			}
			if (shouldHide) {
				items.erase(items.begin() + idx);
			}
		}

		auto& entryList = runtime.itemList->entryList;
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
			if (formID == 0 || !IsFormIDHidden(hiddenFormIDs, formID)) {
				continue;
			}
			entryList.RemoveElements(idx, 1);
		}
	}

	bool Manager::IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID)
	{
		return std::find(a_hidden.begin(), a_hidden.end(), a_formID) != a_hidden.end();
	}

	bool Manager::IsOrganizerSpell(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}
		const auto type = a_spell->GetSpellType();
		return type != RE::MagicSystem::SpellType::kPower &&
		       type != RE::MagicSystem::SpellType::kLesserPower &&
		       type != RE::MagicSystem::SpellType::kVoicePower;
	}

	bool Manager::IsOrganizerPower(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}
		const auto type = a_spell->GetSpellType();
		return type == RE::MagicSystem::SpellType::kPower ||
		       type == RE::MagicSystem::SpellType::kLesserPower ||
		       type == RE::MagicSystem::SpellType::kVoicePower;
	}

	bool Manager::IsOrganizerShout(const RE::TESShout* a_shout)
	{
		return a_shout != nullptr;
	}

	std::string Manager::BuildSpellDisplayName(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return "<None>";
		}
		const char* name = a_spell->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_spell->GetFormID());
		}
		const char* editorID = a_spell->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_spell->GetFormID());
		}
		return std::format("<{:08X}>", a_spell->GetFormID());
	}

	std::string Manager::BuildPowerShoutDisplayName(const RE::SpellItem* a_spell)
	{
		return BuildSpellDisplayName(a_spell);
	}

	std::string Manager::BuildPowerShoutDisplayName(const RE::TESShout* a_shout)
	{
		if (!a_shout) {
			return "<None>";
		}
		const char* name = a_shout->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_shout->GetFormID());
		}
		const char* editorID = a_shout->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_shout->GetFormID());
		}
		return std::format("<{:08X}>", a_shout->GetFormID());
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
		auto* target = player ? player->GetMagicTarget() : nullptr;
		auto* activeList = target ? target->GetActiveEffectList() : nullptr;
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
}
