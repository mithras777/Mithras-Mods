#include "ui/UI.h"

#include "kick/KickManager.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <vector>

namespace UI
{
	namespace
	{
		constexpr float kDefaultObjectForce = 450.0f;
		constexpr float kDefaultObjectUpwardBias = 0.25f;
		constexpr float kDefaultObjectRange = 220.0f;
		constexpr float kDefaultObjectCooldown = 0.35f;
		constexpr float kDefaultObjectRaySpread = 0.20f;
		constexpr float kDefaultObjectStaminaCost = 0.0f;
		constexpr float kDefaultNPCForce = 1200.0f;
		constexpr float kDefaultNPCUpwardBias = 0.0f;
		constexpr float kDefaultNPCRange = 220.0f;
		constexpr float kDefaultNPCCooldown = 0.35f;
		constexpr float kDefaultNPCRaySpread = 0.20f;
		constexpr float kDefaultNPCStaminaCost = 0.0f;
		constexpr float kDefaultNPCStaminaDrain = 0.0f;
		constexpr float kDefaultNPCDamageFlat = 10.0f;
		constexpr float kDefaultNPCDamagePercent = 0.0f;
		constexpr bool kDefaultGuardBreakKick = false;

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

			if (ImGui::BeginTabBar("KickTabs")) {
				if (ImGui::BeginTabItem("General")) {
					ImGui::Checkbox("Enable Kick", &cfg.enabled);
					ImGui::Checkbox("Guard Break Kick", &cfg.guardBreakKick);

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

					if (ImGui::Button("Defaults##General")) {
						cfg.guardBreakKick = kDefaultGuardBreakKick;
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Objects")) {
					if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::SliderFloat("Object Range", &cfg.objectRange, 64.0f, 600.0f, "%.0f");
						ImGui::SliderFloat("Object Force", &cfg.objectForce, 50.0f, 3000.0f, "%.0f");
						ImGui::SliderFloat("Object Upward bias", &cfg.objectUpwardBias, 0.0f, 1.0f, "%.2f");
						ImGui::SliderFloat("Object Cooldown (seconds)", &cfg.objectCooldownSeconds, 0.0f, 3.0f, "%.2f");
						ImGui::SliderFloat("Object Ray spread", &cfg.objectRaySpread, 0.0f, 0.8f, "%.2f");
						ImGui::SliderFloat("Object Stamina Cost", &cfg.objectStaminaCost, 0.0f, 500.0f, "%.0f");

						if (ImGui::Button("Defaults##Objects")) {
							cfg.objectRange = kDefaultObjectRange;
							cfg.objectForce = kDefaultObjectForce;
							cfg.objectUpwardBias = kDefaultObjectUpwardBias;
							cfg.objectCooldownSeconds = kDefaultObjectCooldown;
							cfg.objectRaySpread = kDefaultObjectRaySpread;
							cfg.objectStaminaCost = kDefaultObjectStaminaCost;
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("NPCs")) {
					if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::SliderFloat("NPC Range", &cfg.npcRange, 64.0f, 600.0f, "%.0f");
						ImGui::SliderFloat("NPC Force", &cfg.npcForce, 50.0f, 3000.0f, "%.0f");
						ImGui::SliderFloat("NPC Upward bias", &cfg.npcUpwardBias, 0.0f, 1.0f, "%.2f");
						ImGui::SliderFloat("NPC Cooldown (seconds)", &cfg.npcCooldownSeconds, 0.0f, 3.0f, "%.2f");
						ImGui::SliderFloat("NPC Ray spread", &cfg.npcRaySpread, 0.0f, 0.8f, "%.2f");
						ImGui::SliderFloat("NPC Stamina Cost", &cfg.npcStaminaCost, 0.0f, 500.0f, "%.0f");
						ImGui::SliderFloat("NPC Stamina Drain", &cfg.npcStaminaDrain, 0.0f, 500.0f, "%.0f");
						ImGui::SliderFloat("NPC Damage (Flat)", &cfg.npcDamageFlat, 0.0f, 500.0f, "%.0f");
						ImGui::SliderFloat("NPC Damage (%)", &cfg.npcDamagePercent, 0.0f, 100.0f, "%.1f%%");
						ImGui::Checkbox("Guard Break Kick", &cfg.guardBreakKick);

						if (ImGui::Button("Defaults##NPCs")) {
							cfg.npcRange = kDefaultNPCRange;
							cfg.npcForce = kDefaultNPCForce;
							cfg.npcUpwardBias = kDefaultNPCUpwardBias;
							cfg.npcCooldownSeconds = kDefaultNPCCooldown;
							cfg.npcRaySpread = kDefaultNPCRaySpread;
							cfg.npcStaminaCost = kDefaultNPCStaminaCost;
							cfg.npcStaminaDrain = kDefaultNPCStaminaDrain;
							cfg.npcDamageFlat = kDefaultNPCDamageFlat;
							cfg.npcDamagePercent = kDefaultNPCDamagePercent;
							cfg.guardBreakKick = kDefaultGuardBreakKick;
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			if (before.enabled != cfg.enabled ||
				before.hotkey != cfg.hotkey ||
				before.objectRange != cfg.objectRange ||
				before.objectForce != cfg.objectForce ||
				before.objectUpwardBias != cfg.objectUpwardBias ||
				before.objectCooldownSeconds != cfg.objectCooldownSeconds ||
				before.objectRaySpread != cfg.objectRaySpread ||
				before.objectStaminaCost != cfg.objectStaminaCost ||
				before.npcRange != cfg.npcRange ||
				before.npcForce != cfg.npcForce ||
				before.npcUpwardBias != cfg.npcUpwardBias ||
				before.npcCooldownSeconds != cfg.npcCooldownSeconds ||
				before.npcRaySpread != cfg.npcRaySpread ||
				before.npcStaminaCost != cfg.npcStaminaCost ||
				before.npcStaminaDrain != cfg.npcStaminaDrain ||
				before.npcDamageFlat != cfg.npcDamageFlat ||
				before.npcDamagePercent != cfg.npcDamagePercent ||
				before.guardBreakKick != cfg.guardBreakKick) {
				manager->SetConfig(cfg);
			}
		}
	}
}
