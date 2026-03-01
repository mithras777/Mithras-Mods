#include "spellbinding/SpellBindingManager.h"

#include "event/AttackAnimationEventSink.h"
#include "ui/PrismaBridge.h"
#include "util/LogUtil.h"

#include "RE/M/MagicMenu.h"
#include "RE/S/SendHUDMessage.h"

#include "json/single_include/nlohmann/json.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>

namespace SBIND
{
	using json = nlohmann::json;

	namespace
	{
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kBindingsRecord = 'SBND';
		constexpr std::uint32_t kConfigVersion = 1;
	}

	void Manager::Initialize()
	{
		std::scoped_lock lock(m_lock);
		m_config = {};
		m_config.version = kConfigVersion;
		m_runtime = {};
		m_runtime.unarmedKey = BuildUnarmedKey();
		m_runtime.worldTimeSec = 0.0f;
		LoadConfig();
		ClampConfig(m_config);
	}

	void Manager::OnRevert()
	{
		std::scoped_lock lock(m_lock);
		m_bindings.clear();
		m_runtime = {};
		m_runtime.unarmedKey = BuildUnarmedKey();
		m_runtime.worldTimeSec = 0.0f;
		LoadConfig();
		ClampConfig(m_config);
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc) {
			return;
		}

		std::scoped_lock lock(m_lock);
		if (!a_intfc->OpenRecord(kBindingsRecord, kSerializationVersion)) {
			return;
		}

		const auto writeString = [a_intfc](const std::string& a_value) {
			const std::uint32_t length = static_cast<std::uint32_t>(a_value.size());
			a_intfc->WriteRecordData(length);
			for (char ch : a_value) {
				a_intfc->WriteRecordData(ch);
			}
		};

		const std::uint32_t count = static_cast<std::uint32_t>(m_bindings.size());
		a_intfc->WriteRecordData(count);
		for (const auto& [key, spell] : m_bindings) {
			writeString(key.pluginName);
			a_intfc->WriteRecordData(key.localFormID);
			a_intfc->WriteRecordData(key.uniqueID);
			const auto slot = static_cast<std::uint32_t>(key.handSlot);
			a_intfc->WriteRecordData(slot);
			a_intfc->WriteRecordData(key.isUnarmed);
			writeString(key.displayName);
			writeString(spell.spellFormKey);
			writeString(spell.displayName);
			a_intfc->WriteRecordData(spell.lastKnownCost);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		std::unordered_map<BindingKey, BoundSpell, BindingKeyHash> loaded{};

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kBindingsRecord || version != kSerializationVersion) {
				continue;
			}

			const auto readString = [a_intfc](std::string& a_value) {
				std::uint32_t size = 0;
				a_intfc->ReadRecordData(size);
				a_value.clear();
				a_value.reserve(size);
				for (std::uint32_t i = 0; i < size; ++i) {
					char ch = '\0';
					a_intfc->ReadRecordData(ch);
					a_value.push_back(ch);
				}
			};

			std::uint32_t count = 0;
			a_intfc->ReadRecordData(count);
			for (std::uint32_t i = 0; i < count; ++i) {
				BindingKey key{};
				BoundSpell spell{};
				std::uint32_t slot = 0;

				readString(key.pluginName);
				a_intfc->ReadRecordData(key.localFormID);
				a_intfc->ReadRecordData(key.uniqueID);
				a_intfc->ReadRecordData(slot);
				a_intfc->ReadRecordData(key.isUnarmed);
				readString(key.displayName);
				readString(spell.spellFormKey);
				readString(spell.displayName);
				a_intfc->ReadRecordData(spell.lastKnownCost);
				key.handSlot = static_cast<HandSlot>(slot);
				loaded[key] = std::move(spell);
			}
		}

		std::scoped_lock lock(m_lock);
		m_bindings = std::move(loaded);
		m_runtime.unarmedKey = BuildUnarmedKey();
	}

	SpellBindingConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetConfig(const SpellBindingConfig& a_config, bool a_save)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			ClampConfig(m_config);
		}
		if (a_save) {
			SaveConfig();
		}
		PushUISnapshot();
	}

	void Manager::Update(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		if (!a_player || a_deltaTime <= 0.0f) {
			return;
		}
		SB_EVENT::AttackAnimationEventSink::Register();

		const auto config = GetConfig();
		bool menuBlocked = false;
		{
			std::scoped_lock lock(m_lock);
			menuBlocked = m_runtime.menuBlocked;
			m_runtime.worldTimeSec += a_deltaTime;
		}
		if (!config.enabled || a_player->IsDead() || menuBlocked) {
			std::scoped_lock lock(m_lock);
			m_runtime.wasAttacking = a_player->IsAttacking();
			return;
		}

		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(a_player);
		}
	}

	void Manager::OnEquipChanged()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(player);
		}
		PushUISnapshot();
	}

	void Manager::OnMenuStateChanged(bool a_blockingMenuOpen)
	{
		std::scoped_lock lock(m_lock);
		m_runtime.menuBlocked = a_blockingMenuOpen;
	}

	void Manager::OnAttackAnimationEvent(std::string_view a_tag, std::string_view a_payload)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		const auto config = GetConfig();
		if (!config.enabled || player->IsDead()) {
			return;
		}

		auto toLower = [](std::string_view text) {
			std::string out(text);
			std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			return out;
		};
		const auto tag = toLower(a_tag);
		const auto payload = toLower(a_payload);
		const bool looksLikeAttack =
			((tag.find("attack") != std::string::npos || tag.find("swing") != std::string::npos) &&
			 (tag.find("start") != std::string::npos || tag.find("swing") != std::string::npos) &&
			 tag.find("stop") == std::string::npos) ||
			(payload.find("attack") != std::string::npos && payload.find("start") != std::string::npos);
		if (!looksLikeAttack) {
			return;
		}

		bool preferLeft = tag.find("left") != std::string::npos || payload.find("left") != std::string::npos;
		const bool isPowerAttack =
			player->IsPowerAttacking() ||
			tag.find("power") != std::string::npos ||
			payload.find("power") != std::string::npos;

		bool menuBlocked = false;
		std::optional<BindingKey> keyOpt;
		{
			std::scoped_lock lock(m_lock);
			menuBlocked = m_runtime.menuBlocked;
			RefreshEquippedKeysLocked(player);
			if (m_runtime.rightWeapon.has_value() && m_runtime.leftWeapon.has_value()) {
				if (preferLeft || IsLikelyLeftHandAttack(player)) {
					keyOpt = m_runtime.leftWeapon;
				} else {
					keyOpt = m_runtime.rightWeapon;
				}
			} else if (m_runtime.rightWeapon.has_value()) {
				keyOpt = m_runtime.rightWeapon;
			} else if (m_runtime.leftWeapon.has_value()) {
				keyOpt = m_runtime.leftWeapon;
			} else {
				keyOpt = m_runtime.unarmedKey;
			}
		}

		if (menuBlocked || !keyOpt.has_value()) {
			return;
		}

		bool triggered = false;
		{
			std::scoped_lock lock(m_lock);
			triggered = TryTriggerBoundSpellLocked(player, *keyOpt, isPowerAttack);
		}
		if (triggered) {
			PushUISnapshot();
		}
	}

	void Manager::ToggleUI()
	{
		auto* bridge = UI::PRISMA::Bridge::GetSingleton();
		bridge->Initialize();
		if (!bridge->IsAvailable()) {
			Notify("SpellBinding: Prisma UI is not installed");
			return;
		}
		bridge->Toggle();
	}

	void Manager::PushUISnapshot()
	{
		UI::PRISMA::Bridge::GetSingleton()->PushSnapshot(GetSnapshot().json);
	}

	ManagerSnapshot Manager::GetSnapshot() const
	{
		std::scoped_lock lock(m_lock);
		return { BuildSnapshotJsonLocked() };
	}

	bool Manager::TryBindSelectedMagicMenuSpell()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		const auto selectedFormID = ResolveSelectedSpellFormIDFromMagicMenu();
		if (!selectedFormID.has_value()) {
			Notify("SpellBinding: no selected spell in Magic Menu");
			return false;
		}

		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*selectedFormID);
		if (!spell) {
			Notify("SpellBinding: selected entry is not a spell");
			return false;
		}
		if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
			Notify("SpellBinding: concentration spells are not supported");
			return false;
		}
		if (!IsSupportedSpell(spell)) {
			Notify("SpellBinding: selected spell type is not supported");
			return false;
		}

		const std::string formKey = BuildFormKey(spell);
		if (formKey.empty()) {
			Notify("SpellBinding: failed to resolve spell form key");
			return false;
		}

		BindingKey bindingKey{};
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(player);
			const auto keyOpt = ResolveCurrentBindingKeyLocked(player);
			if (!keyOpt.has_value()) {
				Notify("SpellBinding: no equipped weapon/unarmed key available");
				return false;
			}
			bindingKey = *keyOpt;
			m_bindings[bindingKey] = BoundSpell{
				formKey,
				spell->GetName() ? spell->GetName() : "",
				spell->CalculateMagickaCost(player)
			};
			m_runtime.lastError.clear();
		}

		const auto handName = bindingKey.handSlot == HandSlot::kLeft ? "Left" : (bindingKey.handSlot == HandSlot::kRight ? "Right" : "Unarmed");
		Notify(std::format("SpellBinding: bound '{}' to {} ({})", spell->GetName(), bindingKey.displayName, handName));
		PushUISnapshot();
		return true;
	}

	bool Manager::UnbindWeaponFromSerializedKey(const std::string& a_key)
	{
		json payload;
		try {
			payload = json::parse(a_key);
		} catch (...) {
			return false;
		}

		BindingKey key{};
		key.pluginName = payload.value("pluginName", "");
		key.localFormID = payload.value("localFormID", 0u);
		key.uniqueID = static_cast<std::uint16_t>(payload.value("uniqueID", 0u));
		key.handSlot = static_cast<HandSlot>(payload.value("handSlot", 0u));
		key.isUnarmed = payload.value("isUnarmed", false);
		key.displayName = payload.value("displayName", "");

		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			removed = m_bindings.erase(key) > 0;
		}

		if (removed) {
			Notify(std::format("SpellBinding: unbound {}", key.displayName));
			PushUISnapshot();
		}
		return removed;
	}

	std::string Manager::GetUIHotkeyName() const
	{
		std::scoped_lock lock(m_lock);
		return GetKeyboardKeyName(m_config.uiToggleKey);
	}

	std::string Manager::GetBindHotkeyName() const
	{
		std::scoped_lock lock(m_lock);
		return GetKeyboardKeyName(m_config.bindKey);
	}

	void Manager::LoadConfig()
	{
		std::ifstream file(std::string(kConfigPath), std::ios::in);
		if (!file.is_open()) {
			SaveConfig();
			return;
		}

		json root;
		try {
			file >> root;
		} catch (...) {
			return;
		}

		m_config.version = root.value("version", m_config.version);
		m_config.enabled = root.value("enabled", m_config.enabled);
		m_config.uiToggleKey = root.value("uiToggleKey", m_config.uiToggleKey);
		m_config.bindKey = root.value("bindKey", m_config.bindKey);
		m_config.powerDamageScale = root.value("powerDamageScale", m_config.powerDamageScale);
		m_config.powerMagickaScale = root.value("powerMagickaScale", m_config.powerMagickaScale);
		m_config.showHudNotifications = root.value("showHudNotifications", m_config.showHudNotifications);
	}

	void Manager::SaveConfig() const
	{
		const json root = {
			{ "version", m_config.version },
			{ "enabled", m_config.enabled },
			{ "uiToggleKey", m_config.uiToggleKey },
			{ "bindKey", m_config.bindKey },
			{ "powerDamageScale", m_config.powerDamageScale },
			{ "powerMagickaScale", m_config.powerMagickaScale },
			{ "showHudNotifications", m_config.showHudNotifications }
		};

		try {
			const std::filesystem::path path{ std::string(kConfigPath) };
			std::filesystem::create_directories(path.parent_path());
			std::ofstream file(path, std::ios::out | std::ios::trunc);
			file << root.dump(2);
		} catch (...) {}
	}

	void Manager::ClampConfig(SpellBindingConfig& a_config) const
	{
		a_config.version = kConfigVersion;
		a_config.powerDamageScale = std::clamp(a_config.powerDamageScale, 0.1f, 5.0f);
		a_config.powerMagickaScale = std::clamp(a_config.powerMagickaScale, 0.1f, 5.0f);
	}

	void Manager::RefreshEquippedKeysLocked(RE::PlayerCharacter* a_player)
	{
		const auto* rightEntry = a_player->GetEquippedEntryData(false);
		const auto* leftEntry = a_player->GetEquippedEntryData(true);

		m_runtime.rightWeapon = BuildWeaponKeyFromEntry(rightEntry, HandSlot::kRight);
		m_runtime.leftWeapon = BuildWeaponKeyFromEntry(leftEntry, HandSlot::kLeft);
		m_runtime.unarmedKey = BuildUnarmedKey();

		if (m_runtime.rightWeapon && m_runtime.rightWeapon->displayName.empty()) {
			m_runtime.rightWeapon->displayName = "Right Weapon";
		}
		if (m_runtime.leftWeapon && m_runtime.leftWeapon->displayName.empty()) {
			m_runtime.leftWeapon->displayName = "Left Weapon";
		}
	}

	std::optional<BindingKey> Manager::ResolveCurrentBindingKeyLocked(RE::PlayerCharacter* a_player) const
	{
		(void)a_player;
		if (m_runtime.rightWeapon.has_value()) {
			return m_runtime.rightWeapon;
		}
		if (m_runtime.leftWeapon.has_value()) {
			return m_runtime.leftWeapon;
		}
		return m_runtime.unarmedKey;
	}

	std::optional<BindingKey> Manager::BuildWeaponKeyFromEntry(const RE::InventoryEntryData* a_entry, HandSlot a_slot)
	{
		if (!a_entry || !a_entry->object) {
			return std::nullopt;
		}

		auto* weapon = a_entry->object->As<RE::TESObjectWEAP>();
		if (!weapon) {
			return std::nullopt;
		}

		auto* file = weapon->GetFile(0);
		if (!file) {
			return std::nullopt;
		}

		std::uint16_t uniqueID = 0;
		if (a_entry->extraLists) {
			for (const auto& xList : *a_entry->extraLists) {
				if (!xList) {
					continue;
				}
				const auto* id = xList->GetByType<RE::ExtraUniqueID>();
				if (id) {
					uniqueID = static_cast<std::uint16_t>(id->uniqueID);
					break;
				}
			}
		}

		BindingKey key{};
		key.pluginName = file->GetFilename();
		key.localFormID = weapon->GetLocalFormID();
		key.uniqueID = uniqueID;
		key.handSlot = a_slot;
		key.isUnarmed = false;
		key.displayName = weapon->GetName() ? weapon->GetName() : "";
		return key;
	}

	BindingKey Manager::BuildUnarmedKey()
	{
		BindingKey key{};
		key.handSlot = HandSlot::kUnarmed;
		key.isUnarmed = true;
		key.displayName = "Unarmed";
		return key;
	}

	std::string Manager::BuildFormKey(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}
		const auto* file = a_form->GetFile(0);
		if (!file) {
			return {};
		}
		const auto filename = file->GetFilename();
		if (filename.empty()) {
			return {};
		}
		return std::format("{}|0x{:08X}", filename, a_form->GetLocalFormID());
	}

	bool Manager::ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID)
	{
		const auto split = a_formKey.find('|');
		if (split == std::string_view::npos) {
			return false;
		}

		a_pluginName = std::string(a_formKey.substr(0, split));
		auto text = std::string(a_formKey.substr(split + 1));
		if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) {
			text = text.substr(2);
		}

		std::uint32_t parsed = 0;
		const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed, 16);
		if (ec != std::errc{} || ptr != text.data() + text.size()) {
			return false;
		}

		a_localFormID = parsed;
		return true;
	}

	RE::SpellItem* Manager::ResolveSpell(std::string_view a_formKey)
	{
		std::string pluginName{};
		RE::FormID localFormID = 0;
		if (!ParseFormKey(a_formKey, pluginName, localFormID)) {
			return nullptr;
		}

		auto* data = RE::TESDataHandler::GetSingleton();
		if (!data) {
			return nullptr;
		}

		auto* form = data->LookupForm(localFormID, pluginName);
		return form ? form->As<RE::SpellItem>() : nullptr;
	}

	std::optional<RE::FormID> Manager::ResolveSelectedSpellFormIDFromMagicMenu()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			return std::nullopt;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return std::nullopt;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return std::nullopt;
		}

		if (auto* selected = runtime.itemList->GetSelectedItem(); selected && selected->data.baseForm) {
			auto* spell = selected->data.baseForm->As<RE::SpellItem>();
			if (spell) {
				return spell->GetFormID();
			}
		}

		RE::GFxValue val{};
		if (!runtime.itemList->view ||
		    !runtime.itemList->view->GetVariable(&val, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId") ||
		    !val.IsNumber()) {
			return std::nullopt;
		}

		const auto raw = static_cast<std::uint64_t>(val.GetNumber());
		const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
		if (formID == 0) {
			return std::nullopt;
		}

		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
		return spell ? std::optional<RE::FormID>(formID) : std::nullopt;
	}

	bool Manager::TryTriggerOnAttackStart(RE::PlayerCharacter* a_player)
	{
		const bool isAttacking = a_player->IsAttacking();
		bool wasAttacking = false;
		{
			std::scoped_lock lock(m_lock);
			wasAttacking = m_runtime.wasAttacking;
			m_runtime.wasAttacking = isAttacking;
		}

		if (!isAttacking || wasAttacking) {
			return false;
		}

		const bool isPowerAttack = a_player->IsPowerAttacking();
		std::optional<BindingKey> keyOpt;
		{
			std::scoped_lock lock(m_lock);
			if (m_runtime.rightWeapon.has_value() && m_runtime.leftWeapon.has_value()) {
				if (IsLikelyLeftHandAttack(a_player)) {
					keyOpt = m_runtime.leftWeapon;
				} else {
					keyOpt = m_runtime.rightWeapon;
				}
			} else if (m_runtime.rightWeapon.has_value()) {
				keyOpt = m_runtime.rightWeapon;
			} else if (m_runtime.leftWeapon.has_value()) {
				keyOpt = m_runtime.leftWeapon;
			} else {
				keyOpt = m_runtime.unarmedKey;
			}
		}

		if (!keyOpt.has_value()) {
			return false;
		}

		bool triggered = false;
		{
			std::scoped_lock lock(m_lock);
			triggered = TryTriggerBoundSpellLocked(a_player, *keyOpt, isPowerAttack);
		}
		if (triggered) {
			PushUISnapshot();
		}
		return triggered;
	}

	bool Manager::TryTriggerBoundSpellLocked(RE::PlayerCharacter* a_player, const BindingKey& a_key, bool a_isPowerAttack)
	{
		const auto it = m_bindings.find(a_key);
		if (it == m_bindings.end()) {
			return false;
		}

		auto* spell = ResolveSpell(it->second.spellFormKey);
		if (!spell) {
			SetLastErrorLocked("Missing bound spell");
			return false;
		}
		if (!a_player->HasSpell(spell)) {
			SetLastErrorLocked("Player no longer knows bound spell");
			return false;
		}
		if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
			SetLastErrorLocked("Concentration spells are blocked");
			return false;
		}
		const auto spellType = spell->GetSpellType();
		const bool isCooldownPower = spellType == RE::MagicSystem::SpellType::kPower || spellType == RE::MagicSystem::SpellType::kVoicePower;
		if (isCooldownPower) {
			if (a_player->IsInCastPowerList(spell)) {
				SetLastErrorLocked("Power is on cooldown");
				return false;
			}
			const auto cdIt = m_runtime.powerCooldownUntil.find(it->second.spellFormKey);
			if (cdIt != m_runtime.powerCooldownUntil.end() && m_runtime.worldTimeSec < cdIt->second) {
				SetLastErrorLocked("Power cooldown active");
				return false;
			}
		}

		const float magickaScale = a_isPowerAttack ? m_config.powerMagickaScale : 1.0f;
		const float damageScale = a_isPowerAttack ? m_config.powerDamageScale : 1.0f;
		const float rawCost = std::max(0.0f, spell->CalculateMagickaCost(a_player));
		const float finalCost = rawCost * magickaScale;
		auto* avOwner = a_player->AsActorValueOwner();
		if (!avOwner) {
			SetLastErrorLocked("No actor value owner");
			return false;
		}

		const float currentMagicka = avOwner->GetActorValue(RE::ActorValue::kMagicka);
		if (currentMagicka + 0.001f < finalCost) {
			SetLastErrorLocked("Not enough magicka");
			if (m_config.showHudNotifications) {
				Notify("SpellBinding: not enough magicka");
			}
			return false;
		}

		auto* caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) {
			caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
		}
		if (!caster) {
			SetLastErrorLocked("No valid magic caster");
			return false;
		}

		avOwner->DamageActorValue(RE::ActorValue::kMagicka, finalCost);

		const bool selfDelivery = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
		const bool castOnSelf = selfDelivery;
		RE::TESObjectREFR* target = a_player;
		caster->CastSpellImmediate(spell, castOnSelf, target, damageScale, false, 0.0f, a_player);

		if (isCooldownPower) {
			float cd = 0.85f;
			if (spellType == RE::MagicSystem::SpellType::kVoicePower) {
				cd = std::max(cd, a_player->GetVoiceRecoveryTime());
			} else {
				cd = std::max(cd, spell->GetChargeTime() + 0.5f);
			}
			m_runtime.powerCooldownUntil[it->second.spellFormKey] = m_runtime.worldTimeSec + cd;
		}

		m_runtime.lastTriggerWeapon = a_key.displayName;
		m_runtime.lastTriggerSpell = it->second.displayName;
		m_runtime.lastMagickaCost = finalCost;
		m_runtime.lastWasPowerAttack = a_isPowerAttack;
		m_runtime.lastError.clear();
		return true;
	}

	bool Manager::IsSupportedSpell(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}
		if (a_spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
			return false;
		}
		const auto type = a_spell->GetSpellType();
		return type == RE::MagicSystem::SpellType::kSpell ||
		       type == RE::MagicSystem::SpellType::kLesserPower ||
		       type == RE::MagicSystem::SpellType::kPower ||
		       type == RE::MagicSystem::SpellType::kVoicePower;
	}

	std::string Manager::GetKeyboardKeyName(std::uint32_t a_keyCode)
	{
		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		if (!inputMgr) {
			return std::format("Key {}", a_keyCode);
		}
		RE::BSFixedString name{};
		if (inputMgr->GetButtonNameFromID(RE::INPUT_DEVICE::kKeyboard, static_cast<std::int32_t>(a_keyCode), name)) {
			if (const auto* value = name.c_str(); value && value[0] != '\0') {
				return value;
			}
		}
		return std::format("Key {}", a_keyCode);
	}

	bool Manager::IsLikelyLeftHandAttack(RE::PlayerCharacter* a_player)
	{
		bool value = false;
		static constexpr std::array<std::string_view, 6> candidates{
			"IsLeftAttack",
			"IsLeftAttacking",
			"bLeftHandAttack",
			"bIsLeftAttack",
			"AttackLeft",
			"IsAttackingLeft"
		};

		for (const auto& candidate : candidates) {
			if (a_player->GetGraphVariableBool(RE::BSFixedString(candidate.data()), value) && value) {
				return true;
			}
		}
		return false;
	}

	std::string Manager::BuildSnapshotJsonLocked() const
	{
		json root{};
		root["config"] = {
			{ "enabled", m_config.enabled },
			{ "uiToggleKey", GetKeyboardKeyName(m_config.uiToggleKey) },
			{ "bindKey", GetKeyboardKeyName(m_config.bindKey) },
			{ "powerDamageScale", m_config.powerDamageScale },
			{ "powerMagickaScale", m_config.powerMagickaScale },
			{ "showHudNotifications", m_config.showHudNotifications }
		};

		auto current = m_runtime.rightWeapon.has_value() ? m_runtime.rightWeapon : (m_runtime.leftWeapon.has_value() ? m_runtime.leftWeapon : m_runtime.unarmedKey);
		if (current.has_value()) {
			auto it = m_bindings.find(*current);
			root["currentWeapon"] = {
				{ "displayName", current->displayName },
				{ "handSlot", static_cast<std::uint32_t>(current->handSlot) },
				{ "isUnarmed", current->isUnarmed },
				{ "hasBinding", it != m_bindings.end() }
			};
			if (it != m_bindings.end()) {
				root["currentWeapon"]["spellName"] = it->second.displayName;
				root["currentWeapon"]["spellKey"] = it->second.spellFormKey;
				root["currentWeapon"]["lastKnownCost"] = it->second.lastKnownCost;
			}
		}

		root["bindings"] = json::array();
		for (const auto& [key, spell] : m_bindings) {
			json item = {
				{ "key", {
					{ "pluginName", key.pluginName },
					{ "localFormID", key.localFormID },
					{ "uniqueID", key.uniqueID },
					{ "handSlot", static_cast<std::uint32_t>(key.handSlot) },
					{ "isUnarmed", key.isUnarmed },
					{ "displayName", key.displayName }
				} },
				{ "spell", {
					{ "displayName", spell.displayName },
					{ "spellFormKey", spell.spellFormKey },
					{ "lastKnownCost", spell.lastKnownCost }
				} }
			};
			root["bindings"].push_back(std::move(item));
		}

		root["status"] = {
			{ "lastTriggerWeapon", m_runtime.lastTriggerWeapon },
			{ "lastTriggerSpell", m_runtime.lastTriggerSpell },
			{ "lastMagickaCost", m_runtime.lastMagickaCost },
			{ "lastWasPowerAttack", m_runtime.lastWasPowerAttack },
			{ "lastError", m_runtime.lastError }
		};

		return root.dump();
	}

	void Manager::Notify(const std::string& a_message) const
	{
		if (!m_config.showHudNotifications) {
			return;
		}
		RE::SendHUDMessage::ShowHUDMessage(a_message.c_str());
	}

	void Manager::SetLastErrorLocked(std::string a_error)
	{
		m_runtime.lastError = std::move(a_error);
	}
}
