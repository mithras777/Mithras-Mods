#include "Settings.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace DYNAMIC_SPAWNS
{
	namespace
	{
		using json = nlohmann::json;

		std::string ToLower(std::string a_value)
		{
			std::transform(a_value.begin(), a_value.end(), a_value.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return a_value;
		}

		CellType ParseCellType(const std::string& a_value)
		{
			const auto lower = ToLower(a_value);
			if (lower == "interior") {
				return CellType::kInterior;
			}
			if (lower == "exterior") {
				return CellType::kExterior;
			}
			return CellType::kAny;
		}

		std::string CellTypeToString(CellType a_value)
		{
			switch (a_value) {
				case CellType::kInterior:
					return "interior";
				case CellType::kExterior:
					return "exterior";
				default:
					return "any";
			}
		}

		TimeOfDay ParseTimeOfDay(const std::string& a_value)
		{
			const auto lower = ToLower(a_value);
			if (lower == "day") {
				return TimeOfDay::kDay;
			}
			if (lower == "night") {
				return TimeOfDay::kNight;
			}
			return TimeOfDay::kAny;
		}

		std::string TimeOfDayToString(TimeOfDay a_value)
		{
			switch (a_value) {
				case TimeOfDay::kDay:
					return "day";
				case TimeOfDay::kNight:
					return "night";
				default:
					return "any";
			}
		}

		SpawnMethod ParseSpawnMethod(const std::string& a_value)
		{
			const auto lower = ToLower(a_value);
			if (lower == "actorbase") {
				return SpawnMethod::kActorBase;
			}
			if (lower == "formlist") {
				return SpawnMethod::kFormList;
			}
			return SpawnMethod::kLeveledList;
		}

		std::string SpawnMethodToString(SpawnMethod a_value)
		{
			switch (a_value) {
				case SpawnMethod::kActorBase:
					return "actorBase";
				case SpawnMethod::kFormList:
					return "formlist";
				default:
					return "leveledList";
			}
		}
	}

	void Settings::LoadOrCreate()
	{
		std::scoped_lock lk(m_lock);
		const auto path = GetSettingsPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		std::filesystem::create_directories(GetPresetDirectory(), ec);

		if (!std::filesystem::exists(path)) {
			m_settings = MakeDefault();
			SaveDefaults(path);
			Clamp(m_settings);
			FillLowerCaches(m_settings);
			m_loaded = true;
			ApplyLogLevel();
			LOG_INFO("DynamicSpawns config generated: {}", path.string());
			return;
		}

		try {
			std::ifstream file(path);
			json j;
			file >> j;

			m_settings = FromJson(j);
			LoadPresets(m_settings);
			Clamp(m_settings);
			FillLowerCaches(m_settings);
			m_loaded = true;
			ApplyLogLevel();
			LOG_INFO("DynamicSpawns config loaded: {}", path.string());
		} catch (const std::exception& e) {
			LOG_ERROR("Failed to parse settings.json: {}", e.what());
			m_settings = MakeDefault();
			SaveDefaults(path);
			Clamp(m_settings);
			FillLowerCaches(m_settings);
			m_loaded = true;
			ApplyLogLevel();
			LOG_WARN("DynamicSpawns settings reset to defaults");
		}
	}

	void Settings::Reload()
	{
		LoadOrCreate();
	}

	const SettingsData& Settings::Get() const
	{
		return m_settings;
	}

	bool Settings::Save()
	{
		std::scoped_lock lk(m_lock);
		const auto path = GetSettingsPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		try {
			std::ofstream file(path, std::ios::trunc);
			file << ToJson(m_settings).dump(2);
			return true;
		} catch (const std::exception& e) {
			LOG_ERROR("Failed to save settings.json: {}", e.what());
			return false;
		}
	}

	void Settings::UpdateAndSave(const SettingsData& a_updated)
	{
		{
			std::scoped_lock lk(m_lock);
			m_settings = a_updated;
			Clamp(m_settings);
			FillLowerCaches(m_settings);
			ApplyLogLevel();
		}
		Save();
	}

	std::filesystem::path Settings::GetSettingsPath() const
	{
		const auto& plugin = DLLMAIN::Plugin::GetSingleton()->Info();
		return std::filesystem::path(plugin.gameDirectory) / "Data" / "SKSE" / "Plugins" / "DynamicSpawns" / "settings.json";
	}

	std::filesystem::path Settings::GetPresetDirectory() const
	{
		return GetSettingsPath().parent_path() / "presets";
	}

	bool Settings::IsLoaded() const
	{
		return m_loaded;
	}

	SettingsData Settings::MakeDefault()
	{
		SettingsData defaults{};
		defaults.spawnRules.clear();

		auto makeRule = [](std::string name, std::vector<std::string> keywords, std::string form) {
			SpawnRule r{};
			r.name = std::move(name);
			r.match.cellType = CellType::kAny;
			r.match.timeOfDay = TimeOfDay::kAny;
			r.match.worldspace.clear();
			r.match.locationKeywordAny = std::move(keywords);
			r.what.method = SpawnMethod::kLeveledList;
			r.what.form = std::move(form);
			r.budget.chance = 0.40;
			r.budget.minToSpawn = 1;
			r.budget.maxToSpawn = 3;
			return r;
		};

		defaults.spawnRules.emplace_back(makeRule("Draugr", { "LocTypeDraugrCrypt" }, "LCharDraugrMelee1H"));
		defaults.spawnRules.emplace_back(makeRule("Bandits", { "LocTypeBanditCamp", "LocTypeMine" }, "LCharBanditMelee1H"));
		defaults.spawnRules.emplace_back(makeRule("Falmer", { "LocTypeFalmerHive" }, "LCharFalmerMelee"));
		defaults.spawnRules.emplace_back(makeRule("Dwarven", { "LocSetDwarvenRuin", "LocTypeDwarvenAutomatons" }, "LCharDwarvenSpider"));
		defaults.spawnRules.emplace_back(makeRule("Forsworn", { "LocTypeForswornCamp" }, "LCharForswornMelee1H"));
		defaults.spawnRules.emplace_back(makeRule("Vampire", { "LocTypeVampireLair" }, "LCharVampireMelee"));
		defaults.spawnRules.emplace_back(makeRule("Warlock", { "LocTypeWarlockLair" }, "LCharWarlockDestruction"));

		SpawnRule wilderness{};
		wilderness.name = "Default Wilderness";
		wilderness.match.worldspace = "Tamriel";
		wilderness.match.cellType = CellType::kExterior;
		wilderness.match.locationKeywordAny = { "LocTypeWilderness" };
		wilderness.what.method = SpawnMethod::kLeveledList;
		wilderness.what.form = "Skyrim.esm|0x00023AB8";
		defaults.spawnRules.emplace_back(std::move(wilderness));
		return defaults;
	}

	json Settings::ToJson(const SettingsData& a_settings)
	{
		json rules = json::array();
		for (const auto& rule : a_settings.spawnRules) {
			rules.push_back({
				{ "name", rule.name },
				{ "enabled", rule.enabled },
				{ "match",
					{
						{ "worldspace", rule.match.worldspace },
						{ "cellType", CellTypeToString(rule.match.cellType) },
						{ "locationKeywordAny", rule.match.locationKeywordAny },
						{ "timeOfDay", TimeOfDayToString(rule.match.timeOfDay) }
					} },
				{ "budget",
					{
						{ "chance", rule.budget.chance },
						{ "minToSpawn", rule.budget.minToSpawn },
						{ "maxToSpawn", rule.budget.maxToSpawn }
					} },
				{ "what",
					{
						{ "method", SpawnMethodToString(rule.what.method) },
						{ "form", rule.what.form }
					} },
				{ "factions",
					{
						{ "inheritFromEncounterZone", rule.factions.inheritFromEncounterZone }
					} }
			});
		}

		return {
			{ "enabled", a_settings.enabled },
			{ "debug",
				{
					{ "logLevel", a_settings.debug.logLevel },
					{ "drawMarkers", a_settings.debug.drawMarkers },
					{ "notify", a_settings.debug.notify }
				} },
			{ "limits",
				{
					{ "globalMaxAlive", a_settings.limits.globalMaxAlive },
					{ "maxPerCell", a_settings.limits.maxPerCell },
					{ "maxPerEvent", a_settings.limits.maxPerEvent },
					{ "minDistanceFromPlayer", a_settings.limits.minDistanceFromPlayer },
					{ "minDistanceFromOtherSpawn", a_settings.limits.minDistanceFromOtherSpawn },
					{ "despawnDistance", a_settings.limits.despawnDistance },
					{ "despawnOnCellDetach", a_settings.limits.despawnOnCellDetach },
					{ "maxManagedRefs", a_settings.limits.maxManagedRefs }
				} },
			{ "timing",
				{
					{ "cooldownSecondsSameCell", a_settings.timing.cooldownSecondsSameCell },
					{ "cooldownSecondsGlobal", a_settings.timing.cooldownSecondsGlobal },
					{ "deferSpawnSeconds", a_settings.timing.deferSpawnSeconds }
				} },
			{ "filters",
				{
					{ "skipIfInCombat", a_settings.filters.skipIfInCombat },
					{ "skipIfSneaking", a_settings.filters.skipIfSneaking },
					{ "skipIfInterior", a_settings.filters.skipIfInterior },
					{ "skipIfExterior", a_settings.filters.skipIfExterior },
					{ "skipWorldspaces", a_settings.filters.skipWorldspaces },
					{ "skipLocationsByKeyword", a_settings.filters.skipLocationsByKeyword },
					{ "skipCellsByNameContains", a_settings.filters.skipCellsByNameContains },
					{ "skipIfCellHasKeyword", a_settings.filters.skipIfCellHasKeyword },
					{ "onlySpawnIfEncounterZoneExists", a_settings.filters.onlySpawnIfEncounterZoneExists }
				} },
			{ "spawnRules", rules }
			,
			{ "genesis",
				{
					{ "skipIfHostilesNearby", a_settings.genesis.skipIfHostilesNearby },
					{ "hostileScanRadius", a_settings.genesis.hostileScanRadius },
					{ "hostileScanMaxRefs", a_settings.genesis.hostileScanMaxRefs },
					{ "unlevelNPCs", a_settings.genesis.unlevelNPCs },
					{ "healthPctMin", a_settings.genesis.healthPctMin },
					{ "healthPctMax", a_settings.genesis.healthPctMax },
					{ "magickaPctMin", a_settings.genesis.magickaPctMin },
					{ "magickaPctMax", a_settings.genesis.magickaPctMax },
					{ "staminaPctMin", a_settings.genesis.staminaPctMin },
					{ "staminaPctMax", a_settings.genesis.staminaPctMax },
					{ "giveSpawnPotions", a_settings.genesis.giveSpawnPotions },
					{ "potionChancePct", a_settings.genesis.potionChancePct },
					{ "potionCountMin", a_settings.genesis.potionCountMin },
					{ "potionCountMax", a_settings.genesis.potionCountMax },
					{ "spawnPotionPool", a_settings.genesis.spawnPotionPool },
					{ "lootInjectionEnabled", a_settings.genesis.lootInjectionEnabled },
					{ "lootChancePct", a_settings.genesis.lootChancePct },
					{ "lootPotionPool", a_settings.genesis.lootPotionPool },
					{ "lootScrollPool", a_settings.genesis.lootScrollPool },
					{ "lootMiscPool", a_settings.genesis.lootMiscPool },
					{ "lootSackPool", a_settings.genesis.lootSackPool }
				} }
		};
	}

	SettingsData Settings::FromJson(const json& a_json)
	{
		SettingsData out = MakeDefault();

		out.enabled = a_json.value("enabled", out.enabled);

		if (a_json.contains("debug")) {
			const auto& debug = a_json["debug"];
			out.debug.logLevel = debug.value("logLevel", out.debug.logLevel);
			out.debug.drawMarkers = debug.value("drawMarkers", out.debug.drawMarkers);
			out.debug.notify = debug.value("notify", out.debug.notify);
		}

		if (a_json.contains("limits")) {
			const auto& limits = a_json["limits"];
			out.limits.globalMaxAlive = limits.value("globalMaxAlive", out.limits.globalMaxAlive);
			out.limits.maxPerCell = limits.value("maxPerCell", out.limits.maxPerCell);
			out.limits.maxPerEvent = limits.value("maxPerEvent", out.limits.maxPerEvent);
			out.limits.minDistanceFromPlayer = limits.value("minDistanceFromPlayer", out.limits.minDistanceFromPlayer);
			out.limits.minDistanceFromOtherSpawn = limits.value("minDistanceFromOtherSpawn", out.limits.minDistanceFromOtherSpawn);
			out.limits.despawnDistance = limits.value("despawnDistance", out.limits.despawnDistance);
			out.limits.despawnOnCellDetach = limits.value("despawnOnCellDetach", out.limits.despawnOnCellDetach);
			out.limits.maxManagedRefs = limits.value("maxManagedRefs", out.limits.maxManagedRefs);
		}

		if (a_json.contains("timing")) {
			const auto& timing = a_json["timing"];
			out.timing.cooldownSecondsSameCell = timing.value("cooldownSecondsSameCell", out.timing.cooldownSecondsSameCell);
			out.timing.cooldownSecondsGlobal = timing.value("cooldownSecondsGlobal", out.timing.cooldownSecondsGlobal);
			out.timing.deferSpawnSeconds = timing.value("deferSpawnSeconds", out.timing.deferSpawnSeconds);
		}

		if (a_json.contains("filters")) {
			const auto& filters = a_json["filters"];
			out.filters.skipIfInCombat = filters.value("skipIfInCombat", out.filters.skipIfInCombat);
			out.filters.skipIfSneaking = filters.value("skipIfSneaking", out.filters.skipIfSneaking);
			out.filters.skipIfInterior = filters.value("skipIfInterior", out.filters.skipIfInterior);
			out.filters.skipIfExterior = filters.value("skipIfExterior", out.filters.skipIfExterior);
			out.filters.skipWorldspaces = filters.value("skipWorldspaces", out.filters.skipWorldspaces);
			out.filters.skipLocationsByKeyword = filters.value("skipLocationsByKeyword", out.filters.skipLocationsByKeyword);
			out.filters.skipCellsByNameContains = filters.value("skipCellsByNameContains", out.filters.skipCellsByNameContains);
			out.filters.skipIfCellHasKeyword = filters.value("skipIfCellHasKeyword", out.filters.skipIfCellHasKeyword);
			out.filters.onlySpawnIfEncounterZoneExists = filters.value("onlySpawnIfEncounterZoneExists", out.filters.onlySpawnIfEncounterZoneExists);
		}

		out.spawnRules.clear();
		if (a_json.contains("spawnRules") && a_json["spawnRules"].is_array()) {
			for (const auto& jRule : a_json["spawnRules"]) {
				SpawnRule rule{};
				rule.name = jRule.value("name", rule.name);
				rule.enabled = jRule.value("enabled", rule.enabled);

				if (jRule.contains("match")) {
					const auto& match = jRule["match"];
					rule.match.worldspace = match.value("worldspace", rule.match.worldspace);
					rule.match.cellType = ParseCellType(match.value("cellType", std::string("any")));
					rule.match.locationKeywordAny = match.value("locationKeywordAny", rule.match.locationKeywordAny);
					rule.match.timeOfDay = ParseTimeOfDay(match.value("timeOfDay", std::string("any")));
				}

				if (jRule.contains("budget")) {
					const auto& budget = jRule["budget"];
					rule.budget.chance = budget.value("chance", rule.budget.chance);
					rule.budget.minToSpawn = budget.value("minToSpawn", rule.budget.minToSpawn);
					rule.budget.maxToSpawn = budget.value("maxToSpawn", rule.budget.maxToSpawn);
				}

				if (jRule.contains("what")) {
					const auto& what = jRule["what"];
					rule.what.method = ParseSpawnMethod(what.value("method", std::string("leveledList")));
					rule.what.form = what.value("form", rule.what.form);
				}

				if (jRule.contains("factions")) {
					const auto& factions = jRule["factions"];
					rule.factions.inheritFromEncounterZone = factions.value("inheritFromEncounterZone", rule.factions.inheritFromEncounterZone);
				}

				out.spawnRules.push_back(std::move(rule));
			}
		}

		if (out.spawnRules.empty()) {
			out.spawnRules.emplace_back();
		}

		if (a_json.contains("genesis")) {
			const auto& genesis = a_json["genesis"];
			out.genesis.skipIfHostilesNearby = genesis.value("skipIfHostilesNearby", out.genesis.skipIfHostilesNearby);
			out.genesis.hostileScanRadius = genesis.value("hostileScanRadius", out.genesis.hostileScanRadius);
			out.genesis.hostileScanMaxRefs = genesis.value("hostileScanMaxRefs", out.genesis.hostileScanMaxRefs);
			out.genesis.unlevelNPCs = genesis.value("unlevelNPCs", out.genesis.unlevelNPCs);
			out.genesis.healthPctMin = genesis.value("healthPctMin", out.genesis.healthPctMin);
			out.genesis.healthPctMax = genesis.value("healthPctMax", out.genesis.healthPctMax);
			out.genesis.magickaPctMin = genesis.value("magickaPctMin", out.genesis.magickaPctMin);
			out.genesis.magickaPctMax = genesis.value("magickaPctMax", out.genesis.magickaPctMax);
			out.genesis.staminaPctMin = genesis.value("staminaPctMin", out.genesis.staminaPctMin);
			out.genesis.staminaPctMax = genesis.value("staminaPctMax", out.genesis.staminaPctMax);
			out.genesis.giveSpawnPotions = genesis.value("giveSpawnPotions", out.genesis.giveSpawnPotions);
			out.genesis.potionChancePct = genesis.value("potionChancePct", out.genesis.potionChancePct);
			out.genesis.potionCountMin = genesis.value("potionCountMin", out.genesis.potionCountMin);
			out.genesis.potionCountMax = genesis.value("potionCountMax", out.genesis.potionCountMax);
			out.genesis.spawnPotionPool = genesis.value("spawnPotionPool", out.genesis.spawnPotionPool);
			out.genesis.lootInjectionEnabled = genesis.value("lootInjectionEnabled", out.genesis.lootInjectionEnabled);
			out.genesis.lootChancePct = genesis.value("lootChancePct", out.genesis.lootChancePct);
			out.genesis.lootPotionPool = genesis.value("lootPotionPool", out.genesis.lootPotionPool);
			out.genesis.lootScrollPool = genesis.value("lootScrollPool", out.genesis.lootScrollPool);
			out.genesis.lootMiscPool = genesis.value("lootMiscPool", out.genesis.lootMiscPool);
			out.genesis.lootSackPool = genesis.value("lootSackPool", out.genesis.lootSackPool);
		}

		return out;
	}

	void Settings::Clamp(SettingsData& a_settings)
	{
		a_settings.limits.globalMaxAlive = std::max(0, a_settings.limits.globalMaxAlive);
		a_settings.limits.maxPerCell = std::max(0, a_settings.limits.maxPerCell);
		a_settings.limits.maxPerEvent = std::max(0, a_settings.limits.maxPerEvent);
		a_settings.limits.maxManagedRefs = std::max(1, a_settings.limits.maxManagedRefs);
		a_settings.limits.minDistanceFromPlayer = std::max(0.0F, a_settings.limits.minDistanceFromPlayer);
		a_settings.limits.minDistanceFromOtherSpawn = std::max(0.0F, a_settings.limits.minDistanceFromOtherSpawn);
		a_settings.limits.despawnDistance = std::max(1000.0F, a_settings.limits.despawnDistance);

		a_settings.timing.cooldownSecondsSameCell = std::max(0.0F, a_settings.timing.cooldownSecondsSameCell);
		a_settings.timing.cooldownSecondsGlobal = std::max(0.0F, a_settings.timing.cooldownSecondsGlobal);
		a_settings.timing.deferSpawnSeconds = std::max(0.0F, a_settings.timing.deferSpawnSeconds);

		a_settings.genesis.hostileScanRadius = std::max(500.0F, a_settings.genesis.hostileScanRadius);
		a_settings.genesis.hostileScanMaxRefs = std::max(10, a_settings.genesis.hostileScanMaxRefs);
		a_settings.genesis.healthPctMin = std::clamp(a_settings.genesis.healthPctMin, 1.0F, 1000.0F);
		a_settings.genesis.healthPctMax = std::clamp(a_settings.genesis.healthPctMax, a_settings.genesis.healthPctMin, 1000.0F);
		a_settings.genesis.magickaPctMin = std::clamp(a_settings.genesis.magickaPctMin, 1.0F, 1000.0F);
		a_settings.genesis.magickaPctMax = std::clamp(a_settings.genesis.magickaPctMax, a_settings.genesis.magickaPctMin, 1000.0F);
		a_settings.genesis.staminaPctMin = std::clamp(a_settings.genesis.staminaPctMin, 1.0F, 1000.0F);
		a_settings.genesis.staminaPctMax = std::clamp(a_settings.genesis.staminaPctMax, a_settings.genesis.staminaPctMin, 1000.0F);
		a_settings.genesis.potionChancePct = std::clamp(a_settings.genesis.potionChancePct, 0, 100);
		a_settings.genesis.potionCountMin = std::max(1, a_settings.genesis.potionCountMin);
		a_settings.genesis.potionCountMax = std::max(a_settings.genesis.potionCountMin, a_settings.genesis.potionCountMax);
		a_settings.genesis.lootChancePct = std::clamp(a_settings.genesis.lootChancePct, 0, 100);

		for (auto& rule : a_settings.spawnRules) {
			rule.budget.chance = std::clamp(rule.budget.chance, 0.0, 1.0);
			rule.budget.minToSpawn = std::max(0, rule.budget.minToSpawn);
			rule.budget.maxToSpawn = std::max(rule.budget.minToSpawn, rule.budget.maxToSpawn);
		}
	}

	void Settings::FillLowerCaches(SettingsData& a_settings)
	{
		a_settings.filters.skipWorldspacesLower.clear();
		a_settings.filters.skipCellsByNameContainsLower.clear();

		for (const auto& value : a_settings.filters.skipWorldspaces) {
			a_settings.filters.skipWorldspacesLower.push_back(ToLower(value));
		}
		for (const auto& value : a_settings.filters.skipCellsByNameContains) {
			a_settings.filters.skipCellsByNameContainsLower.push_back(ToLower(value));
		}
	}

	void Settings::MergePreset(SettingsData& a_target, const json& a_json)
	{
		if (a_json.contains("spawnRules") && a_json["spawnRules"].is_array()) {
			const auto parsed = FromJson({ { "spawnRules", a_json["spawnRules"] } });
			a_target.spawnRules.insert(a_target.spawnRules.end(), parsed.spawnRules.begin(), parsed.spawnRules.end());
		}
	}

	void Settings::SaveDefaults(const std::filesystem::path& a_path) const
	{
		std::ofstream file(a_path, std::ios::trunc);
		file << ToJson(MakeDefault()).dump(2);
	}

	void Settings::LoadPresets(SettingsData& a_settings) const
	{
		std::error_code ec;
		const auto presetPath = GetPresetDirectory();
		if (!std::filesystem::exists(presetPath, ec)) {
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(presetPath, ec)) {
			if (ec || !entry.is_regular_file()) {
				continue;
			}
			if (entry.path().extension() != ".json") {
				continue;
			}

			try {
				std::ifstream file(entry.path());
				json j;
				file >> j;
				MergePreset(a_settings, j);
				LOG_INFO("Loaded preset: {}", entry.path().filename().string());
			} catch (const std::exception& e) {
				LOG_WARN("Failed to load preset {}: {}", entry.path().filename().string(), e.what());
			}
		}
	}

	void Settings::ApplyLogLevel() const
	{
		auto logger = spdlog::get(UTIL::LOGGER_NAME);
		if (!logger) {
			return;
		}

		const auto level = ToLower(m_settings.debug.logLevel);
		if (level == "trace") {
			logger->set_level(spdlog::level::trace);
		} else if (level == "debug") {
			logger->set_level(spdlog::level::debug);
		} else if (level == "warn") {
			logger->set_level(spdlog::level::warn);
		} else if (level == "error") {
			logger->set_level(spdlog::level::err);
		} else if (level == "critical") {
			logger->set_level(spdlog::level::critical);
		} else {
			logger->set_level(spdlog::level::info);
		}
	}
}
