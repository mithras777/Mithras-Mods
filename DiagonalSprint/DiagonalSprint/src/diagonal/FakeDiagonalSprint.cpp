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
		constexpr float kSprintAssistDuration = 0.10f;
		constexpr float kJumpSuppressDuration = 0.20f;
		constexpr float kJumpLateralAccel = 120.0f;

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
			} else if (userEvent == userEvents->forward) {
				m_forwardDown = button->IsPressed();
			} else if (userEvent == userEvents->sprint) {
				m_sprintDown = button->IsPressed();
			} else if (userEvent == userEvents->jump) {
				if (button->IsPressed()) {
					m_jumpSuppressTimer = kJumpSuppressDuration;
				}
			}
		}

		const float heldStrafeInput = (m_strafeLeftDown != m_strafeRightDown) ? (m_strafeRightDown ? 1.0f : -1.0f) : 0.0f;
		const bool hasSprintIntent = m_sprintDown && m_forwardDown && std::abs(heldStrafeInput) > kEpsilon;
		if (!m_sprintAssistActive && hasSprintIntent && IsAssistContextValid(player)) {
			m_sprintAssistActive = true;
			m_sprintAssistTimer = kSprintAssistDuration;
			m_sprintAssistSide = heldStrafeInput;
		}

		m_blockStrafeNow = IsFeatureActive(player) || m_sprintAssistActive;
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
		const bool sprintActive = IsFeatureActive(player);
		const bool assistContextValid = IsAssistContextValid(player);
		auto* controller = player->GetCharController();
		const auto currentState = controller ? controller->context.currentState : RE::hkpCharacterStateType::kOnGround;
		const bool onGroundState = IsGroundedReliable(player);
		const bool jumpingState = currentState == RE::hkpCharacterStateType::kJumping;
		m_jumpSuppressTimer = std::max(0.0f, m_jumpSuppressTimer - a_dt);
		const float heldStrafeInput = (m_strafeLeftDown != m_strafeRightDown) ? (m_strafeRightDown ? 1.0f : -1.0f) : 0.0f;
		const bool hasSprintIntent = m_sprintDown && m_forwardDown && std::abs(heldStrafeInput) > kEpsilon;
		const bool jumpDiagonalIntent = jumpingState && m_jumpSuppressTimer > 0.0f && std::abs(heldStrafeInput) > kEpsilon;

		if (sprintActive) {
			m_sprintAssistActive = false;
			m_sprintAssistTimer = 0.0f;
			m_sprintAssistSide = 0.0f;
		} else if (hasSprintIntent && assistContextValid) {
			m_sprintAssistActive = true;
			m_sprintAssistTimer = kSprintAssistDuration;
			m_sprintAssistSide = heldStrafeInput;
		} else if (m_sprintAssistActive) {
			m_sprintAssistTimer -= a_dt;
			if (m_sprintAssistTimer <= 0.0f || !assistContextValid || !m_sprintDown || !m_forwardDown) {
				m_sprintAssistActive = false;
				m_sprintAssistTimer = 0.0f;
				m_sprintAssistSide = 0.0f;
			} else if (std::abs(heldStrafeInput) > kEpsilon) {
				m_sprintAssistSide = heldStrafeInput;
			}
		}

		const bool shouldDrive = ((sprintActive || m_sprintAssistActive) && onGroundState) || jumpDiagonalIntent;
		m_blockStrafeNow = shouldDrive;
		if (!shouldDrive) {
			ClearDriftVelocityMod(player);
			return;
		}

		if (auto* controls = RE::PlayerControls::GetSingleton()) {
			controls->data.moveInputVec.x = 0.0f;
			if (!sprintActive && m_sprintAssistActive) {
				controls->data.moveInputVec.y = 1.0f;
			}
		}

		const float targetInput = (sprintActive || jumpDiagonalIntent) ? heldStrafeInput : m_sprintAssistSide;
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
		m_strafeLeftDown = false;
		m_strafeRightDown = false;
		m_forwardDown = false;
		m_sprintDown = false;
		m_jumpSuppressTimer = 0.0f;
		m_sprintAssistActive = false;
		m_sprintAssistTimer = 0.0f;
		m_sprintAssistSide = 0.0f;
		m_blockStrafeNow = false;
	}

	bool FakeDiagonalSprint::ShouldBlockStrafe() const
	{
		std::scoped_lock lock(m_lock);
		return m_blockStrafeNow;
	}

	bool FakeDiagonalSprint::IsFeatureActive(RE::PlayerCharacter* a_player, const char** a_reason) const
	{
		const char* assistReason = "unknown";
		if (!IsAssistContextValid(a_player, &assistReason)) {
			if (a_reason) {
				*a_reason = assistReason;
			}
			return false;
		}

		const auto* state = a_player->AsActorState();
		if (!state || !state->IsSprinting()) {
			if (a_reason) {
				*a_reason = "not-sprinting";
			}
			return false;
		}

		if (a_reason) {
			*a_reason = "active";
		}
		return true;
	}

	bool FakeDiagonalSprint::IsAssistContextValid(RE::PlayerCharacter* a_player, const char** a_reason) const
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
		if (!state) {
			setReason("state-missing");
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
			return controller->context.currentState == RE::hkpCharacterStateType::kOnGround && !a_player->IsInMidair();
		}

		// Fallback when the character controller is unavailable.
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

		// Project lateral drift onto the support plane so uphill/downhill strafe follows terrain.
		RE::NiPoint3 supportNormal{
			controller->supportNorm.quad.m128_f32[0],
			controller->supportNorm.quad.m128_f32[1],
			controller->supportNorm.quad.m128_f32[2]
		};
		const float supportLen = std::sqrt(
			(supportNormal.x * supportNormal.x) +
			(supportNormal.y * supportNormal.y) +
			(supportNormal.z * supportNormal.z));
		if (supportLen <= kEpsilon) {
			supportNormal = { 0.0f, 0.0f, 1.0f };
		} else {
			supportNormal.x /= supportLen;
			supportNormal.y /= supportLen;
			supportNormal.z /= supportLen;
		}
		const float intoNormal =
			(driftHorizontal.x * supportNormal.x) +
			(driftHorizontal.y * supportNormal.y) +
			(driftHorizontal.z * supportNormal.z);
		RE::NiPoint3 driftOnSupportPlane = {
			driftHorizontal.x - (supportNormal.x * intoNormal),
			driftHorizontal.y - (supportNormal.y * intoNormal),
			driftHorizontal.z - (supportNormal.z * intoNormal)
		};
		const float driftPlaneLen = std::sqrt(
			(driftOnSupportPlane.x * driftOnSupportPlane.x) +
			(driftOnSupportPlane.y * driftOnSupportPlane.y) +
			(driftOnSupportPlane.z * driftOnSupportPlane.z));
		if (driftPlaneLen > kEpsilon) {
			const float scale = std::abs(clampedTarget) / driftPlaneLen;
			driftOnSupportPlane.x *= scale;
			driftOnSupportPlane.y *= scale;
			driftOnSupportPlane.z *= scale;
		} else {
			driftOnSupportPlane = driftHorizontal;
		}

		// Drive controller-intended side movement (preferred path).
		controller->velocityMod.quad.m128_f32[0] = driftOnSupportPlane.x;
		controller->velocityMod.quad.m128_f32[1] = driftOnSupportPlane.y;
		controller->velocityMod.quad.m128_f32[2] = 0.0f;
		controller->velocityMod.quad.m128_f32[3] = 0.0f;

		const auto currentState = controller->context.currentState;
		const bool onGroundState = currentState == RE::hkpCharacterStateType::kOnGround && !a_player->IsInMidair();
		const bool jumpingState = currentState == RE::hkpCharacterStateType::kJumping;

		// Grounded sprint can clamp side velocity; inject only a lateral controller step.
		// Skip shortly after jump press to avoid jump/liftoff distortion.
		if (onGroundState && m_jumpSuppressTimer <= 0.0f) {
			RE::hkVector4 pos{};
			controller->GetPositionImpl(pos, false);
			pos.quad.m128_f32[0] += driftOnSupportPlane.x * a_dt;
			pos.quad.m128_f32[1] += driftOnSupportPlane.y * a_dt;
			pos.quad.m128_f32[2] += driftOnSupportPlane.z * a_dt;
			controller->SetPositionImpl(pos, false, false);
			return;
		}

		// Brief jump-takeoff window: apply capped lateral velocity adjustment for diagonal jump feel.
		if (jumpingState && m_jumpSuppressTimer > 0.0f) {
			const float lateralCurrent = (horizontal.x * rightNorm.x) + (horizontal.y * rightNorm.y);
			const float lateralDelta = ClampFloat(
				clampedTarget - lateralCurrent,
				-kJumpLateralAccel * a_dt,
				kJumpLateralAccel * a_dt);
			const RE::NiPoint3 newHorizontal = horizontal + (rightNorm * lateralDelta);
			const RE::hkVector4 patchedVelocity{ newHorizontal.x, newHorizontal.y, vz, vw };
			controller->SetLinearVelocityImpl(patchedVelocity);
		}
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
