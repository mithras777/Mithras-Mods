#pragma once

#if __has_include("SKSEMenuFramework/include/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/include/SKSEMenuFramework.h"
#elif __has_include("SKSEMenuFramework/SKSEMenuFramework.h")
	#include "SKSEMenuFramework/SKSEMenuFramework.h"
#elif __has_include("SKSEMenuFramework.h")
	#include "SKSEMenuFramework.h"
#else
#include <string>
namespace SKSEMenuFramework
{
	inline bool IsInstalled() { return false; }
	inline void SetSection(std::string) {}
	inline void AddSectionItem(std::string, void(__stdcall*)()) {}
}
#endif

namespace UI::MCM
{
	void Register();

	namespace MainPanel
	{
		void __stdcall Render();
	}

	namespace GeneralTab
	{
		void Render();
	}

	namespace CombatTab
	{
		void Render();
	}

	namespace DefaultTab
	{
		void Render();
	}

	namespace MiscTab
	{
		void Render();
	}
}
