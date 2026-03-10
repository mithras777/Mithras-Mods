#pragma once

namespace SB_EVENT
{
	class AttackAnimationEventSink final
	{
	public:
		static void Register();
		static void RebindToPlayer();

	private:
		static RE::BSEventNotifyControl ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
		                                             RE::BSAnimationGraphEvent& a_event,
		                                             RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		static RE::BSEventNotifyControl PlayerProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
		                                                   RE::BSAnimationGraphEvent& a_event,
		                                                   RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		static RE::BSEventNotifyControl CharacterProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
		                                                      RE::BSAnimationGraphEvent& a_event,
		                                                      RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);

		static inline bool g_installed{ false };
		static inline REL::Relocation<decltype(PlayerProcessEvent)> g_playerProcessEvent{};
		static inline REL::Relocation<decltype(CharacterProcessEvent)> g_characterProcessEvent{};
	};
}
