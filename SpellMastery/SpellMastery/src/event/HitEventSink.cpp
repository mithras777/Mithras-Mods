#include "event/HitEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void HitEventSink::Register()
	{
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* sink = HitEventSink::GetSingleton();
		if (!eventSourceHolder || !sink) {
			LOG_WARN("Failed to register TESHitEvent sink");
			return;
		}

		eventSourceHolder->AddEventSink<RE::TESHitEvent>(sink);
		LOG_INFO("Registered TESHitEvent sink");
	}

	RE::BSEventNotifyControl HitEventSink::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::SPELL_MASTERY::Manager::GetSingleton()->OnHitEvent(*a_event);
		return RE::BSEventNotifyControl::kContinue;
	}
}
