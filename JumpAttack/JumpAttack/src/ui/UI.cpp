#include "ui/UI.h"

#include "jumpattack/Settings.h"
#include "util/LogUtil.h"

namespace UI
{
    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            LOG_WARN("SKSE Menu Framework not installed - JumpAttacksNoBehaviors page unavailable");
            return;
        }

        SKSEMenuFramework::SetSection("Jump Attacks");
        SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
        LOG_INFO("Registered SKSE Menu Framework section: Jump Attacks");
    }

    namespace MainPanel
    {
        void __stdcall Render()
        {
            auto* settings = JA::Settings::GetSingleton();
            auto cfg = settings->Get();
            const auto before = cfg;

            if (ImGui::BeginTabBar("JumpAttackTabs")) {
                if (ImGui::BeginTabItem("General")) {
                    int raysCount = static_cast<int>(cfg.raysCount);
                    int swingDelay = static_cast<int>(cfg.swingDelayMs);
                    int cooldown = static_cast<int>(cfg.cooldownMs);
                    int landingGrace = static_cast<int>(cfg.landingGraceMs);

                    ImGui::Checkbox("Enable", &cfg.enable);
                    ImGui::Checkbox("Debug Log", &cfg.debugLog);

                    ImGui::SliderFloat("Range", &cfg.range, 50.0f, 400.0f, "%.0f");
                    ImGui::SliderInt("Rays Count", &raysCount, 1, 9);
                    ImGui::SliderFloat("Cone Angle (deg)", &cfg.coneAngleDegrees, 0.0f, 25.0f, "%.1f");

                    ImGui::SliderInt("Swing Delay (ms)", &swingDelay, 0, 500);
                    ImGui::SliderInt("Cooldown (ms)", &cooldown, 50, 2000);
                    ImGui::SliderInt("Landing Grace (ms)", &landingGrace, 0, 500);

                    cfg.raysCount = static_cast<std::uint32_t>(raysCount);
                    cfg.swingDelayMs = static_cast<std::uint32_t>(swingDelay);
                    cfg.cooldownMs = static_cast<std::uint32_t>(cooldown);
                    cfg.landingGraceMs = static_cast<std::uint32_t>(landingGrace);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Combat")) {
                    ImGui::Checkbox("Allow Through Actors Only", &cfg.allowThroughActorsOnly);
                    ImGui::Checkbox("Allow If Bow Drawn", &cfg.allowIfBowDrawn);
                    ImGui::Checkbox("Allow If Spell Equipped", &cfg.allowIfSpellEquipped);

                    ImGui::Separator();
                    ImGui::Checkbox("Apply Stagger", &cfg.applyStagger);
                    ImGui::SliderFloat("Stagger Magnitude", &cfg.staggerMagnitude, 0.0f, 2.0f, "%.2f");

                    if (ImGui::Button("Defaults")) {
                        cfg = JA::SettingsData{};
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            if (before.enable != cfg.enable ||
                before.range != cfg.range ||
                before.raysCount != cfg.raysCount ||
                before.coneAngleDegrees != cfg.coneAngleDegrees ||
                before.swingDelayMs != cfg.swingDelayMs ||
                before.cooldownMs != cfg.cooldownMs ||
                before.allowThroughActorsOnly != cfg.allowThroughActorsOnly ||
                before.allowIfBowDrawn != cfg.allowIfBowDrawn ||
                before.allowIfSpellEquipped != cfg.allowIfSpellEquipped ||
                before.applyStagger != cfg.applyStagger ||
                before.staggerMagnitude != cfg.staggerMagnitude ||
                before.landingGraceMs != cfg.landingGraceMs ||
                before.debugLog != cfg.debugLog) {
                settings->Set(cfg, true);
            }
        }
    }
}
