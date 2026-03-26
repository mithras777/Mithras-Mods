#pragma once

#include "script/ScriptObject.h"

namespace Script::SkyUI
{
	inline constexpr std::string_view kPluginName = "SkyUI_SE.esp"sv;

	class ConfigManager
	{
	public:
		static ScriptObjectPtr GetInstance();
		static std::vector<ScriptObjectPtr> GetConfigs();
		static ScriptObjectPtr GetConfigByModName(std::string_view a_modName);
	};
}
