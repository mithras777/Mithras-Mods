#pragma once

#if __has_include("SKSEMenuFramework/include/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/include/SKSEMenuFramework.h"
#elif __has_include("SKSEMenuFramework/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/SKSEMenuFramework.h"
#elif __has_include("SKSEMenuFramework.h")
	#include "SKSEMenuFramework.h"
#elif __has_include("../../../Shared/SKSEMenuFramework/SKSEMenuFramework.h")
	#include "../../../Shared/SKSEMenuFramework/SKSEMenuFramework.h"
#else
namespace SKSEMenuFramework
{
	inline bool IsInstalled() { return false; }
	inline void SetSection(std::string) {}
	inline void AddSectionItem(std::string, void(__stdcall*)()) {}
}
#endif

namespace UI
{
	void Register();

	namespace MainPanel
	{
		void __stdcall Render();
	}
}