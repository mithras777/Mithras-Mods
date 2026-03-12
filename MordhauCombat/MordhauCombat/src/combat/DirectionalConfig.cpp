#include "combat/DirectionalConfig.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace MC::DIRECTIONAL
{
	namespace
	{
		std::filesystem::path GetConfigPath()
		{
			return std::filesystem::path("Data/SKSE/Plugins/MordhauCombat.ini");
		}

		std::string Trim(std::string a_value)
		{
			a_value.erase(a_value.begin(), std::find_if(a_value.begin(), a_value.end(), [](unsigned char c) {
				return !std::isspace(c);
			}));
			a_value.erase(std::find_if(a_value.rbegin(), a_value.rend(), [](unsigned char c) {
				return !std::isspace(c);
			}).base(), a_value.end());
			return a_value;
		}

		bool ParseBool(const std::string& a_value)
		{
			return a_value == "1" || a_value == "true" || a_value == "TRUE";
		}
	}

	Config* Config::GetSingleton()
	{
		static Config singleton;
		return std::addressof(singleton);
	}

	const ConfigData& Config::GetData() const
	{
		return m_data;
	}

	ConfigData& Config::GetMutableData()
	{
		return m_data;
	}

	bool Config::Load()
	{
		const auto configPath = GetConfigPath();
		if (!std::filesystem::exists(configPath)) {
			Save();
			return true;
		}

		std::ifstream file(configPath);
		if (!file.is_open()) {
			LOG_WARN("MordhauCombat: failed to open config '{}'; using defaults", configPath.string());
			return false;
		}

		std::string line;
		while (std::getline(file, line)) {
			line = Trim(line);
			if (line.empty() || line.starts_with('#') || line.starts_with(';') || line.starts_with('[')) {
				continue;
			}

			const auto equals = line.find('=');
			if (equals == std::string::npos) {
				continue;
			}

			auto key = Trim(line.substr(0, equals));
			auto value = Trim(line.substr(equals + 1));

			try {
				if (key == "enabled") m_data.enabled = ParseBool(value);
				else if (key == "debugMode") m_data.debugMode = ParseBool(value);
				else if (key == "mouseSensitivity") m_data.mouseSensitivity = std::stof(value);
				else if (key == "deadzone") m_data.deadzone = std::stof(value);
				else if (key == "smoothing") m_data.smoothing = std::stof(value);
				else if (key == "dragMaxAccel") m_data.dragMaxAccel = std::stof(value);
				else if (key == "dragMaxSlow") m_data.dragMaxSlow = std::stof(value);
				else if (key == "dragSmoothing") m_data.dragSmoothing = std::stof(value);
				else if (key == "weaponSpeedMultScale") m_data.weaponSpeedMultScale = std::stof(value);
				else if (key == "overlayScale") m_data.overlayScale = std::stof(value);
				else if (key == "toggleKey") m_data.toggleKey = static_cast<unsigned int>(std::stoul(value));
			} catch (...) {
				LOG_WARN("MordhauCombat: invalid config value '{}={}'", key, value);
			}
		}

		m_data.deadzone = std::clamp(m_data.deadzone, 0.0f, 0.95f);
		m_data.smoothing = std::clamp(m_data.smoothing, 0.0f, 0.95f);
		m_data.dragSmoothing = std::clamp(m_data.dragSmoothing, 0.01f, 0.95f);
		m_data.mouseSensitivity = std::clamp(m_data.mouseSensitivity, 0.05f, 10.0f);
		m_data.overlayScale = std::clamp(m_data.overlayScale, 0.2f, 5.0f);
		m_data.weaponSpeedMultScale = std::clamp(m_data.weaponSpeedMultScale, 0.0f, 3.0f);
		m_data.dragMaxSlow = std::min(m_data.dragMaxSlow, 0.0f);
		m_data.dragMaxAccel = std::max(m_data.dragMaxAccel, 0.0f);

		return true;
	}

	bool Config::Save() const
	{
		const auto configPath = GetConfigPath();
		std::filesystem::create_directories(configPath.parent_path());

		std::ofstream file(configPath, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("MordhauCombat: failed to write config '{}'", configPath.string());
			return false;
		}

		file << "[General]\n";
		file << "enabled=" << (m_data.enabled ? 1 : 0) << "\n";
		file << "debugMode=" << (m_data.debugMode ? 1 : 0) << "\n";
		file << "toggleKey=" << m_data.toggleKey << "\n";
		file << "mouseSensitivity=" << m_data.mouseSensitivity << "\n";
		file << "deadzone=" << m_data.deadzone << "\n";
		file << "smoothing=" << m_data.smoothing << "\n";
		file << "overlayScale=" << m_data.overlayScale << "\n\n";

		file << "[Drag]\n";
		file << "dragMaxAccel=" << m_data.dragMaxAccel << "\n";
		file << "dragMaxSlow=" << m_data.dragMaxSlow << "\n";
		file << "dragSmoothing=" << m_data.dragSmoothing << "\n";
		file << "weaponSpeedMultScale=" << m_data.weaponSpeedMultScale << "\n";
		return true;
	}
}
