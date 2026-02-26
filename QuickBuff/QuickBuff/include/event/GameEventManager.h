#pragma once

namespace GAME_EVENT {

	class Manager final : public REX::Singleton<Manager>,
                         public RE::BSTEventSink<RE::MenuOpenCloseEvent> {

	public:
		static void Register();
		bool IsBlockingMenuOpen() const noexcept;

	private:
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;

	private:
		std::uint32_t m_openBlockingMenus{ 0 };
	};
}
