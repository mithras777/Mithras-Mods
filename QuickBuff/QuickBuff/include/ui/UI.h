#pragma once

#if __has_include("SKSEMenuFramework/include/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/include/SKSEMenuFramework.h"
	#define QUICKBUFF_HAS_MENU_FRAMEWORK 1
#elif __has_include("SKSEMenuFramework/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/SKSEMenuFramework.h"
	#define QUICKBUFF_HAS_MENU_FRAMEWORK 1
#elif __has_include("SKSEMenuFramework.h")
	#include "SKSEMenuFramework.h"
	#define QUICKBUFF_HAS_MENU_FRAMEWORK 1
#elif __has_include("../../../Shared/SKSEMenuFramework.h")
	#include "../../../Shared/SKSEMenuFramework.h"
	#define QUICKBUFF_HAS_MENU_FRAMEWORK 1
#else
#define QUICKBUFF_HAS_MENU_FRAMEWORK 0
#include <string>
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
