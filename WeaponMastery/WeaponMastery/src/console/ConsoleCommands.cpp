#include "console/ConsoleCommands.h"

#include "util/LogUtil.h"
#include "util/NotificationCompat.h"

namespace CONSOLE::COMMANDS {

	void Plugin(RE::Script* a_script)
	{
		if (!a_script) {
			return;
		}

		const std::string command = a_script->text;
		if (command.starts_with("mithras_test")) {
			constexpr auto message = "Mithras: mithras_test OK";
			RE::DebugNotification(message);
			LOG_INFO("{}", message);
		}
	}
}
