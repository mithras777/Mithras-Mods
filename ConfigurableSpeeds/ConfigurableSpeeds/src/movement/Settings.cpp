#include "movement/Settings.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

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

		std::string Trim(std::string a_text)
		{
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front())) != 0) {
				a_text.erase(a_text.begin());
			}
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back())) != 0) {
				a_text.pop_back();
			}
			return a_text;
		}

		std::string CanonicalFormKey(const std::string& a_form)
		{
			auto key = Trim(a_form);
			std::transform(
				key.begin(),
				key.end(),
				key.begin(),
				[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
			return key;
		}

		std::string GroupForName(std::string_view a_name)
		{
			if (a_name.rfind("Player_", 0) == 0) {
				return "Player";
			}
			if (a_name.rfind("Horse_", 0) == 0) {
				return "Horses";
			}
			if (a_name.rfind("NPC_", 0) == 0) {
				if (a_name.find("Horse") != std::string_view::npos) {
					return "Horses";
				}
				if (a_name.find("Attack") != std::string_view::npos ||
				    a_name.find("Blocking") != std::string_view::npos ||
				    a_name.find("Bow") != std::string_view::npos ||
				    a_name.find("Magic") != std::string_view::npos ||
				    a_name.find("PowerAttacking") != std::string_view::npos ||
				    a_name.find("1HM") != std::string_view::npos ||
				    a_name.find("2HM") != std::string_view::npos) {
					return "Combat";
				}
				return "Default";
			}
			return "Default";
		}

		bool IsSprintEntry(std::string_view a_name)
		{
			return a_name.find("Sprinting") != std::string_view::npos ||
			       a_name.find("Sprint") != std::string_view::npos;
		}

		using SpeedMatrix = std::array<std::array<float, 2>, 5>;

		MovementEntry MakeEntry(std::string_view a_name, std::string_view a_form)
		{
			MovementEntry entry{};
			entry.name = a_name;
			entry.form = a_form;
			entry.group = GroupForName(a_name);
			entry.playerOnly = entry.group == "Player";
			entry.enabled = false;
			return entry;
		}

		MovementEntry MakeEntry(std::string_view a_name, std::string_view a_form, float a_playerSpeedMult)
		{
			MovementEntry entry = MakeEntry(a_name, a_form);
			entry.playerSpeedMult = a_playerSpeedMult;
			return entry;
		}

		MovementEntry MakeEntry(std::string_view a_name, std::string_view a_form, const SpeedMatrix& a_speeds)
		{
			MovementEntry entry = MakeEntry(a_name, a_form);
			for (std::size_t i = 0; i < 5; ++i) {
				entry.speeds[i][0] = a_speeds[i][0];
				entry.speeds[i][1] = a_speeds[i][1];
			}
			return entry;
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

	SettingsData Settings::GetDefaults() const
	{
		return Defaults();
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
			{ "restoreOnDisable", snapshot.general.restoreOnDisable }
		};

		nlohmann::json entries = nlohmann::json::array();
		for (const auto& entry : snapshot.entries) {
			nlohmann::json node = {
				{ "name", entry.name },
				{ "form", entry.form },
				{ "group", entry.group },
				{ "playerOnly", entry.playerOnly },
				{ "enabled", entry.enabled }
			};
			if (entry.playerOnly) {
				node["playerSpeedMult"] = entry.playerSpeedMult;
			} else {
				nlohmann::json speeds = nlohmann::json::array();
				for (std::size_t i = 0; i < 5; ++i) {
					speeds.push_back({ entry.speeds[i][0], entry.speeds[i][1] });
				}
				node["speeds"] = std::move(speeds);
			}
			entries.push_back(std::move(node));
		}
		root["entries"] = std::move(entries);

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

		const SettingsData defaults = Defaults();
		SettingsData loaded = defaults;

		const auto loadedVersion = ReadOr<int>(root, "version", -1);
		if (loadedVersion != defaults.version) {
			LOG_INFO("[Movement] Config version mismatch (found {}, expected {}), regenerating defaults", loadedVersion, defaults.version);
			{
				std::scoped_lock lock(m_lock);
				m_settings = defaults;
			}
			Save();
			return;
		}

		if (!root.contains("general") || !root["general"].is_object() ||
		    !root.contains("entries") || !root["entries"].is_array() ||
		    root["entries"].size() != defaults.entries.size()) {
			LOG_INFO("[Movement] Config schema mismatch, regenerating defaults");
			{
				std::scoped_lock lock(m_lock);
				m_settings = defaults;
			}
			Save();
			return;
		}

		const auto& entries = root["entries"];
		for (std::size_t idx = 0; idx < defaults.entries.size(); ++idx) {
			const auto& node = entries[idx];
			if (!node.is_object()) {
				LOG_INFO("[Movement] Invalid entry format at index {}, regenerating defaults", idx);
				{
					std::scoped_lock lock(m_lock);
					m_settings = defaults;
				}
				Save();
				return;
			}

			auto entry = defaults.entries[idx];
			const auto form = ReadOr<std::string>(node, "form", "");
			if (CanonicalFormKey(form) != CanonicalFormKey(entry.form)) {
				LOG_INFO("[Movement] Entry form mismatch at index {}, regenerating defaults", idx);
				{
					std::scoped_lock lock(m_lock);
					m_settings = defaults;
				}
				Save();
				return;
			}

			entry.enabled = ReadOr<bool>(node, "enabled", entry.enabled);
			entry.playerOnly = ReadOr<bool>(node, "playerOnly", entry.playerOnly);

			if (entry.playerOnly) {
				if (!node.contains("playerSpeedMult") || !node["playerSpeedMult"].is_number()) {
					LOG_INFO("[Movement] Missing playerSpeedMult at index {}, regenerating defaults", idx);
					{
						std::scoped_lock lock(m_lock);
						m_settings = defaults;
					}
					Save();
					return;
				}
				entry.playerSpeedMult = ReadOr<float>(node, "playerSpeedMult", entry.playerSpeedMult);
			} else {
				if (!node.contains("speeds") || !node["speeds"].is_array()) {
					LOG_INFO("[Movement] Missing speeds array at index {}, regenerating defaults", idx);
					{
						std::scoped_lock lock(m_lock);
						m_settings = defaults;
					}
					Save();
					return;
				}

				const auto& speeds = node["speeds"];
				for (std::size_t i = 0; i < 5 && i < speeds.size(); ++i) {
					const auto& pair = speeds[i];
					if (pair.is_array() && pair.size() >= 2) {
						if (pair[0].is_number()) {
							entry.speeds[i][0] = pair[0].get<float>();
						}
						if (pair[1].is_number()) {
							entry.speeds[i][1] = pair[1].get<float>();
						}
					}
				}
			}

			loaded.entries[idx] = std::move(entry);
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
		data.version = 13;
		data.entries = {
			MakeEntry("Horse_Default_MT", "Skyrim.esm|0x0001CF24", {{{ 0.000f, 0.000f }, { 0.000f, 0.000f }, { 125.110f, 450.000f }, { 108.080f, 108.080f }, { 1.571f, 4.712f }}}),
			MakeEntry("Horse_Sprint_MT", "Skyrim.esm|0x0004408D", {{{ 0.000f, 0.000f }, { 0.000f, 0.000f }, { 600.000f, 600.000f }, { 0.000f, 0.000f }, { 1.571f, 1.571f }}}),
			MakeEntry("NPC_Blocking_MT", "Skyrim.esm|0x00035B4C", {{{ 81.000f, 81.000f }, { 81.000f, 81.000f }, { 81.000f, 81.000f }, { 71.000f, 71.000f }, { 3.142f, 3.142f }}}),
			MakeEntry("NPC_BowDrawn_MT", "Skyrim.esm|0x0003580C", {{{ 76.810f, 115.000f }, { 74.890f, 115.000f }, { 120.000f, 135.000f }, { 65.110f, 98.000f }, { 1.571f, 1.571f }}}),
			MakeEntry("NPC_Bow_MT", "Skyrim.esm|0x00069CDA", {{{ 80.090f, 370.000f }, { 79.750f, 370.000f }, { 80.100f, 370.000f }, { 71.930f, 205.250f }, { 3.142f, 3.142f }}}),
			MakeEntry("NPC_Default_MT", "Skyrim.esm|0x0003580D", {{{ 80.090f, 370.000f }, { 79.750f, 370.000f }, { 80.100f, 370.000f }, { 71.930f, 205.250f }, { 3.142f, 3.142f }}}),
			MakeEntry("NPC_MagicCasting_MT", "Skyrim.esm|0x00069CDC", {{{ 80.090f, 370.000f }, { 79.750f, 370.000f }, { 80.100f, 370.000f }, { 71.930f, 170.840f }, { 3.142f, 3.142f }}}),
			MakeEntry("NPC_Magic_MT", "Skyrim.esm|0x00069CDB", {{{ 80.090f, 370.000f }, { 79.750f, 370.000f }, { 80.100f, 370.000f }, { 71.930f, 170.840f }, { 3.142f, 3.142f }}}),
			MakeEntry("NPC_Sneaking_MT", "Skyrim.esm|0x0003580B", {{{ 41.440f, 200.000f }, { 41.440f, 200.000f }, { 47.200f, 222.000f }, { 43.380f, 150.000f }, { 1.571f, 3.142f }}}),
			MakeEntry("NPC_Sprinting_MT", "Skyrim.esm|0x00034D9C", {{{ 0.000f, 0.000f }, { 0.000f, 0.000f }, { 500.000f, 500.000f }, { 270.840f, 270.840f }, { 1.571f, 1.571f }}}),
			MakeEntry("Player_Default_MT", "Player|Default", 100.000f),
			MakeEntry("Player_Blocking_MT", "Player|Blocking", 100.000f),
			MakeEntry("Player_BowDrawn_MT", "Player|BowDrawn", 100.000f),
			MakeEntry("Player_MagicCasting_MT", "Player|MagicCasting", 100.000f),
			MakeEntry("Player_Sneaking_MT", "Player|Sneaking", 100.000f)
		};
		return data;
	}

	void Settings::Clamp(SettingsData& a_settings)
	{
		for (auto& entry : a_settings.entries) {
			for (std::size_t i = 0; i < 5; ++i) {
				entry.speeds[i][0] = std::clamp(entry.speeds[i][0], 0.0f, 2000.0f);
				entry.speeds[i][1] = std::clamp(entry.speeds[i][1], 0.0f, 2000.0f);
			}

			if (entry.playerOnly) {
				entry.playerSpeedMult = std::clamp(entry.playerSpeedMult, 1.0f, 2000.0f);
			}

			if (entry.group != "Horses" && entry.group != "Player" && !IsSprintEntry(entry.name)) {
				entry.speeds[2][1] = std::max({ entry.speeds[2][1], entry.speeds[0][1], entry.speeds[1][1] });
			}
		}
	}
}
