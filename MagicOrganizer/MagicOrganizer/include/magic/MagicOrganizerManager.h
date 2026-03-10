#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace SKSE
{
	class SerializationInterface;
}

namespace MITHRAS::MAGIC_ORGANIZER
{
	struct OrganizerConfig
	{
		bool enabled{ true };
		std::uint32_t hotkey{ 0x23 };  // H
	};

	struct SpellEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
	};

	struct PowerShoutEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
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

		[[nodiscard]] std::vector<SpellEntry> GetHiddenSpells() const;
		[[nodiscard]] std::vector<PowerShoutEntry> GetHiddenPowersAndShouts() const;
		[[nodiscard]] std::vector<MagicEffectEntry> GetHiddenEffects() const;

		bool HideSpell(RE::FormID a_formID);
		bool UnhideSpell(RE::FormID a_formID);
		bool HidePowerOrShout(RE::FormID a_formID);
		bool UnhidePowerOrShout(RE::FormID a_formID);
		bool HideEffect(RE::FormID a_formID);
		bool UnhideEffect(RE::FormID a_formID);

		bool HideCurrentlySelectedMagicMenuEntry();
		[[nodiscard]] bool IsPowerShoutHidden(RE::FormID a_formID) const;

		static void RegisterSerialization();
		void SaveHiddenToCosave(SKSE::SerializationInterface* a_serialization) const;
		void LoadHiddenFromCosave(SKSE::SerializationInterface* a_serialization);
		void RevertHiddenFromCosave();

	private:
		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;

		void NormalizeConfig();
		void ApplyTrackedFlags();
		void RefreshMagicMenuIfOpen() const;

		[[nodiscard]] static bool IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID);
		[[nodiscard]] static bool IsOrganizerSpell(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsOrganizerPower(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsOrganizerShout(const RE::TESShout* a_shout);

		[[nodiscard]] static std::string BuildSpellDisplayName(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string BuildPowerShoutDisplayName(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string BuildPowerShoutDisplayName(const RE::TESShout* a_shout);
		[[nodiscard]] static std::string BuildEffectDisplayName(const RE::EffectSetting* a_effect);

		[[nodiscard]] static std::vector<MagicEffectEntry> BuildPlayerActiveEffects();

		mutable std::mutex m_lock;
		OrganizerConfig m_config{};
		std::vector<RE::FormID> m_hiddenSpellFormIDs{};
		std::vector<RE::FormID> m_hiddenPowerShoutFormIDs{};
		std::vector<RE::FormID> m_hiddenEffectFormIDs{};
		bool m_captureHotkey{ false };
	};
}
