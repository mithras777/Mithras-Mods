#include "mastery_weapon/MasteryManager.h"

#include "plugin.h"
#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include "json/single_include/nlohmann/json.hpp"

namespace SBO::MASTERY_WEAPON
{
	namespace
	{
		using json = nlohmann::json;
		constexpr const char* kSharedConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";
		constexpr const char* kConfigSection = "masteryWeapon";

		constexpr std::uint32_t kSerializationVersion = 2;
		constexpr std::uint32_t kMasteryRecord = 'TSMM';

		void SetDefaultTypeTuning(MasteryConfig::WeaponTypeBonusConfig& a_cfg, RE::WEAPON_TYPE a_type)
		{
			a_cfg.enabled = true;
			a_cfg.tuning = {};

			switch (a_type) {
				case RE::WEAPON_TYPE::kOneHandDagger:
					a_cfg.tuning.attackSpeedPerLevel = 0.015f;
					a_cfg.tuning.damagePerLevel = 0.015f;
					a_cfg.tuning.critChancePerLevel = 0.007f;
					break;
				case RE::WEAPON_TYPE::kTwoHandSword:
				case RE::WEAPON_TYPE::kTwoHandAxe:
					a_cfg.tuning.damagePerLevel = 0.03f;
					a_cfg.tuning.attackSpeedPerLevel = 0.006f;
					a_cfg.tuning.damageCap = 0.50f;
					break;
				case RE::WEAPON_TYPE::kBow:
				case RE::WEAPON_TYPE::kCrossbow:
					a_cfg.tuning.critChancePerLevel = 0.008f;
					a_cfg.tuning.attackSpeedPerLevel = 0.008f;
					break;
				case RE::WEAPON_TYPE::kStaff:
					a_cfg.tuning.damagePerLevel = 0.01f;
					a_cfg.tuning.critChancePerLevel = 0.002f;
					a_cfg.tuning.attackSpeedPerLevel = 0.005f;
					break;
				default:
					break;
			}
		}

		void ClampTuning(MasteryConfig::BonusTuning& a_tuning)
		{
			a_tuning.damagePerLevel = std::clamp(a_tuning.damagePerLevel, 0.0f, 1.0f);
			a_tuning.attackSpeedPerLevel = std::clamp(a_tuning.attackSpeedPerLevel, 0.0f, 1.0f);
			a_tuning.critChancePerLevel = std::clamp(a_tuning.critChancePerLevel, 0.0f, 1.0f);
			a_tuning.powerAttackStaminaReductionPerLevel = std::clamp(a_tuning.powerAttackStaminaReductionPerLevel, 0.0f, 1.0f);
			a_tuning.blockStaminaReductionPerLevel = std::clamp(a_tuning.blockStaminaReductionPerLevel, 0.0f, 1.0f);
			a_tuning.damageCap = std::clamp(a_tuning.damageCap, 0.0f, 5.0f);
			a_tuning.attackSpeedCap = std::clamp(a_tuning.attackSpeedCap, 0.0f, 5.0f);
			a_tuning.critChanceCap = std::clamp(a_tuning.critChanceCap, 0.0f, 1.0f);
			a_tuning.powerAttackStaminaFloor = std::clamp(a_tuning.powerAttackStaminaFloor, 0.0f, 1.0f);
			a_tuning.blockStaminaFloor = std::clamp(a_tuning.blockStaminaFloor, 0.0f, 1.0f);
		}

		void ClampConfig(MasteryConfig& a_config)
		{
			a_config.gainMultiplier = std::max(0.1f, a_config.gainMultiplier);
			if (a_config.thresholds.empty()) {
				a_config.thresholds = { 10, 25, 50, 100, 200 };
			}
			for (auto& threshold : a_config.thresholds) {
				threshold = std::max<std::uint32_t>(1, threshold);
			}
			std::sort(a_config.thresholds.begin(), a_config.thresholds.end());
			a_config.thresholds.erase(std::unique(a_config.thresholds.begin(), a_config.thresholds.end()), a_config.thresholds.end());

			ClampTuning(a_config.generalBonuses);
			for (auto& typeCfg : a_config.weaponTypeBonuses) {
				ClampTuning(typeCfg.tuning);
			}
		}

		json ToJson(const MasteryConfig::BonusTuning& a_tuning)
		{
			return {
				{ "damagePerLevel", a_tuning.damagePerLevel },
				{ "attackSpeedPerLevel", a_tuning.attackSpeedPerLevel },
				{ "critChancePerLevel", a_tuning.critChancePerLevel },
				{ "powerAttackStaminaReductionPerLevel", a_tuning.powerAttackStaminaReductionPerLevel },
				{ "blockStaminaReductionPerLevel", a_tuning.blockStaminaReductionPerLevel },
				{ "damageCap", a_tuning.damageCap },
				{ "attackSpeedCap", a_tuning.attackSpeedCap },
				{ "critChanceCap", a_tuning.critChanceCap },
				{ "powerAttackStaminaFloor", a_tuning.powerAttackStaminaFloor },
				{ "blockStaminaFloor", a_tuning.blockStaminaFloor }
			};
		}

		void FromJson(const json& a_json, MasteryConfig::BonusTuning& a_tuning)
		{
			a_tuning.damagePerLevel = a_json.value("damagePerLevel", a_tuning.damagePerLevel);
			a_tuning.attackSpeedPerLevel = a_json.value("attackSpeedPerLevel", a_tuning.attackSpeedPerLevel);
			a_tuning.critChancePerLevel = a_json.value("critChancePerLevel", a_tuning.critChancePerLevel);
			a_tuning.powerAttackStaminaReductionPerLevel = a_json.value("powerAttackStaminaReductionPerLevel", a_tuning.powerAttackStaminaReductionPerLevel);
			a_tuning.blockStaminaReductionPerLevel = a_json.value("blockStaminaReductionPerLevel", a_tuning.blockStaminaReductionPerLevel);
			a_tuning.damageCap = a_json.value("damageCap", a_tuning.damageCap);
			a_tuning.attackSpeedCap = a_json.value("attackSpeedCap", a_tuning.attackSpeedCap);
			a_tuning.critChanceCap = a_json.value("critChanceCap", a_tuning.critChanceCap);
			a_tuning.powerAttackStaminaFloor = a_json.value("powerAttackStaminaFloor", a_tuning.powerAttackStaminaFloor);
			a_tuning.blockStaminaFloor = a_json.value("blockStaminaFloor", a_tuning.blockStaminaFloor);
		}

		json ToJson(const MasteryConfig& a_config)
		{
			json weaponTypes = json::array();
			for (std::size_t i = 0; i < a_config.weaponTypeBonuses.size(); ++i) {
				weaponTypes.push_back({
					{ "weaponType", static_cast<std::uint32_t>(i) },
					{ "enabled", a_config.weaponTypeBonuses[i].enabled },
					{ "tuning", ToJson(a_config.weaponTypeBonuses[i].tuning) }
				});
			}

			return {
				{ "enabled", a_config.enabled },
				{ "gainMultiplier", a_config.gainMultiplier },
				{ "thresholds", a_config.thresholds },
				{ "gainFromKills", a_config.gainFromKills },
				{ "gainFromHits", a_config.gainFromHits },
				{ "gainFromPowerHits", a_config.gainFromPowerHits },
				{ "gainFromBlocks", a_config.gainFromBlocks },
				{ "timeBasedLeveling", a_config.timeBasedLeveling },
				{ "generalBonuses", ToJson(a_config.generalBonuses) },
				{ "weaponTypeBonuses", weaponTypes }
			};
		}

		void FromJson(const json& a_json, MasteryConfig& a_config)
		{
			a_config.enabled = a_json.value("enabled", a_config.enabled);
			a_config.gainMultiplier = a_json.value("gainMultiplier", a_config.gainMultiplier);
			if (a_json.contains("thresholds") && a_json["thresholds"].is_array()) {
				a_config.thresholds = a_json["thresholds"].get<std::vector<std::uint32_t>>();
			}
			a_config.gainFromKills = a_json.value("gainFromKills", a_config.gainFromKills);
			a_config.gainFromHits = a_json.value("gainFromHits", a_config.gainFromHits);
			a_config.gainFromPowerHits = a_json.value("gainFromPowerHits", a_config.gainFromPowerHits);
			a_config.gainFromBlocks = a_json.value("gainFromBlocks", a_config.gainFromBlocks);
			a_config.timeBasedLeveling = a_json.value("timeBasedLeveling", a_config.timeBasedLeveling);

			if (a_json.contains("generalBonuses")) {
				FromJson(a_json["generalBonuses"], a_config.generalBonuses);
			}

			if (a_json.contains("weaponTypeBonuses") && a_json["weaponTypeBonuses"].is_array()) {
				for (const auto& entry : a_json["weaponTypeBonuses"]) {
					const auto index = entry.value("weaponType", static_cast<std::uint32_t>(kWeaponTypeCount));
					if (index >= kWeaponTypeCount) {
						continue;
					}
					a_config.weaponTypeBonuses[index].enabled = entry.value("enabled", a_config.weaponTypeBonuses[index].enabled);
					if (entry.contains("tuning")) {
						FromJson(entry["tuning"], a_config.weaponTypeBonuses[index].tuning);
					}
				}
			}

			ClampConfig(a_config);
		}
	}

	MasteryConfig Manager::DefaultConfig()
	{
		MasteryConfig config{};
		for (std::size_t i = 0; i < kWeaponTypeCount; ++i) {
			SetDefaultTypeTuning(config.weaponTypeBonuses[i], static_cast<RE::WEAPON_TYPE>(i));
		}
		ClampConfig(config);
		return config;
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = DefaultConfig();
		}
		LoadConfigFromJson();
		RefreshEquippedFromPlayer();
		ReapplyBonuses();
	}

	void Manager::OnRevert()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		std::scoped_lock lock(m_lock);
		m_bonusApplier.Clear(player);
		m_mastery.clear();
		m_weapon = {};
		m_shield = {};
	}

	void Manager::OnEquipEvent(const RE::TESEquipEvent& a_event)
	{
		auto* actor = a_event.actor.get();
		if (!actor || !actor->IsPlayerRef()) {
			return;
		}

		const auto* form = RE::TESForm::LookupByID(a_event.baseObject);
		if (!form) {
			return;
		}

		const bool isWeapon = IsTrackedWeapon(form);
		const bool isShield = IsShield(form);
		if (!isWeapon && !isShield) {
			return;
		}

		std::scoped_lock lock(m_lock);
		if (isShield) {
			std::string baseName;
			RE::WEAPON_TYPE weaponType = RE::WEAPON_TYPE::kHandToHandMelee; // Shields don't have weapon types
			if (const char* name = form->GetName(); name && name[0] != '\0') {
				baseName = ExtractBaseWeaponName(name);
			}
			ItemKey key{ baseName, a_event.uniqueID, weaponType, true, true };
			SetEquippedKeyLocked(m_shield, a_event.equipped ? std::optional<ItemKey>(key) : std::nullopt);
		} else {
			std::string baseName;
			RE::WEAPON_TYPE weaponType = RE::WEAPON_TYPE::kHandToHandMelee;
			if (const auto weaponTypeOpt = GetWeaponTypeFromForm(form); weaponTypeOpt.has_value()) {
				weaponType = *weaponTypeOpt;
			}
			if (const char* name = form->GetName(); name && name[0] != '\0') {
				baseName = ExtractBaseWeaponName(name);
			}
			ItemKey key{ baseName, a_event.uniqueID, weaponType, false, false };
			SetEquippedKeyLocked(m_weapon, a_event.equipped ? std::optional<ItemKey>(key) : std::nullopt);
		}
		ReapplyBonuses();
	}

	void Manager::OnDeathEvent(const RE::TESDeathEvent& a_event)
	{
		if (!a_event.dead) {
			return;
		}

		auto* killer = a_event.actorKiller.get();
		if (!killer || !killer->IsPlayerRef()) {
			return;
		}

		std::optional<ItemKey> weaponKey;
		MasteryStats updated{};
		bool leveledUp = false;

		{
			std::scoped_lock lock(m_lock);
			if (!m_config.enabled || !m_config.gainFromKills || !m_weapon.key.has_value()) {
				return;
			}

			UpdateEquippedTimeLocked(m_weapon);
			weaponKey = m_weapon.key;
			auto& stats = m_mastery[*weaponKey];

			const auto previousLevel = stats.level;
			const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(std::max(0.1f, m_config.gainMultiplier))));
			stats.kills += gain;
			RefreshLevelLocked(stats);
			updated = stats;
			leveledUp = stats.level > previousLevel;

			if (leveledUp) {
				ReapplyBonuses();
			}
		}

		if (leveledUp && weaponKey.has_value()) {
			const std::string message = std::format("Mithras: {} Mastery Level {}", GetItemName(*weaponKey), updated.level);
			RE::DebugNotification(message.c_str());
			LOG_INFO("{}", message);
		}
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc) {
			return;
		}

		std::scoped_lock lock(m_lock);

		if (!a_intfc->OpenRecord(kMasteryRecord, kSerializationVersion)) {
			return;
		}

		const std::uint32_t count = static_cast<std::uint32_t>(m_mastery.size());
		a_intfc->WriteRecordData(count);
		for (const auto& [key, stats] : m_mastery) {
			// Write baseName string
			const std::uint32_t nameLength = static_cast<std::uint32_t>(key.baseName.size());
			a_intfc->WriteRecordData(nameLength);
			for (char c : key.baseName) {
				a_intfc->WriteRecordData(c);
			}
			a_intfc->WriteRecordData(key.uniqueID);
			a_intfc->WriteRecordData(key.weaponType);
			a_intfc->WriteRecordData(key.leftHand);
			a_intfc->WriteRecordData(key.shield);
			a_intfc->WriteRecordData(stats.kills);
			a_intfc->WriteRecordData(stats.hits);
			a_intfc->WriteRecordData(stats.powerHits);
			a_intfc->WriteRecordData(stats.blocks);
			a_intfc->WriteRecordData(stats.secondsEquipped);
			a_intfc->WriteRecordData(stats.level);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		std::unordered_map<ItemKey, MasteryStats, ItemKeyHash> loadedMap;

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version != kSerializationVersion) {
				continue;
			}

			if (type == kMasteryRecord) {
				std::uint32_t count = 0;
				a_intfc->ReadRecordData(count);
				for (std::uint32_t i = 0; i < count; ++i) {
					ItemKey key{};
					MasteryStats stats{};
					// Read baseName string
					std::uint32_t nameLength = 0;
					a_intfc->ReadRecordData(nameLength);
					key.baseName.resize(nameLength);
					for (std::uint32_t j = 0; j < nameLength; ++j) {
						char c;
						a_intfc->ReadRecordData(c);
						key.baseName[j] = c;
					}
					a_intfc->ReadRecordData(key.uniqueID);
					a_intfc->ReadRecordData(key.weaponType);
					a_intfc->ReadRecordData(key.leftHand);
					a_intfc->ReadRecordData(key.shield);
					a_intfc->ReadRecordData(stats.kills);
					a_intfc->ReadRecordData(stats.hits);
					a_intfc->ReadRecordData(stats.powerHits);
					a_intfc->ReadRecordData(stats.blocks);
					a_intfc->ReadRecordData(stats.secondsEquipped);
					a_intfc->ReadRecordData(stats.level);
					loadedMap[key] = stats;
				}
			}
		}

		LoadConfigFromJson();
		std::scoped_lock lock(m_lock);
		m_mastery = std::move(loadedMap);
		RefreshEquippedFromPlayer();
		ReapplyBonuses();
	}

	MasteryConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetConfig(const MasteryConfig& a_config, bool a_writeJson)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			ClampConfig(m_config);
			ReapplyBonuses();
		}

		if (a_writeJson) {
			SaveConfigToJson();
		}
	}

	void Manager::ResetAllConfigToDefault(bool a_writeJson)
	{
		SetConfig(DefaultConfig(), a_writeJson);
	}

	std::optional<ItemKey> Manager::GetCurrentWeaponKey() const
	{
		std::scoped_lock lock(m_lock);
		return m_weapon.key;
	}

	std::optional<ItemKey> Manager::GetCurrentShieldKey() const
	{
		std::scoped_lock lock(m_lock);
		return m_shield.key;
	}

	std::string Manager::GetCurrentWeaponTypeName() const
	{
		std::scoped_lock lock(m_lock);
		const auto type = GetCurrentWeaponTypeLocked();
		if (!type.has_value()) {
			return "None";
		}
		return std::string(WeaponTypeName(*type));
	}

	MasteryStats Manager::GetStats(const ItemKey& a_key) const
	{
		std::scoped_lock lock(m_lock);
		auto it = m_mastery.find(a_key);
		if (it == m_mastery.end()) {
			return {};
		}

		MasteryStats stats = it->second;
		if (m_weapon.key.has_value() && *m_weapon.key == a_key) {
			const auto now = Clock::now();
			stats.secondsEquipped += std::chrono::duration<float>(now - m_weapon.equippedAt).count();
		}
		if (m_shield.key.has_value() && *m_shield.key == a_key) {
			const auto now = Clock::now();
			stats.secondsEquipped += std::chrono::duration<float>(now - m_shield.equippedAt).count();
		}
		return stats;
	}

	std::string Manager::GetItemName(const ItemKey& a_key) const
	{
		return a_key.baseName;
	}

	std::string Manager::ExtractBaseWeaponName(const std::string& a_fullName)
	{
		// Common enchantments and effects to strip
		static const std::vector<std::string> suffixes = {
			" of Fire", " of Frost", " of Shock", " of Poison", " of Absorb Health", " of Absorb Magicka", " of Absorb Stamina",
			" of Soul Trap", " of Fear", " of Silence", " of Paralysis", " of Banishment",
			" of Burning", " of Freezing", " of Shocking", " of Draining", " of Weakening",
			" of the Vampire", " of the Mage", " of the Warrior", " of the Thief",
			" of Flames", " of Ice", " of Storms", " of Venom",
			" of Health", " of Magicka", " of Stamina",
			" of Regeneration", " of Restoration", " of Fortification",
			" of Damage", " of Destruction", " of Alteration", " of Conjuration", " of Restoration", " of Illusion",
			" of the Crusader", " of the Sentinel", " of the Guardian", " of the Beast", " of the Dragon",
			" of the Sun", " of the Moon", " of the Stars", " of the Night",
			" of Arcane", " of Mystic", " of Ancient", " of Divine", " of Daedric"
		};

		std::string result = a_fullName;

		// Remove common suffixes
		for (const auto& suffix : suffixes) {
			if (result.size() > suffix.size() &&
				result.substr(result.size() - suffix.size()) == suffix) {
				result = result.substr(0, result.size() - suffix.size());
				break; // Only remove one suffix
			}
		}

		return result;
	}

	std::size_t Manager::GetDatabaseSize() const
	{
		std::scoped_lock lock(m_lock);
		return m_mastery.size();
	}

	void Manager::ResetEquippedItemMastery()
	{
		std::scoped_lock lock(m_lock);
		if (m_weapon.key.has_value()) {
			m_mastery.erase(*m_weapon.key);
		}
		if (m_shield.key.has_value()) {
			m_mastery.erase(*m_shield.key);
		}
		ReapplyBonuses();
	}

	void Manager::ClearDatabase()
	{
		std::scoped_lock lock(m_lock);
		m_mastery.clear();
		ReapplyBonuses();
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		MasteryConfig config;
		{
			std::scoped_lock lock(m_lock);
			config = m_config;
		}

		json root = json::object();
		if (std::ifstream in(path); in.is_open()) {
			try {
				in >> root;
			} catch (...) {
				root = json::object();
			}
		}

		json db = json::array();
		{
			std::scoped_lock lock(m_lock);
			for (const auto& [key, stats] : m_mastery) {
				db.push_back({
					{ "baseName", key.baseName },
					{ "uniqueID", key.uniqueID },
					{ "weaponType", static_cast<std::uint32_t>(key.weaponType) },
					{ "leftHand", key.leftHand },
					{ "shield", key.shield },
					{ "kills", stats.kills },
					{ "hits", stats.hits },
					{ "powerHits", stats.powerHits },
					{ "blocks", stats.blocks },
					{ "secondsEquipped", stats.secondsEquipped },
					{ "level", stats.level }
				});
			}
		}

		root[kConfigSection] = {
			{ "config", ToJson(config) },
			{ "db", std::move(db) }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("Failed to open config for write: {}", path.string());
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
			LOG_WARN("Failed to open config for read: {}", path.string());
			return;
		}

		json parsed = json::object();
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("Failed parsing JSON config ({}), rewriting defaults", e.what());
			ResetAllConfigToDefault(true);
			return;
		}

		MasteryConfig cfg = DefaultConfig();
		const auto section = parsed.contains(kConfigSection) && parsed[kConfigSection].is_object() ? parsed[kConfigSection] : parsed;
		const auto configNode = section.contains("config") && section["config"].is_object() ? section["config"] : section;
		FromJson(configNode, cfg);
		SetConfig(cfg, false);

		if (section.contains("db") && section["db"].is_array()) {
			std::unordered_map<ItemKey, MasteryStats, ItemKeyHash> loaded{};
			for (const auto& row : section["db"]) {
				if (!row.is_object()) {
					continue;
				}
				ItemKey key{};
				key.baseName = row.value("baseName", std::string{});
				key.uniqueID = row.value("uniqueID", static_cast<std::uint16_t>(0));
				key.weaponType = static_cast<RE::WEAPON_TYPE>(row.value("weaponType", 0u));
				key.leftHand = row.value("leftHand", false);
				key.shield = row.value("shield", false);
				if (key.baseName.empty()) {
					continue;
				}
				MasteryStats stats{};
				stats.kills = row.value("kills", 0u);
				stats.hits = row.value("hits", 0u);
				stats.powerHits = row.value("powerHits", 0u);
				stats.blocks = row.value("blocks", 0u);
				stats.secondsEquipped = row.value("secondsEquipped", 0.0f);
				stats.level = row.value("level", 0u);
				loaded.emplace(std::move(key), stats);
			}
			std::scoped_lock lock(m_lock);
			m_mastery = std::move(loaded);
			ReapplyBonuses();
		}
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		return std::filesystem::path(kSharedConfigPath);
	}

	void Manager::RefreshEquippedFromPlayer()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		const auto* rightEntry = player->GetEquippedEntryData(false);
		const auto* leftEntry = player->GetEquippedEntryData(true);

		std::optional<ItemKey> rightKey;
		std::optional<ItemKey> shieldKey;

		if (rightEntry && rightEntry->object && IsTrackedWeapon(rightEntry->object)) {
			std::uint16_t rightUniqueID = 0;
			std::string rightBaseName;
			RE::WEAPON_TYPE rightWeaponType = RE::WEAPON_TYPE::kHandToHandMelee;
			if (const auto weaponTypeOpt = GetWeaponTypeFromForm(rightEntry->object); weaponTypeOpt.has_value()) {
				rightWeaponType = *weaponTypeOpt;
			}
			if (const char* name = rightEntry->object->GetName(); name && name[0] != '\0') {
				rightBaseName = ExtractBaseWeaponName(name);
			}

			if (rightEntry->extraLists) {
				for (const auto& xList : *rightEntry->extraLists) {
					if (xList) {
						const auto* uniqueID = xList->GetByType<RE::ExtraUniqueID>();
						if (uniqueID) {
							rightUniqueID = uniqueID->uniqueID;
							break;
						}
					}
				}
			}
			rightKey = ItemKey{ rightBaseName, rightUniqueID, rightWeaponType, false, false };
		}
		if (leftEntry && leftEntry->object && IsShield(leftEntry->object)) {
			std::uint16_t leftUniqueID = 0;
			std::string leftBaseName;
			RE::WEAPON_TYPE leftWeaponType = RE::WEAPON_TYPE::kHandToHandMelee; // Shields don't have weapon types
			if (const char* name = leftEntry->object->GetName(); name && name[0] != '\0') {
				leftBaseName = ExtractBaseWeaponName(name);
			}

			if (leftEntry->extraLists) {
				for (const auto& xList : *leftEntry->extraLists) {
					if (xList) {
						const auto* uniqueID = xList->GetByType<RE::ExtraUniqueID>();
						if (uniqueID) {
							leftUniqueID = uniqueID->uniqueID;
							break;
						}
					}
				}
			}
			shieldKey = ItemKey{ leftBaseName, leftUniqueID, leftWeaponType, true, true };
		}

		SetEquippedKeyLocked(m_weapon, rightKey);
		SetEquippedKeyLocked(m_shield, shieldKey);
	}

	void Manager::ReapplyBonuses()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		if (!m_config.enabled || !m_weapon.key.has_value()) {
			m_bonusApplier.Clear(player);
			return;
		}

		UpdateEquippedTimeLocked(m_weapon);
		auto& weaponStats = m_mastery[*m_weapon.key];
		RefreshLevelLocked(weaponStats);
		const MasteryBonuses bonuses = ComputeBonusesLocked(weaponStats, false);
		m_bonusApplier.Apply(player, bonuses, m_shield.key.has_value());
	}

	void Manager::UpdateEquippedTimeLocked(EquippedState& a_slot)
	{
		if (!a_slot.key.has_value()) {
			return;
		}
		auto it = m_mastery.find(*a_slot.key);
		if (it == m_mastery.end()) {
			return;
		}

		const auto now = Clock::now();
		if (a_slot.equippedAt.time_since_epoch().count() != 0) {
			it->second.secondsEquipped += std::chrono::duration<float>(now - a_slot.equippedAt).count();
		}
		a_slot.equippedAt = now;
	}

	void Manager::SetEquippedKeyLocked(EquippedState& a_slot, std::optional<ItemKey> a_key)
	{
		UpdateEquippedTimeLocked(a_slot);
		a_slot.key = std::move(a_key);
		a_slot.equippedAt = Clock::now();
		if (a_slot.key.has_value()) {
			m_mastery.try_emplace(*a_slot.key, MasteryStats{});
		}
	}

	std::optional<RE::WEAPON_TYPE> Manager::GetCurrentWeaponTypeLocked() const
	{
		if (!m_weapon.key.has_value()) {
			return std::nullopt;
		}
		return m_weapon.key->weaponType;
	}

	void Manager::RefreshLevelLocked(MasteryStats& a_stats) const
	{
		std::uint32_t level = 0;
		for (const auto threshold : m_config.thresholds) {
			if (a_stats.kills >= threshold) {
				++level;
			}
		}
		a_stats.level = level;
	}

	MasteryBonuses Manager::ComputeBonusesLocked(const MasteryStats& a_stats, bool) const
	{
		MasteryBonuses bonuses{};
		const float level = static_cast<float>(a_stats.level);

		const MasteryConfig::BonusTuning* tuning = &m_config.generalBonuses;
		if (const auto type = GetCurrentWeaponTypeLocked(); type.has_value()) {
			const auto index = static_cast<std::size_t>(*type);
			if (index < m_config.weaponTypeBonuses.size() && m_config.weaponTypeBonuses[index].enabled) {
				tuning = &m_config.weaponTypeBonuses[index].tuning;
			}
		}

		bonuses.dmgMult = std::min(tuning->damageCap, tuning->damagePerLevel * level);
		bonuses.atkSpeed = std::min(tuning->attackSpeedCap, tuning->attackSpeedPerLevel * level);
		bonuses.critChance = std::min(tuning->critChanceCap, tuning->critChancePerLevel * level);
		bonuses.staminaPowerCostMult = std::max(tuning->powerAttackStaminaFloor, 1.0f - (tuning->powerAttackStaminaReductionPerLevel * level));
		bonuses.blockStaminaMult = std::max(tuning->blockStaminaFloor, 1.0f - (tuning->blockStaminaReductionPerLevel * level));
		return bonuses;
	}

	bool Manager::IsTrackedWeapon(const RE::TESForm* a_form) const
	{
		return a_form && a_form->Is(RE::FormType::Weapon);
	}

	bool Manager::IsShield(const RE::TESForm* a_form) const
	{
		const auto* armor = a_form ? a_form->As<RE::TESObjectARMO>() : nullptr;
		return armor && armor->IsShield();
	}
}
