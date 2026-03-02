#include "input/InputEventSink.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

#include <Windows.h>

namespace SB_INPUT
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
			LOG_WARN("SpellBinding: failed to register InputEvent sink");
			return;
		}

		inputMgr->AddEventSink(sink);
		g_registered = true;
		LOG_INFO("SpellBinding: InputEvent sink registered");
	}

	RE::BSEventNotifyControl EventSink::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_event || !*a_event || IsInputBlocked()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* manager = SBIND::Manager::GetSingleton();
		const auto config = manager->GetConfig();
		auto* ui = RE::UI::GetSingleton();
		const bool magicMenuOpen = ui && ui->IsMenuOpen(RE::MagicMenu::MENU_NAME);

		for (auto* input = *a_event; input; input = input->next) {
			if (input->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			const auto* button = input->AsButtonEvent();
			if (!button || input->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) {
				continue;
			}

			const auto keyCode = static_cast<std::uint32_t>(button->GetIDCode());
			if (keyCode == config.uiToggleKey) {
				manager->ToggleUI();
				continue;
			}

			if (magicMenuOpen && keyCode == config.bindKey) {
				const bool cycleModifier = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
				if (cycleModifier) {
					manager->CycleBindSlotMode();
				} else {
					manager->TryBindSelectedMagicMenuSpell();
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
