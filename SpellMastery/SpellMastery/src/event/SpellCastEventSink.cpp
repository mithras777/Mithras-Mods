#include "event/SpellCastEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void SpellCastEventSink::Register()
	{
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		auto* sink = SpellCastEventSink::GetSingleton();
		if (!eventSourceHolder || !sink) {
			LOG_WARN("Failed to register TESSpellCastEvent sink");
			return;
		}

		eventSourceHolder->AddEventSink<RE::TESSpellCastEvent>(sink);
		LOG_INFO("Registered TESSpellCastEvent sink");
	}

	RE::BSEventNotifyControl SpellCastEventSink::ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::SPELL_MASTERY::Manager::GetSingleton()->OnSpellCastEvent(*a_event);
		return RE::BSEventNotifyControl::kContinue;
	}
}
