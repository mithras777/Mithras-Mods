#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace QUICK_BUFF
{
	enum class TriggerID : std::uint32_t
	{
		kCombatStart = 0,
		kCombatEnd,
		kHealthBelow70,
		kHealthBelow50,
		kHealthBelow30,
		kCrouchStart,
		kSprintStart,
		kWeaponDraw,
		kPowerAttackStart,
		kShoutStart,
		kTotal
	};

	struct TriggerConditions
	{
		bool firstPersonOnly{ true };
		bool weaponDrawnOnly{ true };
		bool requireInCombat{ false };
		bool requireOutOfCombat{ false };
	};

	struct TriggerRules
	{
		float requireMagickaPercentAbove{ 0.0f };  // 0..1
		bool skipIfEffectAlreadyActive{ true };
	};

	struct TriggerConfig
	{
		bool enabled{ true };
		std::string spellFormID{};
		float cooldownSec{ 5.0f };
		TriggerConditions conditions{};
		TriggerRules rules{};

		float thresholdHealthPercent{ 0.40f };  // healthBelow only
		float hysteresisMargin{ 0.05f };        // healthBelow only
		float internalLockoutSec{ 0.30f };      // power/shout only
		float concentrationDurationSec{ 3.0f }; // concentration only
	};

	struct GlobalConfig
	{
		bool enabled{ true };
		bool firstPersonOnlyDefault{ true };
		bool weaponDrawnOnlyDefault{ true };
		bool preventCastingInMenus{ true };
		bool preventCastingWhileStaggered{ true };
		bool preventCastingWhileRagdoll{ true };
		float minTimeAfterLoadSeconds{ 2.0f };
	};

	struct Config
	{
		std::uint32_t version{ 1 };
		GlobalConfig global{};
		TriggerConfig combatStart{};
		TriggerConfig combatEnd{};
		TriggerConfig healthBelow70{};
		TriggerConfig healthBelow50{};
		TriggerConfig healthBelow30{};
		TriggerConfig crouchStart{};
		TriggerConfig sprintStart{};
		TriggerConfig weaponDraw{};
		TriggerConfig powerAttackStart{};
		TriggerConfig shoutStart{};

		[[nodiscard]] const TriggerConfig& Get(TriggerID a_id) const;
		[[nodiscard]] TriggerConfig& Get(TriggerID a_id);
	};

	struct KnownSpellOption
	{
		std::string name{};
		std::string formKey{};
		RE::FormID formID{ 0 };
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		static constexpr std::string_view kConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";

		void Initialize();
		void ResetRuntime();
		void ResetConfigToDefaults(bool a_saveToDisk);
		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);
		void OnRevert();

		[[nodiscard]] Config GetConfig() const;
		void SetConfig(const Config& a_config, bool a_saveToDisk);
		[[nodiscard]] static Config GetDefaultConfig();

		void Update(RE::PlayerCharacter* a_player, float a_deltaTime);
		bool TryTestCast(TriggerID a_id);

		[[nodiscard]] std::vector<KnownSpellOption> GetKnownSpells() const;
		[[nodiscard]] bool IsSpellConcentration(std::string_view a_formKey) const;
		[[nodiscard]] static std::string_view GetTriggerName(TriggerID a_id);

	private:
		struct RuntimeState
		{
			bool initialized{ false };
			bool wasInCombat{ false };
			bool wasSneaking{ false };
			bool wasSprinting{ false };
			bool wasWeaponDrawn{ false };
			bool wasPowerAttacking{ false };
			bool wasShouting{ false };
			float lastHealthPercent{ 1.0f };
			std::array<bool, 3> healthBelowArmed{ true, true, true };
			float timeSinceLoad{ 0.0f };
			std::array<float, static_cast<std::size_t>(TriggerID::kTotal)> cooldownTimers{};
			float powerAttackLockout{ 0.0f };
			float shoutLockout{ 0.0f };
		};

		struct PendingConcentration
		{
			RE::FormID spellFormID{ 0 };
			float remainingSec{ 0.0f };
			float tickTimer{ 0.0f };
			bool castOnSelf{ true };
			RE::ObjectRefHandle targetHandle{};
		};

		struct LiveState
		{
			bool firstPerson{ false };
			bool inCombat{ false };
			bool sneaking{ false };
			bool sprinting{ false };
			bool weaponDrawn{ false };
			bool powerAttacking{ false };
			bool shouting{ false };
			bool staggered{ false };
			bool ragdoll{ false };
			float healthPercent{ 1.0f };
			float magickaPercent{ 1.0f };
		};

		static Config DefaultConfig();
		void ApplyGlobalDefaults(Config& a_config) const;
		void LoadConfig();
		void SaveConfig() const;
		void ClampConfig(Config& a_config) const;

		[[nodiscard]] bool ShouldSkipGlobally(const LiveState& a_liveState) const;
		[[nodiscard]] LiveState BuildLiveState(RE::PlayerCharacter* a_player) const;
		void UpdateTimers(float a_deltaTime);
		void UpdateConcentration(RE::PlayerCharacter* a_player, float a_deltaTime, const LiveState& a_liveState);
		void SyncPreviousStates(const LiveState& a_liveState);
		void InitializeRuntimeFrom(const LiveState& a_liveState);

		bool AttemptTrigger(RE::PlayerCharacter* a_player, TriggerID a_id, const LiveState& a_liveState, bool a_ignoreCooldown);
		bool CastSpellSelf(RE::PlayerCharacter* a_player, const TriggerConfig& a_trigger, const LiveState& a_liveState, bool a_ignoreCooldown);
		bool CastResolvedSpell(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell, RE::TESObjectREFR* a_target, bool a_castOnSelf);
		[[nodiscard]] RE::TESObjectREFR* GetCrosshairTarget() const;
		[[nodiscard]] RE::TESObjectREFR* ResolveSpellTarget(RE::PlayerCharacter* a_player, const RE::SpellItem* a_spell, bool& a_castOnSelf) const;

		[[nodiscard]] static RE::SpellItem* ResolveSpell(std::string_view a_formKey);
		[[nodiscard]] static std::string BuildFormKey(const RE::TESForm* a_form);
		[[nodiscard]] static bool ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID);
		[[nodiscard]] static bool IsSpellAlreadyActive(RE::PlayerCharacter* a_player, const RE::SpellItem* a_spell);
		[[nodiscard]] static float Percent01(float a_value, float a_maximum);

	private:
		Config m_config{};
		RuntimeState m_runtime{};
		std::vector<PendingConcentration> m_pendingConcentration{};
	};
}
