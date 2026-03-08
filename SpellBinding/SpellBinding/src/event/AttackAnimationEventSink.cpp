#include "event/AttackAnimationEventSink.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

namespace SB_EVENT
{
	namespace
	{
		bool g_registered = false;

		bool BindToPlayer(bool a_forceRebind)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* sink = AttackAnimationEventSink::GetSingleton();
			if (!player || !sink) {
				LOG_WARN("SpellBinding: failed to bind attack animation sink (player not ready)");
				return false;
			}

			if (a_forceRebind) {
				player->RemoveAnimationGraphEventSink(sink);
				g_registered = false;
			} else if (g_registered) {
				return true;
			}

			if (!player->AddAnimationGraphEventSink(sink)) {
				LOG_WARN("SpellBinding: AddAnimationGraphEventSink failed");
				return false;
			}

			g_registered = true;
			return true;
		}
	}

	void AttackAnimationEventSink::Register()
	{
		if (BindToPlayer(false)) {
			LOG_INFO("SpellBinding: attack animation sink registered");
		}
	}

	void AttackAnimationEventSink::RebindToPlayer()
	{
		if (BindToPlayer(true)) {
			LOG_INFO("SpellBinding: attack animation sink rebound");
		}
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
