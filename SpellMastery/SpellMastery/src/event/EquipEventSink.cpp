#include "event/EquipEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void EquipEventSink::Register()
	{
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* sink = EquipEventSink::GetSingleton();
		if (!eventSourceHolder || !sink) {
			LOG_WARN("Failed to register TESEquipEvent sink");
			return;
		}

		eventSourceHolder->AddEventSink<RE::TESEquipEvent>(sink);
		LOG_INFO("Registered TESEquipEvent sink");
	}

	RE::BSEventNotifyControl EquipEventSink::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::SPELL_MASTERY::Manager::GetSingleton()->OnEquipEvent(*a_event);
		return RE::BSEventNotifyControl::kContinue;
	}
}
