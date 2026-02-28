#include "console/ConsoleCommands.h"

#include "SpawnManager.h"
#include "plugin.h"

#include <sstream>
#include <string>
#include <vector>

namespace CONSOLE::COMMANDS
{
	void Plugin(RE::Script* a_script)
	{
		if (!a_script) {
			return;
		}

		auto* console = RE::ConsoleLog::GetSingleton();
		if (!console) {
			return;
		}

		std::stringstream args(a_script->GetCommand());
		std::vector<std::string> tokens{};
		std::string token{};
		while (args >> token) {
			tokens.push_back(token);
			if (tokens.size() >= 4) {
				break;
			}
		}

		if (tokens.empty()) {
			return;
		}

		const auto& command = tokens[0];
		if (command == "ds_spawnNow") {
			DYNAMIC_SPAWNS::SpawnManager::GetSingleton()->ForceEvaluateNow();
			console->Print("DynamicSpawns: forced spawn evaluation");
			return;
		}

		if (command == "ds_clear") {
			DYNAMIC_SPAWNS::SpawnManager::GetSingleton()->ClearManagedNow();
			console->Print("DynamicSpawns: cleared managed spawns");
			return;
		}

		if (command == "ds_version") {
			const auto& info = DLLMAIN::Plugin::GetSingleton()->Info();
			console->Print("%s: version %s", info.name.c_str(), info.version.string(".").c_str());
			return;
		}

		console->Print("DynamicSpawns: unknown command '%s'", command.c_str());
	}
}
