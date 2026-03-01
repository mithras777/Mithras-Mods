#include "buff/QuickBuffManager.h"

#include "event/GameEventManager.h"
#include "util/LogUtil.h"
#include "RE/C/CrosshairPickData.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>
#include <nlohmann/json.hpp>
#include <system_error>

namespace QUICK_BUFF
{
	namespace
	{
		using json = nlohmann::json;

		constexpr std::uint32_t kConfigVersion = 1;
		constexpr std::uint32_t kSerializationVersion = 1;
		constexpr std::uint32_t kConfigRecord = 'QBCF';

		std::string Trim(std::string_view a_text)
		{
			const auto begin = a_text.find_first_not_of(" \t\r\n");
			if (begin == std::string_view::npos) {
				return {};
			}
			const auto end = a_text.find_last_not_of(" \t\r\n");
			return std::string(a_text.substr(begin, end - begin + 1));
		}

		float Clamp01(float a_value)
		{
			return std::clamp(a_value, 0.0f, 1.0f);
		}
	}

	const TriggerConfig& Config::Get(TriggerID a_id) const
	{
		switch (a_id) {
			case TriggerID::kCombatStart:
				return combatStart;
			case TriggerID::kCombatEnd:
				return combatEnd;
			case TriggerID::kHealthBelow70:
				return healthBelow70;
			case TriggerID::kHealthBelow50:
				return healthBelow50;
			case TriggerID::kHealthBelow30:
				return healthBelow30;
			case TriggerID::kCrouchStart:
				return crouchStart;
			case TriggerID::kSprintStart:
				return sprintStart;
			case TriggerID::kWeaponDraw:
				return weaponDraw;
			case TriggerID::kPowerAttackStart:
				return powerAttackStart;
			case TriggerID::kShoutStart:
				return shoutStart;
			case TriggerID::kTotal:
			default:
				return combatStart;
		}
	}

	TriggerConfig& Config::Get(TriggerID a_id)
	{
		return const_cast<TriggerConfig&>(std::as_const(*this).Get(a_id));
	}

	void Manager::Initialize()
	{
		m_config = DefaultConfig();
		LoadConfig();
		ClampConfig(m_config);
		ResetRuntime();
	}

	void Manager::ResetRuntime()
	{
		m_runtime = RuntimeState{};
		m_pendingConcentration.clear();
		m_pendingConcentration.shrink_to_fit();
	}

	void Manager::ResetConfigToDefaults(bool a_saveToDisk)
	{
		m_config = DefaultConfig();
		ClampConfig(m_config);
		(void)a_saveToDisk;
		ResetRuntime();
	}

	Config Manager::GetConfig() const
	{
		return m_config;
	}

	void Manager::SetConfig(const Config& a_config, bool a_saveToDisk)
	{
		m_config = a_config;
		ClampConfig(m_config);
		(void)a_saveToDisk;
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc) {
			return;
		}
		if (!a_intfc->OpenRecord(kConfigRecord, kSerializationVersion)) {
			return;
		}

		const auto writeString = [a_intfc](const std::string& a_value) {
			const std::uint32_t length = static_cast<std::uint32_t>(a_value.size());
			a_intfc->WriteRecordData(length);
			for (const auto ch : a_value) {
				a_intfc->WriteRecordData(ch);
			}
		};

		const auto writeTrigger = [a_intfc, &writeString](const TriggerConfig& a_trigger) {
			a_intfc->WriteRecordData(a_trigger.enabled);
			writeString(a_trigger.spellFormID);
			a_intfc->WriteRecordData(a_trigger.cooldownSec);
			a_intfc->WriteRecordData(a_trigger.conditions.firstPersonOnly);
			a_intfc->WriteRecordData(a_trigger.conditions.weaponDrawnOnly);
			a_intfc->WriteRecordData(a_trigger.conditions.requireInCombat);
			a_intfc->WriteRecordData(a_trigger.conditions.requireOutOfCombat);
			a_intfc->WriteRecordData(a_trigger.rules.requireMagickaPercentAbove);
			a_intfc->WriteRecordData(a_trigger.rules.skipIfEffectAlreadyActive);
			a_intfc->WriteRecordData(a_trigger.thresholdHealthPercent);
			a_intfc->WriteRecordData(a_trigger.hysteresisMargin);
			a_intfc->WriteRecordData(a_trigger.internalLockoutSec);
			a_intfc->WriteRecordData(a_trigger.concentrationDurationSec);
		};

		a_intfc->WriteRecordData(m_config.version);
		a_intfc->WriteRecordData(m_config.global.enabled);
		a_intfc->WriteRecordData(m_config.global.firstPersonOnlyDefault);
		a_intfc->WriteRecordData(m_config.global.weaponDrawnOnlyDefault);
		a_intfc->WriteRecordData(m_config.global.preventCastingInMenus);
		a_intfc->WriteRecordData(m_config.global.preventCastingWhileStaggered);
		a_intfc->WriteRecordData(m_config.global.preventCastingWhileRagdoll);
		a_intfc->WriteRecordData(m_config.global.minTimeAfterLoadSeconds);

		writeTrigger(m_config.combatStart);
		writeTrigger(m_config.combatEnd);
		writeTrigger(m_config.healthBelow70);
		writeTrigger(m_config.healthBelow50);
		writeTrigger(m_config.healthBelow30);
		writeTrigger(m_config.crouchStart);
		writeTrigger(m_config.sprintStart);
		writeTrigger(m_config.weaponDraw);
		writeTrigger(m_config.powerAttackStart);
		writeTrigger(m_config.shoutStart);
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		auto loadedConfig = m_config;
		bool loaded = false;

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kConfigRecord || version != kSerializationVersion) {
				continue;
			}

			const auto readString = [a_intfc](std::string& a_value) {
				std::uint32_t length = 0;
				a_intfc->ReadRecordData(length);
				a_value.clear();
				a_value.reserve(length);
				for (std::uint32_t i = 0; i < length; ++i) {
					char ch = '\0';
					a_intfc->ReadRecordData(ch);
					a_value.push_back(ch);
				}
			};

			const auto readTrigger = [a_intfc, &readString](TriggerConfig& a_trigger) {
				a_intfc->ReadRecordData(a_trigger.enabled);
				readString(a_trigger.spellFormID);
				a_intfc->ReadRecordData(a_trigger.cooldownSec);
				a_intfc->ReadRecordData(a_trigger.conditions.firstPersonOnly);
				a_intfc->ReadRecordData(a_trigger.conditions.weaponDrawnOnly);
				a_intfc->ReadRecordData(a_trigger.conditions.requireInCombat);
				a_intfc->ReadRecordData(a_trigger.conditions.requireOutOfCombat);
				a_intfc->ReadRecordData(a_trigger.rules.requireMagickaPercentAbove);
				a_intfc->ReadRecordData(a_trigger.rules.skipIfEffectAlreadyActive);
				a_intfc->ReadRecordData(a_trigger.thresholdHealthPercent);
				a_intfc->ReadRecordData(a_trigger.hysteresisMargin);
				a_intfc->ReadRecordData(a_trigger.internalLockoutSec);
				a_intfc->ReadRecordData(a_trigger.concentrationDurationSec);
			};

			a_intfc->ReadRecordData(loadedConfig.version);
			a_intfc->ReadRecordData(loadedConfig.global.enabled);
			a_intfc->ReadRecordData(loadedConfig.global.firstPersonOnlyDefault);
			a_intfc->ReadRecordData(loadedConfig.global.weaponDrawnOnlyDefault);
			a_intfc->ReadRecordData(loadedConfig.global.preventCastingInMenus);
			a_intfc->ReadRecordData(loadedConfig.global.preventCastingWhileStaggered);
			a_intfc->ReadRecordData(loadedConfig.global.preventCastingWhileRagdoll);
			a_intfc->ReadRecordData(loadedConfig.global.minTimeAfterLoadSeconds);

			readTrigger(loadedConfig.combatStart);
			readTrigger(loadedConfig.combatEnd);
			readTrigger(loadedConfig.healthBelow70);
			readTrigger(loadedConfig.healthBelow50);
			readTrigger(loadedConfig.healthBelow30);
			readTrigger(loadedConfig.crouchStart);
			readTrigger(loadedConfig.sprintStart);
			readTrigger(loadedConfig.weaponDraw);
			readTrigger(loadedConfig.powerAttackStart);
			readTrigger(loadedConfig.shoutStart);

			loaded = true;
		}

		if (loaded) {
			ClampConfig(loadedConfig);
			m_config = std::move(loadedConfig);
		}
		ResetRuntime();
	}

	void Manager::OnRevert()
	{
		m_config = DefaultConfig();
		LoadConfig();
		ClampConfig(m_config);
		ResetRuntime();
	}

	Config Manager::GetDefaultConfig()
	{
		return DefaultConfig();
	}

	void Manager::Update(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		if (!a_player || !a_player->IsPlayerRef()) {
			return;
		}

		if (a_deltaTime <= 0.0f) {
			return;
		}

		UpdateTimers(a_deltaTime);
		m_runtime.timeSinceLoad += a_deltaTime;

		const auto live = BuildLiveState(a_player);
		if (!m_runtime.initialized) {
			InitializeRuntimeFrom(live);
		}

		if (!m_config.global.enabled || a_player->IsDead() || ShouldSkipGlobally(live)) {
			SyncPreviousStates(live);
			return;
		}

		if (m_runtime.timeSinceLoad < m_config.global.minTimeAfterLoadSeconds) {
			SyncPreviousStates(live);
			return;
		}

		UpdateConcentration(a_player, a_deltaTime, live);

		if (!m_runtime.wasInCombat && live.inCombat) {
			(void)AttemptTrigger(a_player, TriggerID::kCombatStart, live, false);
		}
		if (m_runtime.wasInCombat && !live.inCombat) {
			(void)AttemptTrigger(a_player, TriggerID::kCombatEnd, live, false);
		}
		if (!m_runtime.wasSneaking && live.sneaking) {
			(void)AttemptTrigger(a_player, TriggerID::kCrouchStart, live, false);
		}
		if (!m_runtime.wasSprinting && live.sprinting) {
			(void)AttemptTrigger(a_player, TriggerID::kSprintStart, live, false);
		}
		if (!m_runtime.wasWeaponDrawn && live.weaponDrawn) {
			(void)AttemptTrigger(a_player, TriggerID::kWeaponDraw, live, false);
		}

		const auto processHealthThreshold = [&](std::size_t a_index, TriggerID a_id) {
			const auto& healthCfg = m_config.Get(a_id);
			const float threshold = healthCfg.thresholdHealthPercent;
			const float margin = healthCfg.hysteresisMargin;
			if (m_runtime.healthBelowArmed[a_index] &&
				m_runtime.lastHealthPercent >= threshold &&
				live.healthPercent < threshold) {
				(void)AttemptTrigger(a_player, a_id, live, false);
				m_runtime.healthBelowArmed[a_index] = false;
			}
			if (live.healthPercent > threshold + margin) {
				m_runtime.healthBelowArmed[a_index] = true;
			}
		};

		processHealthThreshold(0, TriggerID::kHealthBelow70);
		processHealthThreshold(1, TriggerID::kHealthBelow50);
		processHealthThreshold(2, TriggerID::kHealthBelow30);

		if (!m_runtime.wasPowerAttacking && live.powerAttacking && m_runtime.powerAttackLockout <= 0.0f) {
			(void)AttemptTrigger(a_player, TriggerID::kPowerAttackStart, live, false);
			m_runtime.powerAttackLockout = m_config.powerAttackStart.internalLockoutSec;
		}

		if (!m_runtime.wasShouting && live.shouting && m_runtime.shoutLockout <= 0.0f) {
			(void)AttemptTrigger(a_player, TriggerID::kShoutStart, live, false);
			m_runtime.shoutLockout = m_config.shoutStart.internalLockoutSec;
		}

		SyncPreviousStates(live);
	}

	bool Manager::TryTestCast(TriggerID a_id)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		const auto live = BuildLiveState(player);
		if (!m_config.global.enabled || player->IsDead() || ShouldSkipGlobally(live)) {
			return false;
		}

		return AttemptTrigger(player, a_id, live, true);
	}

	std::vector<KnownSpellOption> Manager::GetKnownSpells() const
	{
		std::vector<KnownSpellOption> out;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return out;
		}

		struct Visitor final : RE::Actor::ForEachSpellVisitor
		{
			explicit Visitor(std::vector<KnownSpellOption>& a_out) :
				out(a_out)
			{}

			RE::BSContainer::ForEachResult Visit(RE::SpellItem* a_spell) override
			{
				if (!a_spell) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				const auto spellType = a_spell->GetSpellType();
				if (spellType != RE::MagicSystem::SpellType::kSpell &&
					spellType != RE::MagicSystem::SpellType::kLesserPower &&
					spellType != RE::MagicSystem::SpellType::kPower) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				const auto key = Manager::BuildFormKey(a_spell);
				if (key.empty()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				KnownSpellOption opt;
				opt.name = a_spell->GetName();
				if (opt.name.empty()) {
					opt.name = key;
				}
				opt.formKey = key;
				opt.formID = a_spell->GetFormID();
				out.push_back(std::move(opt));
				return RE::BSContainer::ForEachResult::kContinue;
			}

			std::vector<KnownSpellOption>& out;
		} visitor(out);

		player->VisitSpells(visitor);
		std::sort(out.begin(), out.end(), [](const KnownSpellOption& a_lhs, const KnownSpellOption& a_rhs) {
			if (a_lhs.name == a_rhs.name) {
				return a_lhs.formID < a_rhs.formID;
			}
			return a_lhs.name < a_rhs.name;
		});
		return out;
	}

	std::string_view Manager::GetTriggerName(TriggerID a_id)
	{
		switch (a_id) {
			case TriggerID::kCombatStart:
				return "combatStart";
			case TriggerID::kCombatEnd:
				return "combatEnd";
			case TriggerID::kHealthBelow70:
				return "healthBelow70";
			case TriggerID::kHealthBelow50:
				return "healthBelow50";
			case TriggerID::kHealthBelow30:
				return "healthBelow30";
			case TriggerID::kCrouchStart:
				return "crouchStart";
			case TriggerID::kSprintStart:
				return "sprintStart";
			case TriggerID::kWeaponDraw:
				return "weaponDraw";
			case TriggerID::kPowerAttackStart:
				return "powerAttackStart";
			case TriggerID::kShoutStart:
				return "shoutStart";
			case TriggerID::kTotal:
			default:
				return "unknown";
		}
	}

	bool Manager::IsSpellConcentration(std::string_view a_formKey) const
	{
		auto* spell = ResolveSpell(a_formKey);
		return spell && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
	}

	Config Manager::DefaultConfig()
	{
		Config cfg{};
		cfg.version = kConfigVersion;

		cfg.combatStart.cooldownSec = 10.0f;
		cfg.combatStart.conditions.requireInCombat = true;

		cfg.combatEnd.cooldownSec = 10.0f;
		cfg.combatEnd.conditions.requireOutOfCombat = true;

		cfg.healthBelow70.cooldownSec = 15.0f;
		cfg.healthBelow70.thresholdHealthPercent = 0.70f;
		cfg.healthBelow70.hysteresisMargin = 0.05f;

		cfg.healthBelow50.cooldownSec = 15.0f;
		cfg.healthBelow50.thresholdHealthPercent = 0.50f;
		cfg.healthBelow50.hysteresisMargin = 0.05f;

		cfg.healthBelow30.cooldownSec = 15.0f;
		cfg.healthBelow30.thresholdHealthPercent = 0.30f;
		cfg.healthBelow30.hysteresisMargin = 0.05f;

		cfg.crouchStart.cooldownSec = 5.0f;
		cfg.sprintStart.cooldownSec = 5.0f;
		cfg.weaponDraw.cooldownSec = 5.0f;

		cfg.powerAttackStart.cooldownSec = 3.0f;
		cfg.powerAttackStart.internalLockoutSec = 0.30f;

		cfg.shoutStart.cooldownSec = 5.0f;
		cfg.shoutStart.internalLockoutSec = 0.30f;

		return cfg;
	}

	void Manager::ApplyGlobalDefaults(Config& a_config) const
	{
		const auto apply = [&a_config](TriggerConfig& a_trigger) {
			a_trigger.conditions.firstPersonOnly = a_config.global.firstPersonOnlyDefault;
			a_trigger.conditions.weaponDrawnOnly = a_config.global.weaponDrawnOnlyDefault;
		};

		apply(a_config.combatStart);
		apply(a_config.combatEnd);
		apply(a_config.healthBelow70);
		apply(a_config.healthBelow50);
		apply(a_config.healthBelow30);
		apply(a_config.crouchStart);
		apply(a_config.sprintStart);
		apply(a_config.weaponDraw);
		apply(a_config.powerAttackStart);
		apply(a_config.shoutStart);
	}

	void Manager::LoadConfig()
	{
		std::ifstream ifs(std::string(kConfigPath), std::ios::in);
		if (!ifs.is_open()) {
			SaveConfig();
			return;
		}

		json root;
		try {
			ifs >> root;
		} catch (const std::exception&) {
			LOG_WARN("QuickBuff: invalid JSON, using defaults");
			return;
		}

		Config cfg = DefaultConfig();
		if (root.contains("version") && root["version"].is_number_unsigned()) {
			cfg.version = root["version"].get<std::uint32_t>();
		}

		if (root.contains("global") && root["global"].is_object()) {
			const auto& g = root["global"];
			cfg.global.enabled = g.value("enabled", cfg.global.enabled);
			cfg.global.firstPersonOnlyDefault = g.value("firstPersonOnlyDefault", cfg.global.firstPersonOnlyDefault);
			cfg.global.weaponDrawnOnlyDefault = g.value("weaponDrawnOnlyDefault", cfg.global.weaponDrawnOnlyDefault);
			cfg.global.preventCastingInMenus = g.value("preventCastingInMenus", cfg.global.preventCastingInMenus);
			cfg.global.preventCastingWhileStaggered = g.value("preventCastingWhileStaggered", cfg.global.preventCastingWhileStaggered);
			cfg.global.preventCastingWhileRagdoll = g.value("preventCastingWhileRagdoll", cfg.global.preventCastingWhileRagdoll);
			cfg.global.minTimeAfterLoadSeconds = g.value("minTimeAfterLoadSeconds", cfg.global.minTimeAfterLoadSeconds);
		}

		ApplyGlobalDefaults(cfg);

		const auto parseTrigger = [](const json& a_node, TriggerConfig& a_cfg) {
			a_cfg.enabled = a_node.value("enabled", a_cfg.enabled);
			a_cfg.spellFormID = a_node.value("spellFormID", a_cfg.spellFormID);
			a_cfg.cooldownSec = a_node.value("cooldownSec", a_cfg.cooldownSec);
			a_cfg.thresholdHealthPercent = a_node.value("thresholdHealthPercent", a_cfg.thresholdHealthPercent);
			a_cfg.hysteresisMargin = a_node.value("hysteresisMargin", a_cfg.hysteresisMargin);
			a_cfg.internalLockoutSec = a_node.value("internalLockoutSec", a_cfg.internalLockoutSec);
			a_cfg.concentrationDurationSec = a_node.value("concentrationDurationSec", a_cfg.concentrationDurationSec);

			if (a_node.contains("conditions") && a_node["conditions"].is_object()) {
				const auto& c = a_node["conditions"];
				a_cfg.conditions.firstPersonOnly = c.value("firstPersonOnly", a_cfg.conditions.firstPersonOnly);
				a_cfg.conditions.weaponDrawnOnly = c.value("weaponDrawnOnly", a_cfg.conditions.weaponDrawnOnly);
				a_cfg.conditions.requireInCombat = c.value("requireInCombat", a_cfg.conditions.requireInCombat);
				a_cfg.conditions.requireOutOfCombat = c.value("requireOutOfCombat", a_cfg.conditions.requireOutOfCombat);
			}

			if (a_node.contains("rules") && a_node["rules"].is_object()) {
				const auto& r = a_node["rules"];
				a_cfg.rules.requireMagickaPercentAbove = r.value("requireMagickaPercentAbove", a_cfg.rules.requireMagickaPercentAbove);
				a_cfg.rules.skipIfEffectAlreadyActive = true;
			}
		};

		if (root.contains("triggers") && root["triggers"].is_object()) {
			const auto& t = root["triggers"];
			if (t.contains("combatStart")) {
				parseTrigger(t["combatStart"], cfg.combatStart);
			}
			if (t.contains("combatEnd")) {
				parseTrigger(t["combatEnd"], cfg.combatEnd);
			}
			if (t.contains("healthBelow70")) {
				parseTrigger(t["healthBelow70"], cfg.healthBelow70);
			}
			if (t.contains("healthBelow50")) {
				parseTrigger(t["healthBelow50"], cfg.healthBelow50);
			}
			if (t.contains("healthBelow30")) {
				parseTrigger(t["healthBelow30"], cfg.healthBelow30);
			}
			if (t.contains("healthBelow")) {
				parseTrigger(t["healthBelow"], cfg.healthBelow50);
			}
			if (t.contains("crouchStart")) {
				parseTrigger(t["crouchStart"], cfg.crouchStart);
			}
			if (t.contains("sprintStart")) {
				parseTrigger(t["sprintStart"], cfg.sprintStart);
			}
			if (t.contains("weaponDraw")) {
				parseTrigger(t["weaponDraw"], cfg.weaponDraw);
			}
			if (t.contains("powerAttackStart")) {
				parseTrigger(t["powerAttackStart"], cfg.powerAttackStart);
			}
			if (t.contains("shoutStart")) {
				parseTrigger(t["shoutStart"], cfg.shoutStart);
			}
		}

		ClampConfig(cfg);
		m_config = cfg;
	}

	void Manager::SaveConfig() const
	{
		const auto serializeTrigger = [](const TriggerConfig& a_cfg) {
			return json{
				{ "enabled", a_cfg.enabled },
				{ "spellFormID", a_cfg.spellFormID },
				{ "cooldownSec", a_cfg.cooldownSec },
				{ "thresholdHealthPercent", a_cfg.thresholdHealthPercent },
				{ "hysteresisMargin", a_cfg.hysteresisMargin },
				{ "internalLockoutSec", a_cfg.internalLockoutSec },
				{ "concentrationDurationSec", a_cfg.concentrationDurationSec },
				{ "conditions", {
					{ "firstPersonOnly", a_cfg.conditions.firstPersonOnly },
					{ "weaponDrawnOnly", a_cfg.conditions.weaponDrawnOnly },
					{ "requireInCombat", a_cfg.conditions.requireInCombat },
					{ "requireOutOfCombat", a_cfg.conditions.requireOutOfCombat }
				} },
				{ "rules", {
					{ "requireMagickaPercentAbove", a_cfg.rules.requireMagickaPercentAbove },
					{ "skipIfEffectAlreadyActive", a_cfg.rules.skipIfEffectAlreadyActive }
				} }
			};
		};

		const json root = {
			{ "version", m_config.version },
			{ "global", {
				{ "enabled", m_config.global.enabled },
				{ "firstPersonOnlyDefault", m_config.global.firstPersonOnlyDefault },
				{ "weaponDrawnOnlyDefault", m_config.global.weaponDrawnOnlyDefault },
				{ "preventCastingInMenus", m_config.global.preventCastingInMenus },
				{ "preventCastingWhileStaggered", m_config.global.preventCastingWhileStaggered },
				{ "preventCastingWhileRagdoll", m_config.global.preventCastingWhileRagdoll },
				{ "minTimeAfterLoadSeconds", m_config.global.minTimeAfterLoadSeconds }
			} },
			{ "triggers", {
				{ "combatStart", serializeTrigger(m_config.combatStart) },
				{ "combatEnd", serializeTrigger(m_config.combatEnd) },
				{ "healthBelow70", serializeTrigger(m_config.healthBelow70) },
				{ "healthBelow50", serializeTrigger(m_config.healthBelow50) },
				{ "healthBelow30", serializeTrigger(m_config.healthBelow30) },
				{ "crouchStart", serializeTrigger(m_config.crouchStart) },
				{ "sprintStart", serializeTrigger(m_config.sprintStart) },
				{ "weaponDraw", serializeTrigger(m_config.weaponDraw) },
				{ "powerAttackStart", serializeTrigger(m_config.powerAttackStart) },
				{ "shoutStart", serializeTrigger(m_config.shoutStart) }
			} }
		};

		try {
			const std::filesystem::path path{ std::string(kConfigPath) };
			std::filesystem::create_directories(path.parent_path());
			std::ofstream ofs(path, std::ios::out | std::ios::trunc);
			ofs << root.dump(2);
		} catch (const std::exception&) {
			LOG_WARN("QuickBuff: failed to save config");
		}
	}

	void Manager::ClampConfig(Config& a_config) const
	{
		a_config.version = kConfigVersion;
		a_config.global.minTimeAfterLoadSeconds = std::clamp(a_config.global.minTimeAfterLoadSeconds, 0.0f, 30.0f);

		const auto clampTrigger = [](TriggerConfig& a_cfg) {
			a_cfg.cooldownSec = std::clamp(a_cfg.cooldownSec, 0.0f, 120.0f);
			a_cfg.thresholdHealthPercent = std::clamp(a_cfg.thresholdHealthPercent, 0.01f, 0.99f);
			a_cfg.hysteresisMargin = std::clamp(a_cfg.hysteresisMargin, 0.0f, 0.20f);
			a_cfg.internalLockoutSec = std::clamp(a_cfg.internalLockoutSec, 0.0f, 1.0f);
			a_cfg.concentrationDurationSec = std::clamp(a_cfg.concentrationDurationSec, 1.0f, 10.0f);
			a_cfg.rules.requireMagickaPercentAbove = Clamp01(a_cfg.rules.requireMagickaPercentAbove);
			a_cfg.rules.skipIfEffectAlreadyActive = true;
		};

		clampTrigger(a_config.combatStart);
		clampTrigger(a_config.combatEnd);
		clampTrigger(a_config.healthBelow70);
		clampTrigger(a_config.healthBelow50);
		clampTrigger(a_config.healthBelow30);
		clampTrigger(a_config.crouchStart);
		clampTrigger(a_config.sprintStart);
		clampTrigger(a_config.weaponDraw);
		clampTrigger(a_config.powerAttackStart);
		clampTrigger(a_config.shoutStart);

		a_config.healthBelow70.thresholdHealthPercent = 0.70f;
		a_config.healthBelow50.thresholdHealthPercent = 0.50f;
		a_config.healthBelow30.thresholdHealthPercent = 0.30f;
	}

	bool Manager::ShouldSkipGlobally(const LiveState& a_liveState) const
	{
		if (m_config.global.preventCastingInMenus && GAME_EVENT::Manager::GetSingleton()->IsBlockingMenuOpen()) {
			return true;
		}
		if (m_config.global.preventCastingWhileStaggered && a_liveState.staggered) {
			return true;
		}
		if (m_config.global.preventCastingWhileRagdoll && a_liveState.ragdoll) {
			return true;
		}
		return false;
	}

	Manager::LiveState Manager::BuildLiveState(RE::PlayerCharacter* a_player) const
	{
		LiveState state{};
		auto* camera = RE::PlayerCamera::GetSingleton();
		state.firstPerson = camera && camera->IsInFirstPerson();
		state.inCombat = a_player->IsInCombat();
		state.powerAttacking = a_player->IsPowerAttacking();
		state.staggered = a_player->IsStaggering();
		state.ragdoll = a_player->IsInRagdollState();

		if (const auto* actorState = a_player->AsActorState()) {
			state.sneaking = actorState->IsSneaking();
			state.sprinting = actorState->IsSprinting();
			state.weaponDrawn = actorState->IsWeaponDrawn();
			state.staggered = state.staggered || actorState->IsStaggered();
		}

		bool isShouting = false;
		(void)a_player->GetGraphVariableBool(RE::BSFixedString("IsShouting"), isShouting);
		state.shouting = isShouting;

		const auto* avOwner = a_player->AsActorValueOwner();
		const float health = avOwner ? avOwner->GetActorValue(RE::ActorValue::kHealth) : 0.0f;
		const float healthMax = a_player->GetActorValueMax(RE::ActorValue::kHealth);
		state.healthPercent = Percent01(health, healthMax);

		const float magicka = avOwner ? avOwner->GetActorValue(RE::ActorValue::kMagicka) : 0.0f;
		const float magickaMax = a_player->GetActorValueMax(RE::ActorValue::kMagicka);
		state.magickaPercent = Percent01(magicka, magickaMax);
		return state;
	}

	void Manager::UpdateTimers(float a_deltaTime)
	{
		for (auto& timer : m_runtime.cooldownTimers) {
			timer = std::max(0.0f, timer - a_deltaTime);
		}
		m_runtime.powerAttackLockout = std::max(0.0f, m_runtime.powerAttackLockout - a_deltaTime);
		m_runtime.shoutLockout = std::max(0.0f, m_runtime.shoutLockout - a_deltaTime);
	}

	void Manager::UpdateConcentration(RE::PlayerCharacter* a_player, float a_deltaTime, const LiveState&)
	{
		if (m_pendingConcentration.empty()) {
			return;
		}

		auto* caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) {
			caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
		}
		if (!caster) {
			m_pendingConcentration.clear();
			return;
		}

		constexpr float kConcentrationTick = 0.30f;

		for (auto& pending : m_pendingConcentration) {
			pending.remainingSec = std::max(0.0f, pending.remainingSec - a_deltaTime);
			pending.tickTimer = std::max(0.0f, pending.tickTimer - a_deltaTime);
			if (pending.remainingSec <= 0.0f || pending.tickTimer > 0.0f) {
				continue;
			}

			auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(pending.spellFormID);
			if (!spell) {
				pending.remainingSec = 0.0f;
				continue;
			}
			auto targetPtr = pending.targetHandle.get();
			auto* target = pending.castOnSelf ? static_cast<RE::TESObjectREFR*>(a_player) : targetPtr.get();
			if (!target) {
				pending.remainingSec = 0.0f;
				continue;
			}

			caster->CastSpellImmediate(spell, pending.castOnSelf, target, 1.0f, false, 0.0f, a_player);
			pending.tickTimer = kConcentrationTick;
		}

		m_pendingConcentration.erase(
			std::remove_if(
				m_pendingConcentration.begin(),
				m_pendingConcentration.end(),
				[](const PendingConcentration& a_pending) {
					return a_pending.remainingSec <= 0.0f;
				}),
			m_pendingConcentration.end());
	}

	void Manager::SyncPreviousStates(const LiveState& a_liveState)
	{
		m_runtime.wasInCombat = a_liveState.inCombat;
		m_runtime.wasSneaking = a_liveState.sneaking;
		m_runtime.wasSprinting = a_liveState.sprinting;
		m_runtime.wasWeaponDrawn = a_liveState.weaponDrawn;
		m_runtime.wasPowerAttacking = a_liveState.powerAttacking;
		m_runtime.wasShouting = a_liveState.shouting;
		m_runtime.lastHealthPercent = a_liveState.healthPercent;
	}

	void Manager::InitializeRuntimeFrom(const LiveState& a_liveState)
	{
		m_runtime.initialized = true;
		SyncPreviousStates(a_liveState);
		const auto initHealthArmed = [&](std::size_t a_index, TriggerID a_id) {
			const auto& cfg = m_config.Get(a_id);
			m_runtime.healthBelowArmed[a_index] = a_liveState.healthPercent > (cfg.thresholdHealthPercent + cfg.hysteresisMargin);
		};
		initHealthArmed(0, TriggerID::kHealthBelow70);
		initHealthArmed(1, TriggerID::kHealthBelow50);
		initHealthArmed(2, TriggerID::kHealthBelow30);
	}

	bool Manager::AttemptTrigger(RE::PlayerCharacter* a_player, TriggerID a_id, const LiveState& a_liveState, bool a_ignoreCooldown)
	{
		const auto idx = static_cast<std::size_t>(a_id);
		const auto& cfg = m_config.Get(a_id);

		if (!cfg.enabled || cfg.spellFormID.empty()) {
			return false;
		}
		if (!a_ignoreCooldown && m_runtime.cooldownTimers[idx] > 0.0f) {
			return false;
		}
		if (cfg.conditions.firstPersonOnly && !a_liveState.firstPerson) {
			return false;
		}
		if (cfg.conditions.weaponDrawnOnly && !a_liveState.weaponDrawn) {
			return false;
		}
		if (cfg.conditions.requireInCombat && !a_liveState.inCombat) {
			return false;
		}
		if (cfg.conditions.requireOutOfCombat && a_liveState.inCombat) {
			return false;
		}

		if (!CastSpellSelf(a_player, cfg, a_liveState, a_ignoreCooldown)) {
			return false;
		}

		if (!a_ignoreCooldown) {
			m_runtime.cooldownTimers[idx] = cfg.cooldownSec;
		}
		return true;
	}

	bool Manager::CastSpellSelf(RE::PlayerCharacter* a_player, const TriggerConfig& a_trigger, const LiveState& a_liveState, bool)
	{
		if (a_trigger.rules.requireMagickaPercentAbove > 0.0f &&
			a_liveState.magickaPercent < a_trigger.rules.requireMagickaPercentAbove) {
			return false;
		}

		auto* spell = ResolveSpell(a_trigger.spellFormID);
		if (!spell || !a_player->HasSpell(spell)) {
			return false;
		}

		if (a_trigger.rules.skipIfEffectAlreadyActive && IsSpellAlreadyActive(a_player, spell)) {
			return false;
		}

		bool castOnSelf = true;
		auto* target = ResolveSpellTarget(a_player, spell, castOnSelf);
		if (!target) {
			return false;
		}

		if (!CastResolvedSpell(a_player, spell, target, castOnSelf)) {
			return false;
		}

		if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
			constexpr float kConcentrationTick = 0.30f;
			PendingConcentration pending{};
			pending.spellFormID = spell->GetFormID();
			pending.remainingSec = a_trigger.concentrationDurationSec;
			pending.tickTimer = kConcentrationTick;
			pending.castOnSelf = castOnSelf;
			pending.targetHandle = castOnSelf ? RE::ObjectRefHandle{} : target->CreateRefHandle();
			m_pendingConcentration.push_back(pending);
		}
		return true;
	}

	bool Manager::CastResolvedSpell(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell, RE::TESObjectREFR* a_target, bool a_castOnSelf)
	{
		if (!a_player || !a_spell || !a_target) {
			return false;
		}

		auto* caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) {
			caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
		}
		if (!caster) {
			return false;
		}

		caster->CastSpellImmediate(a_spell, a_castOnSelf, a_target, 1.0f, false, 0.0f, a_player);
		return true;
	}

	RE::TESObjectREFR* Manager::GetCrosshairTarget() const
	{
		auto* pickData = RE::CrosshairPickData::GetSingleton();
		if (!pickData) {
			return nullptr;
		}

		auto actorTarget = pickData->targetActor.get();
		if (actorTarget) {
			return actorTarget.get();
		}

		auto target = pickData->target.get();
		return target ? target.get() : nullptr;
	}

	RE::TESObjectREFR* Manager::ResolveSpellTarget(RE::PlayerCharacter* a_player, const RE::SpellItem* a_spell, bool& a_castOnSelf) const
	{
		if (!a_player || !a_spell) {
			return nullptr;
		}

		const auto delivery = a_spell->GetDelivery();
		if (delivery == RE::MagicSystem::Delivery::kSelf) {
			a_castOnSelf = true;
			return a_player;
		}

		auto* crosshairTarget = GetCrosshairTarget();
		if (!crosshairTarget) {
			return nullptr;
		}

		a_castOnSelf = false;
		return crosshairTarget;
	}

	RE::SpellItem* Manager::ResolveSpell(std::string_view a_formKey)
	{
		std::string pluginName;
		RE::FormID localFormID = 0;
		if (!ParseFormKey(a_formKey, pluginName, localFormID)) {
			return nullptr;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return nullptr;
		}

		auto* form = dataHandler->LookupForm(localFormID, pluginName);
		return form ? form->As<RE::SpellItem>() : nullptr;
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

		a_pluginName = Trim(a_formKey.substr(0, split));
		auto idText = Trim(a_formKey.substr(split + 1));
		if (a_pluginName.empty() || idText.empty()) {
			return false;
		}

		if (idText.rfind("0x", 0) == 0 || idText.rfind("0X", 0) == 0) {
			idText = idText.substr(2);
		}

		std::uint32_t parsed = 0;
		const auto* begin = idText.data();
		const auto* end = begin + idText.size();
		const auto [ptr, ec] = std::from_chars(begin, end, parsed, 16);
		if (ec != std::errc{} || ptr != end) {
			return false;
		}

		a_localFormID = parsed;
		return true;
	}

	bool Manager::IsSpellAlreadyActive(RE::PlayerCharacter* a_player, const RE::SpellItem* a_spell)
	{
		if (!a_player || !a_spell) {
			return false;
		}

		struct ActiveEffectVisitor final : RE::MagicTarget::ForEachActiveEffectVisitor
		{
			explicit ActiveEffectVisitor(const RE::SpellItem* a_targetSpell) :
				targetSpell(a_targetSpell)
			{}

			RE::BSContainer::ForEachResult Accept(RE::ActiveEffect* a_effect) override
			{
				if (a_effect && a_effect->spell == targetSpell) {
					found = true;
					return RE::BSContainer::ForEachResult::kStop;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			}

			const RE::SpellItem* targetSpell{ nullptr };
			bool found{ false };
		};

		auto* magicTarget = a_player->AsMagicTarget();
		if (!magicTarget) {
			return false;
		}

		ActiveEffectVisitor visitor(a_spell);
		magicTarget->VisitEffects(visitor);

		return visitor.found;
	}

	float Manager::Percent01(float a_value, float a_maximum)
	{
		if (a_maximum <= 0.0f) {
			return 0.0f;
		}
		return Clamp01(a_value / a_maximum);
	}
}
