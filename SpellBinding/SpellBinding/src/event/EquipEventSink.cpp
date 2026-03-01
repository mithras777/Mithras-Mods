#include "event/EquipEventSink.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

namespace SB_EVENT
{
	void EquipEventSink::Register()
	{
		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* sink = EquipEventSink::GetSingleton();
		if (!holder || !sink) {
			LOG_WARN("SpellBinding: failed to register TESEquipEvent sink");
			return;
		}

		holder->AddEventSink<RE::TESEquipEvent>(sink);
		LOG_INFO("SpellBinding: TESEquipEvent sink registered");
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

		SBIND::Manager::GetSingleton()->OnEquipChanged();
		return RE::BSEventNotifyControl::kContinue;
	}
}
