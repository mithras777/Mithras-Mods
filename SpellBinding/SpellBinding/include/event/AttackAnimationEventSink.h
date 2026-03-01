#pragma once

namespace SB_EVENT
{
	class AttackAnimationEventSink final : public REX::Singleton<AttackAnimationEventSink>,
	                                       public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;
	};
}
