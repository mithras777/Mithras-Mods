#include "hook/InputHook.h"

#include "console/ConsoleCommands.h"
#include "diagonal/FakeDiagonalSprint.h"

namespace HOOK::INPUT {

	static constexpr auto CONSOLE_PREFIX = "test";

	struct ProcessConsole {

		static void thunk(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name, RE::TESObjectREFR* a_objectRefr)
		{
			const std::string text = a_script->text;
			if (text.starts_with(CONSOLE_PREFIX)) {
				CONSOLE::COMMANDS::Plugin(a_script);
				return;
			}

			func(a_script, a_compiler, a_name, a_objectRefr);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PlayerControlsInput {
		static RE::BSEventNotifyControl thunk(RE::PlayerControls* a_this, RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_source)
		{
			DIAGONAL::FakeDiagonalSprint::GetSingleton()->OnInputEvent(a_event);
			return func(a_this, a_event, a_source);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PlayerUpdate {
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			if (a_this && a_this->IsPlayerRef()) {
				DIAGONAL::FakeDiagonalSprint::GetSingleton()->OnPlayerUpdate(a_delta);
			}
			func(a_this, a_delta);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		DIAGONAL::FakeDiagonalSprint::GetSingleton()->Initialize();

		stl::Hook::call<ProcessConsole>(RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52));
		stl::Hook::virtual_function<RE::PlayerControls, PlayerControlsInput>(0, 0x1);
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
	}
}
