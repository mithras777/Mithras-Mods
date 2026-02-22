#include "hook/MainHook.h"

#include "jumpattack/InputHandler.h"
#include "util/LogUtil.h"

namespace HOOK::MAIN {

	void Install()
	{
		LOG_INFO("[JA] Initializing");
		JA::InputHandler::GetSingleton()->Install();
	}
}
