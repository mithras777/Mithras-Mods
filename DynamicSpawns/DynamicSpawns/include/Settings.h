#pragma once

#include <json/single_include/nlohmann/json.hpp>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace DYNAMIC_SPAWNS
{
	enum class CellType
	{
		kAny,
		kInterior,
		kExterior
	};

	enum class TimeOfDay
	{
		kAny,
		kDay,
		kNight
	};

	enum class SpawnMethod
	{
		kLeveledList,
		kActorBase,
		kFormList
	};

	struct DebugSettings
	{
		std::string logLevel{ "info" };
		bool        drawMarkers{ false };
		bool        notify{ false };
	};

	struct LimitsSettings
	{
		std::int32_t globalMaxAlive{ 25 };
		std::int32_t maxPerCell{ 6 };
		std::int32_t maxPerEvent{ 3 };
		float        minDistanceFromPlayer{ 1200.0F };
		float        minDistanceFromOtherSpawn{ 600.0F };
		float        despawnDistance{ 9000.0F };
		bool         despawnOnCellDetach{ true };
		std::int32_t maxManagedRefs{ 200 };
	};

	struct TimingSettings
	{
		float cooldownSecondsSameCell{ 180.0F };
		float cooldownSecondsGlobal{ 8.0F };
		float deferSpawnSeconds{ 0.5F };
	};

	struct FiltersSettings
	{
		bool                     skipIfInCombat{ false };
		bool                     skipIfSneaking{ false };
		bool                     skipIfInterior{ false };
		bool                     skipIfExterior{ false };
		std::vector<std::string> skipWorldspaces{ "TamrielUNUSED_EXAMPLE" };
		std::vector<std::string> skipLocationsByKeyword{ "LocTypeCity", "LocTypeTown", "LocTypePlayerHouse" };
		std::vector<std::string> skipCellsByNameContains{ "Whiterun", "Solitude", "Riften" };
		std::vector<std::string> skipIfCellHasKeyword{};
		bool                     onlySpawnIfEncounterZoneExists{ false };

		std::vector<std::string> skipWorldspacesLower{};
		std::vector<std::string> skipCellsByNameContainsLower{};
	};

	struct RuleMatch
	{
		std::string              worldspace{ "Tamriel" };
		CellType                 cellType{ CellType::kExterior };
		std::vector<std::string> locationKeywordAny{ "LocTypeWilderness" };
		TimeOfDay                timeOfDay{ TimeOfDay::kAny };
	};

	struct RuleBudget
	{
		double       chance{ 0.35 };
		std::int32_t minToSpawn{ 1 };
		std::int32_t maxToSpawn{ 3 };
	};

	struct RuleWhat
	{
		SpawnMethod  method{ SpawnMethod::kLeveledList };
		std::string  form{ "Skyrim.esm|0x00023AB8" };
	};

	struct RuleFactions
	{
		bool inheritFromEncounterZone{ true };
	};

	struct SpawnRule
	{
		std::string  name{ "Default Wilderness" };
		bool         enabled{ true };
		RuleMatch    match{};
		RuleBudget   budget{};
		RuleWhat     what{};
		RuleFactions factions{};
	};

	struct GenesisCompatibilitySettings
	{
		bool  skipIfHostilesNearby{ false };
		float hostileScanRadius{ 8000.0F };
		std::int32_t hostileScanMaxRefs{ 200 };

		bool  unlevelNPCs{ false };
		float healthPctMin{ 85.0F };
		float healthPctMax{ 125.0F };
		float magickaPctMin{ 85.0F };
		float magickaPctMax{ 125.0F };
		float staminaPctMin{ 85.0F };
		float staminaPctMax{ 125.0F };

		bool                     giveSpawnPotions{ false };
		std::int32_t             potionChancePct{ 25 };
		std::int32_t             potionCountMin{ 1 };
		std::int32_t             potionCountMax{ 3 };
		std::vector<std::string> spawnPotionPool{
			"LItemPotionHealthBest75",
			"LItemPotionMagickaBest75",
			"LItemPotionStaminaBest75"
		};

		bool                     lootInjectionEnabled{ false };
		std::int32_t             lootChancePct{ 33 };
		std::vector<std::string> lootPotionPool{ "LItemPotionHealthBest75" };
		std::vector<std::string> lootScrollPool{ "LItemSpellTomes75" };
		std::vector<std::string> lootMiscPool{ "LItemMiscVendorMiscItems75" };
		std::vector<std::string> lootSackPool{ "LItemMiscVendorMiscItems75" };
	};

	struct SettingsData
	{
		bool                    enabled{ true };
		DebugSettings           debug{};
		LimitsSettings          limits{};
		TimingSettings          timing{};
		FiltersSettings         filters{};
		std::vector<SpawnRule>  spawnRules{};
		GenesisCompatibilitySettings genesis{};
	};

	class Settings final : public REX::Singleton<Settings>
	{
	public:
		void LoadOrCreate();
		void Reload();
		bool Save();
		void UpdateAndSave(const SettingsData& a_updated);

		[[nodiscard]] const SettingsData& Get() const;
		[[nodiscard]] std::filesystem::path GetSettingsPath() const;
		[[nodiscard]] std::filesystem::path GetPresetDirectory() const;
		[[nodiscard]] bool IsLoaded() const;

	private:
		static SettingsData MakeDefault();
		static nlohmann::json ToJson(const SettingsData& a_settings);
		static SettingsData FromJson(const nlohmann::json& a_json);
		static void Clamp(SettingsData& a_settings);
		static void FillLowerCaches(SettingsData& a_settings);
		static void MergePreset(SettingsData& a_target, const nlohmann::json& a_json);

		void SaveDefaults(const std::filesystem::path& a_path) const;
		void LoadPresets(SettingsData& a_settings) const;
		void ApplyLogLevel() const;

		SettingsData m_settings{};
		bool         m_loaded{ false };
		mutable std::mutex m_lock;
	};
}
