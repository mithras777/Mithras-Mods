#include "ui/PrismaBridge.h"

#include "overhaul/SpellbladeOverhaulManager.h"
#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"
#include "util/StringUtil.h"

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

		if (m_viewReady && m_view != 0) {
			return;
		}

		m_view = m_api->CreateView(kMenuViewPath, [](PrismaView view) -> void {
			auto* bridge = Bridge::GetSingleton();
			if (bridge) {
				bridge->RegisterListeners(view);
				bridge->m_viewReady = true;
				bridge->PushSnapshot(view, SB_OVERHAUL::Manager::GetSingleton()->BuildSnapshotJson());
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

		if (m_hudViewReady && m_hudView != 0) {
			return;
		}

		m_hudView = m_api->CreateView(kHudViewPath, [](PrismaView view) -> void {
			auto* bridge = Bridge::GetSingleton();
			if (bridge) {
				bridge->RegisterHUDListeners(view);
				bridge->m_hudViewReady = true;
			}
		});
		if (m_hudView == 0) {
			LOG_WARN("SpellBinding: failed to create Prisma HUD view");
			return;
		}

		m_api->SetOrder(m_hudView, 40);
		LOG_INFO("SpellBinding: Prisma HUD view created");
	}

	void Bridge::RegisterListeners(PrismaView a_view)
	{
		if (!m_api || a_view == 0 || m_listenersRegistered) {
			return;
		}

		m_api->RegisterJSListener(a_view, "sbo_toggle_ui", [](const char*) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->ToggleUI();
		});

		m_api->RegisterJSListener(a_view, "sbo_request_snapshot", [](const char*) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->PushUISnapshot();
		});

		m_api->RegisterJSListener(a_view, "sbo_set_setting", [](const char* payload) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->HandleSetSetting(payload ? payload : "");
		});

		m_api->RegisterJSListener(a_view, "sbo_action", [](const char* payload) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->HandleAction(payload ? payload : "");
		});

		m_api->RegisterJSListener(a_view, "sbo_hud_action", [](const char* payload) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->HandleHudAction(payload ? payload : "");
		});

		m_listenersRegistered = true;
	}

	void Bridge::RegisterHUDListeners(PrismaView a_view)
	{
		if (!m_api || a_view == 0 || m_hudListenersRegistered) {
			return;
		}

		m_api->RegisterJSListener(a_view, "sbo_hud_drag_update", [](const char* payload) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->HandleHudAction(payload ? payload : "");
		});

		m_api->RegisterJSListener(a_view, "sbo_hud_drag_commit", [](const char* payload) -> void {
			SB_OVERHAUL::Manager::GetSingleton()->HandleHudAction(payload ? payload : "");
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
			SB_OVERHAUL::Manager::GetSingleton()->PushUISnapshot();
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
		if (m_view == 0 || !m_viewReady) {
			return;
		}
		const auto safeJson = UTIL::STRING::SanitizeUtf8(a_json);
		m_api->InteropCall(m_view, "sbo_renderSnapshot", safeJson.c_str());
	}

	void Bridge::PushSnapshot(PrismaView a_view, const std::string& a_json)
	{
		if (!m_api || a_view == 0) {
			return;
		}

		m_api->InteropCall(a_view, "sbo_renderSnapshot", a_json.c_str());
	}

	void Bridge::PushHUDSnapshot(const std::string& a_json)
	{
		if (!m_api) {
			return;
		}
		EnsureHUDViewCreated();
		if (m_hudView == 0 || !m_hudViewReady) {
			return;
		}
		const auto safeJson = UTIL::STRING::SanitizeUtf8(a_json);
		m_api->InteropCall(m_hudView, "sbo_renderHud", safeJson.c_str());
	}

	void Bridge::PushHUDSnapshot(PrismaView a_view, const std::string& a_json)
	{
		if (!m_api || a_view == 0) {
			return;
		}

		m_api->InteropCall(a_view, "sbo_renderHud", a_json.c_str());
	}

	void Bridge::ShowToast(const std::string& a_text)
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_viewReady) {
			return;
		}
		const auto safeText = UTIL::STRING::SanitizeUtf8(a_text);
		m_api->InteropCall(m_view, "sbo_showToast", safeText.c_str());
	}

	void Bridge::SendEscapeToMenu()
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_viewReady) {
			return;
		}
		m_api->InteropCall(m_view, "sbo_native_escape", "");
	}

	void Bridge::SendCloseRequestToMenu()
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_viewReady) {
			return;
		}
		m_api->InteropCall(m_view, "sbo_close_ui", "");
	}

	void Bridge::SetFocusState(bool a_focused)
	{
		if (!m_api) {
			return;
		}
		EnsureViewCreated();
		if (m_view == 0 || !m_viewReady) {
			return;
		}
		const auto json = std::format("{{\"focused\":{}}}", a_focused ? "true" : "false");
		m_api->InteropCall(m_view, "sbo_setFocusState", json.c_str());
	}

	bool Bridge::IsMenuOpen() const
	{
		if (!m_api || m_view == 0 || !m_viewReady) {
			return false;
		}
		return m_api->IsValid(m_view) && !m_api->IsHidden(m_view);
	}

	bool Bridge::IsMenuFocused() const
	{
		if (!m_api || m_view == 0 || !m_viewReady) {
			return false;
		}
		return m_api->HasFocus(m_view);
	}
}
