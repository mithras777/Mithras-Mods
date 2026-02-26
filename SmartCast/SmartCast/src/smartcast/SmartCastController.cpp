#include "smartcast/SmartCastController.h"

#include "RE/C/CrosshairPickData.h"
#include "RE/S/SendHUDMessage.h"
#include "RE/T/TESShout.h"
#include "event/GameEventManager.h"

#include <Windows.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>
#include "json/single_include/nlohmann/json.hpp"

namespace SMART_CAST
{
	using json = nlohmann::json;

	namespace
	{
		constexpr std::uint32_t kConfigVersion = 1;
		void Notify(const std::string& a_text)
		{
			RE::SendHUDMessage::ShowHUDMessage(a_text.c_str());
		}

		std::string Trim(std::string_view text)
		{
			const auto begin = text.find_first_not_of(" \t\r\n");
			if (begin == std::string_view::npos) {
				return {};
			}
			const auto end = text.find_last_not_of(" \t\r\n");
			return std::string(text.substr(begin, end - begin + 1));
		}

		const char* ToString(StepType v) { return v == StepType::kConcentration ? "concentration" : "fireAndForget"; }
		StepType ToStepType(const std::string& s) { return s == "concentration" ? StepType::kConcentration : StepType::kFireAndForget; }
		const char* ToString(CastOn v)
		{
			switch (v) {
				case CastOn::kCrosshair: return "crosshair";
				case CastOn::kAimed: return "aimed";
				default: return "self";
			}
		}
		CastOn ToCastOn(const std::string& s)
		{
			if (s == "crosshair") return CastOn::kCrosshair;
			if (s == "aimed") return CastOn::kAimed;
			return CastOn::kSelf;
		}
	}

	Config Controller::GetDefaultConfig()
	{
		Config cfg{};
		cfg.version = kConfigVersion;
		cfg.global.maxChains = 6;
		cfg.global.maxStepsPerChain = 0;
		cfg.global.playback.defaultChainIndex = 1;
		cfg.global.record.ignorePowers = false;
		cfg.global.record.ignoreShouts = false;
		cfg.global.record.ignoreScrolls = true;
		cfg.global.record.maxIdleSec = 6.0f;
		cfg.chains.resize(static_cast<std::size_t>(cfg.global.maxChains));
		for (std::size_t i = 0; i < cfg.chains.size(); ++i) {
			cfg.chains[i].name = std::format("Chain {}", i + 1);
			cfg.chains[i].enabled = true;
			cfg.chains[i].hotkey = "None";
		}
		return cfg;
	}

	void Controller::Initialize()
	{
		m_config = GetDefaultConfig();
		LoadConfig();
		ClampConfig(m_config);
		EnsureChainCount(m_config);
		ResetRuntime();
	}

	void Controller::ResetRuntime()
	{
		m_runtime = RuntimeState{};
		m_leftCapture = CastCapture{};
		m_rightCapture = CastCapture{};
	}

	Config Controller::GetConfig() const { return m_config; }

	void Controller::SetConfig(const Config& a_config, bool a_save)
	{
		m_config = a_config;
		ClampConfig(m_config);
		EnsureChainCount(m_config);
		if (a_save) {
			SaveConfig();
		}
	}

	void Controller::LoadConfig()
	{
		std::ifstream ifs(std::string(kConfigPath), std::ios::in);
		if (!ifs.is_open()) {
			SaveConfig();
			return;
		}

		json root;
		try { ifs >> root; } catch (...) { return; }

		Config cfg = GetDefaultConfig();
		cfg.version = root.value("version", cfg.version);
		if (root.contains("global") && root["global"].is_object()) {
			const auto& g = root["global"];
			cfg.global.enabled = g.value("enabled", cfg.global.enabled);
			cfg.global.firstPersonOnly = g.value("firstPersonOnly", cfg.global.firstPersonOnly);
			cfg.global.preventInMenus = g.value("preventInMenus", cfg.global.preventInMenus);
			cfg.global.preventWhileStaggered = g.value("preventWhileStaggered", cfg.global.preventWhileStaggered);
			cfg.global.preventWhileRagdoll = g.value("preventWhileRagdoll", cfg.global.preventWhileRagdoll);
			cfg.global.minTimeAfterLoadSeconds = g.value("minTimeAfterLoadSeconds", cfg.global.minTimeAfterLoadSeconds);
			cfg.global.maxChains = g.value("maxChains", cfg.global.maxChains);
			cfg.global.maxStepsPerChain = g.value("maxStepsPerChain", cfg.global.maxStepsPerChain);
			if (g.contains("record") && g["record"].is_object()) {
				const auto& r = g["record"];
				cfg.global.record.toggleKey = r.value("toggleKey", cfg.global.record.toggleKey);
				cfg.global.record.cancelKey = r.value("cancelKey", cfg.global.record.cancelKey);
				cfg.global.record.maxIdleSec = r.value("maxIdleSec", cfg.global.record.maxIdleSec);
				cfg.global.record.recordOnlySuccessfulCasts = r.value("recordOnlySuccessfulCasts", cfg.global.record.recordOnlySuccessfulCasts);
				cfg.global.record.ignorePowers = r.value("ignorePowers", cfg.global.record.ignorePowers);
				cfg.global.record.ignoreShouts = r.value("ignoreShouts", cfg.global.record.ignoreShouts);
				cfg.global.record.ignoreScrolls = r.value("ignoreScrolls", cfg.global.record.ignoreScrolls);
			}
			if (g.contains("playback") && g["playback"].is_object()) {
				const auto& p = g["playback"];
				cfg.global.playback.playKey = p.value("playKey", cfg.global.playback.playKey);
				cfg.global.playback.cancelKey = p.value("cancelKey", cfg.global.playback.cancelKey);
				cfg.global.playback.defaultChainIndex = p.value("defaultChainIndex", cfg.global.playback.defaultChainIndex);
				cfg.global.playback.stepDelaySec = p.value("stepDelaySec", cfg.global.playback.stepDelaySec);
				cfg.global.playback.abortOnFail = p.value("abortOnFail", cfg.global.playback.abortOnFail);
				cfg.global.playback.skipOnFail = p.value("skipOnFail", cfg.global.playback.skipOnFail);
				cfg.global.playback.stopIfPlayerHit = p.value("stopIfPlayerHit", cfg.global.playback.stopIfPlayerHit);
				cfg.global.playback.stopIfAttackPressed = p.value("stopIfAttackPressed", cfg.global.playback.stopIfAttackPressed);
				cfg.global.playback.stopIfBlockPressed = p.value("stopIfBlockPressed", cfg.global.playback.stopIfBlockPressed);
			}
			if (g.contains("concentration") && g["concentration"].is_object()) {
				const auto& c = g["concentration"];
				cfg.global.concentration.minHoldSec = c.value("minHoldSec", cfg.global.concentration.minHoldSec);
				cfg.global.concentration.maxHoldSec = c.value("maxHoldSec", cfg.global.concentration.maxHoldSec);
				cfg.global.concentration.sampleGranularitySec = c.value("sampleGranularitySec", cfg.global.concentration.sampleGranularitySec);
				cfg.global.concentration.releasePaddingSec = c.value("releasePaddingSec", cfg.global.concentration.releasePaddingSec);
			}
			if (g.contains("targeting") && g["targeting"].is_object()) {
				const auto& t = g["targeting"];
				cfg.global.targeting.mode = t.value("mode", cfg.global.targeting.mode);
				cfg.global.targeting.preferCrosshairActor = t.value("preferCrosshairActor", cfg.global.targeting.preferCrosshairActor);
				cfg.global.targeting.fallbackToSelfIfNoTarget = t.value("fallbackToSelfIfNoTarget", cfg.global.targeting.fallbackToSelfIfNoTarget);
			}
		}

		if (root.contains("chains") && root["chains"].is_array()) {
			cfg.chains.clear();
			for (const auto& chainNode : root["chains"]) {
				ChainConfig chain{};
				chain.name = chainNode.value("name", chain.name);
				chain.enabled = chainNode.value("enabled", chain.enabled);
				chain.hotkey = chainNode.value("hotkey", chain.hotkey);
				if (chainNode.contains("steps") && chainNode["steps"].is_array()) {
					for (const auto& stepNode : chainNode["steps"]) {
						ChainStep step{};
						step.spellFormID = stepNode.value("spellFormID", step.spellFormID);
						step.type = ToStepType(stepNode.value("type", std::string("fireAndForget")));
						step.castOn = ToCastOn(stepNode.value("castOn", std::string("self")));
						step.holdSec = stepNode.value("holdSec", step.holdSec);
						chain.steps.push_back(std::move(step));
					}
				}
				cfg.chains.push_back(std::move(chain));
			}
		}

		ClampConfig(cfg);
		EnsureChainCount(cfg);
		m_config = cfg;
	}

	void Controller::SaveConfig() const
	{
		json chains = json::array();
		for (const auto& chain : m_config.chains) {
			json steps = json::array();
			for (const auto& step : chain.steps) {
				steps.push_back({ { "spellFormID", step.spellFormID }, { "type", ToString(step.type) }, { "castOn", ToString(step.castOn) }, { "holdSec", step.holdSec } });
			}
			chains.push_back({ { "name", chain.name }, { "enabled", chain.enabled }, { "hotkey", chain.hotkey }, { "steps", steps } });
		}

		const json root = {
			{ "version", m_config.version },
			{ "global", {
				{ "enabled", m_config.global.enabled },
				{ "firstPersonOnly", m_config.global.firstPersonOnly },
				{ "preventInMenus", m_config.global.preventInMenus },
				{ "preventWhileStaggered", m_config.global.preventWhileStaggered },
				{ "preventWhileRagdoll", m_config.global.preventWhileRagdoll },
				{ "minTimeAfterLoadSeconds", m_config.global.minTimeAfterLoadSeconds },
				{ "maxChains", m_config.global.maxChains },
				{ "maxStepsPerChain", m_config.global.maxStepsPerChain },
				{ "record", {
					{ "toggleKey", m_config.global.record.toggleKey },
					{ "cancelKey", m_config.global.record.cancelKey },
					{ "maxIdleSec", m_config.global.record.maxIdleSec },
					{ "recordOnlySuccessfulCasts", m_config.global.record.recordOnlySuccessfulCasts },
					{ "ignorePowers", m_config.global.record.ignorePowers },
					{ "ignoreShouts", m_config.global.record.ignoreShouts },
					{ "ignoreScrolls", m_config.global.record.ignoreScrolls }
				} },
				{ "playback", {
					{ "playKey", m_config.global.playback.playKey },
					{ "cancelKey", m_config.global.playback.cancelKey },
					{ "defaultChainIndex", m_config.global.playback.defaultChainIndex },
					{ "stepDelaySec", m_config.global.playback.stepDelaySec },
					{ "abortOnFail", m_config.global.playback.abortOnFail },
					{ "skipOnFail", m_config.global.playback.skipOnFail },
					{ "requireWeaponSheathed", m_config.global.playback.requireWeaponSheathed },
					{ "autoSheathDuringPlayback", m_config.global.playback.autoSheathDuringPlayback },
					{ "stopIfPlayerHit", m_config.global.playback.stopIfPlayerHit },
					{ "stopIfAttackPressed", m_config.global.playback.stopIfAttackPressed },
					{ "stopIfBlockPressed", m_config.global.playback.stopIfBlockPressed }
				} },
				{ "concentration", {
					{ "minHoldSec", m_config.global.concentration.minHoldSec },
					{ "maxHoldSec", m_config.global.concentration.maxHoldSec },
					{ "sampleGranularitySec", m_config.global.concentration.sampleGranularitySec },
					{ "releasePaddingSec", m_config.global.concentration.releasePaddingSec }
				} },
				{ "targeting", {
					{ "mode", m_config.global.targeting.mode },
					{ "raycastRange", m_config.global.targeting.raycastRange },
					{ "aimConeDegrees", m_config.global.targeting.aimConeDegrees },
					{ "preferCrosshairActor", m_config.global.targeting.preferCrosshairActor },
					{ "fallbackToSelfIfNoTarget", m_config.global.targeting.fallbackToSelfIfNoTarget }
				} }
			} },
			{ "chains", chains }
		};

		try {
			const std::filesystem::path path{ std::string(kConfigPath) };
			std::filesystem::create_directories(path.parent_path());
			std::ofstream ofs(path, std::ios::out | std::ios::trunc);
			ofs << root.dump(2);
		} catch (...) {}
	}

	void Controller::ClampConfig(Config& cfg) const
	{
		cfg.version = kConfigVersion;
		cfg.global.maxChains = 6;
		cfg.global.maxStepsPerChain = 0;
		cfg.global.playback.defaultChainIndex = std::clamp(cfg.global.playback.defaultChainIndex, 1, cfg.global.maxChains);
		cfg.global.minTimeAfterLoadSeconds = std::clamp(cfg.global.minTimeAfterLoadSeconds, 0.0f, 10.0f);
		cfg.global.record.maxIdleSec = std::clamp(cfg.global.record.maxIdleSec, 0.5f, 30.0f);
		cfg.global.record.cancelKey = cfg.global.record.toggleKey;
		cfg.global.record.ignorePowers = false;
		cfg.global.record.ignoreShouts = false;
		cfg.global.record.ignoreScrolls = true;
		cfg.global.playback.stepDelaySec = std::clamp(cfg.global.playback.stepDelaySec, 0.0f, 2.0f);
		cfg.global.playback.cancelKey = cfg.global.playback.playKey;
		cfg.global.concentration.minHoldSec = std::clamp(cfg.global.concentration.minHoldSec, 0.05f, 10.0f);
		cfg.global.concentration.maxHoldSec = std::clamp(cfg.global.concentration.maxHoldSec, cfg.global.concentration.minHoldSec, 10.0f);
		cfg.global.concentration.sampleGranularitySec = std::clamp(cfg.global.concentration.sampleGranularitySec, 0.01f, 1.0f);
		cfg.global.concentration.releasePaddingSec = std::clamp(cfg.global.concentration.releasePaddingSec, 0.0f, 1.0f);
		for (auto& chain : cfg.chains) {
			if (chain.name.empty()) chain.name = "Chain";
			chain.enabled = true;
			for (auto& step : chain.steps) {
				step.holdSec = std::clamp(step.holdSec, 0.05f, 10.0f);
			}
		}
	}

	void Controller::EnsureChainCount(Config& cfg) const
	{
		const auto target = static_cast<std::size_t>(6);
		if (cfg.chains.size() < target) {
			const auto old = cfg.chains.size();
			cfg.chains.resize(target);
			for (std::size_t i = old; i < target; ++i) {
				cfg.chains[i].name = std::format("Chain {}", i + 1);
				cfg.chains[i].enabled = true;
				cfg.chains[i].hotkey = "None";
			}
		} else if (cfg.chains.size() > target) {
			cfg.chains.resize(target);
		}
	}

	std::int32_t Controller::ResolveChainIndex(std::int32_t a_index1Based) const
	{
		if (m_config.chains.empty()) {
			return -1;
		}
		return std::clamp(a_index1Based, 1, static_cast<std::int32_t>(m_config.chains.size())) - 1;
	}

	void Controller::StartRecording(std::int32_t a_chainIndex1Based)
	{
		const auto idx = ResolveChainIndex(a_chainIndex1Based);
		if (idx < 0) return;
		if (m_runtime.mode == Mode::kPlaying) StopPlayback(true);
		m_runtime.mode = Mode::kRecording;
		m_runtime.activeChainIndex = idx;
		m_runtime.recordIdleTimer = 0.0f;
		Notify(std::format("SmartCast: Recording chain {}", idx + 1));
	}

	void Controller::StopRecording()
	{
		if (m_runtime.mode == Mode::kRecording) {
			const auto chain = m_runtime.activeChainIndex + 1;
			m_runtime.mode = Mode::kIdle;
			m_runtime.recordIdleTimer = 0.0f;
			m_leftCapture = CastCapture{};
			m_rightCapture = CastCapture{};
			Notify(std::format("SmartCast: Stopped recording chain {}", chain));
		}
	}

	void Controller::StartPlayback(std::int32_t a_chainIndex1Based)
	{
		const auto idx = ResolveChainIndex(a_chainIndex1Based);
		if (idx < 0) return;
		const auto& chain = m_config.chains[static_cast<std::size_t>(idx)];
		if (chain.steps.empty()) return;
		if (m_runtime.mode == Mode::kRecording) StopRecording();
		m_runtime.mode = Mode::kPlaying;
		m_runtime.activeChainIndex = idx;
		m_runtime.playbackStepIndex = 0;
		m_runtime.stepDelayTimer = 0.0f;
		m_runtime.concentrationHoldRemaining = 0.0f;
		m_runtime.releasePaddingRemaining = 0.0f;
		m_runtime.concentrationTickTimer = 0.0f;
		m_runtime.activeConcentrationSpell.clear();
		m_runtime.playbackTarget = RE::ObjectRefHandle{};
		m_runtime.playbackCastOnSelf = true;
		Notify(std::format("SmartCast: Playing chain {}", idx + 1));
	}

	void Controller::StopPlayback(bool)
	{
		if (m_runtime.mode == Mode::kPlaying) {
			const auto chain = m_runtime.activeChainIndex + 1;
			m_runtime.mode = Mode::kIdle;
			m_runtime.playbackStepIndex = 0;
			m_runtime.stepDelayTimer = 0.0f;
			m_runtime.concentrationHoldRemaining = 0.0f;
			m_runtime.releasePaddingRemaining = 0.0f;
			m_runtime.concentrationTickTimer = 0.0f;
			m_runtime.activeConcentrationSpell.clear();
			m_runtime.playbackTarget = RE::ObjectRefHandle{};
			m_runtime.playbackCastOnSelf = true;
			Notify(std::format("SmartCast: Stopped chain {}", chain));
		}
	}

	bool Controller::IsRecording() const { return m_runtime.mode == Mode::kRecording; }
	bool Controller::IsPlaying() const { return m_runtime.mode == Mode::kPlaying; }
	std::int32_t Controller::GetActiveChainIndex1Based() const { return m_runtime.activeChainIndex + 1; }
	float Controller::Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
	float Controller::Quantize(float v, float step) { return step <= 0.0f ? v : (std::round(v / step) * step); }

	bool Controller::ShouldBlockGlobal(RE::PlayerCharacter* player) const
	{
		if (!player) return true;
		if (m_runtime.timeSinceLoad < m_config.global.minTimeAfterLoadSeconds) return true;
		if (m_config.global.firstPersonOnly) {
			auto* camera = RE::PlayerCamera::GetSingleton();
			if (!camera || !camera->IsInFirstPerson()) return true;
		}
		if (m_config.global.preventWhileStaggered && player->IsStaggering()) return true;
		if (m_config.global.preventWhileRagdoll && player->IsInRagdollState()) return true;
		return false;
	}

	bool Controller::IsKeyDown(const std::string& key) const
	{
		if (key.empty() || key == "None") return false;
		if (key.size() == 1) {
			char c = static_cast<char>(std::toupper(static_cast<unsigned char>(key[0])));
			if (c >= 'A' && c <= 'Z') {
				return (::GetAsyncKeyState(static_cast<int>(c)) & 0x8000) != 0;
			}
		}
		if (key.rfind("F", 0) == 0 && key.size() <= 3) {
			int fn = 0;
			const auto [ptr, ec] = std::from_chars(key.data() + 1, key.data() + key.size(), fn);
			if (ec == std::errc{} && ptr == key.data() + key.size() && fn >= 1 && fn <= 12) {
				return (::GetAsyncKeyState(VK_F1 + fn - 1) & 0x8000) != 0;
			}
		}
		if (key == "Mouse4") return (::GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
		if (key == "Mouse5") return (::GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
		return false;
	}

	bool Controller::IsHotkeyPressed(HotkeySlot slot, const std::string& key)
	{
		const auto idx = static_cast<std::size_t>(slot);
		const bool now = IsKeyDown(key);
		const bool pressed = now && !m_runtime.keyWasDown[idx];
		m_runtime.keyWasDown[idx] = now;
		return pressed;
	}

	void Controller::UpdateHotkeys(RE::PlayerCharacter*)
	{
		const bool recordTogglePressed = IsHotkeyPressed(HotkeySlot::kRecordToggle, m_config.global.record.toggleKey);
		const bool recordCancelPressed = (m_config.global.record.cancelKey != "None") &&
			IsHotkeyPressed(HotkeySlot::kRecordCancel, m_config.global.record.cancelKey);
		const bool playPressed = IsHotkeyPressed(HotkeySlot::kPlay, m_config.global.playback.playKey);
		const bool playCancelPressed = (m_config.global.playback.cancelKey != "None") &&
			IsHotkeyPressed(HotkeySlot::kPlayCancel, m_config.global.playback.cancelKey);
		auto* ui = RE::UI::GetSingleton();
		const bool menuBlocked = GAME_EVENT::Manager::GetSingleton()->IsBlockingMenuOpen();
		const bool paused = !ui || ui->GameIsPaused();
		const bool allowPlaybackHotkeys = !menuBlocked && !paused;

		if (recordTogglePressed) {
			if (m_runtime.mode == Mode::kRecording) StopRecording(); else StartRecording(m_config.global.playback.defaultChainIndex);
		}
		if (!recordTogglePressed && recordCancelPressed && m_runtime.mode == Mode::kRecording) {
			StopRecording();
		}

		const bool samePlayCancelKey = (m_config.global.playback.playKey == m_config.global.playback.cancelKey);
		if (allowPlaybackHotkeys && playPressed) {
			if (m_runtime.mode == Mode::kPlaying) {
				StopPlayback(true);
			} else {
				StartPlayback(m_config.global.playback.defaultChainIndex);
			}
		}
		if (allowPlaybackHotkeys && !playPressed && !samePlayCancelKey && playCancelPressed && m_runtime.mode == Mode::kPlaying) {
			StopPlayback(true);
		}
	}

	void Controller::ProcessCastCapture(RE::PlayerCharacter* player, CastCapture& cap, bool castingNow, RE::TESForm* equipped)
	{
		auto* spell = equipped ? equipped->As<RE::SpellItem>() : nullptr;
		if (!spell) {
			cap = CastCapture{};
			return;
		}
		if (!cap.started && castingNow) {
			cap.started = true;
			cap.spell = spell;
			cap.startTime = m_runtime.timeSinceLoad;
			RecordStepOnCastStart(player, spell);
			return;
		}
		if (cap.started && !castingNow) {
			cap = CastCapture{};
		}
	}

	void Controller::RecordStepOnCastStart(RE::PlayerCharacter*, RE::SpellItem* spell)
	{
		if (!spell || m_runtime.mode != Mode::kRecording) return;
		const auto idx = static_cast<std::size_t>(m_runtime.activeChainIndex);
		if (idx >= m_config.chains.size()) return;
		auto& chain = m_config.chains[idx];
		const auto spellType = spell->GetSpellType();
		if (m_config.global.record.ignoreScrolls && spellType == RE::MagicSystem::SpellType::kScroll) return;

		ChainStep step{};
		step.spellFormID = BuildFormKey(spell);
		step.type = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration ? StepType::kConcentration : StepType::kFireAndForget;
		step.holdSec = step.type == StepType::kConcentration ? m_config.global.concentration.minHoldSec : 0.0f;
		step.castOn = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf ? CastOn::kSelf : CastOn::kCrosshair;

		if (!chain.steps.empty()) {
			const auto& last = chain.steps.back();
			if (last.spellFormID == step.spellFormID) {
				return;
			}
		}

		chain.steps.push_back(std::move(step));
		m_runtime.recordIdleTimer = 0.0f;
	}

	void Controller::RecordConcentrationDuration(RE::SpellItem* spell, float duration)
	{
		if (!spell || m_runtime.mode != Mode::kRecording) return;
		const auto idx = static_cast<std::size_t>(m_runtime.activeChainIndex);
		if (idx >= m_config.chains.size() || m_config.chains[idx].steps.empty()) return;
		const auto key = BuildFormKey(spell);
		for (auto it = m_config.chains[idx].steps.rbegin(); it != m_config.chains[idx].steps.rend(); ++it) {
			if (it->spellFormID == key && it->type == StepType::kConcentration) {
				float h = std::clamp(duration, m_config.global.concentration.minHoldSec, m_config.global.concentration.maxHoldSec);
				it->holdSec = Quantize(h, m_config.global.concentration.sampleGranularitySec);
				break;
			}
		}
	}

	bool Controller::ExecuteStep(RE::PlayerCharacter* player, const ChainStep& step)
	{
		auto* spell = ResolveSpell(step.spellFormID);
		if (!spell || !player->HasSpell(spell)) return false;
		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		const auto castOn = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf ? CastOn::kSelf : CastOn::kCrosshair;
		RE::ObjectRefHandle handle{};
		bool castOnSelf = true;
		const bool ok = CastSpellForStep(player, spell, castOn, isConcentration, &handle, &castOnSelf);
		if (!ok) return false;
		if (isConcentration) {
			m_runtime.concentrationHoldRemaining = std::clamp(step.holdSec, 0.05f, 10.0f);
			m_runtime.releasePaddingRemaining = m_config.global.concentration.releasePaddingSec;
			m_runtime.concentrationTickTimer = 0.30f;
			m_runtime.activeConcentrationSpell = step.spellFormID;
			m_runtime.playbackTarget = handle;
			m_runtime.playbackCastOnSelf = castOnSelf;
		} else {
			m_runtime.concentrationTickTimer = 0.0f;
			m_runtime.activeConcentrationSpell.clear();
			m_runtime.playbackTarget = RE::ObjectRefHandle{};
			m_runtime.playbackCastOnSelf = true;
		}
		return true;
	}

	bool Controller::CastSpellForStep(RE::PlayerCharacter* player, RE::SpellItem* spell, CastOn castOn, bool, RE::ObjectRefHandle* targetOut, bool* castOnSelfOut)
	{
		auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
		if (!caster) return false;

		RE::TESObjectREFR* target = player;
		bool castOnSelf = true;

		if (spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf || castOn == CastOn::kSelf) {
			target = player;
			castOnSelf = true;
		} else {
			// "Aimed" behavior: let the engine cast forward from current view/aim without crosshair target forcing.
			target = player;
			castOnSelf = false;
		}

		caster->CastSpellImmediate(spell, castOnSelf, target, 1.0f, false, 0.0f, player);
		if (targetOut) *targetOut = castOnSelf ? RE::ObjectRefHandle{} : target->CreateRefHandle();
		if (castOnSelfOut) *castOnSelfOut = castOnSelf;
		return true;
	}

	void Controller::UpdateRecording(RE::PlayerCharacter* player, float delta)
	{
		if (m_runtime.mode != Mode::kRecording) return;
		if (ShouldBlockGlobal(player)) { StopRecording(); return; }
		m_runtime.recordIdleTimer += delta;
		if (m_runtime.recordIdleTimer >= m_config.global.record.maxIdleSec) { StopRecording(); return; }
		auto* leftForm = player->GetEquippedObject(true);
		auto* rightForm = player->GetEquippedObject(false);
		auto* leftSpell = leftForm ? leftForm->As<RE::SpellItem>() : nullptr;
		auto* rightSpell = rightForm ? rightForm->As<RE::SpellItem>() : nullptr;
		ProcessCastCapture(player, m_leftCapture, leftSpell && player->IsCasting(leftSpell), leftForm);
		ProcessCastCapture(player, m_rightCapture, rightSpell && player->IsCasting(rightSpell), rightForm);

		auto* selectedPowerSpell = ResolveSelectedPowerSpell(player);
		bool isShouting = false;
		(void)player->GetGraphVariableBool(RE::BSFixedString("IsShouting"), isShouting);
		const bool powerOrShoutCasting = (selectedPowerSpell && player->IsCasting(selectedPowerSpell)) || isShouting;
		if (!m_runtime.wasPowerOrShoutCasting && powerOrShoutCasting && selectedPowerSpell) {
			RecordStepOnCastStart(player, selectedPowerSpell);
		}
		m_runtime.wasPowerOrShoutCasting = powerOrShoutCasting;
	}

	void Controller::UpdatePlayback(RE::PlayerCharacter* player, float delta)
	{
		if (m_runtime.mode != Mode::kPlaying) return;
		if (ShouldBlockGlobal(player)) { StopPlayback(true); return; }
		if (m_config.global.playback.stopIfPlayerHit && player->IsStaggering()) { StopPlayback(true); return; }
		if (m_runtime.stepDelayTimer > 0.0f) { m_runtime.stepDelayTimer = std::max(0.0f, m_runtime.stepDelayTimer - delta); return; }
		if (m_runtime.concentrationHoldRemaining > 0.0f) {
			m_runtime.concentrationHoldRemaining = std::max(0.0f, m_runtime.concentrationHoldRemaining - delta);
			m_runtime.concentrationTickTimer = std::max(0.0f, m_runtime.concentrationTickTimer - delta);
			if (m_runtime.concentrationTickTimer <= 0.0f && !m_runtime.activeConcentrationSpell.empty()) {
				if (auto* spell = ResolveSpell(m_runtime.activeConcentrationSpell)) {
					auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
					if (!caster) {
						caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
					}
					if (caster) {
						auto targetPtr = m_runtime.playbackTarget.get();
						auto* target = m_runtime.playbackCastOnSelf ? static_cast<RE::TESObjectREFR*>(player) : targetPtr.get();
						if (target) {
							caster->CastSpellImmediate(spell, m_runtime.playbackCastOnSelf, target, 1.0f, false, 0.0f, player);
						}
					}
				}
				m_runtime.concentrationTickTimer = 0.30f;
			}
			return;
		}
		if (m_runtime.releasePaddingRemaining > 0.0f) { m_runtime.releasePaddingRemaining = std::max(0.0f, m_runtime.releasePaddingRemaining - delta); return; }
		const auto chainIdx = static_cast<std::size_t>(m_runtime.activeChainIndex);
		if (chainIdx >= m_config.chains.size()) { StopPlayback(true); return; }
		const auto& chain = m_config.chains[chainIdx];
		if (m_runtime.playbackStepIndex >= chain.steps.size()) { StopPlayback(true); return; }
		const bool ok = ExecuteStep(player, chain.steps[m_runtime.playbackStepIndex]);
		if (!ok && m_config.global.playback.abortOnFail) { StopPlayback(true); return; }
		++m_runtime.playbackStepIndex;
		m_runtime.stepDelayTimer = m_config.global.playback.stepDelaySec;
	}

	void Controller::Update(RE::PlayerCharacter* player, float delta)
	{
		if (!player || delta <= 0.0f) return;
		m_runtime.timeSinceLoad += delta;
		UpdateHotkeys(player);
		if (!m_config.global.enabled || player->IsDead()) {
			if (m_runtime.mode != Mode::kIdle) { StopRecording(); StopPlayback(true); }
			return;
		}
		UpdateRecording(player, delta);
		UpdatePlayback(player, delta);
	}

	RE::SpellItem* Controller::ResolveSpell(std::string_view key)
	{
		std::string pluginName{};
		RE::FormID localFormID = 0;
		if (!ParseFormKey(key, pluginName, localFormID)) return nullptr;
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) return nullptr;
		auto* form = dh->LookupForm(localFormID, pluginName);
		return form ? form->As<RE::SpellItem>() : nullptr;
	}

	bool Controller::ParseFormKey(std::string_view key, std::string& pluginName, RE::FormID& local)
	{
		const auto split = key.find('|');
		if (split == std::string_view::npos) return false;
		pluginName = Trim(key.substr(0, split));
		auto idText = Trim(key.substr(split + 1));
		if (pluginName.empty() || idText.empty()) return false;
		if (idText.rfind("0x", 0) == 0 || idText.rfind("0X", 0) == 0) idText = idText.substr(2);
		std::uint32_t parsed = 0;
		const auto [ptr, ec] = std::from_chars(idText.data(), idText.data() + idText.size(), parsed, 16);
		if (ec != std::errc{} || ptr != idText.data() + idText.size()) return false;
		local = parsed;
		return true;
	}

	std::string Controller::BuildFormKey(const RE::TESForm* form)
	{
		if (!form) return {};
		const auto* file = form->GetFile(0);
		if (!file) return {};
		const auto filename = file->GetFilename();
		if (filename.empty()) return {};
		return std::format("{}|0x{:08X}", filename, form->GetLocalFormID());
	}

	RE::TESObjectREFR* Controller::GetCrosshairTarget(bool preferActor)
	{
		auto* pick = RE::CrosshairPickData::GetSingleton();
		if (!pick) return nullptr;
		if (preferActor) {
			auto actor = pick->targetActor.get();
			if (actor) return actor.get();
		}
		auto target = pick->target.get();
		if (target) return target.get();
		if (!preferActor) {
			auto actor = pick->targetActor.get();
			if (actor) return actor.get();
		}
		return nullptr;
	}

	RE::SpellItem* Controller::ResolveSelectedPowerSpell(RE::PlayerCharacter* player)
	{
		if (!player) {
			return nullptr;
		}

		auto* selectedPower = player->GetActorRuntimeData().selectedPower;
		if (!selectedPower) {
			return nullptr;
		}

		if (auto* spell = selectedPower->As<RE::SpellItem>()) {
			return spell;
		}

		if (auto* shout = selectedPower->As<RE::TESShout>()) {
			const auto level = std::clamp(player->GetCurrentShoutLevel(), 0, 2);
			if (auto* levelSpell = shout->variations[level].spell) {
				return levelSpell;
			}
			for (const auto& variation : shout->variations) {
				if (variation.spell) {
					return variation.spell;
				}
			}
		}

		return nullptr;
	}

bool Controller::IsSpellConcentration(std::string_view key) const
{
	auto* spell = ResolveSpell(key);
	return spell && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
}

bool Controller::IsSpellSelfDelivery(std::string_view key) const
{
	auto* spell = ResolveSpell(key);
	return spell && spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
}

	std::vector<KnownSpellOption> Controller::GetKnownSpells() const
	{
		std::vector<KnownSpellOption> out;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return out;
		struct Visitor final : RE::Actor::ForEachSpellVisitor {
			explicit Visitor(std::vector<KnownSpellOption>& o) : out(o) {}
			RE::BSContainer::ForEachResult Visit(RE::SpellItem* spell) override {
				if (!spell) return RE::BSContainer::ForEachResult::kContinue;
				const auto st = spell->GetSpellType();
				if (st != RE::MagicSystem::SpellType::kSpell &&
					st != RE::MagicSystem::SpellType::kLesserPower &&
					st != RE::MagicSystem::SpellType::kPower &&
					st != RE::MagicSystem::SpellType::kVoicePower) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				KnownSpellOption s{};
				s.name = spell->GetName();
				s.formKey = Controller::BuildFormKey(spell);
				s.formID = spell->GetFormID();
				if (!s.formKey.empty()) out.push_back(std::move(s));
				return RE::BSContainer::ForEachResult::kContinue;
			}
			std::vector<KnownSpellOption>& out;
		} visitor(out);
		player->VisitSpells(visitor);
		std::sort(out.begin(), out.end(), [](const KnownSpellOption& a, const KnownSpellOption& b) {
			if (a.name == b.name) return a.formID < b.formID;
			return a.name < b.name;
		});
		return out;
	}
}
