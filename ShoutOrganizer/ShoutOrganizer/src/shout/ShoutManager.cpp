#include "shout/ShoutManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <optional>

#include "json/single_include/nlohmann/json.hpp"

namespace MITHRAS::SHOUT_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kHiddenPowerShoutRecord = 'HPSH';
		constexpr std::uint32_t kSerializationID = 'SHOG';

		std::optional<RE::FormID> ResolveSelectedPowerOrShoutFormID(RE::MagicItemList::Item* a_selected)
		{
			if (!a_selected || !a_selected->data.baseForm) {
				return std::nullopt;
			}

			if (auto* spell = a_selected->data.baseForm->As<RE::SpellItem>()) {
				const auto type = spell->GetSpellType();
				const bool isPowerLike = type == RE::MagicSystem::SpellType::kPower ||
				                         type == RE::MagicSystem::SpellType::kLesserPower ||
				                         type == RE::MagicSystem::SpellType::kVoicePower;
				if (!isPowerLike) {
					return std::nullopt;
				}
				const auto formID = spell->GetFormID();
				return formID ? std::optional<RE::FormID>(formID) : std::nullopt;
			}

			if (auto* shout = a_selected->data.baseForm->As<RE::TESShout>()) {
				return shout->GetFormID();
			}

			return std::nullopt;
		}

		std::optional<RE::FormID> ResolveSelectedFormIDFromMovieView(const RE::MagicItemList* a_itemList)
		{
			if (!a_itemList || !a_itemList->view) {
				return std::nullopt;
			}

			RE::GFxValue val{};
			if (!a_itemList->view->GetVariable(&val, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId")) {
				return std::nullopt;
			}
			if (!val.IsNumber()) {
				return std::nullopt;
			}

			const auto raw = static_cast<std::uint64_t>(val.GetNumber());
			const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
			if (formID == 0) {
				return std::nullopt;
			}
			return formID;
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

		bool RemovePlayerShout(RE::PlayerCharacter* a_player, RE::TESShout* a_shout)
		{
			if (!a_player || !a_shout) {
				return false;
			}

			bool removed = false;
			if (auto* spellList = GetPlayerSpellList(a_player)) {
				removed = spellList->RemoveShout(a_shout) || removed;
			}

			return removed || !a_player->HasShout(a_shout);
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

			return removed || !a_player->HasSpell(a_spell);
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
		}

		LoadConfigFromJson();
		ApplyTrackedFlags();
	}

	void Manager::RegisterSerialization()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("ShoutOrganizer: Serialization interface unavailable");
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
			LOG_INFO("ShoutOrganizer: Hotkey updated to {}", a_keyCode);
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

	std::vector<PowerShoutEntry> Manager::GetHiddenEntries() const
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
				hidden.push_back({ formID, BuildDisplayName(spell) });
				continue;
			}

			auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
			if (shout && IsOrganizerShout(shout)) {
				hidden.push_back({ formID, BuildDisplayName(shout) });
			} else {
				hidden.push_back({ formID, std::format("<Missing {:08X}>", formID) });
			}
		}

		return hidden;
	}

	bool Manager::IsEntryHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}

		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenPowerShoutFormIDs, a_formID);
	}

	bool Manager::HideEntry(RE::FormID a_formID)
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
		{
			std::scoped_lock lock(m_lock);
			enabled = m_config.enabled;
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
			if (player && isPower && player->HasSpell(spell)) {
				RemovePlayerSpell(player, spell);
			}
			if (player && isShout && player->HasShout(shout)) {
				RemovePlayerShout(player, shout);
			}
		}

		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideEntry(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenPowerShoutFormIDs;
			auto it = std::remove(hidden.begin(), hidden.end(), a_formID);
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}

		if (!removed) {
			return false;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		auto* shout = RE::TESForm::LookupByID<RE::TESShout>(a_formID);
		if (player && spell && IsOrganizerPower(spell)) {
			AddPlayerSpell(player, spell);
		}
		if (player && shout && IsOrganizerShout(shout) && !player->HasShout(shout)) {
			AddPlayerShout(player, shout);
		}

		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideCurrentlySelectedMagicMenuPowerOrShout()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			LOG_INFO("ShoutOrganizer: Hide request ignored (MagicMenu not open)");
			return false;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			LOG_INFO("ShoutOrganizer: Hide request failed (MagicMenu handle unavailable)");
			return false;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			LOG_INFO("ShoutOrganizer: Hide request failed (MagicMenu itemList unavailable)");
			return false;
		}

		auto* selected = runtime.itemList->GetSelectedItem();
		const auto selectedFormID = selected ?
			ResolveSelectedPowerOrShoutFormID(selected) :
			ResolveSelectedFormIDFromMovieView(runtime.itemList);
		if (!selectedFormID.has_value()) {
			if (selected) {
				const char* selectedName = selected->data.GetName();
				const auto* baseForm = selected->data.baseForm;
				LOG_INFO(
					"ShoutOrganizer: Hotkey resolve miss (name='{}', baseForm={:08X}, type={})",
					selectedName ? selectedName : "",
					baseForm ? baseForm->GetFormID() : 0,
					baseForm ? static_cast<std::uint32_t>(baseForm->GetFormType()) : 0U);
			} else {
				LOG_INFO("ShoutOrganizer: Hide request failed (no selected item/formId in MagicMenu; itemCount={})", runtime.itemList->items.size());
			}
			return false;
		}

		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*selectedFormID);
		auto* shout = RE::TESForm::LookupByID<RE::TESShout>(*selectedFormID);
		const bool hideable = (spell && IsOrganizerPower(spell)) || (shout && IsOrganizerShout(shout));
		if (!hideable) {
			LOG_INFO("ShoutOrganizer: Hide request failed (resolved form {:08X} was not a hideable power/shout)", *selectedFormID);
			return false;
		}

		if (HideEntry(*selectedFormID)) {
			if (spell && IsOrganizerPower(spell)) {
				LOG_INFO("ShoutOrganizer: Removed power from player list {}", BuildDisplayName(spell));
			} else if (shout && IsOrganizerShout(shout)) {
				LOG_INFO("ShoutOrganizer: Removed shout from player list {}", BuildDisplayName(shout));
			}
			return true;
		}
		LOG_INFO("ShoutOrganizer: Hide request ignored (entry {:08X} already tracked)", *selectedFormID);
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
			{ "hotkey", cfg.hotkey }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("ShoutOrganizer: Failed to write config {}", path.string());
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
			LOG_WARN("ShoutOrganizer: Failed to read config {}", path.string());
			return;
		}

		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("ShoutOrganizer: Config parse failed ({}), rewriting defaults", e.what());
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
		{
			std::scoped_lock lock(m_lock);
			m_config = cfg;
			NormalizeConfig();
		}
	}

	void Manager::SaveHiddenToCosave(SKSE::SerializationInterface* a_serialization) const
	{
		if (!a_serialization) {
			return;
		}

		std::vector<RE::FormID> hidden;
		{
			std::scoped_lock lock(m_lock);
			hidden = m_hiddenPowerShoutFormIDs;
		}

		if (!a_serialization->OpenRecord(kHiddenPowerShoutRecord, kSerializationVersion)) {
			LOG_WARN("ShoutOrganizer: Failed to open cosave record");
			return;
		}

		const std::uint32_t count = static_cast<std::uint32_t>(hidden.size());
		if (!a_serialization->WriteRecordData(count)) {
			LOG_WARN("ShoutOrganizer: Failed to write cosave hidden entry count");
			return;
		}

		for (const auto formID : hidden) {
			if (!a_serialization->WriteRecordData(formID)) {
				LOG_WARN("ShoutOrganizer: Failed to write cosave entry {:08X}", formID);
				return;
			}
		}
	}

	void Manager::LoadHiddenFromCosave(SKSE::SerializationInterface* a_serialization)
	{
		if (!a_serialization) {
			return;
		}

		std::vector<RE::FormID> loadedHidden;
		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_serialization->GetNextRecordInfo(type, version, length)) {
			if (type != kHiddenPowerShoutRecord || version != kSerializationVersion) {
				continue;
			}

			std::uint32_t count = 0;
			if (!a_serialization->ReadRecordData(count)) {
				LOG_WARN("ShoutOrganizer: Failed to read cosave hidden entry count");
				continue;
			}

			loadedHidden.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i) {
				RE::FormID stored = 0;
				if (!a_serialization->ReadRecordData(stored)) {
					LOG_WARN("ShoutOrganizer: Failed to read cosave entry at index {}", i);
					break;
				}

				RE::FormID resolved = 0;
				if (a_serialization->ResolveFormID(stored, resolved)) {
					loadedHidden.push_back(resolved);
				} else if ((stored & 0xFF000000) == 0) {
					loadedHidden.push_back(stored);
				}
			}
		}

		std::vector<RE::FormID> previousHidden;
		bool enabled = false;
		{
			std::scoped_lock lock(m_lock);
			previousHidden = m_hiddenPowerShoutFormIDs;
			enabled = m_config.enabled;
		}

		if (enabled) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				for (const auto formID : previousHidden) {
					if (auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID); spell && IsOrganizerPower(spell)) {
						AddPlayerSpell(player, spell);
						continue;
					}
					if (auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID); shout && IsOrganizerShout(shout)) {
						AddPlayerShout(player, shout);
					}
				}
			}
		}

		{
			std::scoped_lock lock(m_lock);
			m_hiddenPowerShoutFormIDs = std::move(loadedHidden);
			NormalizeConfig();
		}

		ApplyTrackedFlags();
	}

	void Manager::RevertHiddenFromCosave()
	{
		std::vector<RE::FormID> previousHidden;
		bool enabled = false;
		{
			std::scoped_lock lock(m_lock);
			previousHidden = m_hiddenPowerShoutFormIDs;
			enabled = m_config.enabled;
			m_hiddenPowerShoutFormIDs.clear();
		}

		if (enabled) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				for (const auto formID : previousHidden) {
					if (auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID); spell && IsOrganizerPower(spell)) {
						AddPlayerSpell(player, spell);
						continue;
					}
					if (auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID); shout && IsOrganizerShout(shout)) {
						AddPlayerShout(player, shout);
					}
				}
			}
		}

		RefreshMagicMenuIfOpen();
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"ShoutOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "ShoutOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		auto& hidden = m_hiddenPowerShoutFormIDs;
		hidden.erase(
			std::remove_if(hidden.begin(), hidden.end(), [](const RE::FormID id) { return id == 0; }),
			hidden.end());
		std::sort(hidden.begin(), hidden.end());
		hidden.erase(std::unique(hidden.begin(), hidden.end()), hidden.end());
	}

	void Manager::ApplyTrackedFlags()
	{
		OrganizerConfig cfg{};
		std::vector<RE::FormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenFormIDs = m_hiddenPowerShoutFormIDs;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		for (const auto formID : hiddenFormIDs) {
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
			if (spell && IsOrganizerPower(spell)) {
				if (cfg.enabled) {
					RemovePlayerSpell(player, spell);
				} else {
					AddPlayerSpell(player, spell);
				}
				continue;
			}

			auto* shout = RE::TESForm::LookupByID<RE::TESShout>(formID);
			if (shout && IsOrganizerShout(shout)) {
				if (cfg.enabled) {
					if (player->HasShout(shout)) {
						RemovePlayerShout(player, shout);
					}
				} else {
					AddPlayerShout(player, shout);
				}
			}
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
			if (!item) {
				continue;
			}

			const auto* baseForm = item->data.baseForm;
			if (!baseForm) {
				continue;
			}

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

	std::string Manager::BuildDisplayName(const RE::SpellItem* a_spell)
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

	std::string Manager::BuildDisplayName(const RE::TESShout* a_shout)
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
}
