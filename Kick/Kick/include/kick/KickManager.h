#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace MITHRAS::KICK
{
	struct KickConfig
	{
		bool enabled{ true };
		bool debugLogging{ false };
		std::uint32_t hotkey{ 0x21 };  // F
		float range{ 220.0f };
		float force{ 450.0f };
		float upwardBias{ 0.25f };
		float cooldownSeconds{ 0.35f };
		float raySpread{ 0.20f };
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();
		void SetConfig(const KickConfig& a_config);
		[[nodiscard]] KickConfig GetConfig() const;

		void SetCaptureHotkey(bool a_capture);
		[[nodiscard]] bool IsCapturingHotkey() const;
		void OnHotkeyPressed(std::uint32_t a_keyCode);
		bool PollCaptureHotkey();
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);
		void DebugForceKick();
		void TryKick();

	private:
		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;

		struct RaycastResult
		{
			RE::TESObjectREFR* reference{ nullptr };
			RE::NiPoint3 origin{};
			RE::NiPoint3 hitPoint{};
			RE::NiPoint3 direction{};
			float distance{ 0.0f };
			bool hasHit{ false };
		};

		[[nodiscard]] static RE::NiPoint3 NormalizeOrZero(const RE::NiPoint3& a_vector);
		[[nodiscard]] static RE::NiPoint3 BuildLookDirection(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_origin);
		[[nodiscard]] static std::optional<RaycastResult> RaycastFrom(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_origin, const RE::NiPoint3& a_direction, float a_range);
		[[nodiscard]] static std::optional<RaycastResult> AcquireKickTarget(RE::PlayerCharacter* a_player, const KickConfig& a_cfg);
		static bool ApplyKickImpulse(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_dir, float a_force, float a_upwardBias);

		mutable std::mutex m_lock;
		KickConfig m_config{};
		bool m_captureHotkey{ false };
		std::chrono::steady_clock::time_point m_lastKick{};
	};
}
