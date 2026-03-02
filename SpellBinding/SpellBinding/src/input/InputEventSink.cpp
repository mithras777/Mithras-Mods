#include "input/InputEventSink.h"

#include "spellbinding/SpellBindingManager.h"
#include "ui/PrismaBridge.h"
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

		bool IsModifierDown(std::uint32_t a_keyCode)
		{
			switch (a_keyCode) {
				case 0x2A:
				case 0x36:
					return (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
				case 0x1D:
					return (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
				case 0x38:
					return (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
				default:
					return false;
			}
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
		auto* prismaBridge = UI::PRISMA::Bridge::GetSingleton();
		const bool magicMenuOpen = ui && ui->IsMenuOpen(RE::MagicMenu::MENU_NAME);
		const bool prismaMenuOpen = prismaBridge->IsMenuOpen();
		const bool prismaMenuFocused = prismaBridge->IsMenuFocused();
		bool consumeEvent = false;

		for (auto* input = *a_event; input; input = input->next) {
			if (input->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			const auto* button = input->AsButtonEvent();
			if (!button || input->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) {
				continue;
			}

			const auto keyCode = static_cast<std::uint32_t>(button->GetIDCode());
			const auto userEvent = input->QUserEvent();
			if (button->IsDown()) {
				auto* userEvents = RE::UserEvents::GetSingleton();
				if (userEvents && userEvent == userEvents->attackPowerStart) {
					manager->OnPowerAttackInputStart();
				}
			}

			if (keyCode == 0x01 && prismaMenuOpen && !prismaMenuFocused && !magicMenuOpen) {
				manager->ToggleUI();
				consumeEvent = true;
				continue;
			}

			if (keyCode == config.uiToggleKey) {
				manager->ToggleUI();
				continue;
			}

			if (keyCode == config.bindKey) {
				if (IsModifierDown(config.cycleSlotModifierKey) && (magicMenuOpen || prismaMenuOpen)) {
					manager->CycleBindSlotMode();
					consumeEvent = magicMenuOpen;
					continue;
				}
				if (magicMenuOpen) {
					manager->TryBindSelectedMagicMenuSpell();
					consumeEvent = true;
				}
			}
		}

		return consumeEvent ? RE::BSEventNotifyControl::kStop : RE::BSEventNotifyControl::kContinue;
	}
}
