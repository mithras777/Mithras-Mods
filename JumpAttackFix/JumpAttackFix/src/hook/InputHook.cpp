#include "hook/InputHook.h"

#include "console/ConsoleCommands.h"

namespace HOOK::INPUT {

	static constexpr auto CONSOLE_PREFIX = "test";
	static constexpr float kJumpLandEndCooldown = 0.10f;
	static constexpr float kMinAirborneTime = 0.02f;
	static constexpr float kDelayedSendAfterLanding = 0.20f;

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
			if (a_this && a_this->IsPlayerRef() && a_delta > 0.0f) {
				auto* camera = RE::PlayerCamera::GetSingleton();
				const bool firstPerson = camera && camera->IsInFirstPerson();
				auto* controller = a_this->GetCharController();
				const auto currentState = controller ? controller->context.currentState : RE::hkpCharacterStateType::kOnGround;
				const bool midairFlag = a_this->IsInMidair();
				const bool airborneNow = controller ?
					(currentState != RE::hkpCharacterStateType::kOnGround || midairFlag) :
					midairFlag;

				if (s_cooldown > 0.0f) {
					s_cooldown = std::max(0.0f, s_cooldown - a_delta);
				}
				if (s_delayedSendTimer > 0.0f) {
					s_delayedSendTimer = std::max(0.0f, s_delayedSendTimer - a_delta);
				}

				const bool landedNow = s_wasAirborne && !airborneNow;
				if (landedNow && firstPerson && s_airborneTime >= kMinAirborneTime && s_cooldown <= 0.0f) {
					s_delayedSendPending = true;
					s_delayedSendTimer = kDelayedSendAfterLanding;
				}

				if (s_delayedSendPending && s_delayedSendTimer <= 0.0f) {
					if (firstPerson && !airborneNow) {
						(void)a_this->NotifyAnimationGraph("JumpLandEnd");
						s_cooldown = kJumpLandEndCooldown;
					}
					s_delayedSendPending = false;
					s_delayedSendTimer = 0.0f;
				}

				if (airborneNow) {
					s_airborneTime += a_delta;
				} else {
					s_airborneTime = 0.0f;
				}
				s_wasAirborne = airborneNow;
			}

			func(a_this, a_delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;
		static inline bool s_wasAirborne{ false };
		static inline float s_airborneTime{ 0.0f };
		static inline float s_cooldown{ 0.0f };
		static inline bool s_delayedSendPending{ false };
		static inline float s_delayedSendTimer{ 0.0f };
	};

	void Install()
	{
		// Called when ProcessConsole is updated
		stl::Hook::call<ProcessConsole>(RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52));
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
	}
}
