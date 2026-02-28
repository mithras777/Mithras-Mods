#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace MITHRAS::MAGIC_EFFECT_ORGANIZER
{
	struct OrganizerConfig
	{
		bool enabled{ true };
		std::uint32_t hotkey{ 0x23 };  // H
		std::vector<RE::FormID> hiddenEffectFormIDs{};
	};

	struct MagicEffectEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();

		[[nodiscard]] OrganizerConfig GetConfig() const;
		void SetConfig(const OrganizerConfig& a_config, bool a_save);
		void SetEnabled(bool a_enabled);
		void SetHotkey(std::uint32_t a_hotkey);

		void SetCaptureHotkey(bool a_capture);
		[[nodiscard]] bool IsCapturingHotkey() const;
		void OnHotkeyPressed(std::uint32_t a_keyCode);
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);

		[[nodiscard]] std::vector<MagicEffectEntry> GetVisibleEffects() const;
		[[nodiscard]] std::vector<MagicEffectEntry> GetHiddenEffects() const;

		bool HideEffect(RE::FormID a_formID);
		bool UnhideEffect(RE::FormID a_formID);
		bool HideCurrentlySelectedMagicMenuEffect();

	private:
		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;

		void NormalizeConfig();
		void ApplyTrackedFlags();
		void RefreshMagicMenuIfOpen() const;

		[[nodiscard]] static std::string BuildEffectDisplayName(const RE::EffectSetting* a_effect);
		[[nodiscard]] static std::vector<MagicEffectEntry> BuildPlayerActiveEffects();
		[[nodiscard]] static bool IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID);

		mutable std::mutex m_lock;
		OrganizerConfig m_config{};
		bool m_captureHotkey{ false };
	};
}
