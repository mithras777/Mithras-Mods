#include "event/DeathEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void DeathEventSink::Register()
	{
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* sink = DeathEventSink::GetSingleton();
		if (!eventSourceHolder || !sink) {
			LOG_WARN("Failed to register TESDeathEvent sink");
			return;
		}

		eventSourceHolder->AddEventSink<RE::TESDeathEvent>(sink);
		LOG_INFO("Registered TESDeathEvent sink");
	}

	RE::BSEventNotifyControl DeathEventSink::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::MASTERY::Manager::GetSingleton()->OnDeathEvent(*a_event);
		return RE::BSEventNotifyControl::kContinue;
	}
}
