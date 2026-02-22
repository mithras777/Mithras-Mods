#pragma once

namespace GAME_EVENT
{
	class ShoutAttackEventSink final :
		public REX::Singleton<ShoutAttackEventSink>,
		public RE::BSTEventSink<RE::ShoutAttack::Event>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::ShoutAttack::Event* a_event,
			RE::BSTEventSource<RE::ShoutAttack::Event>* a_eventSource) override;
	};
}
