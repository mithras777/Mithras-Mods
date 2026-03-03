#include "hook/MainHook.h"

#include "perilous/PerilousHooks.h"

#include "util/LogUtil.h"

namespace HOOK::MAIN {

	void Install()
	{
		LOG_INFO("Perilous hooks initializing...");
		SKSE::AllocTrampoline(1 << 10);
		PERILOUS::Hooks::Install();
	}
}
