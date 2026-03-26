#include "hook/MainHook.h"

#include "movement/MovementPatcher.h"
#include "movement/Settings.h"
#include "util/HookUtil.h"

#include "RE/A/ActorState.h"
#include "RE/P/PlayerCharacter.h"

namespace
{
	struct PlayerMovementSpeedHook
	{
		static float thunk(RE::ActorState* a_this)
		{
			const auto speed = func(a_this);
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || a_this != player->AsActorState()) {
				return speed;
			}

			const auto* settings = MOVEMENT::Settings::GetSingleton();
			if (!settings) {
				return speed;
			}

			const auto current = settings->Get();
			if (!current.general.enabled) {
				return speed;
			}

			const auto multiplier = MOVEMENT::MovementPatcher::GetSingleton()->GetPlayerSpeedMultiplier(current, *player);
			return speed * (multiplier / 100.0f);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
}

namespace HOOK::MAIN {

	void Install()
	{
		stl::Hook::virtual_function<RE::ActorState, PlayerMovementSpeedHook>(0, 5);
	}
}
