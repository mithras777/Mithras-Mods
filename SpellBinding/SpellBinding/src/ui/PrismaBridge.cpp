#include "ui/PrismaBridge.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

#include <format>

namespace UI::PRISMA
{
	namespace
	{
		constexpr const char* kMenuViewPath = "SpellBinding/index.html";
		constexpr const char* kHudViewPath = "SpellBinding/hud.html";
	}

	void Bridge::Initialize()
	{
		if (!m_api) {
			m_api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
				PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		}

		if (!m_api) {
			LOG_WARN("SpellBinding: PrismaUI API unavailable, UI disabled");
			return;
		}

		EnsureViewCreated();
		EnsureHUDViewCreated();
	}

	void Bridge::EnsureViewCreated()
	{
		if (!m_api) {
			return;
		}

		if (m_view != 0 && m_api->IsValid(m_view)) {
			return;
		}

		m_view = m_api->CreateView(kMenuViewPath, [](PrismaView) -> void {
			auto* bridge = Bridge::GetSingleton();
			if (bridge) {
				bridge->RegisterListeners();
				SBIND::Manager::GetSingleton()->PushUISnapshot();
			}
		});
		if (m_view == 0) {
			LOG_WARN("SpellBinding: failed to create Prisma menu view");
			return;
		}

		m_api->Hide(m_view);
		m_api->SetOrder(m_view, 20);
		LOG_INFO("SpellBinding: Prisma menu view created");
	}

	void Bridge::EnsureHUDViewCreated()
	{
		if (!m_api) {
			return;
		}

		if (m_hudView != 0 && m_api->IsValid(m_hudView)) {
			return;
		}

		m_hudView = m_api->CreateView(kHudViewPath, [](PrismaView) -> void {
			auto* bridge = Bridge::GetSingleton();
			if (bridge) {
				bridge->RegisterHUDListeners();
				SBIND::Manager::GetSingleton()->PushHUDSnapshot();
			}
		});
		if (m_hudView == 0) {
			LOG_WARN("SpellBinding: failed to create Prisma HUD view");
			return;
		}

		m_api->Show(m_hudView);
		m_api->SetOrder(m_hudView, 40);
		LOG_INFO("SpellBinding: Prisma HUD view created");
	}

	void Bridge::RegisterListeners()
	{
		if (!m_api || m_view == 0 || m_listenersRegistered) {
			return;
		}

		m_api->RegisterJSListener(m_view, "sb_toggle_ui", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->ToggleUI();
		});

		m_api->RegisterJSListener(m_view, "sb_request_snapshot", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->PushUISnapshot();
		});

		m_api->RegisterJSListener(m_view, "sb_cycle_bind_slot", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->CycleBindSlotMode();
		});

		m_api->RegisterJSListener(m_view, "sb_bind_selected_magic_menu_spell", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->TryBindSelectedMagicMenuSpell();
		});

		m_api->RegisterJSListener(m_view, "sb_bind_spell_for_slot", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->BindSpellForSlotFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_unbind_slot", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->UnbindSlotFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_unbind_weapon", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->UnbindWeaponFromSerializedKey(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_set_setting", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SetSettingFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_set_weapon_setting", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SetWeaponSettingFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_set_blacklist", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SetBlacklistFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_enter_hud_drag_mode", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->EnterHudDragMode();
			SBIND::Manager::GetSingleton()->PushHUDSnapshot();
		});

		m_api->RegisterJSListener(m_view, "sb_save_hud_position", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SaveHudPositionFromJson(payload ? payload : "");
		});

		m_listenersRegistered = true;
	}

	void Bridge::RegisterHUDListeners()
	{
		if (!m_api || m_hudView == 0 || m_hudListenersRegistered) {
			return;
		}

		m_api->RegisterJSListener(m_hudView, "sb_hud_drag_update", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SaveHudPositionFromJson(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_hudView, "sb_hud_drag_commit", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->SaveHudPositionFromJson(payload ? payload : "");
		});

		m_hudListenersRegistered = true;
	}

	void Bridge::Toggle()
	{
		if (!m_api) {
			Initialize();
		}
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		EnsureHUDViewCreated();
		if (m_view == 0 || !m_api->IsValid(m_view)) {
			return;
		}

		if (m_api->IsHidden(m_view)) {
			m_api->Show(m_view);
			const bool focused = m_api->Focus(m_view, false, false);
			SetFocusState(focused);
			SBIND::Manager::GetSingleton()->PushUISnapshot();
		} else {
			m_api->Hide(m_view);
			SetFocusState(false);
		}
	}

	void Bridge::PushSnapshot(const std::string& a_json)
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_api->IsValid(m_view)) {
			return;
		}
		m_api->InteropCall(m_view, "sb_renderSnapshot", a_json.c_str());
	}

	void Bridge::PushHUDSnapshot(const std::string& a_json)
	{
		if (!m_api) {
			return;
		}
		EnsureHUDViewCreated();
		if (m_hudView == 0 || !m_api->IsValid(m_hudView)) {
			return;
		}
		m_api->InteropCall(m_hudView, "sb_renderHud", a_json.c_str());
	}

	void Bridge::ShowToast(const std::string& a_text)
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_api->IsValid(m_view)) {
			return;
		}
		m_api->InteropCall(m_view, "sb_showToast", a_text.c_str());
	}

	void Bridge::SetFocusState(bool a_focused)
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_api->IsValid(m_view)) {
			return;
		}
		const auto json = std::format("{{\"focused\":{}}}", a_focused ? "true" : "false");
		m_api->InteropCall(m_view, "sb_setFocusState", json.c_str());
	}

	bool Bridge::IsMenuOpen() const
	{
		if (!m_api || m_view == 0 || !m_api->IsValid(m_view)) {
			return false;
		}
		return !m_api->IsHidden(m_view);
	}

	bool Bridge::IsMenuFocused() const
	{
		if (!m_api || m_view == 0 || !m_api->IsValid(m_view)) {
			return false;
		}
		return m_api->HasFocus(m_view);
	}
}
