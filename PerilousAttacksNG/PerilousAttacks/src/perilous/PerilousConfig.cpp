#include "perilous/PerilousConfig.h"

#include "util/LogUtil.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <mutex>

namespace
{
	std::string Trim(std::string a_value)
	{
		auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		a_value.erase(a_value.begin(), std::find_if(a_value.begin(), a_value.end(), notSpace));
		a_value.erase(std::find_if(a_value.rbegin(), a_value.rend(), notSpace).base(), a_value.end());
		return a_value;
	}

	bool ParseBool(const std::string& a_value, bool& a_out)
	{
		std::string lowered = a_value;
		std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
			a_out = true;
			return true;
		}
		if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
			a_out = false;
			return true;
		}
		return false;
	}
}

namespace PERILOUS
{
	Config ConfigStore::Get() const
	{
		std::shared_lock lock(_lock);
		return _config;
	}

	void ConfigStore::Set(const Config& a_config)
	{
		Config next = a_config;
		Sanitize(next);

		std::unique_lock lock(_lock);
		_config = next;
	}

	void ConfigStore::Sanitize(Config& a_config) const
	{
		a_config.fPerilousAttack_ChargeTime_Mult = std::clamp(a_config.fPerilousAttack_ChargeTime_Mult, 0.0F, 5.0F);
		a_config.fPerilousAttack_ChargeTime_Duration = std::clamp(a_config.fPerilousAttack_ChargeTime_Duration, 0.0F, 10.0F);
		a_config.fPerilousAttack_Chance_Mult = std::clamp(a_config.fPerilousAttack_Chance_Mult, 0.0F, 10.0F);
		a_config.iPerilous_MaxAttackersPerTarget = std::clamp(a_config.iPerilous_MaxAttackersPerTarget, 1U, 10U);

		a_config.sPerilous_FormsPlugin = Trim(a_config.sPerilous_FormsPlugin);
		if (a_config.sPerilous_FormsPlugin.empty()) {
			a_config.sPerilous_FormsPlugin = "PerilousAttacks.esp";
		}
	}

	void ConfigStore::Load()
	{
		Config loaded{};

		std::ifstream input(SETTINGS_PATH);
		if (!input.is_open()) {
			LOG_WARN("Perilous settings file not found at {}. Using defaults.", SETTINGS_PATH);
			Set(loaded);
			Save();
			return;
		}

		bool inPerilousSection = false;
		for (std::string line; std::getline(input, line);) {
			line = Trim(line);
			if (line.empty() || line.starts_with(';') || line.starts_with('#')) {
				continue;
			}

			if (line.front() == '[' && line.back() == ']') {
				const auto section = Trim(line.substr(1, line.size() - 2));
				inPerilousSection = _stricmp(section.c_str(), "Perilous") == 0;
				continue;
			}

			if (!inPerilousSection) {
				continue;
			}

			const auto eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}

			auto key = Trim(line.substr(0, eq));
			auto value = Trim(line.substr(eq + 1));

			bool boolValue = false;
			if (_stricmp(key.c_str(), "bPerilousAttack_Enable") == 0 && ParseBool(value, boolValue)) {
				loaded.bPerilousAttack_Enable = boolValue;
			} else if (_stricmp(key.c_str(), "bPerilousAttack_ChargeTime_Enable") == 0 && ParseBool(value, boolValue)) {
				loaded.bPerilousAttack_ChargeTime_Enable = boolValue;
			} else if (_stricmp(key.c_str(), "bPerilous_VFX_Enable") == 0 && ParseBool(value, boolValue)) {
				loaded.bPerilous_VFX_Enable = boolValue;
			} else if (_stricmp(key.c_str(), "bPerilous_SFX_Enable") == 0 && ParseBool(value, boolValue)) {
				loaded.bPerilous_SFX_Enable = boolValue;
			} else if (_stricmp(key.c_str(), "fPerilousAttack_ChargeTime_Mult") == 0) {
				loaded.fPerilousAttack_ChargeTime_Mult = std::strtof(value.c_str(), nullptr);
			} else if (_stricmp(key.c_str(), "fPerilousAttack_ChargeTime_Duration") == 0) {
				loaded.fPerilousAttack_ChargeTime_Duration = std::strtof(value.c_str(), nullptr);
			} else if (_stricmp(key.c_str(), "fPerilousAttack_Chance_Mult") == 0) {
				loaded.fPerilousAttack_Chance_Mult = std::strtof(value.c_str(), nullptr);
			} else if (_stricmp(key.c_str(), "iPerilous_MaxAttackersPerTarget") == 0) {
				loaded.iPerilous_MaxAttackersPerTarget = static_cast<std::uint32_t>(std::strtoul(value.c_str(), nullptr, 10));
			} else if (_stricmp(key.c_str(), "sPerilous_FormsPlugin") == 0) {
				loaded.sPerilous_FormsPlugin = value;
			} else if (_stricmp(key.c_str(), "iPerilous_RedVfxFormID") == 0) {
				loaded.iPerilous_RedVfxFormID = static_cast<RE::FormID>(std::strtoul(value.c_str(), nullptr, 0));
			} else if (_stricmp(key.c_str(), "iPerilous_SfxFormID") == 0) {
				loaded.iPerilous_SfxFormID = static_cast<RE::FormID>(std::strtoul(value.c_str(), nullptr, 0));
			}
		}

		Set(loaded);
		Save();

		auto cfg = Get();
		LOG_INFO(
			"Perilous config loaded: enabled={} charge={} mult={} duration={} chanceMult={} maxAttackers={} vfx={} sfx={} plugin={} redVfx={:#x} sfxForm={:#x}",
			cfg.bPerilousAttack_Enable,
			cfg.bPerilousAttack_ChargeTime_Enable,
			cfg.fPerilousAttack_ChargeTime_Mult,
			cfg.fPerilousAttack_ChargeTime_Duration,
			cfg.fPerilousAttack_Chance_Mult,
			cfg.iPerilous_MaxAttackersPerTarget,
			cfg.bPerilous_VFX_Enable,
			cfg.bPerilous_SFX_Enable,
			cfg.sPerilous_FormsPlugin,
			cfg.iPerilous_RedVfxFormID,
			cfg.iPerilous_SfxFormID);
	}

	void ConfigStore::Save() const
	{
		const auto cfg = Get();
		std::filesystem::create_directories(std::filesystem::path(SETTINGS_PATH).parent_path());

		std::ofstream output(SETTINGS_PATH, std::ios::trunc);
		if (!output.is_open()) {
			LOG_ERROR("Failed to write settings file at {}", SETTINGS_PATH);
			return;
		}

		output << "[Perilous]\n";
		output << "bPerilousAttack_Enable=" << (cfg.bPerilousAttack_Enable ? "true" : "false") << "\n";
		output << "bPerilousAttack_ChargeTime_Enable=" << (cfg.bPerilousAttack_ChargeTime_Enable ? "true" : "false") << "\n";
		output << "fPerilousAttack_ChargeTime_Mult=" << cfg.fPerilousAttack_ChargeTime_Mult << "\n";
		output << "fPerilousAttack_ChargeTime_Duration=" << cfg.fPerilousAttack_ChargeTime_Duration << "\n";
		output << "fPerilousAttack_Chance_Mult=" << cfg.fPerilousAttack_Chance_Mult << "\n";
		output << "iPerilous_MaxAttackersPerTarget=" << cfg.iPerilous_MaxAttackersPerTarget << "\n";
		output << "bPerilous_VFX_Enable=" << (cfg.bPerilous_VFX_Enable ? "true" : "false") << "\n";
		output << "bPerilous_SFX_Enable=" << (cfg.bPerilous_SFX_Enable ? "true" : "false") << "\n";
		output << "sPerilous_FormsPlugin=" << cfg.sPerilous_FormsPlugin << "\n";
		output << "iPerilous_RedVfxFormID=0x" << std::uppercase << std::hex << cfg.iPerilous_RedVfxFormID << "\n";
		output << "iPerilous_SfxFormID=0x" << std::uppercase << std::hex << cfg.iPerilous_SfxFormID << "\n";
	}
}
