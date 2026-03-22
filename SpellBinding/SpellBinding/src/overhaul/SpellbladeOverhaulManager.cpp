#include "overhaul/SpellbladeOverhaulManager.h"

#include "buff/QuickBuffManager.h"
#include "mastery_shout/MasteryManager.h"
#include "mastery_shout/MasteryData.h"
#include "mastery_spell/MasteryManager.h"
#include "mastery_spell/MasteryData.h"
#include "mastery_weapon/MasteryManager.h"
#include "mastery_weapon/MasteryData.h"
#include "smartcast/SmartCastController.h"
#include "spellbinding/SpellBindingManager.h"
#include "ui/PrismaBridge.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <unordered_set>

namespace SB_OVERHAUL
{
	namespace
	{
		using json = nlohmann::json;

		[[nodiscard]] QUICK_BUFF::TriggerID ParseTriggerID(std::string_view a_name)
		{
			if (a_name == "combatStart") return QUICK_BUFF::TriggerID::kCombatStart;
			if (a_name == "combatEnd") return QUICK_BUFF::TriggerID::kCombatEnd;
			if (a_name == "healthBelow70") return QUICK_BUFF::TriggerID::kHealthBelow70;
			if (a_name == "healthBelow50") return QUICK_BUFF::TriggerID::kHealthBelow50;
			if (a_name == "healthBelow30") return QUICK_BUFF::TriggerID::kHealthBelow30;
			if (a_name == "crouchStart") return QUICK_BUFF::TriggerID::kCrouchStart;
			if (a_name == "sprintStart") return QUICK_BUFF::TriggerID::kSprintStart;
			if (a_name == "weaponDraw") return QUICK_BUFF::TriggerID::kWeaponDraw;
			if (a_name == "powerAttackStart") return QUICK_BUFF::TriggerID::kPowerAttackStart;
			if (a_name == "shoutStart") return QUICK_BUFF::TriggerID::kShoutStart;
			return QUICK_BUFF::TriggerID::kCombatStart;
		}

		[[nodiscard]] std::string TriggerKey(QUICK_BUFF::TriggerID a_id)
		{
			switch (a_id) {
				case QUICK_BUFF::TriggerID::kCombatStart: return "combatStart";
				case QUICK_BUFF::TriggerID::kCombatEnd: return "combatEnd";
				case QUICK_BUFF::TriggerID::kHealthBelow70: return "healthBelow70";
				case QUICK_BUFF::TriggerID::kHealthBelow50: return "healthBelow50";
				case QUICK_BUFF::TriggerID::kHealthBelow30: return "healthBelow30";
				case QUICK_BUFF::TriggerID::kCrouchStart: return "crouchStart";
				case QUICK_BUFF::TriggerID::kSprintStart: return "sprintStart";
				case QUICK_BUFF::TriggerID::kWeaponDraw: return "weaponDraw";
				case QUICK_BUFF::TriggerID::kPowerAttackStart: return "powerAttackStart";
				case QUICK_BUFF::TriggerID::kShoutStart: return "shoutStart";
				case QUICK_BUFF::TriggerID::kTotal: break;
			}
			return "combatStart";
		}

		[[nodiscard]] json ToJson(const QUICK_BUFF::TriggerConfig& a_cfg)
		{
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
		}

		[[nodiscard]] json ToJson(const QUICK_BUFF::KnownSpellOption& a_spell)
		{
			return json{
				{ "name", a_spell.name },
				{ "formKey", a_spell.formKey },
				{ "formID", a_spell.formID }
			};
		}

		[[nodiscard]] json ToJson(const SMART_CAST::ChainStep& a_step)
		{
			return json{
				{ "spellFormID", a_step.spellFormID },
				{ "type", static_cast<std::uint32_t>(a_step.type) },
				{ "castOn", static_cast<std::uint32_t>(a_step.castOn) },
				{ "holdSec", a_step.holdSec },
				{ "castCount", a_step.castCount }
			};
		}

		[[nodiscard]] json ToJson(const SMART_CAST::ChainConfig& a_chain)
		{
			json steps = json::array();
			for (const auto& step : a_chain.steps) {
				steps.push_back(ToJson(step));
			}
			return json{
				{ "name", a_chain.name },
				{ "enabled", a_chain.enabled },
				{ "hotkey", a_chain.hotkey },
				{ "stepDelaySec", a_chain.stepDelaySec },
				{ "steps", std::move(steps) }
			};
		}

		[[nodiscard]] json ToJson(const SMART_CAST::KnownSpellOption& a_spell)
		{
			return json{
				{ "name", a_spell.name },
				{ "formKey", a_spell.formKey },
				{ "formID", a_spell.formID },
				{ "magickaCost", a_spell.magickaCost },
				{ "concentration", a_spell.concentration }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SPELL::SpellKey& a_key, const SBO::MASTERY_SPELL::MasteryStats& a_stats)
		{
			return {
				{ "name", a_key.name },
				{ "formID", a_key.formID },
				{ "school", static_cast<std::uint32_t>(a_key.school) },
				{ "kills", a_stats.kills },
				{ "uses", a_stats.uses },
				{ "summons", a_stats.summons },
				{ "hits", a_stats.hits },
				{ "equippedSeconds", a_stats.equippedSeconds },
				{ "level", a_stats.level }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SHOUT::ShoutKey& a_key, const SBO::MASTERY_SHOUT::MasteryStats& a_stats)
		{
			return {
				{ "name", a_key.name },
				{ "formID", a_key.formID },
				{ "uses", a_stats.uses },
				{ "level", a_stats.level }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_WEAPON::ItemKey& a_key, const SBO::MASTERY_WEAPON::MasteryStats& a_stats)
		{
			return {
				{ "name", a_key.baseName },
				{ "uniqueID", a_key.uniqueID },
				{ "weaponType", static_cast<std::uint32_t>(a_key.weaponType) },
				{ "leftHand", a_key.leftHand },
				{ "shield", a_key.shield },
				{ "kills", a_stats.kills },
				{ "hits", a_stats.hits },
				{ "powerHits", a_stats.powerHits },
				{ "blocks", a_stats.blocks },
				{ "secondsEquipped", a_stats.secondsEquipped },
				{ "level", a_stats.level }
			};
		}

		struct ProgressSegment
		{
			std::uint32_t current{ 0 };
			std::uint32_t next{ 1 };
			float pct{ 0.0f };
		};

		[[nodiscard]] std::uint32_t ComputeLevelFromProgress(const std::vector<std::uint32_t>& a_thresholds, std::uint32_t a_points)
		{
			std::uint32_t level = 0;
			for (const auto threshold : a_thresholds) {
				if (a_points >= threshold) {
					++level;
				}
			}
			return level;
		}

		[[nodiscard]] ProgressSegment ProgressMeta(const std::vector<std::uint32_t>& a_thresholds, std::uint32_t a_points, std::uint32_t a_level)
		{
			if (a_thresholds.empty()) {
				return ProgressSegment{ a_points, a_points == 0 ? 1u : a_points, a_points > 0 ? 1.0f : 0.0f };
			}

			const auto levelIdx = static_cast<std::size_t>(a_level);
			if (levelIdx >= a_thresholds.size()) {
				return ProgressSegment{ 1u, 1u, 1.0f };
			}

			const std::uint32_t prevThreshold = levelIdx == 0 ? 0u : a_thresholds[levelIdx - 1];
			const std::uint32_t nextThreshold = a_thresholds[levelIdx];
			const std::uint32_t segmentSize = std::max(1u, nextThreshold > prevThreshold ? (nextThreshold - prevThreshold) : 1u);
			const std::uint32_t segmentCurrent = std::clamp(
				a_points > prevThreshold ? (a_points - prevThreshold) : 0u,
				0u,
				segmentSize);
			const float pct = std::clamp(static_cast<float>(segmentCurrent) / static_cast<float>(segmentSize), 0.0f, 1.0f);
			return ProgressSegment{ segmentCurrent, segmentSize, pct };
		}

		[[nodiscard]] json BuildSpellRow(const SBO::MASTERY_SPELL::SpellKey& a_key,
			const SBO::MASTERY_SPELL::MasteryStats& a_stats,
			const SBO::MASTERY_SPELL::MasteryConfig& a_cfg)
		{
			const auto schoolIdx = static_cast<std::size_t>(a_key.school);
			const auto& progression = a_cfg.schools[schoolIdx].progression;
			float points = 0.0f;
			if (progression.gainFromKills) points += static_cast<float>(a_stats.kills);
			if (progression.gainFromUses) points += static_cast<float>(a_stats.uses);
			if (progression.gainFromSummons) points += static_cast<float>(a_stats.summons);
			if (progression.gainFromHits) points += static_cast<float>(a_stats.hits);
			if (progression.gainFromEquipTime) {
				points += a_stats.equippedSeconds / std::max(0.1f, progression.equipSecondsPerPoint);
			}
			points *= std::max(0.0f, a_cfg.gainMultiplier);
			const auto progressPoints = static_cast<std::uint32_t>(std::max(0.0f, points));
			const auto derivedLevel = ComputeLevelFromProgress(a_cfg.thresholds, progressPoints);
			const auto progress = ProgressMeta(a_cfg.thresholds, progressPoints, derivedLevel);

			return {
				{ "name", a_key.name },
				{ "formID", a_key.formID },
				{ "type", std::string(SBO::MASTERY_SPELL::SchoolName(a_key.school)) },
				{ "school", static_cast<std::uint32_t>(a_key.school) },
				{ "uses", a_stats.uses },
				{ "kills", a_stats.kills },
				{ "summons", a_stats.summons },
				{ "hits", a_stats.hits },
				{ "equippedSeconds", a_stats.equippedSeconds },
				{ "level", derivedLevel },
				{ "progressPoints", progress.current },
				{ "nextThreshold", progress.next },
				{ "progressPct", progress.pct }
			};
		}

		[[nodiscard]] json BuildShoutRow(const SBO::MASTERY_SHOUT::ShoutKey& a_key,
			const SBO::MASTERY_SHOUT::MasteryStats& a_stats,
			const SBO::MASTERY_SHOUT::MasteryConfig& a_cfg)
		{
			const float rawPoints = a_cfg.gainFromUses ? static_cast<float>(a_stats.uses) : 0.0f;
			const auto progressPoints = static_cast<std::uint32_t>(std::max(0.0f, rawPoints * std::max(0.0f, a_cfg.gainMultiplier)));
			const auto derivedLevel = ComputeLevelFromProgress(a_cfg.thresholds, progressPoints);
			const auto progress = ProgressMeta(a_cfg.thresholds, progressPoints, derivedLevel);
			return {
				{ "name", a_key.name },
				{ "formID", a_key.formID },
				{ "type", "Shout" },
				{ "uses", a_stats.uses },
				{ "level", derivedLevel },
				{ "progressPoints", progress.current },
				{ "nextThreshold", progress.next },
				{ "progressPct", progress.pct }
			};
		}

		[[nodiscard]] json BuildWeaponRow(const SBO::MASTERY_WEAPON::ItemKey& a_key,
			const SBO::MASTERY_WEAPON::MasteryStats& a_stats,
			const SBO::MASTERY_WEAPON::MasteryConfig& a_cfg)
		{
			float points = 0.0f;
			if (a_cfg.gainFromKills) points += static_cast<float>(a_stats.kills);
			if (a_cfg.gainFromHits) points += static_cast<float>(a_stats.hits);
			if (a_cfg.gainFromPowerHits) points += static_cast<float>(a_stats.powerHits);
			if (a_cfg.gainFromBlocks) points += static_cast<float>(a_stats.blocks);
			if (a_cfg.timeBasedLeveling) points += a_stats.secondsEquipped / 5.0f;
			points *= std::max(0.0f, a_cfg.gainMultiplier);
			const auto progressPoints = static_cast<std::uint32_t>(std::max(0.0f, points));
			const auto derivedLevel = ComputeLevelFromProgress(a_cfg.thresholds, progressPoints);
			const auto progress = ProgressMeta(a_cfg.thresholds, progressPoints, derivedLevel);
			return {
				{ "name", a_key.baseName },
				{ "uniqueID", a_key.uniqueID },
				{ "type", std::string(SBO::MASTERY_WEAPON::WeaponTypeName(a_key.weaponType)) },
				{ "weaponType", static_cast<std::uint32_t>(a_key.weaponType) },
				{ "leftHand", a_key.leftHand },
				{ "shield", a_key.shield },
				{ "kills", a_stats.kills },
				{ "hits", a_stats.hits },
				{ "powerHits", a_stats.powerHits },
				{ "blocks", a_stats.blocks },
				{ "secondsEquipped", a_stats.secondsEquipped },
				{ "level", derivedLevel },
				{ "progressPoints", progress.current },
				{ "nextThreshold", progress.next },
				{ "progressPct", progress.pct }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SPELL::MasteryConfig::BonusTuning& b)
		{
			return {
				{ "skillBonusPerLevel", b.skillBonusPerLevel },
				{ "skillBonusCap", b.skillBonusCap },
				{ "skillAdvancePerLevel", b.skillAdvancePerLevel },
				{ "skillAdvanceCap", b.skillAdvanceCap },
				{ "powerBonusPerLevel", b.powerBonusPerLevel },
				{ "powerBonusCap", b.powerBonusCap },
				{ "costReductionPerLevel", b.costReductionPerLevel },
				{ "costReductionCap", b.costReductionCap },
				{ "magickaRatePerLevel", b.magickaRatePerLevel },
				{ "magickaRateCap", b.magickaRateCap },
				{ "magickaFlatPerLevel", b.magickaFlatPerLevel },
				{ "magickaFlatCap", b.magickaFlatCap }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SPELL::MasteryConfig::ProgressionTuning& p)
		{
			return {
				{ "gainFromKills", p.gainFromKills },
				{ "gainFromUses", p.gainFromUses },
				{ "gainFromSummons", p.gainFromSummons },
				{ "gainFromHits", p.gainFromHits },
				{ "gainFromEquipTime", p.gainFromEquipTime },
				{ "equipSecondsPerPoint", p.equipSecondsPerPoint }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SPELL::MasteryConfig::SchoolConfig& s)
		{
			return {
				{ "progression", ToJson(s.progression) },
				{ "useBonusOverride", s.useBonusOverride },
				{ "bonus", ToJson(s.bonus) }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SPELL::MasteryConfig& cfg)
		{
			json schools = json::array();
			for (const auto& school : cfg.schools) {
				schools.push_back(ToJson(school));
			}
			return {
				{ "enabled", cfg.enabled },
				{ "gainMultiplier", cfg.gainMultiplier },
				{ "thresholds", cfg.thresholds },
				{ "generalBonuses", ToJson(cfg.generalBonuses) },
				{ "schools", std::move(schools) }
			};
		}

		void FromJson(const json& node, SBO::MASTERY_SPELL::MasteryConfig::BonusTuning& b)
		{
			b.skillBonusPerLevel = node.value("skillBonusPerLevel", b.skillBonusPerLevel);
			b.skillBonusCap = node.value("skillBonusCap", b.skillBonusCap);
			b.skillAdvancePerLevel = node.value("skillAdvancePerLevel", b.skillAdvancePerLevel);
			b.skillAdvanceCap = node.value("skillAdvanceCap", b.skillAdvanceCap);
			b.powerBonusPerLevel = node.value("powerBonusPerLevel", b.powerBonusPerLevel);
			b.powerBonusCap = node.value("powerBonusCap", b.powerBonusCap);
			b.costReductionPerLevel = node.value("costReductionPerLevel", b.costReductionPerLevel);
			b.costReductionCap = node.value("costReductionCap", b.costReductionCap);
			b.magickaRatePerLevel = node.value("magickaRatePerLevel", b.magickaRatePerLevel);
			b.magickaRateCap = node.value("magickaRateCap", b.magickaRateCap);
			b.magickaFlatPerLevel = node.value("magickaFlatPerLevel", b.magickaFlatPerLevel);
			b.magickaFlatCap = node.value("magickaFlatCap", b.magickaFlatCap);
		}

		void FromJson(const json& node, SBO::MASTERY_SPELL::MasteryConfig::ProgressionTuning& p)
		{
			p.gainFromKills = node.value("gainFromKills", p.gainFromKills);
			p.gainFromUses = node.value("gainFromUses", p.gainFromUses);
			p.gainFromSummons = node.value("gainFromSummons", p.gainFromSummons);
			p.gainFromHits = node.value("gainFromHits", p.gainFromHits);
			p.gainFromEquipTime = node.value("gainFromEquipTime", p.gainFromEquipTime);
			p.equipSecondsPerPoint = node.value("equipSecondsPerPoint", p.equipSecondsPerPoint);
		}

		void FromJson(const json& node, SBO::MASTERY_SPELL::MasteryConfig::SchoolConfig& s)
		{
			if (node.contains("progression") && node["progression"].is_object()) {
				FromJson(node["progression"], s.progression);
			}
			s.useBonusOverride = node.value("useBonusOverride", s.useBonusOverride);
			if (node.contains("bonus") && node["bonus"].is_object()) {
				FromJson(node["bonus"], s.bonus);
			}
		}

		void FromJson(const json& node, SBO::MASTERY_SPELL::MasteryConfig& cfg)
		{
			cfg.enabled = node.value("enabled", cfg.enabled);
			cfg.gainMultiplier = node.value("gainMultiplier", cfg.gainMultiplier);
			if (node.contains("thresholds") && node["thresholds"].is_array()) {
				cfg.thresholds = node["thresholds"].get<std::vector<std::uint32_t>>();
			}
			if (node.contains("generalBonuses") && node["generalBonuses"].is_object()) {
				FromJson(node["generalBonuses"], cfg.generalBonuses);
			}
			if (node.contains("schools") && node["schools"].is_array()) {
				const auto& arr = node["schools"];
				for (std::size_t i = 0; i < cfg.schools.size() && i < arr.size(); ++i) {
					if (arr[i].is_object()) {
						FromJson(arr[i], cfg.schools[i]);
					}
				}
			}
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SHOUT::MasteryConfig::BonusTuning& b)
		{
			return {
				{ "shoutRecoveryPerLevel", b.shoutRecoveryPerLevel },
				{ "shoutRecoveryCap", b.shoutRecoveryCap },
				{ "shoutPowerPerLevel", b.shoutPowerPerLevel },
				{ "shoutPowerCap", b.shoutPowerCap },
				{ "voiceRatePerLevel", b.voiceRatePerLevel },
				{ "voiceRateCap", b.voiceRateCap },
				{ "voicePointsPerLevel", b.voicePointsPerLevel },
				{ "voicePointsCap", b.voicePointsCap },
				{ "staminaRatePerLevel", b.staminaRatePerLevel },
				{ "staminaRateCap", b.staminaRateCap }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_SHOUT::MasteryConfig& cfg)
		{
			return {
				{ "enabled", cfg.enabled },
				{ "gainMultiplier", cfg.gainMultiplier },
				{ "gainFromUses", cfg.gainFromUses },
				{ "thresholds", cfg.thresholds },
				{ "bonuses", ToJson(cfg.bonuses) }
			};
		}

		void FromJson(const json& node, SBO::MASTERY_SHOUT::MasteryConfig::BonusTuning& b)
		{
			b.shoutRecoveryPerLevel = node.value("shoutRecoveryPerLevel", b.shoutRecoveryPerLevel);
			b.shoutRecoveryCap = node.value("shoutRecoveryCap", b.shoutRecoveryCap);
			b.shoutPowerPerLevel = node.value("shoutPowerPerLevel", b.shoutPowerPerLevel);
			b.shoutPowerCap = node.value("shoutPowerCap", b.shoutPowerCap);
			b.voiceRatePerLevel = node.value("voiceRatePerLevel", b.voiceRatePerLevel);
			b.voiceRateCap = node.value("voiceRateCap", b.voiceRateCap);
			b.voicePointsPerLevel = node.value("voicePointsPerLevel", b.voicePointsPerLevel);
			b.voicePointsCap = node.value("voicePointsCap", b.voicePointsCap);
			b.staminaRatePerLevel = node.value("staminaRatePerLevel", b.staminaRatePerLevel);
			b.staminaRateCap = node.value("staminaRateCap", b.staminaRateCap);
		}

		void FromJson(const json& node, SBO::MASTERY_SHOUT::MasteryConfig& cfg)
		{
			cfg.enabled = node.value("enabled", cfg.enabled);
			cfg.gainMultiplier = node.value("gainMultiplier", cfg.gainMultiplier);
			cfg.gainFromUses = node.value("gainFromUses", cfg.gainFromUses);
			if (node.contains("thresholds") && node["thresholds"].is_array()) {
				cfg.thresholds = node["thresholds"].get<std::vector<std::uint32_t>>();
			}
			if (node.contains("bonuses") && node["bonuses"].is_object()) {
				FromJson(node["bonuses"], cfg.bonuses);
			}
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_WEAPON::MasteryConfig::BonusTuning& b)
		{
			return {
				{ "damagePerLevel", b.damagePerLevel },
				{ "attackSpeedPerLevel", b.attackSpeedPerLevel },
				{ "critChancePerLevel", b.critChancePerLevel },
				{ "powerAttackStaminaReductionPerLevel", b.powerAttackStaminaReductionPerLevel },
				{ "blockStaminaReductionPerLevel", b.blockStaminaReductionPerLevel },
				{ "damageCap", b.damageCap },
				{ "attackSpeedCap", b.attackSpeedCap },
				{ "critChanceCap", b.critChanceCap },
				{ "powerAttackStaminaFloor", b.powerAttackStaminaFloor },
				{ "blockStaminaFloor", b.blockStaminaFloor }
			};
		}

		[[nodiscard]] json ToJson(const SBO::MASTERY_WEAPON::MasteryConfig& cfg)
		{
			json typeBonuses = json::array();
			for (const auto& typeCfg : cfg.weaponTypeBonuses) {
				typeBonuses.push_back({
					{ "enabled", typeCfg.enabled },
					{ "tuning", ToJson(typeCfg.tuning) }
				});
			}
			return {
				{ "enabled", cfg.enabled },
				{ "gainMultiplier", cfg.gainMultiplier },
				{ "thresholds", cfg.thresholds },
				{ "gainFromKills", cfg.gainFromKills },
				{ "gainFromHits", cfg.gainFromHits },
				{ "gainFromPowerHits", cfg.gainFromPowerHits },
				{ "gainFromBlocks", cfg.gainFromBlocks },
				{ "timeBasedLeveling", cfg.timeBasedLeveling },
				{ "generalBonuses", ToJson(cfg.generalBonuses) },
				{ "weaponTypeBonuses", std::move(typeBonuses) }
			};
		}

		void FromJson(const json& node, SBO::MASTERY_WEAPON::MasteryConfig::BonusTuning& b)
		{
			b.damagePerLevel = node.value("damagePerLevel", b.damagePerLevel);
			b.attackSpeedPerLevel = node.value("attackSpeedPerLevel", b.attackSpeedPerLevel);
			b.critChancePerLevel = node.value("critChancePerLevel", b.critChancePerLevel);
			b.powerAttackStaminaReductionPerLevel = node.value("powerAttackStaminaReductionPerLevel", b.powerAttackStaminaReductionPerLevel);
			b.blockStaminaReductionPerLevel = node.value("blockStaminaReductionPerLevel", b.blockStaminaReductionPerLevel);
			b.damageCap = node.value("damageCap", b.damageCap);
			b.attackSpeedCap = node.value("attackSpeedCap", b.attackSpeedCap);
			b.critChanceCap = node.value("critChanceCap", b.critChanceCap);
			b.powerAttackStaminaFloor = node.value("powerAttackStaminaFloor", b.powerAttackStaminaFloor);
			b.blockStaminaFloor = node.value("blockStaminaFloor", b.blockStaminaFloor);
		}

		void FromJson(const json& node, SBO::MASTERY_WEAPON::MasteryConfig& cfg)
		{
			cfg.enabled = node.value("enabled", cfg.enabled);
			cfg.gainMultiplier = node.value("gainMultiplier", cfg.gainMultiplier);
			if (node.contains("thresholds") && node["thresholds"].is_array()) {
				cfg.thresholds = node["thresholds"].get<std::vector<std::uint32_t>>();
			}
			cfg.gainFromKills = node.value("gainFromKills", cfg.gainFromKills);
			cfg.gainFromHits = node.value("gainFromHits", cfg.gainFromHits);
			cfg.gainFromPowerHits = node.value("gainFromPowerHits", cfg.gainFromPowerHits);
			cfg.gainFromBlocks = node.value("gainFromBlocks", cfg.gainFromBlocks);
			cfg.timeBasedLeveling = node.value("timeBasedLeveling", cfg.timeBasedLeveling);
			if (node.contains("generalBonuses") && node["generalBonuses"].is_object()) {
				FromJson(node["generalBonuses"], cfg.generalBonuses);
			}
			if (node.contains("weaponTypeBonuses") && node["weaponTypeBonuses"].is_array()) {
				const auto& arr = node["weaponTypeBonuses"];
				for (std::size_t i = 0; i < cfg.weaponTypeBonuses.size() && i < arr.size(); ++i) {
					if (!arr[i].is_object()) {
						continue;
					}
					cfg.weaponTypeBonuses[i].enabled = arr[i].value("enabled", cfg.weaponTypeBonuses[i].enabled);
					if (arr[i].contains("tuning") && arr[i]["tuning"].is_object()) {
						FromJson(arr[i]["tuning"], cfg.weaponTypeBonuses[i].tuning);
					}
				}
			}
		}

		void ForceEnabledFlags(bool a_save)
		{
			auto* sb = SBIND::Manager::GetSingleton();
			auto sbCfg = sb->GetConfig();
			if (!sbCfg.enabled) {
				sbCfg.enabled = true;
				sb->SetConfig(sbCfg, a_save);
			}

			auto* sc = SMART_CAST::Controller::GetSingleton();
			auto scCfg = sc->GetConfig();
			if (!scCfg.global.enabled) {
				scCfg.global.enabled = true;
				sc->SetConfig(scCfg, a_save);
			}

			auto* qb = QUICK_BUFF::Manager::GetSingleton();
			auto qbCfg = qb->GetConfig();
			if (!qbCfg.global.enabled) {
				qbCfg.global.enabled = true;
				qb->SetConfig(qbCfg, a_save);
			}
		}
	}

	void Manager::Initialize()
	{
		SBIND::Manager::GetSingleton()->Initialize();
		SMART_CAST::Controller::GetSingleton()->Initialize();
		QUICK_BUFF::Manager::GetSingleton()->Initialize();
		SBO::MASTERY_SPELL::Manager::GetSingleton()->Initialize();
		SBO::MASTERY_SHOUT::Manager::GetSingleton()->Initialize();
		SBO::MASTERY_WEAPON::Manager::GetSingleton()->Initialize();
		m_lastActiveChainIndex = SMART_CAST::Controller::GetSingleton()->GetActiveChainIndex1Based();
		ForceEnabledFlags(true);
	}

	void Manager::Update(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		SBIND::Manager::GetSingleton()->Update(a_player, a_deltaTime);
		SMART_CAST::Controller::GetSingleton()->Update(a_player, a_deltaTime);
		QUICK_BUFF::Manager::GetSingleton()->Update(a_player, a_deltaTime);
		const auto activeChain = SMART_CAST::Controller::GetSingleton()->GetActiveChainIndex1Based();
		if (activeChain != m_lastActiveChainIndex) {
			const auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
			std::string chainName = std::format("Chain {}", std::max(1, activeChain));
			if (activeChain > 0 && activeChain <= static_cast<std::int32_t>(cfg.chains.size())) {
				const auto& candidate = cfg.chains[static_cast<std::size_t>(activeChain - 1)].name;
				if (!candidate.empty()) {
					chainName = candidate;
				}
			}
			if (!UI::PRISMA::Bridge::GetSingleton()->IsMenuOpen()) {
				SBIND::Manager::GetSingleton()->NotifyChainSwitch(activeChain, chainName);
				PushHUDSnapshot();
			}
			m_lastActiveChainIndex = activeChain;
		}
		(void)a_deltaTime;
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		SBIND::Manager::GetSingleton()->Serialize(a_intfc);
		SMART_CAST::Controller::GetSingleton()->Serialize(a_intfc);
		QUICK_BUFF::Manager::GetSingleton()->Serialize(a_intfc);
		SBO::MASTERY_SPELL::Manager::GetSingleton()->Serialize(a_intfc);
		SBO::MASTERY_SHOUT::Manager::GetSingleton()->Serialize(a_intfc);
		SBO::MASTERY_WEAPON::Manager::GetSingleton()->Serialize(a_intfc);
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		SBIND::Manager::GetSingleton()->Deserialize(a_intfc);
		SMART_CAST::Controller::GetSingleton()->Deserialize(a_intfc);
		QUICK_BUFF::Manager::GetSingleton()->Deserialize(a_intfc);
		SBO::MASTERY_SPELL::Manager::GetSingleton()->Deserialize(a_intfc);
		SBO::MASTERY_SHOUT::Manager::GetSingleton()->Deserialize(a_intfc);
		SBO::MASTERY_WEAPON::Manager::GetSingleton()->Deserialize(a_intfc);
		m_lastActiveChainIndex = SMART_CAST::Controller::GetSingleton()->GetActiveChainIndex1Based();
		ForceEnabledFlags(false);
	}

	void Manager::OnRevert()
	{
		SBIND::Manager::GetSingleton()->OnRevert();
		SMART_CAST::Controller::GetSingleton()->OnRevert();
		QUICK_BUFF::Manager::GetSingleton()->OnRevert();
		SBO::MASTERY_SPELL::Manager::GetSingleton()->OnRevert();
		SBO::MASTERY_SHOUT::Manager::GetSingleton()->OnRevert();
		SBO::MASTERY_WEAPON::Manager::GetSingleton()->OnRevert();
		m_lastActiveChainIndex = SMART_CAST::Controller::GetSingleton()->GetActiveChainIndex1Based();
		ForceEnabledFlags(false);
	}

	std::string Manager::BuildSnapshotJson() const
	{
		json root{};
		root["meta"] = {
			{ "title", "Spell Binding - A Spellblade Overhaul" }
		};

		const auto sbSnapshotRaw = SBIND::Manager::GetSingleton()->GetSnapshot().json;
		const auto sbSnapshot = json::parse(sbSnapshotRaw, nullptr, false);
		root["spellBinding"] = sbSnapshot.is_discarded() ? json::object() : sbSnapshot;

		{
			const auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
			json chains = json::array();
			for (const auto& chain : cfg.chains) {
				chains.push_back(ToJson(chain));
			}

			json knownSpells = json::array();
			for (const auto& spell : SMART_CAST::Controller::GetSingleton()->GetKnownSpells()) {
				knownSpells.push_back(ToJson(spell));
			}

			root["smartCast"] = {
				{ "config", {
					{ "global", {
						{ "record", {
							{ "toggleKey", cfg.global.record.toggleKey },
							{ "cancelKey", cfg.global.record.cancelKey },
							{ "maxIdleSec", cfg.global.record.maxIdleSec }
						} },
						{ "playback", {
							{ "playKey", cfg.global.playback.playKey },
							{ "cancelKey", cfg.global.playback.cancelKey },
							{ "cycleModifierKey", cfg.global.playback.cycleModifierKey },
							{ "stepDelaySec", cfg.global.playback.stepDelaySec }
						} }
					} },
					{ "chains", std::move(chains) }
				} },
				{ "state", {
					{ "recording", SMART_CAST::Controller::GetSingleton()->IsRecording() },
					{ "playing", SMART_CAST::Controller::GetSingleton()->IsPlaying() },
					{ "activeChainIndex", SMART_CAST::Controller::GetSingleton()->GetActiveChainIndex1Based() }
				} },
				{ "knownSpells", std::move(knownSpells) }
			};
		}

		{
			const auto cfg = QUICK_BUFF::Manager::GetSingleton()->GetConfig();
			json knownSpells = json::array();
			for (const auto& spell : QUICK_BUFF::Manager::GetSingleton()->GetKnownSpells()) {
				knownSpells.push_back(ToJson(spell));
			}

			json triggers{};
			for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(QUICK_BUFF::TriggerID::kTotal); ++i) {
				const auto id = static_cast<QUICK_BUFF::TriggerID>(i);
				triggers[TriggerKey(id)] = ToJson(cfg.Get(id));
			}

			root["quickBuff"] = {
				{ "config", {
					{ "global", {
						{ "firstPersonOnlyDefault", cfg.global.firstPersonOnlyDefault },
						{ "weaponDrawnOnlyDefault", cfg.global.weaponDrawnOnlyDefault },
						{ "preventCastingInMenus", cfg.global.preventCastingInMenus },
						{ "preventCastingWhileStaggered", cfg.global.preventCastingWhileStaggered },
						{ "preventCastingWhileRagdoll", cfg.global.preventCastingWhileRagdoll }
					} },
					{ "triggers", std::move(triggers) }
				} },
				{ "knownSpells", std::move(knownSpells) }
			};
		}

		{
			const auto spellCfg = SBO::MASTERY_SPELL::Manager::GetSingleton()->GetConfig();
			json spellRows = json::array();
			std::unordered_map<std::uint32_t, SBO::MASTERY_SPELL::MasteryStats> spellStatsByForm{};
			for (const auto& [key, stats] : SBO::MASTERY_SPELL::Manager::GetSingleton()->GetMasteryData()) {
				spellRows.push_back(BuildSpellRow(key, stats, spellCfg));
				spellStatsByForm[key.formID] = stats;
			}
			json spellKnownRows = spellRows;
			std::unordered_set<std::uint32_t> spellSeen{};
			for (const auto& row : spellKnownRows) {
				spellSeen.insert(row.value("formID", 0u));
			}
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				struct SpellVisitor final : RE::Actor::ForEachSpellVisitor {
					SpellVisitor(json& a_rows,
						std::unordered_set<std::uint32_t>& a_seen,
						const std::unordered_map<std::uint32_t, SBO::MASTERY_SPELL::MasteryStats>& a_statsByForm,
						const SBO::MASTERY_SPELL::MasteryConfig& a_cfg) :
						rows(a_rows), seen(a_seen), statsByForm(a_statsByForm), cfg(a_cfg)
					{}
					RE::BSContainer::ForEachResult Visit(RE::SpellItem* a_spell) override
					{
						if (!a_spell) {
							return RE::BSContainer::ForEachResult::kContinue;
						}
						const auto school = SBO::MASTERY_SPELL::ActorValueToSchool(a_spell->GetAssociatedSkill());
						if (!school.has_value()) {
							return RE::BSContainer::ForEachResult::kContinue;
						}
						const auto formID = a_spell->GetFormID();
						if (seen.contains(formID)) {
							return RE::BSContainer::ForEachResult::kContinue;
						}
						seen.insert(formID);
						SBO::MASTERY_SPELL::SpellKey key{};
						key.formID = formID;
						key.school = *school;
						key.name = a_spell->GetName() ? a_spell->GetName() : std::format("Spell {:08X}", formID);
						const auto it = statsByForm.find(formID);
						const SBO::MASTERY_SPELL::MasteryStats stats = it != statsByForm.end() ? it->second : SBO::MASTERY_SPELL::MasteryStats{};
						rows.push_back(BuildSpellRow(key, stats, cfg));
						return RE::BSContainer::ForEachResult::kContinue;
					}
					json& rows;
					std::unordered_set<std::uint32_t>& seen;
					const std::unordered_map<std::uint32_t, SBO::MASTERY_SPELL::MasteryStats>& statsByForm;
					const SBO::MASTERY_SPELL::MasteryConfig& cfg;
				} spellVisitor(spellKnownRows, spellSeen, spellStatsByForm, spellCfg);
				player->VisitSpells(spellVisitor);
			}

			const auto shoutCfg = SBO::MASTERY_SHOUT::Manager::GetSingleton()->GetConfig();
			json shoutRows = json::array();
			std::unordered_map<std::uint32_t, SBO::MASTERY_SHOUT::MasteryStats> shoutStatsByForm{};
			for (const auto& [key, stats] : SBO::MASTERY_SHOUT::Manager::GetSingleton()->GetMasteryData()) {
				shoutRows.push_back(BuildShoutRow(key, stats, shoutCfg));
				shoutStatsByForm[key.formID] = stats;
			}
			json shoutKnownRows = shoutRows;
			if (const auto currentShout = SBO::MASTERY_SHOUT::Manager::GetSingleton()->GetCurrentShoutKey(); currentShout.has_value()) {
				bool exists = false;
				for (const auto& row : shoutKnownRows) {
					if (row.value("formID", 0u) == currentShout->formID) {
						exists = true;
						break;
					}
				}
				if (!exists) {
					const auto it = shoutStatsByForm.find(currentShout->formID);
					const SBO::MASTERY_SHOUT::MasteryStats stats = it != shoutStatsByForm.end() ? it->second : SBO::MASTERY_SHOUT::MasteryStats{};
					shoutKnownRows.push_back(BuildShoutRow(*currentShout, stats, shoutCfg));
				}
			}

			const auto weaponCfg = SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetConfig();
			json weaponRows = json::array();
			std::unordered_map<std::string, SBO::MASTERY_WEAPON::MasteryStats> weaponStatsByKey{};
			std::unordered_set<std::string> weaponSeenKeys{};
			for (const auto& [key, stats] : SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetMasteryData()) {
				weaponRows.push_back(BuildWeaponRow(key, stats, weaponCfg));
				const auto keyString = SBO::MASTERY_WEAPON::ToString(key);
				weaponStatsByKey[keyString] = stats;
				weaponSeenKeys.insert(keyString);
			}
			json weaponKnownRows = weaponRows;
			auto pushKnownWeapon = [&](const std::optional<SBO::MASTERY_WEAPON::ItemKey>& maybeKey) {
				if (!maybeKey.has_value()) {
					return;
				}
				const auto keyString = SBO::MASTERY_WEAPON::ToString(*maybeKey);
				if (weaponSeenKeys.contains(keyString)) {
					return;
				}
				weaponSeenKeys.insert(keyString);
				const auto it = weaponStatsByKey.find(keyString);
				const SBO::MASTERY_WEAPON::MasteryStats stats = it != weaponStatsByKey.end() ? it->second : SBO::MASTERY_WEAPON::MasteryStats{};
				weaponKnownRows.push_back(BuildWeaponRow(*maybeKey, stats, weaponCfg));
			};
			pushKnownWeapon(SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetCurrentWeaponKey());
			pushKnownWeapon(SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetCurrentShieldKey());

			root["mastery"] = {
				{ "spell", {
					{ "config", ToJson(spellCfg) },
					{ "rows", std::move(spellRows) },
					{ "knownRows", std::move(spellKnownRows) }
				} },
				{ "shout", {
					{ "config", ToJson(shoutCfg) },
					{ "rows", std::move(shoutRows) },
					{ "knownRows", std::move(shoutKnownRows) }
				} },
				{ "weapon", {
					{ "config", ToJson(weaponCfg) },
					{ "rows", std::move(weaponRows) },
					{ "knownRows", std::move(weaponKnownRows) }
				} }
			};
		}

		return root.dump();
	}

	void Manager::PushUISnapshot() const
	{
		UI::PRISMA::Bridge::GetSingleton()->PushSnapshot(BuildSnapshotJson());
	}

	void Manager::PushHUDSnapshot() const
	{
		UI::PRISMA::Bridge::GetSingleton()->PushHUDSnapshot(SBIND::Manager::GetSingleton()->GetHUDSnapshot());
	}

	void Manager::ToggleUI()
	{
		SBIND::Manager::GetSingleton()->ToggleUI();
	}

	void Manager::HandleSetSetting(const std::string& a_payload)
	{
		try {
			const auto payload = json::parse(a_payload);
			const auto module = payload.value("module", std::string{});
			const auto id = payload.value("id", std::string{});
			if (module == "spellBinding") {
				if (payload.contains("key")) {
					SBIND::Manager::GetSingleton()->SetWeaponSettingFromJson(payload.dump());
				} else if (id == "blacklist") {
					SBIND::Manager::GetSingleton()->SetBlacklistFromJson(payload.dump());
				} else {
					SBIND::Manager::GetSingleton()->SetSettingFromJson(payload.dump());
				}
				PushUISnapshot();
				PushHUDSnapshot();
				return;
			}

			if (module == "smartCast") {
				auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
				if (id == "record.toggleKey") cfg.global.record.toggleKey = payload.value("value", cfg.global.record.toggleKey);
				else if (id == "playback.playKey") cfg.global.playback.playKey = payload.value("value", cfg.global.playback.playKey);
				else if (id == "playback.cycleModifierKey") cfg.global.playback.cycleModifierKey = payload.value("value", cfg.global.playback.cycleModifierKey);
				else if (id == "global.maxChains") cfg.global.maxChains = payload.value("value", cfg.global.maxChains);
				else if (id == "playback.stepDelaySec") cfg.global.playback.stepDelaySec = payload.value("value", cfg.global.playback.stepDelaySec);
				else if (id == "hud.alwaysShowInCombat") {
					auto sbCfg = SBIND::Manager::GetSingleton()->GetConfig();
					sbCfg.hudChainAlwaysShowInCombat = payload.value("value", sbCfg.hudChainAlwaysShowInCombat);
					SBIND::Manager::GetSingleton()->SetConfig(sbCfg, true);
				}
				else if (id == "chain.name") {
					const auto index = payload.value("index", 1);
					const auto idx0 = std::max(0, index - 1);
					if (idx0 < static_cast<int>(cfg.chains.size())) {
						cfg.chains[static_cast<std::size_t>(idx0)].name = payload.value("value", cfg.chains[static_cast<std::size_t>(idx0)].name);
					}
					} else if (id == "chain.hotkey") {
						const auto index = payload.value("index", 1);
						const auto idx0 = std::max(0, index - 1);
						if (idx0 < static_cast<int>(cfg.chains.size())) {
							cfg.chains[static_cast<std::size_t>(idx0)].hotkey = payload.value("value", cfg.chains[static_cast<std::size_t>(idx0)].hotkey);
						}
					} else if (id == "chain.step.stepDelaySec") {
						const auto index = payload.value("index", 1);
						const auto idx0 = std::max(0, index - 1);
						if (idx0 < static_cast<int>(cfg.chains.size())) {
							auto value = payload.value("value", cfg.chains[static_cast<std::size_t>(idx0)].stepDelaySec);
							cfg.chains[static_cast<std::size_t>(idx0)].stepDelaySec = std::clamp(value, 0.5f, 3.0f);
						}
					} else if (id == "chain.step.holdSec" || id == "chain.step.castCount") {
						const auto index = payload.value("index", 1);
						const auto step = payload.value("step", 0);
						const auto idx0 = std::max(0, index - 1);
						if (idx0 < static_cast<int>(cfg.chains.size())) {
							auto& steps = cfg.chains[static_cast<std::size_t>(idx0)].steps;
							if (step >= 0 && step < static_cast<int>(steps.size())) {
								auto& entry = steps[static_cast<std::size_t>(step)];
								if (id == "chain.step.holdSec") {
									entry.holdSec = payload.value("value", entry.holdSec);
								} else {
									entry.castCount = payload.value("value", entry.castCount);
								}
							}
						}
					}
				SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
				PushUISnapshot();
				PushHUDSnapshot();
				return;
			}

			if (module == "quickBuff") {
				auto cfg = QUICK_BUFF::Manager::GetSingleton()->GetConfig();
				if (id == "trigger.enabled" || id == "trigger.spellFormID" || id == "trigger.cooldownSec") {
					const auto trigger = ParseTriggerID(payload.value("trigger", std::string{ "combatStart" }));
					auto& triggerCfg = cfg.Get(trigger);
					if (id == "trigger.enabled") triggerCfg.enabled = payload.value("value", triggerCfg.enabled);
					else if (id == "trigger.spellFormID") triggerCfg.spellFormID = payload.value("value", triggerCfg.spellFormID);
					else if (id == "trigger.cooldownSec") triggerCfg.cooldownSec = payload.value("value", triggerCfg.cooldownSec);
				}
				QUICK_BUFF::Manager::GetSingleton()->SetConfig(cfg, true);
				PushUISnapshot();
				return;
			}

			if (module == "mastery") {
				if (id == "masterySpell.config") {
					auto cfg = SBO::MASTERY_SPELL::Manager::GetSingleton()->GetConfig();
					if (payload.contains("value") && payload["value"].is_object()) {
						FromJson(payload["value"], cfg);
						SBO::MASTERY_SPELL::Manager::GetSingleton()->SetConfig(cfg, true);
					}
				} else if (id == "masterySpell.enabled" || id == "masterySpell.gainMultiplier") {
					auto cfg = SBO::MASTERY_SPELL::Manager::GetSingleton()->GetConfig();
					if (id == "masterySpell.enabled") cfg.enabled = payload.value("value", cfg.enabled);
					if (id == "masterySpell.gainMultiplier") cfg.gainMultiplier = payload.value("value", cfg.gainMultiplier);
					SBO::MASTERY_SPELL::Manager::GetSingleton()->SetConfig(cfg, true);
				} else if (id == "masteryShout.config") {
					auto cfg = SBO::MASTERY_SHOUT::Manager::GetSingleton()->GetConfig();
					if (payload.contains("value") && payload["value"].is_object()) {
						FromJson(payload["value"], cfg);
						SBO::MASTERY_SHOUT::Manager::GetSingleton()->SetConfig(cfg, true);
					}
				} else if (id == "masteryShout.enabled" || id == "masteryShout.gainMultiplier") {
					auto cfg = SBO::MASTERY_SHOUT::Manager::GetSingleton()->GetConfig();
					if (id == "masteryShout.enabled") cfg.enabled = payload.value("value", cfg.enabled);
					if (id == "masteryShout.gainMultiplier") cfg.gainMultiplier = payload.value("value", cfg.gainMultiplier);
					SBO::MASTERY_SHOUT::Manager::GetSingleton()->SetConfig(cfg, true);
				} else if (id == "masteryWeapon.config") {
					auto cfg = SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetConfig();
					if (payload.contains("value") && payload["value"].is_object()) {
						FromJson(payload["value"], cfg);
						SBO::MASTERY_WEAPON::Manager::GetSingleton()->SetConfig(cfg, true);
					}
				} else if (id == "masteryWeapon.enabled" || id == "masteryWeapon.gainMultiplier") {
					auto cfg = SBO::MASTERY_WEAPON::Manager::GetSingleton()->GetConfig();
					if (id == "masteryWeapon.enabled") cfg.enabled = payload.value("value", cfg.enabled);
					if (id == "masteryWeapon.gainMultiplier") cfg.gainMultiplier = payload.value("value", cfg.gainMultiplier);
					SBO::MASTERY_WEAPON::Manager::GetSingleton()->SetConfig(cfg, true);
				}
				PushUISnapshot();
				return;
			}

			if (module == "ui") {
				if (id == "spellBindingWindow") {
					SBIND::Manager::GetSingleton()->SetUIWindowFromJson(payload.dump());
				}
				PushUISnapshot();
			}
		} catch (const std::exception& e) {
			LOG_WARN("SpellbladeOverhaul: HandleSetSetting parse failed - {}", e.what());
		}
	}

	void Manager::HandleAction(const std::string& a_payload)
	{
		try {
			const auto payload = json::parse(a_payload);
			const auto module = payload.value("module", std::string{});
			const auto action = payload.value("action", std::string{});

			if (module == "spellBinding") {
				if (action == "cycleBindSlot") SBIND::Manager::GetSingleton()->CycleBindSlotMode();
				else if (action == "bindSelected") SBIND::Manager::GetSingleton()->TryBindSelectedMagicMenuSpell();
				else if (action == "bindSpellForSlot") SBIND::Manager::GetSingleton()->BindSpellForSlotFromJson(payload.dump());
				else if (action == "unbindSlot") SBIND::Manager::GetSingleton()->UnbindSlotFromJson(payload.dump());
				else if (action == "unbindWeapon") {
					if (payload.contains("key")) {
						SBIND::Manager::GetSingleton()->UnbindWeaponFromSerializedKey(payload["key"].dump());
					} else {
						SBIND::Manager::GetSingleton()->UnbindWeaponFromSerializedKey(payload.dump());
					}
				}
				PushUISnapshot();
				PushHUDSnapshot();
				return;
			}

			if (module == "smartCast") {
				if (action == "startRecording") SMART_CAST::Controller::GetSingleton()->StartRecording(payload.value("index", 1));
				else if (action == "stopRecording") SMART_CAST::Controller::GetSingleton()->StopRecording();
				else if (action == "startPlayback") SMART_CAST::Controller::GetSingleton()->StartPlayback(payload.value("index", 1));
				else if (action == "stopPlayback") SMART_CAST::Controller::GetSingleton()->StopPlayback(true);
				else if (action == "clearChain") {
					auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
					const auto idx = std::max(0, payload.value("index", 1) - 1);
					if (idx < static_cast<int>(cfg.chains.size())) {
						cfg.chains[static_cast<std::size_t>(idx)].steps.clear();
						SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
					}
				} else if (action == "deleteChain") {
					auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
					const auto idx = std::max(0, payload.value("index", 1) - 1);
					if (idx < static_cast<int>(cfg.chains.size())) {
						cfg.chains.erase(cfg.chains.begin() + idx);
						cfg.global.maxChains = std::max(1, static_cast<int>(cfg.chains.size()));
						SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
					}
				} else if (action == "removeStep") {
					auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
					const auto idx = std::max(0, payload.value("index", 1) - 1);
					const auto step = std::max(0, payload.value("step", 0));
					if (idx < static_cast<int>(cfg.chains.size())) {
						auto& steps = cfg.chains[static_cast<std::size_t>(idx)].steps;
						if (step < static_cast<int>(steps.size())) {
							steps.erase(steps.begin() + step);
							SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
						}
					}
				} else if (action == "moveStep") {
					auto cfg = SMART_CAST::Controller::GetSingleton()->GetConfig();
					const auto idx = std::max(0, payload.value("index", 1) - 1);
					const auto step = std::max(0, payload.value("step", 0));
					const auto direction = payload.value("direction", std::string{ "up" });
					if (idx < static_cast<int>(cfg.chains.size())) {
						auto& steps = cfg.chains[static_cast<std::size_t>(idx)].steps;
						if (step < static_cast<int>(steps.size())) {
							const int target = direction == "down" ? (step + 1) : (step - 1);
							if (target >= 0 && target < static_cast<int>(steps.size())) {
								std::swap(steps[static_cast<std::size_t>(step)], steps[static_cast<std::size_t>(target)]);
								SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
							}
						}
					}
				}
				PushUISnapshot();
				return;
			}

			if (module == "quickBuff") {
				if (action == "testTrigger") {
					const auto trigger = ParseTriggerID(payload.value("trigger", std::string{ "combatStart" }));
					QUICK_BUFF::Manager::GetSingleton()->TryTestCast(trigger);
				}
				PushUISnapshot();
				return;
			}

			if (module == "mastery") {
				if (action == "masterySpell.clearDatabase") SBO::MASTERY_SPELL::Manager::GetSingleton()->ClearDatabase();
				else if (action == "masterySpell.resetDefaults") SBO::MASTERY_SPELL::Manager::GetSingleton()->ResetAllConfigToDefault(true);
				else if (action == "masteryShout.clearDatabase") SBO::MASTERY_SHOUT::Manager::GetSingleton()->ClearDatabase();
				else if (action == "masteryShout.resetDefaults") SBO::MASTERY_SHOUT::Manager::GetSingleton()->ResetAllConfigToDefault(true);
				else if (action == "masteryWeapon.clearDatabase") SBO::MASTERY_WEAPON::Manager::GetSingleton()->ClearDatabase();
				else if (action == "masteryWeapon.resetDefaults") SBO::MASTERY_WEAPON::Manager::GetSingleton()->ResetAllConfigToDefault(true);
				PushUISnapshot();
			}
		} catch (const std::exception& e) {
			LOG_WARN("SpellbladeOverhaul: HandleAction parse failed - {}", e.what());
		}
	}

	void Manager::HandleHudAction(const std::string& a_payload)
	{
		try {
			const auto payload = json::parse(a_payload);
			const auto action = payload.value("action", std::string{});
			if (action == "enterDragMode") {
				SBIND::Manager::GetSingleton()->EnterHudDragMode();
			} else if (action == "saveHudPosition" || action.empty()) {
				SBIND::Manager::GetSingleton()->SaveHudPositionFromJson(payload.dump());
			}
			PushHUDSnapshot();
			PushUISnapshot();
		} catch (const std::exception& e) {
			LOG_WARN("SpellbladeOverhaul: HandleHudAction parse failed - {}", e.what());
		}
	}
}
