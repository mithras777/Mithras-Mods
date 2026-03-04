#include "overhaul/SpellbladeOverhaulManager.h"

#include "buff/QuickBuffManager.h"
#include "smartcast/SmartCastController.h"
#include "spellbinding/SpellBindingManager.h"
#include "ui/PrismaBridge.h"
#include "util/LogUtil.h"

#include "json/single_include/nlohmann/json.hpp"

#include <algorithm>

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
				{ "holdSec", a_step.holdSec }
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
				{ "steps", std::move(steps) }
			};
		}

		[[nodiscard]] json ToJson(const SMART_CAST::KnownSpellOption& a_spell)
		{
			return json{
				{ "name", a_spell.name },
				{ "formKey", a_spell.formKey },
				{ "formID", a_spell.formID }
			};
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
		ForceEnabledFlags(true);
	}

	void Manager::Update(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		SBIND::Manager::GetSingleton()->Update(a_player, a_deltaTime);
		SMART_CAST::Controller::GetSingleton()->Update(a_player, a_deltaTime);
		QUICK_BUFF::Manager::GetSingleton()->Update(a_player, a_deltaTime);
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		SBIND::Manager::GetSingleton()->Serialize(a_intfc);
		SMART_CAST::Controller::GetSingleton()->Serialize(a_intfc);
		QUICK_BUFF::Manager::GetSingleton()->Serialize(a_intfc);
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		SBIND::Manager::GetSingleton()->Deserialize(a_intfc);
		SMART_CAST::Controller::GetSingleton()->Deserialize(a_intfc);
		QUICK_BUFF::Manager::GetSingleton()->Deserialize(a_intfc);
		ForceEnabledFlags(false);
	}

	void Manager::OnRevert()
	{
		SBIND::Manager::GetSingleton()->OnRevert();
		SMART_CAST::Controller::GetSingleton()->OnRevert();
		QUICK_BUFF::Manager::GetSingleton()->OnRevert();
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
							{ "defaultChainIndex", cfg.global.playback.defaultChainIndex },
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
						{ "preventCastingWhileRagdoll", cfg.global.preventCastingWhileRagdoll },
						{ "minTimeAfterLoadSeconds", cfg.global.minTimeAfterLoadSeconds }
					} },
					{ "triggers", std::move(triggers) }
				} },
				{ "knownSpells", std::move(knownSpells) }
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
				else if (id == "playback.defaultChainIndex") cfg.global.playback.defaultChainIndex = payload.value("value", cfg.global.playback.defaultChainIndex);
				else if (id == "playback.stepDelaySec") cfg.global.playback.stepDelaySec = payload.value("value", cfg.global.playback.stepDelaySec);
				else if (id == "chain.name") {
					const auto index = payload.value("index", 1);
					const auto idx0 = std::max(0, index - 1);
					if (idx0 < static_cast<int>(cfg.chains.size())) {
						cfg.chains[static_cast<std::size_t>(idx0)].name = payload.value("value", cfg.chains[static_cast<std::size_t>(idx0)].name);
					}
				}
				SMART_CAST::Controller::GetSingleton()->SetConfig(cfg, true);
				PushUISnapshot();
				return;
			}

			if (module == "quickBuff") {
				auto cfg = QUICK_BUFF::Manager::GetSingleton()->GetConfig();
				if (id == "global.minTimeAfterLoadSeconds") {
					cfg.global.minTimeAfterLoadSeconds = payload.value("value", cfg.global.minTimeAfterLoadSeconds);
				} else if (id == "trigger.enabled" || id == "trigger.spellFormID" || id == "trigger.cooldownSec") {
					const auto trigger = ParseTriggerID(payload.value("trigger", std::string{ "combatStart" }));
					auto& triggerCfg = cfg.Get(trigger);
					if (id == "trigger.enabled") triggerCfg.enabled = payload.value("value", triggerCfg.enabled);
					else if (id == "trigger.spellFormID") triggerCfg.spellFormID = payload.value("value", triggerCfg.spellFormID);
					else if (id == "trigger.cooldownSec") triggerCfg.cooldownSec = payload.value("value", triggerCfg.cooldownSec);
				}
				QUICK_BUFF::Manager::GetSingleton()->SetConfig(cfg, true);
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
