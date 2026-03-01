#include "spell/SpellManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <optional>

#include "json/single_include/nlohmann/json.hpp"

namespace MITHRAS::SPELL_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kHiddenSpellsRecord = 'HSPL';
		constexpr std::uint32_t kSerializationID = 'SORG';

		std::optional<RE::FormID> ResolveSelectedSpellFormID(RE::MagicItemList::Item* a_selected)
		{
			if (!a_selected || !a_selected->data.baseForm) {
				return std::nullopt;
			}

			auto* spell = a_selected->data.baseForm->As<RE::SpellItem>();
			if (!spell) {
				return std::nullopt;
			}

			const auto type = spell->GetSpellType();
			if (type == RE::MagicSystem::SpellType::kPower ||
			    type == RE::MagicSystem::SpellType::kLesserPower ||
			    type == RE::MagicSystem::SpellType::kVoicePower) {
				return std::nullopt;
			}

			const auto formID = spell->GetFormID();
			if (formID == 0) {
				return std::nullopt;
			}

			return formID;
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
			LOG_WARN("SpellOrganizer: Serialization interface unavailable");
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
			LOG_INFO("SpellOrganizer: Hotkey updated to {}", a_keyCode);
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
			if (spell) {
				hidden.push_back({ formID, BuildSpellDisplayName(spell) });
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
		{
			std::scoped_lock lock(m_lock);
			enabled = m_config.enabled;
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
				RemovePlayerSpell(player, spell);
			}
		}

		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideSpell(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenSpellFormIDs;
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
		if (player && spell && IsOrganizerSpell(spell)) {
			AddPlayerSpell(player, spell);
		}

		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideCurrentlySelectedMagicMenuSpell()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			LOG_INFO("SpellOrganizer: Hide request ignored (MagicMenu not open)");
			return false;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			LOG_INFO("SpellOrganizer: Hide request failed (MagicMenu handle unavailable)");
			return false;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			LOG_INFO("SpellOrganizer: Hide request failed (MagicMenu itemList unavailable)");
			return false;
		}

		auto* selected = runtime.itemList->GetSelectedItem();
		const auto selectedFormID = selected ?
			ResolveSelectedSpellFormID(selected) :
			ResolveSelectedFormIDFromMovieView(runtime.itemList);
		if (!selectedFormID.has_value()) {
			if (selected) {
				const char* selectedName = selected->data.GetName();
				const auto* baseForm = selected->data.baseForm;
				LOG_INFO(
					"SpellOrganizer: Hotkey resolve miss (name='{}', baseForm={:08X}, type={})",
					selectedName ? selectedName : "",
					baseForm ? baseForm->GetFormID() : 0,
					baseForm ? static_cast<std::uint32_t>(baseForm->GetFormType()) : 0U);
			} else {
				LOG_INFO("SpellOrganizer: Hide request failed (no selected item/formId in MagicMenu; itemCount={})", runtime.itemList->items.size());
			}
			return false;
		}

		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*selectedFormID);
		if (!spell || !IsOrganizerSpell(spell)) {
			LOG_INFO("SpellOrganizer: Hide request failed (resolved form {:08X} was not a hideable spell)", *selectedFormID);
			return false;
		}

		if (HideSpell(*selectedFormID)) {
			LOG_INFO("SpellOrganizer: Removed spell from player list {}", BuildSpellDisplayName(spell));
			return true;
		}
		LOG_INFO("SpellOrganizer: Hide request ignored (spell {:08X} already tracked)", *selectedFormID);
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
			LOG_WARN("SpellOrganizer: Failed to write config {}", path.string());
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
			LOG_WARN("SpellOrganizer: Failed to read config {}", path.string());
			return;
		}

		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("SpellOrganizer: Config parse failed ({}), rewriting defaults", e.what());
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
			hidden = m_hiddenSpellFormIDs;
		}

		if (!a_serialization->OpenRecord(kHiddenSpellsRecord, kSerializationVersion)) {
			LOG_WARN("SpellOrganizer: Failed to open cosave record");
			return;
		}

		const std::uint32_t count = static_cast<std::uint32_t>(hidden.size());
		if (!a_serialization->WriteRecordData(count)) {
			LOG_WARN("SpellOrganizer: Failed to write cosave hidden spell count");
			return;
		}

		for (const auto formID : hidden) {
			if (!a_serialization->WriteRecordData(formID)) {
				LOG_WARN("SpellOrganizer: Failed to write cosave spell {:08X}", formID);
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
			if (type != kHiddenSpellsRecord || version != kSerializationVersion) {
				continue;
			}

			std::uint32_t count = 0;
			if (!a_serialization->ReadRecordData(count)) {
				LOG_WARN("SpellOrganizer: Failed to read cosave hidden spell count");
				continue;
			}

			loadedHidden.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i) {
				RE::FormID stored = 0;
				if (!a_serialization->ReadRecordData(stored)) {
					LOG_WARN("SpellOrganizer: Failed to read cosave spell at index {}", i);
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
			previousHidden = m_hiddenSpellFormIDs;
			enabled = m_config.enabled;
		}

		if (enabled) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				for (const auto formID : previousHidden) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerSpell(spell)) {
						AddPlayerSpell(player, spell);
					}
				}
			}
		}

		{
			std::scoped_lock lock(m_lock);
			m_hiddenSpellFormIDs = std::move(loadedHidden);
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
			previousHidden = m_hiddenSpellFormIDs;
			enabled = m_config.enabled;
			m_hiddenSpellFormIDs.clear();
		}

		if (enabled) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				for (const auto formID : previousHidden) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && IsOrganizerSpell(spell)) {
						AddPlayerSpell(player, spell);
					}
				}
			}
		}

		RefreshMagicMenuIfOpen();
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"SpellOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "SpellOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		auto& hidden = m_hiddenSpellFormIDs;
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
			hiddenFormIDs = m_hiddenSpellFormIDs;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		for (const auto formID : hiddenFormIDs) {
			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
			if (!spell || !IsOrganizerSpell(spell)) {
				continue;
			}

			if (cfg.enabled) {
				RemovePlayerSpell(player, spell);
			} else {
				AddPlayerSpell(player, spell);
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
}
