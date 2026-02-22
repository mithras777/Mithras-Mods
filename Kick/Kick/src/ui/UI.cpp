#include "ui/UI.h"

#include "kick/KickManager.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <vector>

namespace UI
{
	namespace
	{
		struct KeyOption
		{
			std::uint32_t code{ 0 };
			std::string name{};
		};

		std::vector<KeyOption> BuildKeyboardOptions()
		{
			std::vector<KeyOption> out;
			out.reserve(256);
			for (std::uint32_t code = 1; code < 256; ++code) {
				const auto name = MITHRAS::KICK::Manager::GetKeyboardKeyName(code);
				if (name == "Unknown") {
					continue;
				}
				out.push_back({ code, name });
			}
			return out;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SKSE Menu Framework not installed - Kick page unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Kick");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("Registered SKSE Menu Framework section: Kick");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::KICK::Manager::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;
			const auto hotkeyName = MITHRAS::KICK::Manager::GetKeyboardKeyName(cfg.hotkey);
			const auto keyOptions = BuildKeyboardOptions();

			ImGui::Checkbox("Enable Kick", &cfg.enabled);
			ImGui::Checkbox("Debug logging", &cfg.debugLogging);
			ImGui::Text("Hotkey: %s", hotkeyName.c_str());

			if (ImGui::BeginCombo("Keyboard Key", hotkeyName.c_str())) {
				for (const auto& option : keyOptions) {
					const bool selected = (option.code == cfg.hotkey);
					if (ImGui::Selectable(option.name.c_str(), selected)) {
						cfg.hotkey = option.code;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();
			ImGui::SliderFloat("Range", &cfg.range, 64.0f, 600.0f, "%.0f");
			ImGui::SliderFloat("Force", &cfg.force, 50.0f, 3000.0f, "%.0f");
			ImGui::SliderFloat("Upward bias", &cfg.upwardBias, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Cooldown (seconds)", &cfg.cooldownSeconds, 0.0f, 3.0f, "%.2f");
			ImGui::SliderFloat("Ray spread", &cfg.raySpread, 0.0f, 0.8f, "%.2f");

			if (before.enabled != cfg.enabled ||
				before.debugLogging != cfg.debugLogging ||
				before.hotkey != cfg.hotkey ||
				before.range != cfg.range ||
				before.force != cfg.force ||
				before.upwardBias != cfg.upwardBias ||
				before.cooldownSeconds != cfg.cooldownSeconds ||
				before.raySpread != cfg.raySpread) {
				manager->SetConfig(cfg);
			}
		}
	}
}
