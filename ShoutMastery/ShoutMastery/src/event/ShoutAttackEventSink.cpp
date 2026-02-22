#include "event/ShoutAttackEventSink.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT
{
	void ShoutAttackEventSink::Register()
	{
		auto* source = RE::ShoutAttack::GetEventSource();
		auto* sink = ShoutAttackEventSink::GetSingleton();
		if (!source || !sink) {
			LOG_WARN("Failed to register ShoutAttack::Event sink");
			return;
		}

		source->AddEventSink(sink);
		LOG_INFO("Registered ShoutAttack::Event sink");
	}

	RE::BSEventNotifyControl ShoutAttackEventSink::ProcessEvent(const RE::ShoutAttack::Event* a_event, RE::BSTEventSource<RE::ShoutAttack::Event>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MITHRAS::SHOUT_MASTERY::Manager::GetSingleton()->OnShoutAttack(*a_event);
		return RE::BSEventNotifyControl::kContinue;
	}
}
