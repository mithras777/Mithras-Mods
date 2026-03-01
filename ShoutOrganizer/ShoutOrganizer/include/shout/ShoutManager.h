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

namespace MITHRAS::SHOUT_ORGANIZER
{
	struct OrganizerConfig
	{
		bool enabled{ true };
		std::uint32_t hotkey{ 0x23 };  // H
	};

	struct PowerShoutEntry
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

		[[nodiscard]] std::vector<PowerShoutEntry> GetHiddenEntries() const;
		static void RegisterSerialization();
		void SaveHiddenToCosave(SKSE::SerializationInterface* a_serialization) const;
		void LoadHiddenFromCosave(SKSE::SerializationInterface* a_serialization);
		void RevertHiddenFromCosave();

		bool HideEntry(RE::FormID a_formID);
		bool UnhideEntry(RE::FormID a_formID);
		bool HideCurrentlySelectedMagicMenuPowerOrShout();

	private:
		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;

		void NormalizeConfig();
		void ApplyTrackedFlags();
		void RefreshMagicMenuIfOpen() const;

		[[nodiscard]] static bool IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID);
		[[nodiscard]] static bool IsOrganizerPower(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsOrganizerShout(const RE::TESShout* a_shout);
		[[nodiscard]] static std::string BuildDisplayName(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string BuildDisplayName(const RE::TESShout* a_shout);

		mutable std::mutex m_lock;
		OrganizerConfig m_config{};
		std::vector<RE::FormID> m_hiddenPowerShoutFormIDs{};
		bool m_captureHotkey{ false };
	};
}
