#pragma once

#include <filesystem>
#include <mutex>

namespace DIAGONAL
{
	struct FakeDiagonalConfig
	{
		bool enabled{ true };
		float lateralSpeed{ 4.1f };
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
		bool IsFeatureActive(RE::PlayerCharacter* a_player, const char** a_reason = nullptr) const;
		bool IsAssistContextValid(RE::PlayerCharacter* a_player, const char** a_reason = nullptr) const;
		bool IsGroundedReliable(RE::PlayerCharacter* a_player) const;
		RE::NiPoint3 ComputeCameraRightFlat() const;
		void ApplyLateralVelocity(RE::PlayerCharacter* a_player, float a_dt, const RE::NiPoint3& a_rightFlat, float a_targetLateral);
		void ClearDriftVelocityMod(RE::PlayerCharacter* a_player) const;
		void LoadFromDisk();
		std::filesystem::path GetConfigPath() const;

		static float ClampFloat(float a_value, float a_min, float a_max);
		static RE::NiPoint3 Normalize2D(const RE::NiPoint3& a_value, const RE::NiPoint3& a_fallback);

		mutable std::mutex m_lock;
		FakeDiagonalConfig m_config{};

		bool m_strafeLeftDown{ false };
		bool m_strafeRightDown{ false };
		bool m_forwardDown{ false };
		bool m_sprintDown{ false };
		float m_jumpSuppressTimer{ 0.0f };
		bool m_sprintAssistActive{ false };
		float m_sprintAssistTimer{ 0.0f };
		float m_sprintAssistSide{ 0.0f };
		bool m_blockStrafeNow{ false };
	};
}
