#include "event/AttackAnimationEventSink.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

namespace SB_EVENT
{
	namespace
	{
		bool g_registered = false;
	}

	void AttackAnimationEventSink::Register()
	{
		if (g_registered) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* sink = AttackAnimationEventSink::GetSingleton();
		if (!player || !sink) {
			LOG_WARN("SpellBinding: failed to register attack animation sink (player not ready)");
			return;
		}

		if (!player->AddAnimationGraphEventSink(sink)) {
			LOG_WARN("SpellBinding: AddAnimationGraphEventSink failed");
			return;
		}

		g_registered = true;
		LOG_INFO("SpellBinding: attack animation sink registered");
	}

	RE::BSEventNotifyControl AttackAnimationEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
	{
		if (!a_event || !a_event->holder || !a_event->holder->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* tag = a_event->tag.c_str();
		const char* payload = a_event->payload.c_str();
		SBIND::Manager::GetSingleton()->OnAttackAnimationEvent(tag ? tag : "", payload ? payload : "");
		return RE::BSEventNotifyControl::kContinue;
	}
}
