#pragma once

#include <cstdint>
#include <cctype>
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

	enum class AttackSlot : std::uint32_t
	{
		kLight = 0,
		kPower = 1,
		kBash = 2
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
			auto normalize = [](const std::string& text) {
				std::string out = text;
				for (auto& ch : out) {
					ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
				}
				return out;
			};
			if (isUnarmed || a_rhs.isUnarmed) {
				return isUnarmed == a_rhs.isUnarmed;
			}
			return normalize(displayName) == normalize(a_rhs.displayName);
		}
	};

	struct BindingKeyHash
	{
		[[nodiscard]] std::size_t operator()(const BindingKey& a_key) const noexcept
		{
			auto normalize = [](const std::string& text) {
				std::string out = text;
				for (auto& ch : out) {
					ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
				}
				return out;
			};
			std::size_t hash = std::hash<bool>{}(a_key.isUnarmed);
			if (!a_key.isUnarmed) {
				hash ^= std::hash<std::string>{}(normalize(a_key.displayName)) << 1;
			}
			return hash;
		}
	};

	struct SlotBinding
	{
		std::string spellFormKey{};
		std::string displayName{};
		float lastKnownCost{ 0.0f };
		std::uint32_t spellType{ 0 };
		bool enabled{ false };
	};

	struct WeaponBindingProfile
	{
		SlotBinding light{};
		SlotBinding power{};
		SlotBinding bash{};
		float triggerCooldownSec{ 1.5f };
		bool onlyInCombat{ false };
	};

	struct SpellBindingConfig
	{
		std::uint32_t version{ 2 };
		bool enabled{ true };
		std::uint32_t uiToggleKey{ 0x3C };          // F2
		std::uint32_t bindKey{ 0x22 };              // G
		std::uint32_t cycleSlotModifierKey{ 0x2A }; // LShift
		float fallbackDedupeSec{ 1.5f };
		bool showHudNotifications{ true };
		bool enableSoundCues{ true };
		float soundCueVolume{ 0.45f };
		bool hudDonutEnabled{ true };
		bool hudDonutOnlyUnsheathed{ true };
		bool hudShowCooldownSeconds{ true };
		std::string hudAnchor{ "top-right" };
		float hudPosX{ 48.0f };
		float hudPosY{ 48.0f };
		float hudDonutSize{ 88.0f };
		float hudCyclePosX{ 48.0f };
		float hudCyclePosY{ 152.0f };
		float hudCycleSize{ 56.0f };
		float hudChainPosX{ 48.0f };
		float hudChainPosY{ 216.0f };
		float hudChainSize{ 56.0f };
		bool hudChainAlwaysShowInCombat{ false };
		bool blacklistEnabled{ true };
		std::vector<std::string> blacklistedSpellKeys{};
		AttackSlot currentBindSlotMode{ AttackSlot::kLight };
	};

	struct UIWindowConfig
	{
		float x{ 0.0f };
		float y{ 0.0f };
		float width{ 1180.0f };
		float height{ 760.0f };
		bool isFullscreen{ false };
		bool hasSaved{ false };
	};

	struct ManagerSnapshot
	{
		std::string json{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		static constexpr std::string_view kConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";

		void Initialize();
		void Update(RE::PlayerCharacter* a_player, float a_deltaTime);
		void OnRevert();

		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);

		[[nodiscard]] SpellBindingConfig GetConfig() const;
		void SetConfig(const SpellBindingConfig& a_config, bool a_save);
		void SetSettingFromJson(const std::string& a_payload);
		void SetWeaponSettingFromJson(const std::string& a_payload);
		void SetBlacklistFromJson(const std::string& a_payload);
		void SetUIWindowFromJson(const std::string& a_payload);

		void OnEquipChanged();
		void OnMenuStateChanged(bool a_blockingMenuOpen);
		void OnAttackAnimationEvent(std::string_view a_tag, std::string_view a_payload);

		void ToggleUI();
		void PushUISnapshot();
		void PushHUDSnapshot();
		[[nodiscard]] ManagerSnapshot GetSnapshot() const;
		[[nodiscard]] std::string GetHUDSnapshot() const;

		bool TryBindSelectedMagicMenuSpell();
		void CycleBindSlotMode();
		void OnPowerAttackInputStart();
		bool BindSpellForSlotFromJson(const std::string& a_payload);
		bool UnbindSlotFromJson(const std::string& a_payload);
		bool UnbindWeaponFromSerializedKey(const std::string& a_key);
		void EnterHudDragMode();
		void SaveHudPositionFromJson(const std::string& a_payload);
		void NotifyChainSwitch(std::int32_t a_chainIndex1Based, std::string_view a_chainName);
		void NotifyChainRecordingState(std::int32_t a_chainIndex1Based, std::string_view a_chainName, bool a_recording);

		[[nodiscard]] std::string GetUIHotkeyName() const;
		[[nodiscard]] std::string GetBindHotkeyName() const;

	private:
		struct RuntimeState
		{
			bool wasAttacking{ false };
			bool attackChainActive{ false };
			bool pendingLightAttack{ false };
			bool menuBlocked{ false };
			bool hudDragModeActive{ false };
			float worldTimeSec{ 0.0f };
			float lastAttackTriggerWorldTimeSec{ -1000.0f };
			float pendingLightReadyAtWorldTimeSec{ 0.0f };
			float recentPowerInputUntilSec{ 0.0f };
			AttackSlot lastResolvedAttackSlot{ AttackSlot::kLight };
			std::optional<BindingKey> rightWeapon{};
			std::optional<BindingKey> leftWeapon{};
			std::optional<BindingKey> unarmedKey{};
			std::unordered_map<std::string, float> powerCooldownUntil{};
			std::unordered_map<std::string, float> powerCooldownGameDayUntil{};
			std::unordered_map<BindingKey, float, BindingKeyHash> weaponCooldownReadyAt{};
			std::string lastTriggerWeapon{};
			std::string lastTriggerSpell{};
			std::string lastError{};
			float lastMagickaCost{ 0.0f };
			bool lastWasPowerAttack{ false };
			float lastCycleSwitchWorldTimeSec{ -1000.0f };
			float lastChainSwitchWorldTimeSec{ -1000.0f };
			std::string lastChainHudText{ "Chain 1" };
			bool chainRecordingActive{ false };
		};

		void LoadConfig();
		void SaveConfig() const;
		void ClampConfig(SpellBindingConfig& a_config) const;
		void ClampWindowConfig(UIWindowConfig& a_config) const;

		void RefreshEquippedKeysLocked(RE::PlayerCharacter* a_player);
		[[nodiscard]] std::optional<BindingKey> ResolveCurrentBindingKeyLocked(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] static std::optional<BindingKey> BuildWeaponKeyFromEntry(const RE::InventoryEntryData* a_entry, HandSlot a_slot);
		[[nodiscard]] static BindingKey BuildUnarmedKey();

		[[nodiscard]] static std::string BuildFormKey(const RE::TESForm* a_form);
		[[nodiscard]] static bool ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID);
		[[nodiscard]] static RE::SpellItem* ResolveSpell(std::string_view a_formKey);
		[[nodiscard]] static std::optional<RE::FormID> ResolveSelectedSpellFormIDFromMagicMenu();

		[[nodiscard]] AttackSlot ResolveAttackSlot(std::string_view a_tag, std::string_view a_payload, RE::PlayerCharacter* a_player) const;
		bool TryTriggerAttackSlotLocked(RE::PlayerCharacter* a_player, AttackSlot a_slot, bool a_isPowerAttack);
		bool TryTriggerBoundSpellLocked(RE::PlayerCharacter* a_player, const BindingKey& a_key, AttackSlot a_slot, bool a_isPowerAttack);
		[[nodiscard]] WeaponBindingProfile& EnsureProfileLocked(const BindingKey& a_key);
		[[nodiscard]] SlotBinding* GetSlotBinding(WeaponBindingProfile& a_profile, AttackSlot a_slot);
		[[nodiscard]] const SlotBinding* GetSlotBinding(const WeaponBindingProfile& a_profile, AttackSlot a_slot) const;
		[[nodiscard]] bool IsMountedBlocked(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] bool IsKillmoveBlocked(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] bool IsDialogueBlocked() const;
		[[nodiscard]] bool IsBlacklisted(std::string_view a_spellFormKey) const;
		void PlayCueSuccess() const;
		void PlayCueBlocked() const;

		[[nodiscard]] static bool IsSupportedSpell(const RE::SpellItem* a_spell);
		[[nodiscard]] static float GetGameDaysPassed();
		[[nodiscard]] std::string BuildSpellMetricLocked(const SlotBinding& a_spellData, RE::SpellItem* a_spell, RE::PlayerCharacter* a_player) const;
		[[nodiscard]] static std::string SpellTypeLabel(std::uint32_t a_spellType);
		[[nodiscard]] static std::string AttackSlotLabel(AttackSlot a_slot);
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);
		[[nodiscard]] static bool IsLikelyLeftHandAttack(RE::PlayerCharacter* a_player);

		[[nodiscard]] std::string BuildSnapshotJsonLocked() const;
		[[nodiscard]] std::string BuildHUDSnapshotJsonLocked() const;
		void Notify(const std::string& a_message) const;
		void SetLastErrorLocked(std::string a_error);

	private:
		mutable std::mutex m_lock;
		SpellBindingConfig m_config{};
		UIWindowConfig m_uiWindow{};
		std::unordered_map<BindingKey, WeaponBindingProfile, BindingKeyHash> m_bindings{};
		RuntimeState m_runtime{};
	};
}
