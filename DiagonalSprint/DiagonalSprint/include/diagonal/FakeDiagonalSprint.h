#pragma once

#include <filesystem>
#include <mutex>

namespace DIAGONAL
{
	struct SpeedScalingConfig
	{
		bool enabled{ false };
		float sprintSpeedRef{ 360.0f };
		float minScale{ 0.7f };
		float maxScale{ 1.2f };
	};

	struct FakeDiagonalConfig
	{
		bool enabled{ true };
		bool firstPersonOnly{ true };
		bool requireOnGround{ true };
		bool requireForwardInput{ true };
		float baseDriftSpeed{ 10.0f };
		float maxLateralSpeed{ 28.0f };
		float inputTau{ 0.04f };
		float dirTau{ 0.12f };
		SpeedScalingConfig speedScaling{};
		bool disableWhenAttacking{ true };
		bool disableWhenStaggered{ true };
		bool debug{ false };
	};

	class FakeDiagonalSprint final : public REX::Singleton<FakeDiagonalSprint>
	{
	public:
		void Initialize();
		void Reload();
		void Save();
		void ResetToDefaults();

		FakeDiagonalConfig GetConfig() const;
		void SetConfig(const FakeDiagonalConfig& a_config, bool a_writeJson = true);

		void OnInputEvent(RE::InputEvent* const* a_event);
		void OnPlayerUpdate(float a_dt);
		void ResetRuntimeState();

		bool ShouldBlockStrafe() const;

	private:
		bool IsFeatureActive(RE::PlayerCharacter* a_player, bool a_requireForward, const char** a_reason = nullptr) const;
		bool IsForwardActive(RE::PlayerCharacter* a_player) const;
		bool IsGroundedReliable(RE::PlayerCharacter* a_player) const;
		RE::NiPoint3 ComputeCameraRightFlat() const;
		float GetDriftSpeed(RE::PlayerCharacter* a_player, const RE::NiPoint2& a_horizontalVelocity) const;
		void ApplyDriftVelocity(RE::PlayerCharacter* a_player, float a_dt, const RE::NiPoint3& a_rightFlat, float a_targetLateral);
		void ClearDriftVelocityMod(RE::PlayerCharacter* a_player) const;
		void LoadFromDisk();
		std::filesystem::path GetConfigPath() const;

		static float ComputeExpSmoothingAlpha(float a_dt, float a_tau);
		static float ClampFloat(float a_value, float a_min, float a_max);
		static RE::NiPoint3 Normalize2D(const RE::NiPoint3& a_value, const RE::NiPoint3& a_fallback);
		static RE::NiPoint3 Slerp2D(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to, float a_t);
		static float Dot2D(const RE::NiPoint3& a_lhs, const RE::NiPoint3& a_rhs);

		mutable std::mutex m_lock;
		FakeDiagonalConfig m_config{};

		bool m_strafeLeftDown{ false };
		bool m_strafeRightDown{ false };
		bool m_forwardDown{ false };
		bool m_blockStrafeNow{ false };
		float m_smoothedInput{ 0.0f };
		RE::NiPoint3 m_smoothedRight{ 1.0f, 0.0f, 0.0f };
		bool m_lastDebugActive{ false };
		const char* m_lastDebugReason{ "init" };
		float m_debugLogCooldown{ 0.0f };
	};
}
