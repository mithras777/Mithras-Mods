#include "hook/InputHook.h"

#include "console/ConsoleCommands.h"
#include "spellbinding/SpellBindingManager.h"

namespace HOOK::INPUT {

	static constexpr auto CONSOLE_PREFIX = "test";

	struct ProcessConsole {

		static void thunk(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name, RE::TESObjectREFR* a_objectRefr)
		{
			const std::string text = a_script->text;
			// Search for specific prefix.
			if (text.starts_with(CONSOLE_PREFIX)) {
				CONSOLE::COMMANDS::Plugin(a_script);
				return;
			}

			func(a_script, a_compiler, a_name, a_objectRefr);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PlayerUpdate {
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			SBIND::Manager::GetSingleton()->Update(a_this, a_delta);
			func(a_this, a_delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		// Called when ProcessConsole is updated
		stl::Hook::call<ProcessConsole>(RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52));
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
	}
}
