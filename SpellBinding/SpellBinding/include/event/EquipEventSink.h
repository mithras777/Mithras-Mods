#pragma once

namespace SB_EVENT
{
	class EquipEventSink final : public REX::Singleton<EquipEventSink>,
	                             public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
	};
}
