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
		std::uint32_t hotkey{ 0x21 };  // F
		float objectRange{ 220.0f };
		float objectForce{ 450.0f };
		float objectUpwardBias{ 0.25f };
		float objectCooldownSeconds{ 0.35f };
		float objectRaySpread{ 0.20f };
		float objectStaminaCost{ 0.0f };
		float npcRange{ 220.0f };
		float npcForce{ 1200.0f };
		float npcUpwardBias{ 0.0f };
		float npcCooldownSeconds{ 0.35f };
		float npcRaySpread{ 0.20f };
		float npcStaminaCost{ 10.0f };
		float npcStaminaDrain{ 0.0f };
		float npcDamageFlat{ 0.0f };
		float npcDamagePercent{ 0.0f };
		bool guardBreakKick{ false };
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
		[[nodiscard]] static std::optional<RaycastResult> AcquireKickTarget(RE::PlayerCharacter* a_player, float a_range, float a_raySpread, bool a_wantActor);
		static bool ApplyKickImpulse(RE::TESObjectREFR* a_ref, RE::Actor* a_source, const RE::NiPoint3& a_dir, const KickConfig& a_cfg);

		mutable std::mutex m_lock;
		KickConfig m_config{};
		bool m_captureHotkey{ false };
		std::chrono::steady_clock::time_point m_lastObjectKick{};
		std::chrono::steady_clock::time_point m_lastNPCKick{};
	};
}
