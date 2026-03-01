#pragma once

namespace GAME_EVENT {

	class Manager final : public REX::Singleton<Manager>,
                         public RE::BSTEventSink<RE::MenuOpenCloseEvent> {

	public:
		static void Register();
		[[nodiscard]] bool IsBlockingMenuOpen() const noexcept;

	private:
		std::uint32_t m_openBlockingMenus{ 0 };
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;
	};
}
