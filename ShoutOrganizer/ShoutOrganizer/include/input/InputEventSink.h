#pragma once

namespace SHO_INPUT
{
	class EventSink final :
		public REX::Singleton<EventSink>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(
			RE::InputEvent* const* a_event,
			RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
	};
}
