#include "mastery_shout/MasteryManager.h"

#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <format>
#include "json/single_include/nlohmann/json.hpp"

namespace SBO::MASTERY_SHOUT
{
	namespace
	{
		using json = nlohmann::json;
		constexpr const char* kSharedConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";
		constexpr const char* kConfigSection = "masteryShout";

		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kMasteryRecord = 'SHMR';

		void ClampConfig(MasteryConfig& a_config)
		{
			a_config.gainMultiplier = std::clamp(a_config.gainMultiplier, 0.1f, 20.0f);
			if (a_config.thresholds.empty()) {
				a_config.thresholds = { 10, 25, 50, 100, 200 };
			}
			for (auto& threshold : a_config.thresholds) {
				threshold = std::max<std::uint32_t>(1, threshold);
			}
			std::sort(a_config.thresholds.begin(), a_config.thresholds.end());
			a_config.thresholds.erase(std::unique(a_config.thresholds.begin(), a_config.thresholds.end()), a_config.thresholds.end());

			a_config.bonuses.shoutRecoveryPerLevel = std::clamp(a_config.bonuses.shoutRecoveryPerLevel, -20.0f, 20.0f);
			a_config.bonuses.shoutRecoveryCap = std::clamp(a_config.bonuses.shoutRecoveryCap, -100.0f, 100.0f);
			a_config.bonuses.shoutPowerPerLevel = std::clamp(a_config.bonuses.shoutPowerPerLevel, 0.0f, 20.0f);
			a_config.bonuses.shoutPowerCap = std::clamp(a_config.bonuses.shoutPowerCap, 0.0f, 300.0f);
			a_config.bonuses.voiceRatePerLevel = std::clamp(a_config.bonuses.voiceRatePerLevel, 0.0f, 50.0f);
			a_config.bonuses.voiceRateCap = std::clamp(a_config.bonuses.voiceRateCap, 0.0f, 500.0f);
			a_config.bonuses.voicePointsPerLevel = std::clamp(a_config.bonuses.voicePointsPerLevel, 0.0f, 50.0f);
			a_config.bonuses.voicePointsCap = std::clamp(a_config.bonuses.voicePointsCap, 0.0f, 500.0f);
			a_config.bonuses.staminaRatePerLevel = std::clamp(a_config.bonuses.staminaRatePerLevel, 0.0f, 20.0f);
			a_config.bonuses.staminaRateCap = std::clamp(a_config.bonuses.staminaRateCap, 0.0f, 200.0f);
		}

		json ToJson(const MasteryConfig::BonusTuning& a_bonus)
		{
			return {
				{ "shoutRecoveryPerLevel", a_bonus.shoutRecoveryPerLevel },
				{ "shoutRecoveryCap", a_bonus.shoutRecoveryCap },
				{ "shoutPowerPerLevel", a_bonus.shoutPowerPerLevel },
				{ "shoutPowerCap", a_bonus.shoutPowerCap },
				{ "voiceRatePerLevel", a_bonus.voiceRatePerLevel },
				{ "voiceRateCap", a_bonus.voiceRateCap },
				{ "voicePointsPerLevel", a_bonus.voicePointsPerLevel },
				{ "voicePointsCap", a_bonus.voicePointsCap },
				{ "staminaRatePerLevel", a_bonus.staminaRatePerLevel },
				{ "staminaRateCap", a_bonus.staminaRateCap }
			};
		}

		void FromJson(const json& a_json, MasteryConfig::BonusTuning& a_bonus)
		{
			a_bonus.shoutRecoveryPerLevel = a_json.value("shoutRecoveryPerLevel", a_bonus.shoutRecoveryPerLevel);
			a_bonus.shoutRecoveryCap = a_json.value("shoutRecoveryCap", a_bonus.shoutRecoveryCap);
			a_bonus.shoutPowerPerLevel = a_json.value("shoutPowerPerLevel", a_bonus.shoutPowerPerLevel);
			a_bonus.shoutPowerCap = a_json.value("shoutPowerCap", a_bonus.shoutPowerCap);
			a_bonus.voiceRatePerLevel = a_json.value("voiceRatePerLevel", a_bonus.voiceRatePerLevel);
			a_bonus.voiceRateCap = a_json.value("voiceRateCap", a_bonus.voiceRateCap);
			a_bonus.voicePointsPerLevel = a_json.value("voicePointsPerLevel", a_bonus.voicePointsPerLevel);
			a_bonus.voicePointsCap = a_json.value("voicePointsCap", a_bonus.voicePointsCap);
			a_bonus.staminaRatePerLevel = a_json.value("staminaRatePerLevel", a_bonus.staminaRatePerLevel);
			a_bonus.staminaRateCap = a_json.value("staminaRateCap", a_bonus.staminaRateCap);
		}

		json ToJson(const MasteryConfig& a_config)
		{
			return {
				{ "enabled", a_config.enabled },
				{ "gainMultiplier", a_config.gainMultiplier },
				{ "gainFromUses", a_config.gainFromUses },
				{ "thresholds", a_config.thresholds },
				{ "bonuses", ToJson(a_config.bonuses) }
			};
		}

		void FromJson(const json& a_json, MasteryConfig& a_config)
		{
			a_config.enabled = a_json.value("enabled", a_config.enabled);
			a_config.gainMultiplier = a_json.value("gainMultiplier", a_config.gainMultiplier);
			a_config.gainFromUses = a_json.value("gainFromUses", a_config.gainFromUses);
			if (a_json.contains("thresholds") && a_json["thresholds"].is_array()) {
				a_config.thresholds = a_json["thresholds"].get<std::vector<std::uint32_t>>();
			}
			if (a_json.contains("bonuses")) {
				FromJson(a_json["bonuses"], a_config.bonuses);
			}
			ClampConfig(a_config);
		}
	}

	MasteryConfig Manager::DefaultConfig()
	{
		MasteryConfig cfg{};
		ClampConfig(cfg);
		return cfg;
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = DefaultConfig();
			m_mastery.clear();
			m_currentShout.reset();
		}
		LoadConfigFromJson();

		std::scoped_lock lock(m_lock);
		RefreshCurrentShoutLocked();
		ReapplyBonusesLocked();
	}

	void Manager::OnRevert()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		std::scoped_lock lock(m_lock);
		m_bonusApplier.Clear(player);
		m_mastery.clear();
		m_currentShout.reset();
	}

	void Manager::OnSpellCastEvent(const RE::TESSpellCastEvent& a_event)
	{
		const auto* caster = a_event.object.get();
		if (!caster || !caster->IsPlayerRef()) {
			return;
		}

		std::scoped_lock lock(m_lock);
		RefreshCurrentShoutLocked();
		const auto key = TryMapSpellToCurrentShoutLocked(a_event.spell);
		if (key.has_value()) {
			GainUseLocked(*key);
		}
	}

	void Manager::OnHitEvent(const RE::TESHitEvent& a_event)
	{
		const auto* aggressor = a_event.cause.get();
		auto* targetRef = a_event.target.get();
		if (!aggressor || !targetRef || !aggressor->IsPlayerRef()) {
			return;
		}

		auto* targetActor = targetRef->As<RE::Actor>();
		if (!targetActor || targetActor->IsDead()) {
			return;
		}

		float extraDamage = 0.0f;
		{
			std::scoped_lock lock(m_lock);
			const auto key = TryMapSpellToCurrentShoutLocked(a_event.source);
			if (!key.has_value()) {
				return;
			}

			auto it = m_mastery.find(*key);
			if (it == m_mastery.end()) {
				return;
			}

			const float level = static_cast<float>(it->second.level);
			const auto& tuning = m_config.bonuses;
			extraDamage = std::min(tuning.shoutPowerCap, tuning.shoutPowerPerLevel * level);
		}

		if (extraDamage <= 0.0f) {
			return;
		}

		targetActor->AsActorValueOwner()->DamageActorValue(RE::ActorValue::kHealth, extraDamage);
	}

	void Manager::OnShoutAttack(const RE::ShoutAttack::Event& a_event)
	{
		const auto key = BuildKey(a_event.shout);
		if (!key.has_value()) {
			return;
		}

		std::scoped_lock lock(m_lock);
		RefreshCurrentShoutLocked();
		GainUseLocked(*key);
	}

	void Manager::GainUseLocked(const ShoutKey& a_key)
	{
		if (!m_config.enabled || !m_config.gainFromUses) {
			return;
		}

		auto& stats = m_mastery[a_key];
		const auto before = stats.level;
		const std::uint32_t gain = std::max(1u, static_cast<std::uint32_t>(std::round(m_config.gainMultiplier)));
		stats.uses += gain;
		RefreshLevelLocked(stats);
		ReapplyBonusesLocked();

		if (stats.level > before) {
			const auto message = std::format("Mithras: {} Mastery Level {}", a_key.name, stats.level);
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
			std::uint32_t nameLen = static_cast<std::uint32_t>(key.name.size());
			a_intfc->WriteRecordData(nameLen);
			for (char c : key.name) {
				a_intfc->WriteRecordData(c);
			}
			a_intfc->WriteRecordData(stats.uses);
			a_intfc->WriteRecordData(stats.level);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		std::unordered_map<ShoutKey, MasteryStats, ShoutKeyHash> loadedMastery{};

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
					ShoutKey key{};
					MasteryStats stats{};
					a_intfc->ReadRecordData(key.formID);
					std::uint32_t nameLen = 0;
					a_intfc->ReadRecordData(nameLen);
					key.name.resize(nameLen);
					for (std::uint32_t j = 0; j < nameLen; ++j) {
						char c = '\0';
						a_intfc->ReadRecordData(c);
						key.name[j] = c;
					}
					a_intfc->ReadRecordData(stats.uses);
					a_intfc->ReadRecordData(stats.level);
					loadedMastery[key] = stats;
				}
			}
		}

		LoadConfigFromJson();
		std::scoped_lock lock(m_lock);
		m_mastery = std::move(loadedMastery);
		RefreshCurrentShoutLocked();
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

	std::optional<ShoutKey> Manager::GetCurrentShoutKey() const
	{
		std::scoped_lock lock(m_lock);
		return m_currentShout;
	}

	MasteryStats Manager::GetStats(const ShoutKey& a_key) const
	{
		std::scoped_lock lock(m_lock);
		auto it = m_mastery.find(a_key);
		if (it == m_mastery.end()) {
			return {};
		}
		return it->second;
	}

	void Manager::ResetAllConfigToDefault(bool a_writeJson)
	{
		SetConfig(DefaultConfig(), a_writeJson);
	}

	void Manager::ClearDatabase()
	{
		std::scoped_lock lock(m_lock);
		m_mastery.clear();
		ReapplyBonusesLocked();
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		MasteryConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
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
			{ "config", ToJson(cfg) }
		};

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("ShoutMastery: failed to open config for write: {}", path.string());
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
			LOG_WARN("ShoutMastery: failed to open config for read: {}", path.string());
			return;
		}

		json parsed = json::object();
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("ShoutMastery: failed parsing JSON config ({}), rewriting defaults", e.what());
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

	void Manager::RefreshCurrentShoutLocked()
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		m_currentShout = player ? BuildKey(player->GetCurrentShout()) : std::nullopt;
		if (m_currentShout.has_value()) {
			m_mastery.try_emplace(*m_currentShout, MasteryStats{});
		}
	}

	void Manager::ReapplyBonusesLocked()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		if (!m_config.enabled || !m_currentShout.has_value()) {
			m_bonusApplier.Clear(player);
			return;
		}

		m_bonusApplier.Apply(player, ComputeBonusesLocked());
	}

	void Manager::RefreshLevelLocked(MasteryStats& a_stats) const
	{
		std::uint32_t level = 0;
		for (const auto threshold : m_config.thresholds) {
			if (a_stats.uses >= threshold) {
				++level;
			}
		}
		a_stats.level = level;
	}

	MasteryBonuses Manager::ComputeBonusesLocked() const
	{
		MasteryBonuses bonuses{};
		if (!m_currentShout.has_value()) {
			return bonuses;
		}

		auto it = m_mastery.find(*m_currentShout);
		if (it == m_mastery.end()) {
			return bonuses;
		}

		const float level = static_cast<float>(it->second.level);
		const auto& tuning = m_config.bonuses;

		bonuses.shoutRecoveryMult = std::max(tuning.shoutRecoveryCap, tuning.shoutRecoveryPerLevel * level);
		bonuses.shoutPower = std::min(tuning.shoutPowerCap, tuning.shoutPowerPerLevel * level);
		bonuses.voiceRate = std::min(tuning.voiceRateCap, tuning.voiceRatePerLevel * level);
		bonuses.voicePoints = std::min(tuning.voicePointsCap, tuning.voicePointsPerLevel * level);
		bonuses.staminaRateMult = std::min(tuning.staminaRateCap, tuning.staminaRatePerLevel * level);
		return bonuses;
	}

	std::optional<ShoutKey> Manager::TryMapSpellToCurrentShoutLocked(RE::FormID a_spellFormID) const
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		const auto* shout = player ? player->GetCurrentShout() : nullptr;
		if (!shout) {
			return std::nullopt;
		}

		for (const auto& variation : shout->variations) {
			if (variation.spell && variation.spell->GetFormID() == a_spellFormID) {
				return BuildKey(shout);
			}
		}
		return std::nullopt;
	}
}
