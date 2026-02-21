#include "event/EquipEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void EquipEventSink::Register()
	{
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* equipSink = EquipEventSink::GetSingleton();
		if (!eventSourceHolder || !equipSink) {
			LOG_WARN("Failed to register TESEquipEvent sink");
			return;
		}

		eventSourceHolder->AddEventSink<RE::TESEquipEvent>(equipSink);
		LOG_INFO("Registered TESEquipEvent sink");
	}

	RE::BSEventNotifyControl EquipEventSink::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto* actor = a_event->actor.get();
		if (!actor || !actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::MASTERY::Manager::GetSingleton()->OnEquipEvent(*a_event);

		return RE::BSEventNotifyControl::kContinue;
	}
}
