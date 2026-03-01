#include "input/InputEventSink.h"

#include "spell/SpellManager.h"
#include "util/LogUtil.h"

namespace SO_INPUT
{
	namespace
	{
		bool g_registered = false;

		bool IsInputBlocked()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return true;
			}

			return ui->IsMenuOpen(RE::MainMenu::MENU_NAME) ||
			       ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME);
		}
	}

	void EventSink::Register()
	{
		if (g_registered) {
			return;
		}

		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		auto* sink = EventSink::GetSingleton();
		if (!inputMgr || !sink) {
			LOG_WARN("SpellOrganizer: Failed to register InputEvent sink");
			return;
		}

		inputMgr->AddEventSink(sink);
		g_registered = true;
		LOG_INFO("SpellOrganizer: InputEvent sink registered");
	}

	RE::BSEventNotifyControl EventSink::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_event || !*a_event || IsInputBlocked()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* manager = MITHRAS::SPELL_ORGANIZER::Manager::GetSingleton();
		for (auto* input = *a_event; input; input = input->next) {
			if (input->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			const auto* button = input->AsButtonEvent();
			if (!button || input->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) {
				continue;
			}

			const auto keyCode = button->GetIDCode();
			if (manager->IsCapturingHotkey()) {
				manager->OnHotkeyPressed(keyCode);
				continue;
			}

			const auto cfg = manager->GetConfig();
			if (cfg.enabled && cfg.hotkey == keyCode) {
				if (!manager->HideCurrentlySelectedMagicMenuSpell()) {
					LOG_INFO("SpellOrganizer: Hotkey pressed but no hideable selected spell was resolved");
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
