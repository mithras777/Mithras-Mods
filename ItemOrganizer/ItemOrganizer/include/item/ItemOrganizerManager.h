#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace MITHRAS::ITEM_ORGANIZER
{
	struct OrganizerConfig
	{
		bool enabled{ true };
		std::uint32_t hotkey{ 0x23 };  // H
	};

	struct ItemEntry
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
		void Tick();

		void SetCaptureHotkey(bool a_capture);
		[[nodiscard]] bool IsCapturingHotkey() const;
		void OnHotkeyPressed(std::uint32_t a_keyCode);
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);

		[[nodiscard]] std::vector<ItemEntry> GetHiddenItems() const;

		bool HideItem(RE::FormID a_formID);
		bool UnhideItem(RE::FormID a_formID);
		bool HideCurrentlySelectedMenuEntry();
		[[nodiscard]] bool IsItemHidden(RE::FormID a_formID) const;

		void ApplyFilter(RE::ItemList* a_itemList) const;

	private:
		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;

		void NormalizeConfig();
		void RefreshOpenMenus() const;
		[[nodiscard]] static bool IsFormIDHidden(const std::vector<RE::FormID>& a_hidden, RE::FormID a_formID);
		[[nodiscard]] static std::string BuildItemDisplayName(const RE::TESBoundObject* a_item);
		[[nodiscard]] static RE::FormID ResolveItemFormID(const RE::ItemList::Item* a_item);
		[[nodiscard]] static RE::ItemList::Item* ResolveSelectedItemFallback(RE::ItemList* a_itemList);

		mutable std::mutex m_lock;
		OrganizerConfig m_config{};
		std::vector<RE::FormID> m_hiddenItemFormIDs{};
		bool m_captureHotkey{ false };
	};
}
