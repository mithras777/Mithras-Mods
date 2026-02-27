#include "movement/Settings.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

#include <algorithm>
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

		std::string GroupForName(std::string_view a_name)
		{
			if (a_name.find("Default") != std::string_view::npos) {
				return "Default";
			}
			if (a_name.find("Combat") != std::string_view::npos ||
			    a_name.find("Attack") != std::string_view::npos ||
			    a_name.find("Blocking") != std::string_view::npos ||
			    a_name.find("Bow") != std::string_view::npos ||
			    a_name.find("Magic") != std::string_view::npos ||
			    a_name.find("Sprinting") != std::string_view::npos) {
				return "Combat";
			}
			return "Misc";
		}

		MovementEntry MakeEntry(std::string_view a_name, std::string_view a_form)
		{
			MovementEntry entry{};
			entry.name = a_name;
			entry.form = a_form;
			entry.group = GroupForName(a_name);
			entry.enabled = false;
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
			nlohmann::json speeds = nlohmann::json::array();
			for (std::size_t i = 0; i < 5; ++i) {
				speeds.push_back({ entry.speeds[i][0], entry.speeds[i][1] });
			}
			entries.push_back({
				{ "name", entry.name },
				{ "form", entry.form },
				{ "group", entry.group },
				{ "enabled", entry.enabled },
				{ "speeds", std::move(speeds) }
			});
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

		SettingsData loaded = Defaults();
		loaded.version = ReadOr<int>(root, "version", loaded.version);

		if (root.contains("general") && root["general"].is_object()) {
			const auto& general = root["general"];
			loaded.general.enabled = ReadOr<bool>(general, "enabled", loaded.general.enabled);
			loaded.general.restoreOnDisable = ReadOr<bool>(general, "restoreOnDisable", loaded.general.restoreOnDisable);
		}

		if (root.contains("entries") && root["entries"].is_array()) {
			loaded.entries.clear();
			for (const auto& node : root["entries"]) {
				if (!node.is_object()) {
					continue;
				}

				MovementEntry entry{};
				entry.name = ReadOr<std::string>(node, "name", "");
				entry.form = ReadOr<std::string>(node, "form", "");
				entry.group = ReadOr<std::string>(node, "group", GroupForName(entry.name));
				entry.enabled = ReadOr<bool>(node, "enabled", false);

				if (node.contains("speeds") && node["speeds"].is_array()) {
					const auto& speeds = node["speeds"];
					for (std::size_t i = 0; i < 5 && i < speeds.size(); ++i) {
						const auto& pair = speeds[i];
						if (pair.is_array() && pair.size() >= 2) {
							entry.speeds[i][0] = pair[0].get<float>();
							entry.speeds[i][1] = pair[1].get<float>();
						}
					}
				}

				if (!entry.form.empty()) {
					loaded.entries.push_back(std::move(entry));
				}
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
		data.version = 2;
		data.entries = {
			MakeEntry("AIControlledNPC_Sprinting_MT", "Skyrim.esm|0x000F3469"),
			MakeEntry("AtronachFlame_Default", "Skyrim.esm|0x000884A4"),
			MakeEntry("AtronachFrost_Blocking_MT", "Skyrim.esm|0x00059661"),
			MakeEntry("AtronachFrost_Default_MT", "Skyrim.esm|0x00059662"),
			MakeEntry("AtronachStorm_Default", "Skyrim.esm|0x0006B7C8"),
			MakeEntry("Bear_Default_MT", "Skyrim.esm|0x00059656"),
			MakeEntry("Bear_Swimming_MT", "Skyrim.esm|0x00054B36"),
			MakeEntry("ChaurusDefault_MT", "Skyrim.esm|0x00064113"),
			MakeEntry("Chicken_Default_MT", "Skyrim.esm|0x000B0B90"),
			MakeEntry("CowSwimDefault_MT", "Skyrim.esm|0x000C9EDB"),
			MakeEntry("Cow_Default_MT", "Skyrim.esm|0x00059655"),
			MakeEntry("Deer_DefaultRun_MT", "Skyrim.esm|0x000B8F2E"),
			MakeEntry("Deer_Default_MT", "Skyrim.esm|0x00059658"),
			MakeEntry("Dog_Default_MT", "Skyrim.esm|0x0004E5E0"),
			MakeEntry("Dog_Run_MT", "Skyrim.esm|0x000EA64A"),
			MakeEntry("Dragon_Perching_MT", "Skyrim.esm|0x00065AA2"),
			MakeEntry("Draugr1HM_MT", "Skyrim.esm|0x000745A9"),
			MakeEntry("Draugr2GSBlocking_MT", "Skyrim.esm|0x00021A46"),
			MakeEntry("Draugr2HMBlocking_MT", "Skyrim.esm|0x00023EF3"),
			MakeEntry("DraugrBattleAxe_MT", "Skyrim.esm|0x000745AA"),
			MakeEntry("DraugrBlocking_MT", "Skyrim.esm|0x0006773A"),
			MakeEntry("DraugrBowDrawn_MT", "Skyrim.esm|0x000745A8"),
			MakeEntry("DraugrBow_MT", "Skyrim.esm|0x000745AB"),
			MakeEntry("DraugrDefault_MT", "Skyrim.esm|0x0005F5C8"),
			MakeEntry("DraugrGreatSword_MT", "Skyrim.esm|0x000745AC"),
			MakeEntry("Draugr2H_MT", "Skyrim.esm|0x000745AD"),
			MakeEntry("DraugrRanged_MT", "Skyrim.esm|0x00071BEF"),
			MakeEntry("DraugrShieldBlocking_MT", "Skyrim.esm|0x000B7306"),
			MakeEntry("DwarvenSpider_Default_MT", "Skyrim.esm|0x000872AF"),
			MakeEntry("FalmerBowDrawn_MT", "Skyrim.esm|0x000D82F5"),
			MakeEntry("Falmer_1HM_Run", "Skyrim.esm|0x00103273"),
			MakeEntry("Falmer_1HM_Walk", "Skyrim.esm|0x00103272"),
			MakeEntry("Falmer_Default_MT", "Skyrim.esm|0x00063856"),
			MakeEntry("GiantCombatRun_MT", "Skyrim.esm|0x000A5CB2"),
			MakeEntry("GiantCombatWalk_MT", "Skyrim.esm|0x00095568"),
			MakeEntry("GiantDefault_MT", "Skyrim.esm|0x0003C59D"),
			MakeEntry("Horse_Default_MT", "Skyrim.esm|0x0001CF24"),
			MakeEntry("Horse_Fall_MT", "Skyrim.esm|0x00078722"),
			MakeEntry("Horse_Sprint_MT", "Skyrim.esm|0x0004408D"),
			MakeEntry("Horse_Swim_MT", "Skyrim.esm|0x000C2EE2"),
			MakeEntry("IceWraith_Default_MT", "Skyrim.esm|0x0002F585"),
			MakeEntry("Mammoth_Default_MT", "Skyrim.esm|0x0005965B"),
			MakeEntry("MCrab_Default_MT", "Skyrim.esm|0x000BA555"),
			MakeEntry("NPC_1HM_MT", "Skyrim.esm|0x00069CD8"),
			MakeEntry("NPC_2HM_MT", "Skyrim.esm|0x00069CD9"),
			MakeEntry("NPC_Attacking2H_MT", "Skyrim.esm|0x000CEDFC"),
			MakeEntry("NPC_Attacking_MT", "Skyrim.esm|0x000A0BC2"),
			MakeEntry("NPC_Bleedout_MT", "Skyrim.esm|0x00036D41"),
			MakeEntry("NPC_Blocking_MT", "Skyrim.esm|0x00035B4C"),
			MakeEntry("NPC_Blocking_ShieldCharge_MT", "Skyrim.esm|0x000EF541"),
			MakeEntry("NPC_BowDrawn_MT", "Skyrim.esm|0x0003580C"),
			MakeEntry("NPC_BowDrawn_QuickShot_MT", "Skyrim.esm|0x000EF542"),
			MakeEntry("NPC_Bow_MT", "Skyrim.esm|0x00069CDA"),
			MakeEntry("NPC_Default_MT", "Skyrim.esm|0x0003580D"),
			MakeEntry("NPC_Drunk_MT", "Skyrim.esm|0x000CEFCF"),
			MakeEntry("NPC_Horse_MT", "Skyrim.esm|0x00069F74"),
			MakeEntry("NPC_MagicCasting_MT", "Skyrim.esm|0x00069CDC"),
			MakeEntry("NPC_Magic_MT", "Skyrim.esm|0x00069CDB"),
			MakeEntry("NPC_PowerAttacking_MT", "Skyrim.esm|0x000BFF7F"),
			MakeEntry("NPC_Sneaking_MT", "Skyrim.esm|0x0003580B"),
			MakeEntry("NPC_Sprinting_MT", "Skyrim.esm|0x00034D9C"),
			MakeEntry("NPC_Swimming_MT", "Skyrim.esm|0x00036D42"),
			MakeEntry("SabreCat_Default_MT", "Skyrim.esm|0x00059657"),
			MakeEntry("SabreCat_Run_MT", "Skyrim.esm|0x000EA64C"),
			MakeEntry("Skeever_AttackLunge_MT", "Skyrim.esm|0x0004CECF"),
			MakeEntry("Skeever_Default_MT", "Skyrim.esm|0x0004CED0"),
			MakeEntry("SlaughterfishSwim_MT", "Skyrim.esm|0x000D8C5E"),
			MakeEntry("SphereCombat_MT", "Skyrim.esm|0x0007874F"),
			MakeEntry("SphereDefault_MT", "Skyrim.esm|0x0007874E"),
			MakeEntry("SphereRanged_MT", "Skyrim.esm|0x00078750"),
			MakeEntry("SpiderDefault_MT", "Skyrim.esm|0x00045C4E"),
			MakeEntry("Spriggan_Combat", "Skyrim.esm|0x0002BB09"),
			MakeEntry("Spriggan_Default", "Skyrim.esm|0x000973BA"),
			MakeEntry("SteamCombat_MT", "Skyrim.esm|0x000800EE"),
			MakeEntry("SteamDefault_MT", "Skyrim.esm|0x000800ED"),
			MakeEntry("TrollDefault_MT", "Skyrim.esm|0x0005F5C9"),
			MakeEntry("WerewolfBeastDefault_MT", "Skyrim.esm|0x000CDD8B"),
			MakeEntry("WerewolfBeastSprint_MT", "Skyrim.esm|0x000CDD98"),
			MakeEntry("Wisp_Default_MT", "Skyrim.esm|0x0004252A"),
			MakeEntry("Witchlight_Default_MT", "Skyrim.esm|0x00086F47"),
			MakeEntry("Wolf_Default_MT", "Skyrim.esm|0x00055D28"),
			MakeEntry("Wolf_Run_MT", "Skyrim.esm|0x000EA64B")
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
		}
	}
}
