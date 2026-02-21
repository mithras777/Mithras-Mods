#include "mastery/MasteryManager.h"

#include "plugin.h"
#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace MITHRAS::MASTERY
{
	namespace
	{
		using json = nlohmann::json;

		constexpr std::uint32_t kSerializationVersion = 2;
		constexpr std::uint32_t kConfigRecord = 'GFCM';
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
			ItemKey key{ a_event.baseObject, a_event.uniqueID, true, true };
			SetEquippedKeyLocked(m_shield, a_event.equipped ? std::optional<ItemKey>(key) : std::nullopt);
		} else {
			ItemKey key{ a_event.baseObject, a_event.uniqueID, false, false };
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

		if (a_intfc->OpenRecord(kConfigRecord, kSerializationVersion)) {
			a_intfc->WriteRecordData(m_config.enabled);
			a_intfc->WriteRecordData(m_config.gainMultiplier);
			std::uint32_t thresholdCount = static_cast<std::uint32_t>(m_config.thresholds.size());
			a_intfc->WriteRecordData(thresholdCount);
			for (const auto threshold : m_config.thresholds) {
				a_intfc->WriteRecordData(threshold);
			}
			a_intfc->WriteRecordData(m_config.gainFromKills);
			a_intfc->WriteRecordData(m_config.gainFromHits);
			a_intfc->WriteRecordData(m_config.gainFromPowerHits);
			a_intfc->WriteRecordData(m_config.gainFromBlocks);
			a_intfc->WriteRecordData(m_config.timeBasedLeveling);

			a_intfc->WriteRecordData(m_config.generalBonuses.damagePerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.attackSpeedPerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.critChancePerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.powerAttackStaminaReductionPerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.blockStaminaReductionPerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.damageCap);
			a_intfc->WriteRecordData(m_config.generalBonuses.attackSpeedCap);
			a_intfc->WriteRecordData(m_config.generalBonuses.critChanceCap);
			a_intfc->WriteRecordData(m_config.generalBonuses.powerAttackStaminaFloor);
			a_intfc->WriteRecordData(m_config.generalBonuses.blockStaminaFloor);

			for (const auto& typeCfg : m_config.weaponTypeBonuses) {
				a_intfc->WriteRecordData(typeCfg.enabled);
				a_intfc->WriteRecordData(typeCfg.tuning.damagePerLevel);
				a_intfc->WriteRecordData(typeCfg.tuning.attackSpeedPerLevel);
				a_intfc->WriteRecordData(typeCfg.tuning.critChancePerLevel);
				a_intfc->WriteRecordData(typeCfg.tuning.powerAttackStaminaReductionPerLevel);
				a_intfc->WriteRecordData(typeCfg.tuning.blockStaminaReductionPerLevel);
				a_intfc->WriteRecordData(typeCfg.tuning.damageCap);
				a_intfc->WriteRecordData(typeCfg.tuning.attackSpeedCap);
				a_intfc->WriteRecordData(typeCfg.tuning.critChanceCap);
				a_intfc->WriteRecordData(typeCfg.tuning.powerAttackStaminaFloor);
				a_intfc->WriteRecordData(typeCfg.tuning.blockStaminaFloor);
			}
		}

		if (!a_intfc->OpenRecord(kMasteryRecord, kSerializationVersion)) {
			return;
		}

		const std::uint32_t count = static_cast<std::uint32_t>(m_mastery.size());
		a_intfc->WriteRecordData(count);
		for (const auto& [key, stats] : m_mastery) {
			a_intfc->WriteRecordData(key.baseFormID);
			a_intfc->WriteRecordData(key.uniqueID);
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

		MasteryConfig loadedConfig = DefaultConfig();
		std::unordered_map<ItemKey, MasteryStats, ItemKeyHash> loadedMap;

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version != kSerializationVersion) {
				continue;
			}

			if (type == kConfigRecord) {
				a_intfc->ReadRecordData(loadedConfig.enabled);
				a_intfc->ReadRecordData(loadedConfig.gainMultiplier);
				std::uint32_t thresholdCount = 0;
				a_intfc->ReadRecordData(thresholdCount);
				loadedConfig.thresholds.clear();
				loadedConfig.thresholds.reserve(thresholdCount);
				for (std::uint32_t i = 0; i < thresholdCount; ++i) {
					std::uint32_t threshold = 0;
					a_intfc->ReadRecordData(threshold);
					loadedConfig.thresholds.push_back(threshold);
				}
				a_intfc->ReadRecordData(loadedConfig.gainFromKills);
				a_intfc->ReadRecordData(loadedConfig.gainFromHits);
				a_intfc->ReadRecordData(loadedConfig.gainFromPowerHits);
				a_intfc->ReadRecordData(loadedConfig.gainFromBlocks);
				a_intfc->ReadRecordData(loadedConfig.timeBasedLeveling);

				a_intfc->ReadRecordData(loadedConfig.generalBonuses.damagePerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.attackSpeedPerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.critChancePerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.powerAttackStaminaReductionPerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.blockStaminaReductionPerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.damageCap);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.attackSpeedCap);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.critChanceCap);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.powerAttackStaminaFloor);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.blockStaminaFloor);

				for (auto& typeCfg : loadedConfig.weaponTypeBonuses) {
					a_intfc->ReadRecordData(typeCfg.enabled);
					a_intfc->ReadRecordData(typeCfg.tuning.damagePerLevel);
					a_intfc->ReadRecordData(typeCfg.tuning.attackSpeedPerLevel);
					a_intfc->ReadRecordData(typeCfg.tuning.critChancePerLevel);
					a_intfc->ReadRecordData(typeCfg.tuning.powerAttackStaminaReductionPerLevel);
					a_intfc->ReadRecordData(typeCfg.tuning.blockStaminaReductionPerLevel);
					a_intfc->ReadRecordData(typeCfg.tuning.damageCap);
					a_intfc->ReadRecordData(typeCfg.tuning.attackSpeedCap);
					a_intfc->ReadRecordData(typeCfg.tuning.critChanceCap);
					a_intfc->ReadRecordData(typeCfg.tuning.powerAttackStaminaFloor);
					a_intfc->ReadRecordData(typeCfg.tuning.blockStaminaFloor);
				}
			} else if (type == kMasteryRecord) {
				std::uint32_t count = 0;
				a_intfc->ReadRecordData(count);
				for (std::uint32_t i = 0; i < count; ++i) {
					ItemKey key{};
					MasteryStats stats{};
					a_intfc->ReadRecordData(key.baseFormID);
					a_intfc->ReadRecordData(key.uniqueID);
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

		ClampConfig(loadedConfig);
		std::scoped_lock lock(m_lock);
		m_config = loadedConfig;
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
		if (const auto* form = RE::TESForm::LookupByID(a_key.baseFormID)) {
			if (const char* name = form->GetName(); name && name[0] != '\0') {
				return name;
			}
		}
		return std::format("0x{:08X}", a_key.baseFormID);
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

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("Failed to open config for write: {}", path.string());
			return;
		}

		file << ToJson(config).dump(2);
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

		json parsed;
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("Failed parsing JSON config ({}), rewriting defaults", e.what());
			ResetAllConfigToDefault(true);
			return;
		}

		MasteryConfig cfg = DefaultConfig();
		FromJson(parsed, cfg);
		SetConfig(cfg, false);
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		auto base = std::filesystem::path(DLLMAIN::Plugin::GetSingleton()->Info().gameDirectory);
		if (base.empty()) {
			base = std::filesystem::current_path();
		}
		return base / "Data" / "SKSE" / "Plugins" / "WeaponMasterySKSE.json";
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
			rightKey = ItemKey{ rightEntry->object->GetFormID(), rightUniqueID, false, false };
		}
		if (leftEntry && leftEntry->object && IsShield(leftEntry->object)) {
			std::uint16_t leftUniqueID = 0;
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
			shieldKey = ItemKey{ leftEntry->object->GetFormID(), leftUniqueID, true, true };
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
		const auto* form = RE::TESForm::LookupByID(m_weapon.key->baseFormID);
		return GetWeaponTypeFromForm(form);
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
