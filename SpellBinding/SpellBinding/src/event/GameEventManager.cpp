#include "event/GameEventManager.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

namespace GAME_EVENT {
	namespace
	{
		bool IsPassiveMenu(std::string_view a_name)
		{
			// Menus that can be open during normal gameplay and should not suppress attack triggers.
			return a_name == "HUD Menu" ||
			       a_name == "TrueHUD" ||
			       a_name == "Cursor Menu" ||
			       a_name == "CombatAlertOverlayMenu" ||
			       a_name == "LootMenu" ||
			       a_name == "QuickLootMenu" ||
			       a_name == "LoadWaitSpinner" ||
			       a_name == "Fader Menu" ||
			       a_name == "Mist Menu";
		}
	}

	void Manager::Register()
	{
		auto gameEvent = Manager::GetSingleton();
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(gameEvent);
	}

	bool Manager::IsBlockingMenuOpen() const noexcept
	{
		return !m_openBlockingMenuNames.empty();
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
			m_openBlockingMenuNames.emplace(menuName);
		} else {
			if (m_openBlockingMenuNames.erase(std::string(menuName)) == 0) {
				LOG_INFO("SpellBinding Menu: close without matching open -> {}", menuName);
			}
		}
		m_openBlockingMenus = static_cast<std::uint32_t>(m_openBlockingMenuNames.size());

		SBIND::Manager::GetSingleton()->OnMenuStateChanged(IsBlockingMenuOpen());
		LOG_INFO("SpellBinding Menu: {} ({})", menuName, a_event->opening ? "Opening" : "Closing");

		return RE::BSEventNotifyControl::kContinue;
	}
}
