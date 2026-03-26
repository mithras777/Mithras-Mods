#include "hook/MainHook.h"

#include "movement/MovementPatcher.h"
#include "movement/Settings.h"
#include "util/HookUtil.h"

#include "RE/A/Actor.h"
#include "RE/P/PlayerCharacter.h"

namespace
{
	struct PlayerSpeedUpdateHook
	{
		static void thunk(RE::Actor* a_this, float a_delta)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player && a_this == player) {
				const auto* settings = MOVEMENT::Settings::GetSingleton();
				const auto current = settings ? settings->Get() : MOVEMENT::SettingsData{};
				const auto desiredValue = current.general.enabled
					                         ? MOVEMENT::MovementPatcher::GetSingleton()->GetPlayerSpeedMultiplier(current, *player)
					                         : 100.0f;
				const auto currentValue = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeedMult);
				const auto delta = desiredValue - currentValue;
				if (delta != 0.0f) {
					player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kSpeedMult, delta);
				}
			}

			func(a_this, a_delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
}

namespace HOOK::MAIN {

	void Install()
	{
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerSpeedUpdateHook>(0, 0xAD);
	}
}
