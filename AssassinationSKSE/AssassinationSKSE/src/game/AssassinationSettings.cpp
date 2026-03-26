#include "game/AssassinationSettings.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace GAME::ASSASSINATION
{
	namespace
	{
		constexpr auto kConfigFileName = "AssassinationSKSE.json";

		std::filesystem::path GetConfigPath()
		{
			const auto& gameDir = DLLMAIN::Plugin::GetSingleton()->Info().gameDirectory;
			return std::filesystem::path(gameDir) / "Data" / "SKSE" / "Plugins" / kConfigFileName;
		}

		template <class T>
		T ReadOr(const nlohmann::json& a_json, const char* a_key, const T& a_fallback)
		{
			if (!a_json.is_object() || !a_json.contains(a_key)) {
				return a_fallback;
			}

			try {
				return a_json.at(a_key).get<T>();
			} catch (...) {
				return a_fallback;
			}
		}
	}

	void Settings::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_settings = Defaults();
		}

		LoadFromDisk();
	}

	Config Settings::Get() const
	{
		std::scoped_lock lock(m_lock);
		return m_settings;
	}

	Config Settings::GetDefaults() const
	{
		return Defaults();
	}

	void Settings::Set(const Config& a_settings, bool a_writeJson)
	{
		Config incoming = a_settings;
		Clamp(incoming);
		{
			std::scoped_lock lock(m_lock);
			m_settings = incoming;
		}

		if (a_writeJson) {
			Save();
		}
	}

	void Settings::Save()
	{
		Config snapshot{};
		{
			std::scoped_lock lock(m_lock);
			snapshot = m_settings;
		}

		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		nlohmann::json root;
		root["version"] = snapshot.version;
		root["enabled"] = snapshot.enabled;
		root["maxLevelDifference"] = snapshot.maxLevelDifference;
		root["assassinationChance"] = snapshot.assassinationChance;
		root["melee"] = snapshot.melee;
		root["unarmed"] = snapshot.unarmed;
		root["bowsCrossbows"] = snapshot.bowsCrossbows;
		root["excludeBosses"] = snapshot.excludeBosses;
		root["excludeDragons"] = snapshot.excludeDragons;
		root["excludeGiants"] = snapshot.excludeGiants;

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_ERROR("[Assassination] Failed to write config: {}", path.string());
			return;
		}

		file << root.dump(2);
	}

	void Settings::ResetToDefaults(bool a_writeJson)
	{
		{
			std::scoped_lock lock(m_lock);
			m_settings = Defaults();
		}

		if (a_writeJson) {
			Save();
		}
	}

	std::string Settings::GetConfigPathString() const
	{
		return GetConfigPath().string();
	}

	void Settings::LoadFromDisk()
	{
		const auto path = GetConfigPath();
		if (!std::filesystem::exists(path)) {
			Save();
			return;
		}

		std::ifstream file(path);
		if (!file.is_open()) {
			LOG_ERROR("[Assassination] Failed to open config: {}", path.string());
			Save();
			return;
		}

		std::ostringstream buffer;
		buffer << file.rdbuf();

		nlohmann::json root;
		try {
			root = nlohmann::json::parse(buffer.str());
		} catch (...) {
			LOG_ERROR("[Assassination] Invalid JSON, rewriting defaults: {}", path.string());
			Save();
			return;
		}

		const Config defaults = Defaults();
		Config loaded = defaults;

		const auto loadedVersion = ReadOr<int>(root, "version", defaults.version);
		if (loadedVersion != defaults.version) {
			LOG_INFO("[Assassination] Config version mismatch (found {}, expected {}), regenerating defaults", loadedVersion, defaults.version);
			{
				std::scoped_lock lock(m_lock);
				m_settings = defaults;
			}
			Save();
			return;
		}

		loaded.enabled = ReadOr<bool>(root, "enabled", loaded.enabled);
		loaded.maxLevelDifference = ReadOr<int>(root, "maxLevelDifference", loaded.maxLevelDifference);
		loaded.assassinationChance = ReadOr<int>(root, "assassinationChance", loaded.assassinationChance);
		loaded.melee = ReadOr<bool>(root, "melee", loaded.melee);
		loaded.unarmed = ReadOr<bool>(root, "unarmed", loaded.unarmed);
		loaded.bowsCrossbows = ReadOr<bool>(root, "bowsCrossbows", loaded.bowsCrossbows);
		loaded.excludeBosses = ReadOr<bool>(root, "excludeBosses", loaded.excludeBosses);
		loaded.excludeDragons = ReadOr<bool>(root, "excludeDragons", loaded.excludeDragons);
		loaded.excludeGiants = ReadOr<bool>(root, "excludeGiants", loaded.excludeGiants);

		Clamp(loaded);
		{
			std::scoped_lock lock(m_lock);
			m_settings = loaded;
		}
	}

	Config Settings::Defaults()
	{
		return Config{};
	}

	void Settings::Clamp(Config& a_settings)
	{
		a_settings.version = 1;
		a_settings.maxLevelDifference = std::clamp(a_settings.maxLevelDifference, 0, 100);
		a_settings.assassinationChance = std::clamp(a_settings.assassinationChance, 0, 100);
	}

	Config GetConfig()
	{
		return Settings::GetSingleton()->Get();
	}

	void SetConfig(const Config& a_config)
	{
		Settings::GetSingleton()->Set(a_config, true);
	}
}
