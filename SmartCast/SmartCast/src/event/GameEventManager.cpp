#include "event/GameEventManager.h"

#include <string_view>

namespace GAME_EVENT {
	namespace
	{
		bool IsPassiveMenu(std::string_view a_name)
		{
			return a_name == "HUD Menu";
		}
	}

	void Manager::Register()
	{
		auto gameEvent = Manager::GetSingleton();
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(gameEvent);
	}

	bool Manager::IsBlockingMenuOpen() const noexcept
	{
		return m_openBlockingMenus > 0;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const std::string_view menuName = a_event->menuName.c_str();
		if (IsPassiveMenu(menuName)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->opening) {
			++m_openBlockingMenus;
		} else if (m_openBlockingMenus > 0) {
			--m_openBlockingMenus;
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
