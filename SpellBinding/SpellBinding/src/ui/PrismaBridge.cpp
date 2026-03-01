#include "ui/PrismaBridge.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

#include "json/single_include/nlohmann/json.hpp"

#include <format>

namespace UI::PRISMA
{
	namespace
	{
		constexpr const char* kViewPath = "SpellBinding/index.html";
	}

	void Bridge::Initialize()
	{
		if (!m_api) {
			m_api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
				PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		}

		if (!m_api) {
			LOG_WARN("SpellBinding: PrismaUI API unavailable, menu UI disabled");
			return;
		}

		EnsureViewCreated();
	}

	void Bridge::EnsureViewCreated()
	{
		if (!m_api) {
			return;
		}

		if (m_view != 0 && m_api->IsValid(m_view)) {
			return;
		}

		m_view = m_api->CreateView(kViewPath, [](PrismaView) -> void {
			auto* bridge = Bridge::GetSingleton();
			if (bridge) {
				bridge->RegisterListeners();
				SBIND::Manager::GetSingleton()->PushUISnapshot();
			}
		});
		if (m_view == 0) {
			LOG_WARN("SpellBinding: failed to create Prisma view");
			return;
		}

		m_api->Hide(m_view);
		m_api->SetOrder(m_view, 20);
		LOG_INFO("SpellBinding: Prisma view created");
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

		m_api->RegisterJSListener(m_view, "sb_bind_selected_magic_menu_spell", [](const char*) -> void {
			SBIND::Manager::GetSingleton()->TryBindSelectedMagicMenuSpell();
		});

		m_api->RegisterJSListener(m_view, "sb_unbind_weapon", [](const char* payload) -> void {
			SBIND::Manager::GetSingleton()->UnbindWeaponFromSerializedKey(payload ? payload : "");
		});

		m_api->RegisterJSListener(m_view, "sb_set_setting", [](const char* payload) -> void {
			if (!payload || payload[0] == '\0') {
				return;
			}
			try {
				const auto parsed = nlohmann::json::parse(payload);
				auto config = SBIND::Manager::GetSingleton()->GetConfig();
				const auto id = parsed.value("id", std::string{});
				if (id == "enabled") {
					config.enabled = parsed.value("value", config.enabled);
				} else if (id == "showHudNotifications") {
					config.showHudNotifications = parsed.value("value", config.showHudNotifications);
				} else if (id == "powerDamageScale") {
					config.powerDamageScale = parsed.value("value", config.powerDamageScale);
				} else if (id == "powerMagickaScale") {
					config.powerMagickaScale = parsed.value("value", config.powerMagickaScale);
				}
				SBIND::Manager::GetSingleton()->SetConfig(config, true);
			} catch (...) {}
		});

		m_listenersRegistered = true;
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
}
