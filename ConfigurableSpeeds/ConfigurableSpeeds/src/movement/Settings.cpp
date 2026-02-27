#include "movement/Settings.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace MOVEMENT
{
	namespace
	{
		constexpr auto kConfigFileName = "ConfigMovementSpeed.json";

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

	SettingsData Settings::Get() const
	{
		std::scoped_lock lock(m_lock);
		return m_settings;
	}

	void Settings::Set(const SettingsData& a_settings, bool a_writeJson)
	{
		SettingsData incoming = a_settings;
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
		SettingsData snapshot{};
		{
			std::scoped_lock lock(m_lock);
			snapshot = m_settings;
		}

		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		nlohmann::json root;
		root["version"] = snapshot.version;
		root["general"] = {
			{ "enabled", snapshot.general.enabled },
			{ "restoreOnDisable", snapshot.general.restoreOnDisable },
			{ "dumpOnStartup", snapshot.general.dumpOnStartup },
			{ "policy", {
				{ "useMultipliers", snapshot.general.policy.useMultipliers },
				{ "useOverrides", snapshot.general.policy.useOverrides }
			} }
		};
		root["multipliers"] = {
			{ "walk", snapshot.multipliers.walk },
			{ "run", snapshot.multipliers.run },
			{ "sprint", snapshot.multipliers.sprint },
			{ "sneakWalk", snapshot.multipliers.sneakWalk },
			{ "sneakRun", snapshot.multipliers.sneakRun }
		};
		root["overrides"] = {
			{ "walk", {
				{ "enabled", snapshot.overrides.walk.enabled },
				{ "value", snapshot.overrides.walk.value }
			} },
			{ "run", {
				{ "enabled", snapshot.overrides.run.enabled },
				{ "value", snapshot.overrides.run.value }
			} },
			{ "sprint", {
				{ "enabled", snapshot.overrides.sprint.enabled },
				{ "value", snapshot.overrides.sprint.value }
			} },
			{ "sneakWalk", {
				{ "enabled", snapshot.overrides.sneakWalk.enabled },
				{ "value", snapshot.overrides.sneakWalk.value }
			} },
			{ "sneakRun", {
				{ "enabled", snapshot.overrides.sneakRun.enabled },
				{ "value", snapshot.overrides.sneakRun.value }
			} }
		};

		nlohmann::json targets = nlohmann::json::array();
		for (const auto& target : snapshot.targets) {
			targets.push_back({
				{ "name", target.name },
				{ "form", target.form },
				{ "enabled", target.enabled }
			});
		}
		root["targets"] = std::move(targets);

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_ERROR("[Movement] Failed to write config: {}", path.string());
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
			LOG_ERROR("[Movement] Failed to open config: {}", path.string());
			Save();
			return;
		}

		std::ostringstream buffer;
		buffer << file.rdbuf();

		nlohmann::json root;
		try {
			root = nlohmann::json::parse(buffer.str());
		} catch (...) {
			LOG_ERROR("[Movement] Invalid JSON, rewriting defaults: {}", path.string());
			Save();
			return;
		}

		SettingsData loaded = Defaults();
		loaded.version = ReadOr<int>(root, "version", 1);

		if (root.contains("general") && root["general"].is_object()) {
			const auto& general = root["general"];
			loaded.general.enabled = ReadOr<bool>(general, "enabled", loaded.general.enabled);
			loaded.general.restoreOnDisable = ReadOr<bool>(general, "restoreOnDisable", loaded.general.restoreOnDisable);
			loaded.general.dumpOnStartup = ReadOr<bool>(general, "dumpOnStartup", loaded.general.dumpOnStartup);
			if (general.contains("policy") && general["policy"].is_object()) {
				const auto& policy = general["policy"];
				loaded.general.policy.useMultipliers = ReadOr<bool>(policy, "useMultipliers", loaded.general.policy.useMultipliers);
				loaded.general.policy.useOverrides = ReadOr<bool>(policy, "useOverrides", loaded.general.policy.useOverrides);
			}
		}

		if (root.contains("multipliers") && root["multipliers"].is_object()) {
			const auto& multipliers = root["multipliers"];
			loaded.multipliers.walk = ReadOr<float>(multipliers, "walk", loaded.multipliers.walk);
			loaded.multipliers.run = ReadOr<float>(multipliers, "run", loaded.multipliers.run);
			loaded.multipliers.sprint = ReadOr<float>(multipliers, "sprint", loaded.multipliers.sprint);
			loaded.multipliers.sneakWalk = ReadOr<float>(multipliers, "sneakWalk", loaded.multipliers.sneakWalk);
			loaded.multipliers.sneakRun = ReadOr<float>(multipliers, "sneakRun", loaded.multipliers.sneakRun);
		}

		if (root.contains("overrides") && root["overrides"].is_object()) {
			const auto& overrides = root["overrides"];
			auto readOverrideEntry = [&overrides](const char* a_key, MOVEMENT::SpeedOverrides::Entry a_fallback) {
				if (!overrides.contains(a_key)) {
					return a_fallback;
				}

				const auto& node = overrides.at(a_key);
				if (node.is_number()) {
					try {
						const float legacy = node.get<float>();
						if (legacy >= 0.0f) {
							a_fallback.enabled = true;
							a_fallback.value = legacy;
						}
					} catch (...) {
					}
					return a_fallback;
				}

				if (node.is_object()) {
					a_fallback.enabled = ReadOr<bool>(node, "enabled", a_fallback.enabled);
					a_fallback.value = ReadOr<float>(node, "value", a_fallback.value);
				}
				return a_fallback;
			};

			loaded.overrides.walk = readOverrideEntry("walk", loaded.overrides.walk);
			loaded.overrides.run = readOverrideEntry("run", loaded.overrides.run);
			loaded.overrides.sprint = readOverrideEntry("sprint", loaded.overrides.sprint);
			loaded.overrides.sneakWalk = readOverrideEntry("sneakWalk", loaded.overrides.sneakWalk);
			loaded.overrides.sneakRun = readOverrideEntry("sneakRun", loaded.overrides.sneakRun);
		}

		if (root.contains("targets") && root["targets"].is_array()) {
			loaded.targets.clear();
			for (const auto& entry : root["targets"]) {
				if (!entry.is_object()) {
					continue;
				}
				TargetEntry target{};
				target.name = ReadOr<std::string>(entry, "name", "");
				target.form = ReadOr<std::string>(entry, "form", "");
				target.enabled = ReadOr<bool>(entry, "enabled", true);
				loaded.targets.push_back(std::move(target));
			}
		}

		Clamp(loaded);
		{
			std::scoped_lock lock(m_lock);
			m_settings = std::move(loaded);
		}
	}

	SettingsData Settings::Defaults()
	{
		SettingsData data{};
		data.version = 1;
		return data;
	}

	void Settings::Clamp(SettingsData& a_settings)
	{
		auto clampMultiplier = [](float value) { return std::clamp(value, 0.1f, 3.0f); };
		auto clampOverride = [](MOVEMENT::SpeedOverrides::Entry& value) {
			value.value = std::clamp(value.value, 0.0f, 2000.0f);
		};

		a_settings.multipliers.walk = clampMultiplier(a_settings.multipliers.walk);
		a_settings.multipliers.run = clampMultiplier(a_settings.multipliers.run);
		a_settings.multipliers.sprint = clampMultiplier(a_settings.multipliers.sprint);
		a_settings.multipliers.sneakWalk = clampMultiplier(a_settings.multipliers.sneakWalk);
		a_settings.multipliers.sneakRun = clampMultiplier(a_settings.multipliers.sneakRun);

		clampOverride(a_settings.overrides.walk);
		clampOverride(a_settings.overrides.run);
		clampOverride(a_settings.overrides.sprint);
		clampOverride(a_settings.overrides.sneakWalk);
		clampOverride(a_settings.overrides.sneakRun);
	}
}
