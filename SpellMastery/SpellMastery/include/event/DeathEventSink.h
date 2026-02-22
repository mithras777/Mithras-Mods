#pragma once

namespace GAME_EVENT
{
	class DeathEventSink final :
		public REX::Singleton<DeathEventSink>,
		public RE::BSTEventSink<RE::TESDeathEvent>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESDeathEvent* a_event,
			RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) override;
	};
}
