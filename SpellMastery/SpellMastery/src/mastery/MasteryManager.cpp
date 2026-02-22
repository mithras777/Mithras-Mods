#include "mastery/MasteryManager.h"

#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace MITHRAS::SPELL_MASTERY
{
	namespace
	{
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kConfigRecord = 'SMCF';
		constexpr std::uint32_t kMasteryRecord = 'SMMR';

		void ClampTuning(MasteryConfig::BonusTuning& a_tuning)
		{
			a_tuning.skillBonusPerLevel = std::clamp(a_tuning.skillBonusPerLevel, 0.0f, 100.0f);
			a_tuning.skillBonusCap = std::clamp(a_tuning.skillBonusCap, 0.0f, 100.0f);
			a_tuning.magickaRatePerLevel = std::clamp(a_tuning.magickaRatePerLevel, 0.0f, 100.0f);
			a_tuning.magickaRateCap = std::clamp(a_tuning.magickaRateCap, 0.0f, 100.0f);
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
			for (auto& schoolCfg : a_config.schoolBonuses) {
				ClampTuning(schoolCfg.tuning);
			}
		}
	}

	MasteryConfig Manager::DefaultConfig()
	{
		MasteryConfig config{};
		ClampConfig(config);
		return config;
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = DefaultConfig();
			m_mastery = {};
			m_activeSchools = {};
		}
		LoadConfigFromJson();

		std::scoped_lock lock(m_lock);
		RefreshActiveSchoolsLocked();
		ReapplyBonusesLocked();
	}

	void Manager::OnRevert()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		std::scoped_lock lock(m_lock);
		m_bonusApplier.Clear(player);
		m_mastery = {};
		m_activeSchools = {};
	}

	void Manager::OnEquipEvent(const RE::TESEquipEvent& a_event)
	{
		const auto* actor = a_event.actor.get();
		if (!actor || !actor->IsPlayerRef()) {
			return;
		}

		const auto* form = RE::TESForm::LookupByID(a_event.baseObject);
		if (form && !IsTrackedSpellForm(form)) {
			return;
		}

		std::scoped_lock lock(m_lock);
		RefreshActiveSchoolsLocked();
		ReapplyBonusesLocked();
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

		std::vector<std::pair<SpellSchool, std::uint32_t>> levelUps;
		{
			std::scoped_lock lock(m_lock);
			if (!m_config.enabled || !m_config.gainFromKills) {
				return;
			}

			const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(std::max(0.1f, m_config.gainMultiplier))));
			for (std::size_t i = 0; i < kSchoolCount; ++i) {
				if (!m_activeSchools[i]) {
					continue;
				}

				auto& stats = m_mastery[i];
				const auto before = stats.level;
				stats.kills += gain;
				RefreshLevelLocked(stats);
				if (stats.level > before) {
					levelUps.emplace_back(static_cast<SpellSchool>(i), stats.level);
				}
			}

			if (!levelUps.empty()) {
				ReapplyBonusesLocked();
			}
		}

		for (const auto& [school, level] : levelUps) {
			const std::string message = std::format("Mithras: {} Spell Mastery Level {}", SchoolName(school), level);
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

			a_intfc->WriteRecordData(m_config.generalBonuses.skillBonusPerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.skillBonusCap);
			a_intfc->WriteRecordData(m_config.generalBonuses.magickaRatePerLevel);
			a_intfc->WriteRecordData(m_config.generalBonuses.magickaRateCap);

			for (const auto& schoolCfg : m_config.schoolBonuses) {
				a_intfc->WriteRecordData(schoolCfg.enabled);
				a_intfc->WriteRecordData(schoolCfg.tuning.skillBonusPerLevel);
				a_intfc->WriteRecordData(schoolCfg.tuning.skillBonusCap);
				a_intfc->WriteRecordData(schoolCfg.tuning.magickaRatePerLevel);
				a_intfc->WriteRecordData(schoolCfg.tuning.magickaRateCap);
			}
		}

		if (!a_intfc->OpenRecord(kMasteryRecord, kSerializationVersion)) {
			return;
		}

		for (const auto& stats : m_mastery) {
			a_intfc->WriteRecordData(stats.kills);
			a_intfc->WriteRecordData(stats.level);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		MasteryConfig loadedConfig = DefaultConfig();
		std::array<MasteryStats, kSchoolCount> loadedStats{};

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

				a_intfc->ReadRecordData(loadedConfig.generalBonuses.skillBonusPerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.skillBonusCap);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.magickaRatePerLevel);
				a_intfc->ReadRecordData(loadedConfig.generalBonuses.magickaRateCap);

				for (auto& schoolCfg : loadedConfig.schoolBonuses) {
					a_intfc->ReadRecordData(schoolCfg.enabled);
					a_intfc->ReadRecordData(schoolCfg.tuning.skillBonusPerLevel);
					a_intfc->ReadRecordData(schoolCfg.tuning.skillBonusCap);
					a_intfc->ReadRecordData(schoolCfg.tuning.magickaRatePerLevel);
					a_intfc->ReadRecordData(schoolCfg.tuning.magickaRateCap);
				}
			} else if (type == kMasteryRecord) {
				for (auto& stats : loadedStats) {
					a_intfc->ReadRecordData(stats.kills);
					a_intfc->ReadRecordData(stats.level);
				}
			}
		}

		ClampConfig(loadedConfig);
		std::scoped_lock lock(m_lock);
		m_config = loadedConfig;
		m_mastery = loadedStats;
		RefreshActiveSchoolsLocked();
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

	MasteryStats Manager::GetStats(SpellSchool a_school) const
	{
		std::scoped_lock lock(m_lock);
		return m_mastery[static_cast<std::size_t>(a_school)];
	}

	bool Manager::IsSchoolActive(SpellSchool a_school) const
	{
		std::scoped_lock lock(m_lock);
		return m_activeSchools[static_cast<std::size_t>(a_school)];
	}

	void Manager::ResetAllConfigToDefault(bool a_writeJson)
	{
		SetConfig(DefaultConfig(), a_writeJson);
	}

	void Manager::ClearDatabase()
	{
		std::scoped_lock lock(m_lock);
		m_mastery = {};
		ReapplyBonusesLocked();
	}

	void Manager::SaveConfigToJson() const
	{
		// Config is currently persisted through SKSE serialization records.
	}

	void Manager::LoadConfigFromJson()
	{
		// Reserved for future external config support.
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"SpellMastery.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "SpellMastery.json";
	}

	void Manager::RefreshActiveSchoolsLocked()
	{
		m_activeSchools = {};

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		for (const bool leftHand : { false, true }) {
			const auto* equipped = player->GetEquippedObject(leftHand);
			const auto school = GetSpellSchoolFromForm(equipped);
			if (!school.has_value()) {
				continue;
			}
			m_activeSchools[static_cast<std::size_t>(*school)] = true;
		}
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

	MasteryBonuses Manager::ComputeBonusesLocked() const
	{
		MasteryBonuses bonuses{};
		for (std::size_t i = 0; i < kSchoolCount; ++i) {
			if (!m_activeSchools[i]) {
				continue;
			}

			const auto level = static_cast<float>(m_mastery[i].level);
			const auto& schoolCfg = m_config.schoolBonuses[i];
			const auto& tuning = schoolCfg.enabled ? schoolCfg.tuning : m_config.generalBonuses;

			bonuses.schoolSkillBonus[i] = std::min(tuning.skillBonusCap, tuning.skillBonusPerLevel * level);
			bonuses.magickaRateMult += std::min(tuning.magickaRateCap, tuning.magickaRatePerLevel * level);
		}

		bonuses.magickaRateMult = std::min(100.0f, bonuses.magickaRateMult);
		return bonuses;
	}

	bool Manager::IsTrackedSpellForm(const RE::TESForm* a_form)
	{
		return GetSpellSchoolFromForm(a_form).has_value();
	}
}
