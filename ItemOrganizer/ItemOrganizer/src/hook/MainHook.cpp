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
				manager->ApplyFilter(runtime.itemList);
			}
		}

		struct InventoryMenu_PostDisplay
		{
			static void thunk(RE::InventoryMenu* a_this)
			{
				func(a_this);
				FilterMenu(a_this);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct ContainerMenu_PostDisplay
		{
			static void thunk(RE::ContainerMenu* a_this)
			{
				func(a_this);
				FilterMenu(a_this);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct BarterMenu_PostDisplay
		{
			static void thunk(RE::BarterMenu* a_this)
			{
				func(a_this);
				FilterMenu(a_this);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GiftMenu_PostDisplay
		{
			static void thunk(RE::GiftMenu* a_this)
			{
				func(a_this);
				FilterMenu(a_this);
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
		stl::Hook::virtual_function<RE::InventoryMenu, InventoryMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked InventoryMenu::PostDisplay");
		stl::Hook::virtual_function<RE::ContainerMenu, ContainerMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked ContainerMenu::PostDisplay");
		stl::Hook::virtual_function<RE::BarterMenu, BarterMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked BarterMenu::PostDisplay");
		stl::Hook::virtual_function<RE::GiftMenu, GiftMenu_PostDisplay>(0, 6);
		LOG_INFO("Hooked GiftMenu::PostDisplay");
	}
}
