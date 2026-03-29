#include "hook/MainHook.h"

#include "hook/FunctionHook.h"
#include "hook/InputHook.h"
#include "item/ItemOrganizerManager.h"

#include "util/LogUtil.h"

namespace HOOK::MAIN {

	namespace
	{
		template <class MenuT>
		void FilterMenu(MenuT* a_menu)
		{
			if (!a_menu) {
				return;
			}

			auto* manager = MITHRAS::ITEM_ORGANIZER::Manager::GetSingleton();
			const auto cfg = manager->GetConfig();
			if (!cfg.enabled) {
				return;
			}

			auto& runtime = a_menu->GetRuntimeData();
			if (runtime.itemList) {
				manager->RefreshVisibleItemList(runtime.itemList);
			}
		}

		template <class MenuT>
		RE::UI_MESSAGE_RESULTS FilterAfterMessage(MenuT* a_menu, RE::UIMessage& a_message, const auto& a_func)
		{
			const auto result = a_func(a_menu, a_message);
			switch (a_message.type.get()) {
			case RE::UI_MESSAGE_TYPE::kShow:
			case RE::UI_MESSAGE_TYPE::kReshow:
			case RE::UI_MESSAGE_TYPE::kUpdate:
			case RE::UI_MESSAGE_TYPE::kInventoryUpdate:
				FilterMenu(a_menu);
				break;
			default:
				break;
			}
			return result;
		}

		template <class MenuT>
		void FilterAfterDisplay(MenuT* a_menu, const auto& a_func)
		{
			a_func(a_menu);
			FilterMenu(a_menu);
		}

		struct InventoryMenu_ProcessMessage
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::InventoryMenu* a_this, RE::UIMessage& a_message)
			{
				return FilterAfterMessage(a_this, a_message, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct InventoryMenu_PostDisplay
		{
			static void thunk(RE::InventoryMenu* a_this)
			{
				FilterAfterDisplay(a_this, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct ContainerMenu_ProcessMessage
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::ContainerMenu* a_this, RE::UIMessage& a_message)
			{
				return FilterAfterMessage(a_this, a_message, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct ContainerMenu_PostDisplay
		{
			static void thunk(RE::ContainerMenu* a_this)
			{
				FilterAfterDisplay(a_this, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct BarterMenu_ProcessMessage
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::BarterMenu* a_this, RE::UIMessage& a_message)
			{
				return FilterAfterMessage(a_this, a_message, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct BarterMenu_PostDisplay
		{
			static void thunk(RE::BarterMenu* a_this)
			{
				FilterAfterDisplay(a_this, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GiftMenu_ProcessMessage
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::GiftMenu* a_this, RE::UIMessage& a_message)
			{
				return FilterAfterMessage(a_this, a_message, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GiftMenu_PostDisplay
		{
			static void thunk(RE::GiftMenu* a_this)
			{
				FilterAfterDisplay(a_this, func);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Install()
	{
		LOG_INFO("Hooks initializing...");
		// Setup Variables
		HOOK::VARIABLE::fDeltaWorldTime = reinterpret_cast<const float*>(RELOCATION_ID(523660, 410199).address());

		HOOK::INPUT::Install();
		stl::Hook::virtual_function<RE::InventoryMenu, InventoryMenu_ProcessMessage>(0, 4);
		stl::Hook::virtual_function<RE::InventoryMenu, InventoryMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked InventoryMenu::ProcessMessage");
		stl::Hook::virtual_function<RE::ContainerMenu, ContainerMenu_ProcessMessage>(0, 4);
		stl::Hook::virtual_function<RE::ContainerMenu, ContainerMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked ContainerMenu::ProcessMessage");
		stl::Hook::virtual_function<RE::BarterMenu, BarterMenu_ProcessMessage>(0, 4);
		stl::Hook::virtual_function<RE::BarterMenu, BarterMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked BarterMenu::ProcessMessage");
		stl::Hook::virtual_function<RE::GiftMenu, GiftMenu_ProcessMessage>(0, 4);
		stl::Hook::virtual_function<RE::GiftMenu, GiftMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked GiftMenu::ProcessMessage");
	}
}
