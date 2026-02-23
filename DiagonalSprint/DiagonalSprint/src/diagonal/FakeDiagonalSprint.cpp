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
		constexpr float kEpsilon = 1e-4f;

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
			a_os << "  \"requireOnGround\": " << (a_cfg.requireOnGround ? "true" : "false") << ",\n";
			a_os << "  \"lateralSpeed\": " << a_cfg.lateralSpeed << "\n";
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
			m_config.lateralSpeed = ClampFloat(m_config.lateralSpeed, 0.5f, 10.0f);
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
			}
		}

		const char* reason = "unknown";
		m_blockStrafeNow = IsFeatureActive(player, &reason);
		if (!m_blockStrafeNow) {
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
		const bool active = IsFeatureActive(player, &reason);
		m_blockStrafeNow = active;

		if (!active) {
			ClearDriftVelocityMod(player);
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
		const RE::NiPoint3 rightFlat = ComputeCameraRightFlat();
		const float targetLateral = targetInput * m_config.lateralSpeed;

		if (std::abs(targetLateral) <= kEpsilon) {
			ClearDriftVelocityMod(player);
		} else {
			ApplyLateralVelocity(player, a_dt, rightFlat, targetLateral);
		}
	}

	void FakeDiagonalSprint::ResetRuntimeState()
	{
		m_blockStrafeNow = false;
	}

	bool FakeDiagonalSprint::ShouldBlockStrafe() const
	{
		std::scoped_lock lock(m_lock);
		return m_blockStrafeNow;
	}

	bool FakeDiagonalSprint::IsFeatureActive(RE::PlayerCharacter* a_player, const char** a_reason) const
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

		if (!camera->IsInFirstPerson()) {
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

		if (m_config.requireOnGround && !IsGroundedReliable(a_player)) {
			setReason("midair");
			return false;
		}

		if (a_player->IsAttacking()) {
			setReason("attacking");
			return false;
		}

		if (state->IsStaggered()) {
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

		setReason("active");
		return true;
	}

	bool FakeDiagonalSprint::IsGroundedReliable(RE::PlayerCharacter* a_player) const
	{
		if (!a_player) {
			return false;
		}

		auto* controller = a_player->GetCharController();
		if (controller) {
			if (controller->flags.all(RE::CHARACTER_FLAGS::kSupport) ||
				controller->flags.all(RE::CHARACTER_FLAGS::kHasPotentialSupportManifold) ||
				controller->flags.all(RE::CHARACTER_FLAGS::kOnStairs)) {
				return true;
			}
		}

		// Fallback for runtimes where support flags are noisy.
		return !a_player->IsInMidair();
	}

	RE::NiPoint3 FakeDiagonalSprint::ComputeCameraRightFlat() const
	{
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera) {
			return { 1.0f, 0.0f, 0.0f };
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
		return Normalize2D(rightFlat, { 1.0f, 0.0f, 0.0f });
	}

	void FakeDiagonalSprint::ApplyLateralVelocity(RE::PlayerCharacter* a_player, float a_dt, const RE::NiPoint3& a_rightFlat, float a_targetLateral)
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
		const RE::NiPoint3 rightNorm = Normalize2D(a_rightFlat, { 1.0f, 0.0f, 0.0f });

		const float clampedTarget = ClampFloat(a_targetLateral, -m_config.lateralSpeed, m_config.lateralSpeed);
		if (std::abs(clampedTarget) <= kEpsilon) {
			return;
		}
		const RE::NiPoint3 driftHorizontal = rightNorm * clampedTarget;

		// Drive controller-intended side movement (preferred path).
		controller->velocityMod.quad.m128_f32[0] = driftHorizontal.x;
		controller->velocityMod.quad.m128_f32[1] = driftHorizontal.y;
		controller->velocityMod.quad.m128_f32[2] = 0.0f;
		controller->velocityMod.quad.m128_f32[3] = 0.0f;

		// Grounded sprint can clamp side velocity; inject only a lateral controller step.
		if (IsGroundedReliable(a_player)) {
			RE::hkVector4 pos{};
			controller->GetPositionImpl(pos, false);
			pos.quad.m128_f32[0] += driftHorizontal.x * a_dt;
			pos.quad.m128_f32[1] += driftHorizontal.y * a_dt;
			controller->SetPositionImpl(pos, false, false);
			return;
		}

		// In-air fallback: add only lateral drift and preserve vertical velocity.
		const RE::NiPoint3 newHorizontal = horizontal + (driftHorizontal * 0.35f);
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
		ReadBool(text, "requireOnGround", loaded.requireOnGround);
		ReadFloat(text, "lateralSpeed", loaded.lateralSpeed);

		std::scoped_lock lock(m_lock);
		m_config = loaded;
		m_config.lateralSpeed = ClampFloat(m_config.lateralSpeed, 0.5f, 10.0f);
	}

	std::filesystem::path FakeDiagonalSprint::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"DiagonalSprint.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / kConfigFileName;
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

}
