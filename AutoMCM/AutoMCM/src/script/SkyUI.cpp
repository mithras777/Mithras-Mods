#include "script/SkyUI.h"

#undef GetObject

namespace Script::SkyUI
{
	ScriptObjectPtr ConfigManager::GetInstance()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto configManagerForm = dataHandler ?
			dataHandler->LookupForm(0x00000802, kPluginName) :
			nullptr;
		return configManagerForm ? Object::FromForm(configManagerForm, "SKI_ConfigManager") : nullptr;
	}

	std::vector<ScriptObjectPtr> ConfigManager::GetConfigs()
	{
		std::vector<ScriptObjectPtr> configs;
		const auto configManager = GetInstance();
		if (!configManager) {
			return configs;
		}

		auto* variable = Object::GetVariable(configManager, "_modConfigs");
		auto array = variable && variable->IsArray() ? variable->GetArray() : nullptr;
		if (!array) {
			return configs;
		}

		for (std::uint32_t i = 0; i < array->size(); ++i) {
			auto object = array->data()[i].GetObject();
			if (object) {
				configs.emplace_back(object);
			}
		}

		return configs;
	}

	ScriptObjectPtr ConfigManager::GetConfigByModName(std::string_view a_modName)
	{
		for (auto& config : GetConfigs()) {
			auto* modNameVar = config ? config->GetProperty("ModName") : nullptr;
			if (modNameVar && std::string_view(modNameVar->GetString()) == a_modName) {
				return config;
			}
		}
		return nullptr;
	}
}
