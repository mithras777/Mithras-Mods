#pragma once

namespace GAME_EVENT
{
	class HitEventSink final :
		public REX::Singleton<HitEventSink>,
		public RE::BSTEventSink<RE::TESHitEvent>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESHitEvent* a_event,
			RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;
	};
}
