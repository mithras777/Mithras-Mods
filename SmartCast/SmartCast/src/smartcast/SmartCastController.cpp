#include "smartcast/SmartCastController.h"

#include "RE/C/CrosshairPickData.h"
#include "event/GameEventManager.h"

#include <Windows.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>
#include <nlohmann/json.hpp>

namespace SMART_CAST
{
	using json = nlohmann::json;

	namespace
	{
		constexpr std::uint32_t kConfigVersion = 1;

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
		cfg.global.maxChains = 5;
		cfg.global.maxStepsPerChain = 12;
		cfg.global.playback.defaultChainIndex = 1;
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
		cfg.global.maxChains = std::clamp(cfg.global.maxChains, 1, 10);
		cfg.global.maxStepsPerChain = std::clamp(cfg.global.maxStepsPerChain, 1, 30);
		cfg.global.playback.defaultChainIndex = std::clamp(cfg.global.playback.defaultChainIndex, 1, cfg.global.maxChains);
		cfg.global.minTimeAfterLoadSeconds = std::clamp(cfg.global.minTimeAfterLoadSeconds, 0.0f, 10.0f);
		cfg.global.record.maxIdleSec = std::clamp(cfg.global.record.maxIdleSec, 0.5f, 30.0f);
		cfg.global.playback.stepDelaySec = std::clamp(cfg.global.playback.stepDelaySec, 0.0f, 2.0f);
		cfg.global.concentration.minHoldSec = std::clamp(cfg.global.concentration.minHoldSec, 0.05f, 10.0f);
		cfg.global.concentration.maxHoldSec = std::clamp(cfg.global.concentration.maxHoldSec, cfg.global.concentration.minHoldSec, 10.0f);
		cfg.global.concentration.sampleGranularitySec = std::clamp(cfg.global.concentration.sampleGranularitySec, 0.01f, 1.0f);
		cfg.global.concentration.releasePaddingSec = std::clamp(cfg.global.concentration.releasePaddingSec, 0.0f, 1.0f);
		for (auto& chain : cfg.chains) {
			if (chain.name.empty()) chain.name = "Chain";
			if (chain.steps.size() > static_cast<std::size_t>(cfg.global.maxStepsPerChain)) {
				chain.steps.resize(static_cast<std::size_t>(cfg.global.maxStepsPerChain));
			}
			for (auto& step : chain.steps) {
				step.holdSec = std::clamp(step.holdSec, cfg.global.concentration.minHoldSec, cfg.global.concentration.maxHoldSec);
			}
		}
	}

	void Controller::EnsureChainCount(Config& cfg) const
	{
		const auto target = static_cast<std::size_t>(cfg.global.maxChains);
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
		m_config.chains[static_cast<std::size_t>(idx)].steps.clear();
	}

	void Controller::StopRecording()
	{
		if (m_runtime.mode == Mode::kRecording) {
			m_runtime.mode = Mode::kIdle;
			m_runtime.recordIdleTimer = 0.0f;
			m_leftCapture = CastCapture{};
			m_rightCapture = CastCapture{};
		}
	}

	void Controller::StartPlayback(std::int32_t a_chainIndex1Based)
	{
		const auto idx = ResolveChainIndex(a_chainIndex1Based);
		if (idx < 0) return;
		const auto& chain = m_config.chains[static_cast<std::size_t>(idx)];
		if (!chain.enabled || chain.steps.empty()) return;
		if (m_runtime.mode == Mode::kRecording) StopRecording();
		m_runtime.mode = Mode::kPlaying;
		m_runtime.activeChainIndex = idx;
		m_runtime.playbackStepIndex = 0;
		m_runtime.stepDelayTimer = 0.0f;
		m_runtime.concentrationHoldRemaining = 0.0f;
		m_runtime.releasePaddingRemaining = 0.0f;
	}

	void Controller::StopPlayback(bool)
	{
		if (m_runtime.mode == Mode::kPlaying) {
			m_runtime.mode = Mode::kIdle;
			m_runtime.playbackStepIndex = 0;
			m_runtime.stepDelayTimer = 0.0f;
			m_runtime.concentrationHoldRemaining = 0.0f;
			m_runtime.releasePaddingRemaining = 0.0f;
		}
	}
