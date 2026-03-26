#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <PROXY/Proxies.h>

#include <REX/REX/Singleton.h>

#include "util/HookUtil.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

#if defined(SKYRIM_SUPPORT_NG)
	#define RELOCATION_OFFSET(a_se, a_ae) REL::VariantOffset(a_se, a_ae, 0x0).offset()
#else
	#if defined(SKYRIM_SUPPORT_AE)
		#define RELOCATION_OFFSET(se, ae) ae
	#else
		#define RELOCATION_OFFSET(se, ae) se
	#endif
#endif
