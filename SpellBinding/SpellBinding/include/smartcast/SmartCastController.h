#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SMART_CAST
{
	enum class StepType : std::uint32_t
	{
		kFireAndForget = 0,
		kConcentration
	};

	enum class CastOn : std::uint32_t
	{
		kSelf = 0,
		kCrosshair,
		kAimed
	};

	enum class Mode : std::uint32_t
	{
		kIdle = 0,
		kRecording,
		kPlaying
	};

	struct ChainStep
	{
		std::string spellFormID{};
		StepType type{ StepType::kFireAndForget };
		CastOn castOn{ CastOn::kSelf };
		float holdSec{ 0.0f };
		std::uint32_t castCount{ 1 };
	};

	struct ChainConfig
	{
		std::string name{ "Chain" };
		bool enabled{ true };
		std::string hotkey{ "None" };
		std::vector<ChainStep> steps{};
		float stepDelaySec{ 1.0f };
	};

	struct RecordConfig
	{
		std::string toggleKey{ "H" };
		std::string cancelKey{ "None" };
		float maxIdleSec{ 4.0f };
		bool recordOnlySuccessfulCasts{ true };
		bool ignorePowers{ true };
		bool ignoreShouts{ true };
		bool ignoreScrolls{ true };
	};

	struct PlaybackConfig
	{
		std::string playKey{ "X" };
		std::string cancelKey{ "G" };
		std::int32_t defaultChainIndex{ 1 };
		std::string cycleModifierKey{ "Shift" };
		float stepDelaySec{ 1.0f };
		bool abortOnFail{ false };
		bool skipOnFail{ true };
		bool requireWeaponSheathed{ false };
		bool autoSheathDuringPlayback{ false };
		bool stopIfPlayerHit{ true };
		bool stopIfAttackPressed{ true };
		bool stopIfBlockPressed{ true };
	};

	struct ConcentrationConfig
	{
		float minHoldSec{ 0.15f };
		float maxHoldSec{ 2.50f };
		float sampleGranularitySec{ 0.05f };
		float releasePaddingSec{ 0.05f };
	};

	struct TargetingConfig
	{
		std::string mode{ "hybrid" };
		float raycastRange{ 4096.0f };
		float aimConeDegrees{ 6.0f };
		bool preferCrosshairActor{ true };
		bool fallbackToSelfIfNoTarget{ true };
	};

	struct GlobalConfig
	{
		bool enabled{ true };
		bool firstPersonOnly{ true };
		bool preventInMenus{ true };
		bool preventWhileStaggered{ true };
		bool preventWhileRagdoll{ true };
		float minTimeAfterLoadSeconds{ 2.0f };
		std::int32_t maxChains{ 1 };
		std::int32_t maxStepsPerChain{ 12 };
		RecordConfig record{};
		PlaybackConfig playback{};
		ConcentrationConfig concentration{};
		TargetingConfig targeting{};
	};

	struct Config
	{
		std::uint32_t version{ 1 };
		GlobalConfig global{};
		std::vector<ChainConfig> chains{};
	};

	struct KnownSpellOption
	{
		std::string name{};
		std::string formKey{};
		RE::FormID formID{ 0 };
		float magickaCost{ 0.0f };
		bool concentration{ false };
	};

	class Controller final : public REX::Singleton<Controller>
	{
	public:
		static constexpr std::string_view kConfigPath = "Data/SKSE/Plugins/SpellbladeOverhaul.json";

		void Initialize();
		void Update(RE::PlayerCharacter* a_player, float a_deltaTime);
		void ResetRuntime();
		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);
		void OnRevert();

		[[nodiscard]] Config GetConfig() const;
		void SetConfig(const Config& a_config, bool a_save);
		[[nodiscard]] static Config GetDefaultConfig();

		void StartRecording(std::int32_t a_chainIndex1Based);
		void StopRecording();
		void StartPlayback(std::int32_t a_chainIndex1Based);
		void StopPlayback(bool a_forceStopCast);

		[[nodiscard]] bool IsSpellConcentration(std::string_view a_formKey) const;
		[[nodiscard]] bool IsSpellSelfDelivery(std::string_view a_formKey) const;
		[[nodiscard]] std::vector<KnownSpellOption> GetKnownSpells() const;
		[[nodiscard]] bool IsRecording() const;
		[[nodiscard]] bool IsPlaying() const;
		[[nodiscard]] std::int32_t GetActiveChainIndex1Based() const;

	private:
		struct RuntimeState
		{
			Mode mode{ Mode::kIdle };
			std::int32_t activeChainIndex{ 0 };  // 0-based
			std::int32_t playbackChainIndex{ -1 };  // 0-based, independent from selected active chain
			float timeSinceLoad{ 0.0f };
			float recordIdleTimer{ 0.0f };
			float stepDelayTimer{ 0.0f };
			std::size_t playbackStepIndex{ 0 };
			float concentrationHoldRemaining{ 0.0f };
			float releasePaddingRemaining{ 0.0f };
			float concentrationTickTimer{ 0.0f };
			bool initialized{ false };
			bool wasCastingLeft{ false };
			bool wasCastingRight{ false };
			float castStartLeft{ 0.0f };
			float castStartRight{ 0.0f };
			std::string activeConcentrationSpell{};
			bool wasAttacking{ false };
			bool wasBlocking{ false };
			std::array<bool, 8> keyWasDown{};
			std::vector<bool> chainKeyWasDown{};
			RE::ObjectRefHandle playbackTarget{};
			bool playbackCastOnSelf{ true };
			bool wasPowerOrShoutCasting{ false };
		};

		enum class HotkeySlot : std::uint32_t
		{
			kRecordToggle = 0,
			kRecordCancel,
			kPlay,
			kPlayCancel,
			kCount
		};

		struct CastCapture
		{
			RE::SpellItem* spell{ nullptr };
			bool started{ false };
			float startTime{ 0.0f };
		};

		void LoadConfig();
		void SaveConfig() const;
		void ClampConfig(Config& a_cfg) const;
		void EnsureChainCount(Config& a_cfg) const;

		void UpdateHotkeys(RE::PlayerCharacter* a_player);
		void UpdateRecording(RE::PlayerCharacter* a_player, float a_deltaTime);
		void UpdatePlayback(RE::PlayerCharacter* a_player, float a_deltaTime);

		void ProcessCastCapture(RE::PlayerCharacter* a_player, CastCapture& a_capture, bool a_castingNow, RE::TESForm* a_equippedForm);
		void RecordStepOnCastStart(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell);
		void RecordConcentrationDuration(RE::SpellItem* a_spell, float a_durationSec);

		[[nodiscard]] bool ExecuteStep(RE::PlayerCharacter* a_player, const ChainStep& a_step);
		[[nodiscard]] bool CastSpellForStep(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell, CastOn a_castOn, bool a_isConcentration, RE::ObjectRefHandle* a_targetOut, bool* a_castOnSelfOut);

		[[nodiscard]] bool ShouldBlockGlobal(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] bool IsKeyDown(const std::string& a_keyName) const;
		[[nodiscard]] bool IsHotkeyPressed(HotkeySlot a_slot, const std::string& a_keyName);
		[[nodiscard]] std::int32_t ResolveChainIndex(std::int32_t a_index1Based) const;

		[[nodiscard]] static RE::SpellItem* ResolveSpell(std::string_view a_formKey);
		[[nodiscard]] static bool ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID);
		[[nodiscard]] static std::string BuildFormKey(const RE::TESForm* a_form);
		[[nodiscard]] static RE::TESObjectREFR* GetCrosshairTarget(bool a_preferActor);
		[[nodiscard]] static RE::SpellItem* ResolveSelectedPowerSpell(RE::PlayerCharacter* a_player);
		[[nodiscard]] static float Clamp01(float a_value);
		[[nodiscard]] static float Quantize(float a_value, float a_step);

	private:
		Config m_config{};
		RuntimeState m_runtime{};
		CastCapture m_leftCapture{};
		CastCapture m_rightCapture{};
	};
}
