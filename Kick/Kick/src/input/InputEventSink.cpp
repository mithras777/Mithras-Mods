#include "input/InputEventSink.h"

#include "kick/KickManager.h"
#include "util/LogUtil.h"

namespace KICK_INPUT
{
	namespace
	{
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
		auto* mgr = RE::BSInputDeviceManager::GetSingleton();
		auto* sink = EventSink::GetSingleton();
		if (!mgr || !sink) {
			LOG_WARN("Kick: Failed to register InputEvent sink");
			return;
		}

		mgr->AddEventSink(sink);
		LOG_INFO("Kick: Registered InputEvent sink");
	}

	RE::BSEventNotifyControl EventSink::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_event || !*a_event || IsInputBlocked()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* manager = MITHRAS::KICK::Manager::GetSingleton();
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
			if (cfg.enabled && keyCode == cfg.hotkey) {
				if (cfg.debugLogging) {
					LOG_INFO("Kick Debug: hotkey press detected key={} (match)", keyCode);
				}
				manager->TryKick();
			} else if (cfg.debugLogging && input->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
				LOG_INFO("Kick Debug: key={} pressed (hotkey={})", keyCode, cfg.hotkey);
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
