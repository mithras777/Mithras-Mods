#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace SBIND
{
	enum class HandSlot : std::uint32_t
	{
		kRight = 0,
		kLeft = 1,
		kUnarmed = 2
	};

	struct BindingKey
	{
		std::string pluginName{};
		RE::FormID localFormID{ 0 };
		std::uint16_t uniqueID{ 0 };
		HandSlot handSlot{ HandSlot::kRight };
		bool isUnarmed{ false };
		std::string displayName{};

		[[nodiscard]] bool operator==(const BindingKey& a_rhs) const noexcept
		{
			return pluginName == a_rhs.pluginName &&
			       localFormID == a_rhs.localFormID &&
			       uniqueID == a_rhs.uniqueID &&
			       handSlot == a_rhs.handSlot &&
			       isUnarmed == a_rhs.isUnarmed;
		}
	};

	struct BindingKeyHash
	{
		[[nodiscard]] std::size_t operator()(const BindingKey& a_key) const noexcept
		{
			std::size_t hash = std::hash<std::string>{}(a_key.pluginName);
			hash ^= static_cast<std::size_t>(a_key.localFormID) << 1;
			hash ^= static_cast<std::size_t>(a_key.uniqueID) << 5;
			hash ^= static_cast<std::size_t>(a_key.handSlot) << 10;
			hash ^= static_cast<std::size_t>(a_key.isUnarmed) << 14;
			return hash;
		}
	};

	struct BoundSpell
	{
		std::string spellFormKey{};
		std::string displayName{};
		float lastKnownCost{ 0.0f };
	};

	struct SpellBindingConfig
	{
		std::uint32_t version{ 1 };
		bool enabled{ true };
		std::uint32_t uiToggleKey{ 0x3C };  // F2
		std::uint32_t bindKey{ 0x22 };      // G
		float powerDamageScale{ 2.0f };
		float powerMagickaScale{ 2.0f };
		bool showHudNotifications{ true };
	};

	struct ManagerSnapshot
	{
		std::string json{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		static constexpr std::string_view kConfigPath = "Data/SKSE/Plugins/SpellBinding.json";

		void Initialize();
		void Update(RE::PlayerCharacter* a_player, float a_deltaTime);
		void OnRevert();

		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);

		[[nodiscard]] SpellBindingConfig GetConfig() const;
		void SetConfig(const SpellBindingConfig& a_config, bool a_save);

		void OnEquipChanged();
		void OnMenuStateChanged(bool a_blockingMenuOpen);

		void ToggleUI();
		void PushUISnapshot();
		[[nodiscard]] ManagerSnapshot GetSnapshot() const;

		bool TryBindSelectedMagicMenuSpell();
		bool UnbindWeaponFromSerializedKey(const std::string& a_key);

		[[nodiscard]] std::string GetUIHotkeyName() const;
		[[nodiscard]] std::string GetBindHotkeyName() const;

	private:
		struct RuntimeState
		{
			bool wasAttacking{ false };
			bool menuBlocked{ false };
			std::optional<BindingKey> rightWeapon{};
			std::optional<BindingKey> leftWeapon{};
			std::optional<BindingKey> unarmedKey{};
			std::string lastTriggerWeapon{};
			std::string lastTriggerSpell{};
			std::string lastError{};
			float lastMagickaCost{ 0.0f };
			bool lastWasPowerAttack{ false };
		};

		void LoadConfig();
		void SaveConfig() const;
		void ClampConfig(SpellBindingConfig& a_config) const;

		void RefreshEquippedKeysLocked(RE::PlayerCharacter* a_player);
		[[nodiscard]] std::optional<BindingKey> ResolveCurrentBindingKeyLocked(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] static std::optional<BindingKey> BuildWeaponKeyFromEntry(const RE::InventoryEntryData* a_entry, HandSlot a_slot);
		[[nodiscard]] static BindingKey BuildUnarmedKey();

		[[nodiscard]] static std::string BuildFormKey(const RE::TESForm* a_form);
		[[nodiscard]] static bool ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID);
		[[nodiscard]] static RE::SpellItem* ResolveSpell(std::string_view a_formKey);
		[[nodiscard]] static std::optional<RE::FormID> ResolveSelectedSpellFormIDFromMagicMenu();

		bool TryTriggerOnAttackStart(RE::PlayerCharacter* a_player);
		bool TryTriggerBoundSpellLocked(RE::PlayerCharacter* a_player, const BindingKey& a_key, bool a_isPowerAttack);

		[[nodiscard]] static bool IsSupportedSpell(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);
		[[nodiscard]] static bool IsLikelyLeftHandAttack(RE::PlayerCharacter* a_player);

		[[nodiscard]] std::string BuildSnapshotJsonLocked() const;
		void Notify(const std::string& a_message) const;
		void SetLastErrorLocked(std::string a_error);

	private:
		mutable std::mutex m_lock;
		SpellBindingConfig m_config{};
		std::unordered_map<BindingKey, BoundSpell, BindingKeyHash> m_bindings{};
		RuntimeState m_runtime{};
	};
}
