#include "input/InputEventSink.h"

#include "combat/DirectionalConfig.h"
#include "combat/DirectionalController.h"
#include "util/LogUtil.h"

namespace MC::INPUT
{
	namespace
	{
		bool g_registered = false;
	}

	void EventSink::Register()
	{
		if (g_registered) {
			return;
		}

		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		auto* sink = EventSink::GetSingleton();
		if (!inputMgr || !sink) {
			LOG_WARN("MordhauCombat: failed to register InputEvent sink");
			return;
		}

		inputMgr->AddEventSink(sink);
		g_registered = true;
		LOG_INFO("MordhauCombat: InputEvent sink registered");
	}

	RE::BSEventNotifyControl EventSink::ProcessEvent(RE::InputEvent* const* a_event,
	                                                 RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_event || !*a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* controller = MC::DIRECTIONAL::Controller::GetSingleton();
		auto* userEvents = RE::UserEvents::GetSingleton();
		const auto toggleKey = MC::DIRECTIONAL::Config::GetSingleton()->GetData().toggleKey;

		for (auto* input = *a_event; input; input = input->next) {
			if (input->GetEventType() == RE::INPUT_EVENT_TYPE::kMouseMove) {
				auto* mouse = input->AsMouseMoveEvent();
				if (mouse) {
					controller->OnMouseDelta(mouse->mouseInputX, mouse->mouseInputY);
				}
				continue;
			}

			if (input->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
				continue;
			}

			auto* button = input->AsButtonEvent();
			if (!button || !button->IsDown()) {
				continue;
			}

			const auto eventName = button->QUserEvent();
			if (userEvents) {
				if (eventName == userEvents->attackStart || eventName == userEvents->rightAttack) {
					controller->OnAttackPressed(false);
				}
				if (eventName == userEvents->attackPowerStart) {
					controller->OnAttackPressed(true);
				}
			}

			if (input->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
				if (button->GetIDCode() == toggleKey) {
					controller->ToggleEnabled();
					const bool enabled = controller->IsEnabled();
					RE::SendHUDMessage::ShowHUDMessage(enabled ? "MordhauCombat: enabled" : "MordhauCombat: disabled");
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
