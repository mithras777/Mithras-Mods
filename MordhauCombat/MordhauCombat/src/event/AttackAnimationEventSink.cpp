#include "event/AttackAnimationEventSink.h"

#include "combat/DirectionalController.h"
#include "util/LogUtil.h"

namespace MC::EVENT
{
	namespace
	{
		thread_local int g_inProcessEvent = 0;

		struct ProcessEventGuard
		{
			ProcessEventGuard() { ++g_inProcessEvent; }
			~ProcessEventGuard() { --g_inProcessEvent; }
		};
	}

	void AttackAnimationEventSink::Register()
	{
		if (g_installed) {
			return;
		}

		REL::Relocation<std::uintptr_t> playerVtbl{ RE::VTABLE_PlayerCharacter[2] };
		REL::Relocation<std::uintptr_t> characterVtbl{ RE::VTABLE_Character[2] };
		g_playerProcessEvent = playerVtbl.write_vfunc(0x1, &PlayerProcessEvent);
		g_characterProcessEvent = characterVtbl.write_vfunc(0x1, &CharacterProcessEvent);
		g_installed = true;
		LOG_INFO("MordhauCombat: attack animation event hooks installed");
	}

	RE::BSEventNotifyControl AttackAnimationEventSink::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>*,
	                                                                RE::BSAnimationGraphEvent& a_event,
	                                                                RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
	{
		if (g_inProcessEvent > 0) {
			return RE::BSEventNotifyControl::kContinue;
		}
		ProcessEventGuard guard;

		if (!a_event.holder || !a_event.holder->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MC::DIRECTIONAL::Controller::GetSingleton()->OnAnimationEvent(a_event.tag.c_str(), a_event.payload.c_str());
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl AttackAnimationEventSink::PlayerProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
	                                                                      RE::BSAnimationGraphEvent& a_event,
	                                                                      RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		ProcessEvent(a_this, a_event, a_source);
		return g_playerProcessEvent(a_this, a_event, a_source);
	}

	RE::BSEventNotifyControl AttackAnimationEventSink::CharacterProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
	                                                                         RE::BSAnimationGraphEvent& a_event,
	                                                                         RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		ProcessEvent(a_this, a_event, a_source);
		return g_characterProcessEvent(a_this, a_event, a_source);
	}
}
