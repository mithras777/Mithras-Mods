#include "mastery_spell/MasteryManager.h"

#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <format>
#include <nlohmann/json.hpp>

namespace SBO::MASTERY_SPELL
{
	namespace
	{
		using json = nlohmann::json;
		constexpr const char* kSharedConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";
		constexpr const char* kConfigSection = "masterySpell";

		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kMasteryRecord = 'SMMR';
		constexpr auto kKillCreditWindow = std::chrono::seconds(10);

		void ClampProgression(MasteryConfig::ProgressionTuning& a_tuning)
		{
			a_tuning.equipSecondsPerPoint = std::clamp(a_tuning.equipSecondsPerPoint, 0.1f, 600.0f);
		}

		void ClampBonuses(MasteryConfig::BonusTuning& a_tuning)
		{
			a_tuning.skillBonusPerLevel = std::clamp(a_tuning.skillBonusPerLevel, 0.0f, 20.0f);
			a_tuning.skillBonusCap = std::clamp(a_tuning.skillBonusCap, 0.0f, 200.0f);
			a_tuning.skillAdvancePerLevel = std::clamp(a_tuning.skillAdvancePerLevel, 0.0f, 30.0f);
			a_tuning.skillAdvanceCap = std::clamp(a_tuning.skillAdvanceCap, 0.0f, 300.0f);
			a_tuning.powerBonusPerLevel = std::clamp(a_tuning.powerBonusPerLevel, 0.0f, 20.0f);
			a_tuning.powerBonusCap = std::clamp(a_tuning.powerBonusCap, 0.0f, 200.0f);
			a_tuning.costReductionPerLevel = std::clamp(a_tuning.costReductionPerLevel, 0.0f, 20.0f);
			a_tuning.costReductionCap = std::clamp(a_tuning.costReductionCap, 0.0f, 100.0f);
			a_tuning.magickaRatePerLevel = std::clamp(a_tuning.magickaRatePerLevel, 0.0f, 20.0f);
			a_tuning.magickaRateCap = std::clamp(a_tuning.magickaRateCap, 0.0f, 100.0f);
			a_tuning.magickaFlatPerLevel = std::clamp(a_tuning.magickaFlatPerLevel, 0.0f, 50.0f);
			a_tuning.magickaFlatCap = std::clamp(a_tuning.magickaFlatCap, 0.0f, 1000.0f);
		}

		void ClampConfig(MasteryConfig& a_config)
		{
			static constexpr std::array<std::uint32_t, 5> kDefaultThresholds{ 10, 25, 50, 100, 200 };
			a_config.gainMultiplier = std::clamp(a_config.gainMultiplier, 0.1f, 20.0f);
			if (a_config.thresholds.empty()) {
				a_config.thresholds = { kDefaultThresholds.begin(), kDefaultThresholds.end() };
			}
			for (auto& threshold : a_config.thresholds) {
				threshold = std::max<std::uint32_t>(1, threshold);
			}
			std::sort(a_config.thresholds.begin(), a_config.thresholds.end());
			a_config.thresholds.erase(std::unique(a_config.thresholds.begin(), a_config.thresholds.end()), a_config.thresholds.end());
			if (a_config.thresholds.size() < kDefaultThresholds.size()) {
				for (const auto threshold : kDefaultThresholds) {
					a_config.thresholds.push_back(threshold);
				}
				std::sort(a_config.thresholds.begin(), a_config.thresholds.end());
				a_config.thresholds.erase(std::unique(a_config.thresholds.begin(), a_config.thresholds.end()), a_config.thresholds.end());
				while (a_config.thresholds.size() < kDefaultThresholds.size()) {
					const auto last = a_config.thresholds.back();
					a_config.thresholds.push_back(last + std::max(1u, last));
				}
			}

			ClampBonuses(a_config.generalBonuses);
			for (auto& schoolCfg : a_config.schools) {
				ClampProgression(schoolCfg.progression);
				ClampBonuses(schoolCfg.bonus);
			}
		}

		json ToJson(const MasteryConfig::BonusTuning& a_bonus)
		{
			return {
				{ "skillBonusPerLevel", a_bonus.skillBonusPerLevel },
				{ "skillBonusCap", a_bonus.skillBonusCap },
				{ "skillAdvancePerLevel", a_bonus.skillAdvancePerLevel },
				{ "skillAdvanceCap", a_bonus.skillAdvanceCap },
				{ "powerBonusPerLevel", a_bonus.powerBonusPerLevel },
				{ "powerBonusCap", a_bonus.powerBonusCap },
				{ "costReductionPerLevel", a_bonus.costReductionPerLevel },
				{ "costReductionCap", a_bonus.costReductionCap },
				{ "magickaRatePerLevel", a_bonus.magickaRatePerLevel },
				{ "magickaRateCap", a_bonus.magickaRateCap },
				{ "magickaFlatPerLevel", a_bonus.magickaFlatPerLevel },
				{ "magickaFlatCap", a_bonus.magickaFlatCap }
			};
		}

		void FromJson(const json& a_json, MasteryConfig::BonusTuning& a_bonus)
		{
			a_bonus.skillBonusPerLevel = a_json.value("skillBonusPerLevel", a_bonus.skillBonusPerLevel);
			a_bonus.skillBonusCap = a_json.value("skillBonusCap", a_bonus.skillBonusCap);
			a_bonus.skillAdvancePerLevel = a_json.value("skillAdvancePerLevel", a_bonus.skillAdvancePerLevel);
			a_bonus.skillAdvanceCap = a_json.value("skillAdvanceCap", a_bonus.skillAdvanceCap);
			a_bonus.powerBonusPerLevel = a_json.value("powerBonusPerLevel", a_bonus.powerBonusPerLevel);
			a_bonus.powerBonusCap = a_json.value("powerBonusCap", a_bonus.powerBonusCap);
			a_bonus.costReductionPerLevel = a_json.value("costReductionPerLevel", a_bonus.costReductionPerLevel);
			a_bonus.costReductionCap = a_json.value("costReductionCap", a_bonus.costReductionCap);
			a_bonus.magickaRatePerLevel = a_json.value("magickaRatePerLevel", a_bonus.magickaRatePerLevel);
			a_bonus.magickaRateCap = a_json.value("magickaRateCap", a_bonus.magickaRateCap);
			a_bonus.magickaFlatPerLevel = a_json.value("magickaFlatPerLevel", a_bonus.magickaFlatPerLevel);
			a_bonus.magickaFlatCap = a_json.value("magickaFlatCap", a_bonus.magickaFlatCap);
		}

		json ToJson(const MasteryConfig::ProgressionTuning& a_progression)
		{
			return {
				{ "gainFromKills", a_progression.gainFromKills },
				{ "gainFromUses", a_progression.gainFromUses },
				{ "gainFromSummons", a_progression.gainFromSummons },
				{ "gainFromHits", a_progression.gainFromHits },
				{ "gainFromEquipTime", a_progression.gainFromEquipTime },
				{ "equipSecondsPerPoint", a_progression.equipSecondsPerPoint }
			};
		}

		void FromJson(const json& a_json, MasteryConfig::ProgressionTuning& a_progression)
		{
			a_progression.gainFromKills = a_json.value("gainFromKills", a_progression.gainFromKills);
			a_progression.gainFromUses = a_json.value("gainFromUses", a_progression.gainFromUses);
			a_progression.gainFromSummons = a_json.value("gainFromSummons", a_progression.gainFromSummons);
			a_progression.gainFromHits = a_json.value("gainFromHits", a_progression.gainFromHits);
			a_progression.gainFromEquipTime = a_json.value("gainFromEquipTime", a_progression.gainFromEquipTime);
			a_progression.equipSecondsPerPoint = a_json.value("equipSecondsPerPoint", a_progression.equipSecondsPerPoint);
		}

		json ToJson(const MasteryConfig::SchoolConfig& a_school)
		{
			return {
				{ "progression", ToJson(a_school.progression) },
				{ "useBonusOverride", a_school.useBonusOverride },
				{ "bonus", ToJson(a_school.bonus) }
			};
		}

		void FromJson(const json& a_json, MasteryConfig::SchoolConfig& a_school)
		{
			if (a_json.contains("progression")) {
				FromJson(a_json["progression"], a_school.progression);
			}
			a_school.useBonusOverride = a_json.value("useBonusOverride", a_school.useBonusOverride);
			if (a_json.contains("bonus")) {
				FromJson(a_json["bonus"], a_school.bonus);
			}
		}

		json ToJson(const MasteryConfig& a_config)
		{
			json schools = json::array();
			for (std::size_t i = 0; i < kSchoolCount; ++i) {
				schools.push_back({
					{ "school", static_cast<std::uint32_t>(i) },
					{ "config", ToJson(a_config.schools[i]) }
				});
			}

			return {
				{ "enabled", a_config.enabled },
				{ "gainMultiplier", a_config.gainMultiplier },
				{ "thresholds", a_config.thresholds },
				{ "generalBonuses", ToJson(a_config.generalBonuses) },
				{ "schools", schools }
			};
		}

		void FromJson(const json& a_json, MasteryConfig& a_config)
		{
			a_config.enabled = a_json.value("enabled", a_config.enabled);
			a_config.gainMultiplier = a_json.value("gainMultiplier", a_config.gainMultiplier);
			if (a_json.contains("thresholds") && a_json["thresholds"].is_array()) {
				a_config.thresholds = a_json["thresholds"].get<std::vector<std::uint32_t>>();
			}

			if (a_json.contains("generalBonuses")) {
				FromJson(a_json["generalBonuses"], a_config.generalBonuses);
			}

			if (a_json.contains("schools") && a_json["schools"].is_array()) {
				for (const auto& schoolEntry : a_json["schools"]) {
					const auto index = schoolEntry.value("school", static_cast<std::uint32_t>(kSchoolCount));
					if (index >= kSchoolCount || !schoolEntry.contains("config")) {
						continue;
					}
					FromJson(schoolEntry["config"], a_config.schools[index]);
				}
			}

			ClampConfig(a_config);
		}
	}

	MasteryConfig Manager::DefaultConfig()
	{
		MasteryConfig cfg{};

		cfg.schools[static_cast<std::size_t>(SpellSchool::kDestruction)].progression.gainFromKills = true;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kConjuration)].progression.gainFromSummons = true;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kAlteration)].progression.gainFromUses = true;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kIllusion)].progression.gainFromUses = true;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kRestoration)].progression.gainFromEquipTime = true;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kRestoration)].progression.equipSecondsPerPoint = 3.0f;

		cfg.schools[static_cast<std::size_t>(SpellSchool::kDestruction)].bonus.powerBonusPerLevel = 1.5f;
		cfg.schools[static_cast<std::size_t>(SpellSchool::kDestruction)].bonus.powerBonusCap = 40.0f;

		ClampConfig(cfg);
		return cfg;
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = DefaultConfig();
			m_mastery.clear();
			m_leftSpell = {};
			m_rightSpell = {};
			m_lastCast = {};
		}

		LoadConfigFromJson();

		std::scoped_lock lock(m_lock);
		RefreshEquippedFromPlayerLocked();
		ReapplyBonusesLocked();
	}

	void Manager::OnRevert()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		std::scoped_lock lock(m_lock);
		m_bonusApplier.Clear(player);
		m_mastery.clear();
		m_leftSpell = {};
		m_rightSpell = {};
		m_lastCast = {};
	}

	void Manager::OnEquipEvent(const RE::TESEquipEvent& a_event)
	{
		const auto* actor = a_event.actor.get();
		if (!actor || !actor->IsPlayerRef()) {
			return;
		}

		std::scoped_lock lock(m_lock);
		RefreshEquippedFromPlayerLocked();
		ReapplyBonusesLocked();
	}

	void Manager::OnSpellCastEvent(const RE::TESSpellCastEvent& a_event)
	{
		const auto* caster = a_event.object.get();
		if (!caster || !caster->IsPlayerRef()) {
			return;
		}

		const auto* form = RE::TESForm::LookupByID(a_event.spell);
		const auto* spell = GetSpellFromForm(form);
		OnDirectSpellCast(spell);
	}

	void Manager::OnDirectSpellCast(const RE::SpellItem* a_spell)
	{
		const auto keyOpt = BuildSpellKey(a_spell);
		if (!keyOpt.has_value()) {
			return;
		}

		bool leveledUp = false;
		std::uint32_t level = 0;
		SpellKey key = *keyOpt;
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedFromPlayerLocked();
			auto& stats = m_mastery[key];
			const auto before = stats.level;

			const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(m_config.gainMultiplier)));
			stats.uses += gain;
			if (IsSummonLikeSpell(a_spell)) {
				stats.summons += gain;
			}

			RefreshLevelLocked(key, stats);
			leveledUp = stats.level > before;
			level = stats.level;

			m_lastCast[static_cast<std::size_t>(key.school)].key = key;
			m_lastCast[static_cast<std::size_t>(key.school)].castAt = Clock::now();
			ReapplyBonusesLocked();
		}

		if (leveledUp) {
			const std::string message = std::format("Mithras: {} ({}) Mastery Level {}", key.name, SchoolName(key.school), level);
			RE::DebugNotification(message.c_str());
			LOG_INFO("{}", message);
		}
	}

	void Manager::OnDeathEvent(const RE::TESDeathEvent& a_event)
	{
		if (!a_event.dead) {
			return;
		}

		const auto* killer = a_event.actorKiller.get();
		if (!killer || !killer->IsPlayerRef()) {
			return;
		}

		bool leveledUp = false;
		std::uint32_t level = 0;
		SpellKey leveledKey{};

		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedFromPlayerLocked();

			const auto now = Clock::now();
			std::optional<SpellKey> creditKey;
			const auto destructionIndex = static_cast<std::size_t>(SpellSchool::kDestruction);
			if (m_config.schools[destructionIndex].progression.gainFromKills &&
				m_lastCast[destructionIndex].key.has_value() &&
				now - m_lastCast[destructionIndex].castAt <= kKillCreditWindow) {
				creditKey = m_lastCast[destructionIndex].key;
			}

			if (!creditKey.has_value()) {
				for (const auto& state : { m_rightSpell, m_leftSpell }) {
					if (!state.key.has_value() || state.key->school != SpellSchool::kDestruction) {
						continue;
					}
					creditKey = state.key;
					break;
				}
			}

			if (!creditKey.has_value()) {
				return;
			}

			auto& stats = m_mastery[*creditKey];
			const auto before = stats.level;
			const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(m_config.gainMultiplier)));
			stats.kills += gain;
			RefreshLevelLocked(*creditKey, stats);
			leveledUp = stats.level > before;
			level = stats.level;
			leveledKey = *creditKey;
			ReapplyBonusesLocked();
		}

		if (leveledUp) {
			const std::string message = std::format("Mithras: {} ({}) Mastery Level {}", leveledKey.name, SchoolName(leveledKey.school), level);
			RE::DebugNotification(message.c_str());
			LOG_INFO("{}", message);
		}
	}

	void Manager::OnHitEvent(const RE::TESHitEvent& a_event)
	{
		const auto* aggressor = a_event.cause.get();
		if (!aggressor || !aggressor->IsPlayerRef()) {
			return;
		}

		const auto* sourceForm = RE::TESForm::LookupByID(a_event.source);
		const auto* spell = GetSpellFromForm(sourceForm);
		const auto keyOpt = BuildSpellKey(spell);
		if (!keyOpt.has_value()) {
			return;
		}

		bool leveledUp = false;
		std::uint32_t level = 0;
		SpellKey key = *keyOpt;
		{
			std::scoped_lock lock(m_lock);
			auto& stats = m_mastery[key];
			const auto before = stats.level;
			const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(m_config.gainMultiplier)));
			stats.hits += gain;
			RefreshLevelLocked(key, stats);
			leveledUp = stats.level > before;
			level = stats.level;
			ReapplyBonusesLocked();
		}

		if (leveledUp) {
			const std::string message = std::format("Mithras: {} ({}) Mastery Level {}", key.name, SchoolName(key.school), level);
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

		std::uint32_t count = static_cast<std::uint32_t>(m_mastery.size());
		a_intfc->WriteRecordData(count);
		for (const auto& [key, stats] : m_mastery) {
			a_intfc->WriteRecordData(key.formID);
			a_intfc->WriteRecordData(key.school);
			std::uint32_t nameLen = static_cast<std::uint32_t>(key.name.size());
			a_intfc->WriteRecordData(nameLen);
			for (char c : key.name) {
				a_intfc->WriteRecordData(c);
			}

			a_intfc->WriteRecordData(stats.kills);
			a_intfc->WriteRecordData(stats.uses);
			a_intfc->WriteRecordData(stats.summons);
			a_intfc->WriteRecordData(stats.hits);
			a_intfc->WriteRecordData(stats.equippedSeconds);
			a_intfc->WriteRecordData(stats.level);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		std::unordered_map<SpellKey, MasteryStats, SpellKeyHash> loadedMastery{};

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
					SpellKey key{};
					MasteryStats stats{};
					a_intfc->ReadRecordData(key.formID);
					a_intfc->ReadRecordData(key.school);

					std::uint32_t nameLen = 0;
					a_intfc->ReadRecordData(nameLen);
					key.name.resize(nameLen);
					for (std::uint32_t j = 0; j < nameLen; ++j) {
						char c = '\0';
						a_intfc->ReadRecordData(c);
						key.name[j] = c;
					}

					a_intfc->ReadRecordData(stats.kills);
					a_intfc->ReadRecordData(stats.uses);
					a_intfc->ReadRecordData(stats.summons);
					a_intfc->ReadRecordData(stats.hits);
					a_intfc->ReadRecordData(stats.equippedSeconds);
					a_intfc->ReadRecordData(stats.level);
					loadedMastery[key] = stats;
				}
			}
		}

		LoadConfigFromJson();
		std::scoped_lock lock(m_lock);
		m_mastery = std::move(loadedMastery);
		m_lastCast = {};
		RefreshEquippedFromPlayerLocked();
		ReapplyBonusesLocked();
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
			ReapplyBonusesLocked();
		}
		if (a_writeJson) {
			SaveConfigToJson();
		}
	}

	MasteryStats Manager::GetStats(const SpellKey& a_key) const
	{
		std::scoped_lock lock(m_lock);
		auto it = m_mastery.find(a_key);
		if (it == m_mastery.end()) {
			return {};
		}

		MasteryStats stats = it->second;
		const auto now = Clock::now();
		auto appendEquipped = [&](const EquippedState& a_slot) {
			if (!a_slot.key.has_value() || *a_slot.key != a_key) {
				return;
			}
			if (a_slot.equippedAt.time_since_epoch().count() == 0) {
				return;
			}
			stats.equippedSeconds += std::chrono::duration<float>(now - a_slot.equippedAt).count();
		};
		appendEquipped(m_leftSpell);
		appendEquipped(m_rightSpell);
		return stats;
	}

	std::uint32_t Manager::GetProgressCount(const SpellKey& a_key) const
	{
		std::scoped_lock lock(m_lock);
		auto it = m_mastery.find(a_key);
		if (it == m_mastery.end()) {
			return 0;
		}
		return ComputeProgressCountLocked(a_key, it->second);
	}

	std::vector<SpellKey> Manager::GetEquippedSpellKeys() const
	{
		std::scoped_lock lock(m_lock);
		std::vector<SpellKey> result;
		if (m_leftSpell.key.has_value()) {
			result.push_back(*m_leftSpell.key);
		}
		if (m_rightSpell.key.has_value() &&
			(!m_leftSpell.key.has_value() || *m_rightSpell.key != *m_leftSpell.key)) {
			result.push_back(*m_rightSpell.key);
		}
		return result;
	}

	void Manager::ResetAllConfigToDefault(bool a_writeJson)
	{
		SetConfig(DefaultConfig(), a_writeJson);
	}

	void Manager::ClearDatabase()
	{
		std::scoped_lock lock(m_lock);
		m_mastery.clear();
		m_lastCast = {};
		ReapplyBonusesLocked();
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		MasteryConfig config{};
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

		root[kConfigSection] = {
			{ "config", ToJson(config) }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("SpellMastery: failed to open config for write: {}", path.string());
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
			LOG_WARN("SpellMastery: failed to open config for read: {}", path.string());
			return;
		}

		json parsed = json::object();
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("SpellMastery: failed to parse JSON config ({}), rewriting defaults", e.what());
			ResetAllConfigToDefault(true);
			return;
		}

		MasteryConfig cfg = DefaultConfig();
		const auto section = parsed.contains(kConfigSection) && parsed[kConfigSection].is_object() ? parsed[kConfigSection] : parsed;
		const auto configNode = section.contains("config") && section["config"].is_object() ? section["config"] : section;
		FromJson(configNode, cfg);
		SetConfig(cfg, false);
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		return std::filesystem::path(kSharedConfigPath);
	}

	void Manager::RefreshEquippedFromPlayerLocked()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			SetEquippedKeyLocked(m_leftSpell, std::nullopt);
			SetEquippedKeyLocked(m_rightSpell, std::nullopt);
			return;
		}

		const auto* leftSpell = GetSpellFromForm(player->GetEquippedObject(true));
		const auto* rightSpell = GetSpellFromForm(player->GetEquippedObject(false));

		SetEquippedKeyLocked(m_leftSpell, BuildSpellKey(leftSpell));
		SetEquippedKeyLocked(m_rightSpell, BuildSpellKey(rightSpell));
	}

	void Manager::ReapplyBonusesLocked()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		if (!m_config.enabled) {
			m_bonusApplier.Clear(player);
			return;
		}

		m_bonusApplier.Apply(player, ComputeBonusesLocked());
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
			it->second.equippedSeconds += std::chrono::duration<float>(now - a_slot.equippedAt).count();
			RefreshLevelLocked(*a_slot.key, it->second);
		}
		a_slot.equippedAt = now;
	}

	void Manager::SetEquippedKeyLocked(EquippedState& a_slot, std::optional<SpellKey> a_key)
	{
		UpdateEquippedTimeLocked(a_slot);
		a_slot.key = std::move(a_key);
		a_slot.equippedAt = Clock::now();
		if (a_slot.key.has_value()) {
			m_mastery.try_emplace(*a_slot.key, MasteryStats{});
		}
	}

	void Manager::RefreshLevelLocked(const SpellKey& a_key, MasteryStats& a_stats) const
	{
		const auto progress = ComputeProgressCountLocked(a_key, a_stats);
		std::uint32_t level = 0;
		for (const auto threshold : m_config.thresholds) {
			if (progress >= threshold) {
				++level;
			}
		}
		a_stats.level = level;
	}

	std::uint32_t Manager::ComputeProgressCountLocked(const SpellKey& a_key, const MasteryStats& a_stats) const
	{
		const auto& progression = m_config.schools[static_cast<std::size_t>(a_key.school)].progression;
		float points = 0.0f;
		if (progression.gainFromKills) {
			points += static_cast<float>(a_stats.kills);
		}
		if (progression.gainFromUses) {
			points += static_cast<float>(a_stats.uses);
		}
		if (progression.gainFromSummons) {
			points += static_cast<float>(a_stats.summons);
		}
		if (progression.gainFromHits) {
			points += static_cast<float>(a_stats.hits);
		}
		if (progression.gainFromEquipTime) {
			points += a_stats.equippedSeconds / std::max(0.1f, progression.equipSecondsPerPoint);
		}

		points *= std::max(0.1f, m_config.gainMultiplier);
		return static_cast<std::uint32_t>(std::max(0.0f, std::floor(points)));
	}

	MasteryBonuses Manager::ComputeBonusesLocked() const
	{
		MasteryBonuses bonuses{};

		std::vector<SpellKey> equipped;
		if (m_leftSpell.key.has_value()) {
			equipped.push_back(*m_leftSpell.key);
		}
		if (m_rightSpell.key.has_value() &&
			(!m_leftSpell.key.has_value() || *m_rightSpell.key != *m_leftSpell.key)) {
			equipped.push_back(*m_rightSpell.key);
		}

		for (const auto& key : equipped) {
			auto it = m_mastery.find(key);
			if (it == m_mastery.end()) {
				continue;
			}

			const float level = static_cast<float>(it->second.level);
			const auto schoolIdx = static_cast<std::size_t>(key.school);
			const auto& schoolCfg = m_config.schools[schoolIdx];
			const auto& tuning = schoolCfg.useBonusOverride ? schoolCfg.bonus : m_config.generalBonuses;

			bonuses.schoolSkillBonus[schoolIdx] += std::min(tuning.skillBonusCap, tuning.skillBonusPerLevel * level);
			bonuses.schoolSkillAdvanceBonus[schoolIdx] += std::min(tuning.skillAdvanceCap, tuning.skillAdvancePerLevel * level);
			bonuses.schoolPowerBonus[schoolIdx] += std::min(tuning.powerBonusCap, tuning.powerBonusPerLevel * level);
			bonuses.schoolCostReduction[schoolIdx] += std::min(tuning.costReductionCap, tuning.costReductionPerLevel * level);
			bonuses.magickaRateMult += std::min(tuning.magickaRateCap, tuning.magickaRatePerLevel * level);
			bonuses.magickaFlat += std::min(tuning.magickaFlatCap, tuning.magickaFlatPerLevel * level);
		}

		bonuses.magickaRateMult = std::clamp(bonuses.magickaRateMult, 0.0f, 100.0f);
		return bonuses;
	}

	std::optional<SpellKey> Manager::BuildSpellKey(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return std::nullopt;
		}

		const auto school = ActorValueToSchool(a_spell->GetAssociatedSkill());
		if (!school.has_value()) {
			return std::nullopt;
		}

		SpellKey key{};
		key.formID = a_spell->GetFormID();
		key.school = *school;
		if (const auto* name = a_spell->GetName(); name && name[0] != '\0') {
			key.name = name;
		} else {
			key.name = std::format("Spell {:08X}", key.formID);
		}
		return key;
	}

	bool Manager::IsTrackedSpellForm(const RE::TESForm* a_form)
	{
		return BuildSpellKey(GetSpellFromForm(a_form)).has_value();
	}
}
