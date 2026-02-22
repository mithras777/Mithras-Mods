#pragma once

namespace GAME_EVENT
{
	class SpellCastEventSink final :
		public REX::Singleton<SpellCastEventSink>,
		public RE::BSTEventSink<RE::TESSpellCastEvent>
	{
	public:
		static void Register();

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESSpellCastEvent* a_event,
			RE::BSTEventSource<RE::TESSpellCastEvent>* a_eventSource) override;
	};
}
