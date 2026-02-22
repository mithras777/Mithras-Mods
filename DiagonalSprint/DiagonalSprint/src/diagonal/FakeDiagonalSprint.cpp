#include "diagonal/FakeDiagonalSprint.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>

namespace DIAGONAL
{
	namespace
	{
		constexpr auto kConfigFileName = L"DiagonalSprint.json";
		constexpr float kMinTau = 0.001f;
		constexpr float kEpsilon = 1e-4f;
		constexpr float kDebugLogPeriod = 0.35f;

		std::string ReadTextFile(const std::filesystem::path& a_path)
		{
			std::ifstream file(a_path);
			if (!file.is_open()) {
				return {};
			}

			std::ostringstream buffer;
			buffer << file.rdbuf();
			return buffer.str();
		}

		std::string ExtractObject(const std::string& a_text, const std::string& a_key)
		{
			const auto keyPos = a_text.find(std::format("\"{}\"", a_key));
			if (keyPos == std::string::npos) {
				return {};
			}

			const auto openPos = a_text.find('{', keyPos);
			if (openPos == std::string::npos) {
				return {};
			}

			int depth = 0;
			for (std::size_t i = openPos; i < a_text.size(); ++i) {
				if (a_text[i] == '{') {
					++depth;
				} else if (a_text[i] == '}') {
					--depth;
					if (depth == 0) {
						return a_text.substr(openPos, (i - openPos) + 1);
					}
				}
			}

			return {};
		}

		void ReadBool(const std::string& a_text, const std::string& a_key, bool& a_value)
		{
			const auto pos = a_text.find(std::format("\"{}\"", a_key));
			if (pos == std::string::npos) {
				return;
			}

			const auto colon = a_text.find(':', pos);
			if (colon == std::string::npos) {
				return;
			}

			const auto tokenStart = a_text.find_first_not_of(" \t\r\n", colon + 1);
			if (tokenStart == std::string::npos) {
				return;
			}

			if (a_text.compare(tokenStart, 4, "true") == 0) {
				a_value = true;
			} else if (a_text.compare(tokenStart, 5, "false") == 0) {
				a_value = false;
			}
		}

		void ReadFloat(const std::string& a_text, const std::string& a_key, float& a_value)
		{
			const auto pos = a_text.find(std::format("\"{}\"", a_key));
			if (pos == std::string::npos) {
				return;
			}

			const auto colon = a_text.find(':', pos);
			if (colon == std::string::npos) {
				return;
			}

			const auto tokenStart = a_text.find_first_not_of(" \t\r\n", colon + 1);
			if (tokenStart == std::string::npos) {
				return;
			}

			const auto tokenEnd = a_text.find_first_of(",}\r\n", tokenStart);
			const std::string token = a_text.substr(tokenStart, tokenEnd == std::string::npos ? std::string::npos : tokenEnd - tokenStart);

			try {
				a_value = std::stof(token);
			} catch (...) {
			}
		}

		void WriteConfigJson(std::ostream& a_os, const FakeDiagonalConfig& a_cfg)
		{
			a_os << "{\n";
			a_os << "  \"enabled\": " << (a_cfg.enabled ? "true" : "false") << ",\n";
			a_os << "  \"firstPersonOnly\": " << (a_cfg.firstPersonOnly ? "true" : "false") << ",\n";
			a_os << "  \"requireOnGround\": " << (a_cfg.requireOnGround ? "true" : "false") << ",\n";
			a_os << "  \"requireForwardInput\": " << (a_cfg.requireForwardInput ? "true" : "false") << ",\n";
			a_os << "  \"baseDriftSpeed\": " << a_cfg.baseDriftSpeed << ",\n";
			a_os << "  \"maxLateralSpeed\": " << a_cfg.maxLateralSpeed << ",\n";
			a_os << "  \"inputTau\": " << a_cfg.inputTau << ",\n";
			a_os << "  \"dirTau\": " << a_cfg.dirTau << ",\n";
			a_os << "  \"speedScaling\": {\n";
			a_os << "    \"enabled\": " << (a_cfg.speedScaling.enabled ? "true" : "false") << ",\n";
			a_os << "    \"sprintSpeedRef\": " << a_cfg.speedScaling.sprintSpeedRef << ",\n";
			a_os << "    \"minScale\": " << a_cfg.speedScaling.minScale << ",\n";
			a_os << "    \"maxScale\": " << a_cfg.speedScaling.maxScale << "\n";
			a_os << "  },\n";
			a_os << "  \"disableWhenAttacking\": " << (a_cfg.disableWhenAttacking ? "true" : "false") << ",\n";
			a_os << "  \"disableWhenStaggered\": " << (a_cfg.disableWhenStaggered ? "true" : "false") << ",\n";
			a_os << "  \"debug\": " << (a_cfg.debug ? "true" : "false") << "\n";
			a_os << "}\n";
		}
	}

	void FakeDiagonalSprint::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			ResetRuntimeState();
		}
		LoadFromDisk();
	}

	void FakeDiagonalSprint::Reload()
	{
		LoadFromDisk();
		std::scoped_lock lock(m_lock);
		ResetRuntimeState();
	}

	void FakeDiagonalSprint::Save()
	{
		FakeDiagonalConfig config{};
		{
			std::scoped_lock lock(m_lock);
			config = m_config;
		}

		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("DiagonalSprint: failed to write config {}", path.string());
			return;
		}

		WriteConfigJson(file, config);
	}

	void FakeDiagonalSprint::ResetToDefaults()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			ResetRuntimeState();
		}
		Save();
	}

	FakeDiagonalConfig FakeDiagonalSprint::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void FakeDiagonalSprint::SetConfig(const FakeDiagonalConfig& a_config, bool a_writeJson)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			m_config.baseDriftSpeed = std::max(0.0f, m_config.baseDriftSpeed);
			m_config.maxLateralSpeed = std::max(1.0f, m_config.maxLateralSpeed);
			m_config.inputTau = ClampFloat(m_config.inputTau, 0.01f, 0.30f);
			m_config.dirTau = ClampFloat(m_config.dirTau, 0.01f, 0.30f);
			m_config.speedScaling.sprintSpeedRef = std::max(1.0f, m_config.speedScaling.sprintSpeedRef);
			m_config.speedScaling.minScale = std::max(0.1f, m_config.speedScaling.minScale);
			m_config.speedScaling.maxScale = std::max(m_config.speedScaling.minScale, m_config.speedScaling.maxScale);
		}

		if (a_writeJson) {
			Save();
		}
	}

	void FakeDiagonalSprint::OnInputEvent(RE::InputEvent* const* a_event)
	{
		if (!a_event || !*a_event) {
			return;
		}

		auto* userEvents = RE::UserEvents::GetSingleton();
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!userEvents || !player) {
			return;
		}

		std::scoped_lock lock(m_lock);
		for (auto* event = *a_event; event; event = event->next) {
			if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			auto* button = event->AsButtonEvent();
			if (!button) {
				continue;
			}

			const auto& userEvent = button->GetUserEvent();
			if (userEvent == userEvents->strafeLeft) {
				m_strafeLeftDown = button->IsPressed();
			} else if (userEvent == userEvents->strafeRight) {
				m_strafeRightDown = button->IsPressed();
			} else if (userEvent == userEvents->forward) {
				m_forwardDown = button->IsPressed();
			}
		}

		const char* reason = "unknown";
		m_blockStrafeNow = IsFeatureActive(player, m_config.requireForwardInput, &reason);
		if (!m_blockStrafeNow) {
			if (m_config.debug) {
				const bool hasStrafeInput = m_strafeLeftDown || m_strafeRightDown;
				if (hasStrafeInput && reason != m_lastDebugReason) {
					LOG_DEBUG("DiagonalSprint: strafe blocked=0 reason={}", reason);
					m_lastDebugReason = reason;
				}
			}
			return;
		}

		for (auto* event = *a_event; event; event = event->next) {
			if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			auto* button = event->AsButtonEvent();
			if (!button) {
				continue;
			}

			const auto& userEvent = button->GetUserEvent();
			if (userEvent == userEvents->strafeLeft || userEvent == userEvents->strafeRight) {
				button->GetRuntimeData().value = 0.0f;
				if (m_config.debug && button->IsPressed()) {
					LOG_DEBUG(
						"DiagonalSprint: captured {} (L={}, R={}, F={})",
						userEvent.c_str(),
						m_strafeLeftDown,
						m_strafeRightDown,
						m_forwardDown);
				}
			}
		}
	}

	void FakeDiagonalSprint::OnPlayerUpdate(float a_dt)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || a_dt <= 0.0f) {
			std::scoped_lock lock(m_lock);
			ResetRuntimeState();
			return;
		}

		std::scoped_lock lock(m_lock);
		const char* reason = "unknown";
		const bool active = IsFeatureActive(player, m_config.requireForwardInput, &reason);
		m_blockStrafeNow = active;
		m_debugLogCooldown -= a_dt;

		if (!active) {
			ClearDriftVelocityMod(player);
			if (m_config.debug && (!m_lastDebugActive || reason != m_lastDebugReason || m_debugLogCooldown <= 0.0f)) {
				LOG_DEBUG("DiagonalSprint: inactive reason={}", reason);
				m_lastDebugReason = reason;
				m_debugLogCooldown = kDebugLogPeriod;
			}
			m_lastDebugActive = false;
			ResetRuntimeState();
			return;
		}

		if (auto* controls = RE::PlayerControls::GetSingleton()) {
			controls->data.moveInputVec.x = 0.0f;
		}

		float targetInput = 0.0f;
		if (m_strafeLeftDown != m_strafeRightDown) {
			targetInput = m_strafeRightDown ? 1.0f : -1.0f;
		}

		const float inputAlpha = ComputeExpSmoothingAlpha(a_dt, m_config.inputTau);
		m_smoothedInput += (targetInput - m_smoothedInput) * inputAlpha;

		const RE::NiPoint3 rightFlatTarget = ComputeCameraRightFlat();
		const float dirAlpha = ComputeExpSmoothingAlpha(a_dt, m_config.dirTau);
		m_smoothedRight = Slerp2D(m_smoothedRight, rightFlatTarget, dirAlpha);

		auto* controller = player->GetCharController();
		if (!controller) {
			return;
		}

		RE::hkVector4 currentVelocity;
		controller->GetLinearVelocityImpl(currentVelocity);
		const RE::NiPoint2 horizontalCurrent{ currentVelocity.quad.m128_f32[0], currentVelocity.quad.m128_f32[1] };

		const float driftSpeed = GetDriftSpeed(horizontalCurrent);
		const float targetLateral = ClampFloat(m_smoothedInput * driftSpeed, -m_config.maxLateralSpeed, m_config.maxLateralSpeed);
		ApplyDriftVelocity(player, m_smoothedRight, targetLateral);

		if (m_config.debug && (targetInput != 0.0f || !m_lastDebugActive || m_debugLogCooldown <= 0.0f)) {
			LOG_DEBUG(
				"DiagonalSprint: active=1 reason={} dt={:.4f} targetInput={:.2f} smoothInput={:.2f} driftSpeed={:.2f} targetLat={:.2f}",
				reason,
				a_dt,
				targetInput,
				m_smoothedInput,
				driftSpeed,
				targetLateral);
			m_debugLogCooldown = kDebugLogPeriod;
		}
		m_lastDebugActive = true;
		m_lastDebugReason = reason;
	}

	void FakeDiagonalSprint::ResetRuntimeState()
	{
		m_smoothedInput = 0.0f;
		m_smoothedRight = { 1.0f, 0.0f, 0.0f };
		m_blockStrafeNow = false;
	}

	bool FakeDiagonalSprint::ShouldBlockStrafe() const
	{
		std::scoped_lock lock(m_lock);
		return m_blockStrafeNow;
	}

	bool FakeDiagonalSprint::IsFeatureActive(RE::PlayerCharacter* a_player, bool a_requireForward, const char** a_reason) const
	{
		auto setReason = [a_reason](const char* a_value) {
			if (a_reason) {
				*a_reason = a_value;
			}
		};

		if (!a_player || !a_player->IsPlayerRef() || !m_config.enabled) {
			setReason("player-invalid-or-disabled");
			return false;
		}

		auto* ui = RE::UI::GetSingleton();
		if (!ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::MainMenu::MENU_NAME) || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
			setReason("menu-or-paused");
			return false;
		}

		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera) {
			setReason("camera-missing");
			return false;
		}

		if (m_config.firstPersonOnly && !camera->IsInFirstPerson()) {
			setReason("not-first-person");
			return false;
		}

		const auto* state = a_player->AsActorState();
		if (!state || !state->IsSprinting()) {
			setReason("not-sprinting");
			return false;
		}

		if (a_player->IsInRagdollState() || a_player->IsInKillMove()) {
			setReason("ragdoll-or-killmove");
			return false;
		}

		if (state->GetKnockState() != RE::KNOCK_STATE_ENUM::kNormal || state->IsSitting() || state->IsSwimming()) {
			setReason("knock-sit-swim");
			return false;
		}

		if (m_config.requireOnGround && a_player->IsInMidair()) {
			setReason("midair");
			return false;
		}

		if (m_config.disableWhenAttacking && a_player->IsAttacking()) {
			setReason("attacking");
			return false;
		}

		if (m_config.disableWhenStaggered && state->IsStaggered()) {
			setReason("staggered");
			return false;
		}

		auto* controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap || !controlMap->IsMovementControlsEnabled()) {
			setReason("movement-controls-disabled");
			return false;
		}

		auto* controls = RE::PlayerControls::GetSingleton();
		if (controls && controls->blockPlayerInput) {
			setReason("player-input-blocked");
			return false;
		}

		if (a_requireForward && !IsForwardActive(a_player)) {
			setReason("forward-not-detected");
			return false;
		}

		setReason("active");
		return true;
	}

	bool FakeDiagonalSprint::IsForwardActive(RE::PlayerCharacter* a_player) const
	{
		if (m_forwardDown) {
			return true;
		}

		if (!a_player) {
			return false;
		}

		if (const auto* controls = RE::PlayerControls::GetSingleton(); controls && controls->data.moveInputVec.y > 0.05f) {
			return true;
		}

		const auto* state = a_player->AsActorState();
		if (state && static_cast<bool>(state->actorState1.movingForward)) {
			return true;
		}

		if (auto* controller = a_player->GetCharController()) {
			RE::hkVector4 velocity{};
			controller->GetLinearVelocityImpl(velocity);
			const RE::NiPoint3 horizontal{ velocity.quad.m128_f32[0], velocity.quad.m128_f32[1], 0.0f };
			const float speedSq = horizontal.SqrLength();
			if (speedSq > 25.0f) {
				const float yaw = a_player->GetAngleZ();
				const RE::NiPoint3 actorForward{ std::sin(yaw), std::cos(yaw), 0.0f };
				const RE::NiPoint3 horizontalNorm = Normalize2D(horizontal, actorForward);
				return Dot2D(horizontalNorm, actorForward) > 0.2f;
			}
		}

		return false;
	}

	RE::NiPoint3 FakeDiagonalSprint::ComputeCameraRightFlat() const
	{
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera) {
			return m_smoothedRight;
		}

		RE::NiPoint3 forwardFlat{};
		if (camera->cameraRoot) {
			const RE::NiPoint3 camForward = camera->cameraRoot->world.rotate.GetVectorY();
			forwardFlat = { camForward.x, camForward.y, 0.0f };
		}

		if (forwardFlat.SqrLength() <= kEpsilon) {
			const float yaw = camera->GetRuntimeData2().yaw;
			forwardFlat = { std::sin(yaw), std::cos(yaw), 0.0f };
		}

		forwardFlat = Normalize2D(forwardFlat, { 0.0f, 1.0f, 0.0f });
		const RE::NiPoint3 rightFlat = { forwardFlat.y, -forwardFlat.x, 0.0f };
		return Normalize2D(rightFlat, m_smoothedRight);
	}

	float FakeDiagonalSprint::GetDriftSpeed(const RE::NiPoint2& a_horizontalVelocity) const
	{
		float driftSpeed = m_config.baseDriftSpeed;
		if (!m_config.speedScaling.enabled) {
			return driftSpeed;
		}

		const float speed = std::sqrt((a_horizontalVelocity.x * a_horizontalVelocity.x) + (a_horizontalVelocity.y * a_horizontalVelocity.y));
		const float ref = std::max(1.0f, m_config.speedScaling.sprintSpeedRef);
		const float ratio = speed / ref;
		const float scale = ClampFloat(ratio, m_config.speedScaling.minScale, m_config.speedScaling.maxScale);
		return driftSpeed * scale;
	}

	void FakeDiagonalSprint::ApplyDriftVelocity(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_rightFlat, float a_targetLateral)
	{
		auto* controller = a_player ? a_player->GetCharController() : nullptr;
		if (!controller) {
			return;
		}

		RE::hkVector4 current;
		controller->GetLinearVelocityImpl(current);
		const float vx = current.quad.m128_f32[0];
		const float vy = current.quad.m128_f32[1];
		const float vz = current.quad.m128_f32[2];
		const float vw = current.quad.m128_f32[3];

		const RE::NiPoint3 horizontal{ vx, vy, 0.0f };
		const RE::NiPoint3 rightNorm = Normalize2D(a_rightFlat, m_smoothedRight);

		const float clampedTarget = ClampFloat(a_targetLateral, -m_config.maxLateralSpeed, m_config.maxLateralSpeed);
		const RE::NiPoint3 driftHorizontal = rightNorm * clampedTarget;

		// Drive controller-intended side movement (preferred path for sprint state compatibility).
		controller->velocityMod.quad.m128_f32[0] = driftHorizontal.x;
		controller->velocityMod.quad.m128_f32[1] = driftHorizontal.y;
		controller->velocityMod.quad.m128_f32[2] = 0.0f;
		controller->velocityMod.quad.m128_f32[3] = 0.0f;

		// Fallback for runtimes where velocityMod is ignored.
		const RE::NiPoint3 newHorizontal = horizontal + driftHorizontal;
		const RE::hkVector4 patchedVelocity{ newHorizontal.x, newHorizontal.y, vz, vw };
		controller->SetLinearVelocityImpl(patchedVelocity);
	}

	void FakeDiagonalSprint::ClearDriftVelocityMod(RE::PlayerCharacter* a_player) const
	{
		auto* controller = a_player ? a_player->GetCharController() : nullptr;
		if (!controller) {
			return;
		}

		controller->velocityMod.quad.m128_f32[0] = 0.0f;
		controller->velocityMod.quad.m128_f32[1] = 0.0f;
		controller->velocityMod.quad.m128_f32[2] = 0.0f;
		controller->velocityMod.quad.m128_f32[3] = 0.0f;
	}

	void FakeDiagonalSprint::LoadFromDisk()
	{
		const auto path = GetConfigPath();
		if (!std::filesystem::exists(path)) {
			Save();
			return;
		}

		const std::string text = ReadTextFile(path);
		if (text.empty()) {
			LOG_WARN("DiagonalSprint: empty or unreadable config {}, keeping defaults", path.string());
			Save();
			return;
		}

		FakeDiagonalConfig loaded = {};
		ReadBool(text, "enabled", loaded.enabled);
		ReadBool(text, "firstPersonOnly", loaded.firstPersonOnly);
		ReadBool(text, "requireOnGround", loaded.requireOnGround);
		ReadBool(text, "requireForwardInput", loaded.requireForwardInput);
		ReadFloat(text, "baseDriftSpeed", loaded.baseDriftSpeed);
		ReadFloat(text, "maxLateralSpeed", loaded.maxLateralSpeed);
		ReadFloat(text, "inputTau", loaded.inputTau);
		ReadFloat(text, "dirTau", loaded.dirTau);
		ReadBool(text, "disableWhenAttacking", loaded.disableWhenAttacking);
		ReadBool(text, "disableWhenStaggered", loaded.disableWhenStaggered);
		ReadBool(text, "debug", loaded.debug);

		const std::string scalingObject = ExtractObject(text, "speedScaling");
		if (!scalingObject.empty()) {
			ReadBool(scalingObject, "enabled", loaded.speedScaling.enabled);
			ReadFloat(scalingObject, "sprintSpeedRef", loaded.speedScaling.sprintSpeedRef);
			ReadFloat(scalingObject, "minScale", loaded.speedScaling.minScale);
			ReadFloat(scalingObject, "maxScale", loaded.speedScaling.maxScale);
		}

		std::scoped_lock lock(m_lock);
		m_config = loaded;
		m_config.baseDriftSpeed = std::max(0.0f, m_config.baseDriftSpeed);
		m_config.maxLateralSpeed = std::max(1.0f, m_config.maxLateralSpeed);
		m_config.inputTau = ClampFloat(m_config.inputTau, 0.01f, 0.30f);
		m_config.dirTau = ClampFloat(m_config.dirTau, 0.01f, 0.30f);
		m_config.speedScaling.sprintSpeedRef = std::max(1.0f, m_config.speedScaling.sprintSpeedRef);
		m_config.speedScaling.minScale = std::max(0.1f, m_config.speedScaling.minScale);
		m_config.speedScaling.maxScale = std::max(m_config.speedScaling.minScale, m_config.speedScaling.maxScale);
	}

	std::filesystem::path FakeDiagonalSprint::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"DiagonalSprint.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / kConfigFileName;
	}

	float FakeDiagonalSprint::ComputeExpSmoothingAlpha(float a_dt, float a_tau)
	{
		if (a_tau <= kMinTau) {
			return 1.0f;
		}
		return 1.0f - std::exp(-a_dt / a_tau);
	}

	float FakeDiagonalSprint::ClampFloat(float a_value, float a_min, float a_max)
	{
		return std::clamp(a_value, a_min, a_max);
	}

	RE::NiPoint3 FakeDiagonalSprint::Normalize2D(const RE::NiPoint3& a_value, const RE::NiPoint3& a_fallback)
	{
		RE::NiPoint3 out = { a_value.x, a_value.y, 0.0f };
		if (out.SqrLength() <= kEpsilon) {
			out = { a_fallback.x, a_fallback.y, 0.0f };
		}

		const float len = std::sqrt((out.x * out.x) + (out.y * out.y));
		if (len <= kEpsilon) {
			return { 1.0f, 0.0f, 0.0f };
		}

		out.x /= len;
		out.y /= len;
		return out;
	}

	RE::NiPoint3 FakeDiagonalSprint::Slerp2D(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to, float a_t)
	{
		const auto from = Normalize2D(a_from, { 1.0f, 0.0f, 0.0f });
		const auto to = Normalize2D(a_to, from);

		const float t = ClampFloat(a_t, 0.0f, 1.0f);
		float dot = Dot2D(from, to);
		dot = ClampFloat(dot, -1.0f, 1.0f);

		if (dot > 0.9995f || dot < -0.9995f) {
			const RE::NiPoint3 mixed = {
				from.x + (to.x - from.x) * t,
				from.y + (to.y - from.y) * t,
				0.0f
			};
			return Normalize2D(mixed, from);
		}

		const float theta = std::acos(dot) * t;
		const RE::NiPoint3 relative = Normalize2D(to - (from * dot), from);
		const RE::NiPoint3 result = (from * std::cos(theta)) + (relative * std::sin(theta));
		return Normalize2D(result, from);
	}

	float FakeDiagonalSprint::Dot2D(const RE::NiPoint3& a_lhs, const RE::NiPoint3& a_rhs)
	{
		return (a_lhs.x * a_rhs.x) + (a_lhs.y * a_rhs.y);
	}
}
