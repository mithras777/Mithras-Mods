#include "ui/UI.h"

#include "diagonal/FakeDiagonalSprint.h"
#include "util/LogUtil.h"

namespace UI
{
	namespace
	{
		constexpr float kMinInputTau = 0.01f;
		constexpr float kMaxInputTau = 0.30f;
		constexpr float kMinDirTau = 0.01f;
		constexpr float kMaxDirTau = 0.30f;

		void ResetGeneralDefaults(DIAGONAL::FakeDiagonalConfig& a_cfg)
		{
			const DIAGONAL::FakeDiagonalConfig defaults{};
			a_cfg.enabled = defaults.enabled;
			a_cfg.firstPersonOnly = defaults.firstPersonOnly;
			a_cfg.requireOnGround = defaults.requireOnGround;
			a_cfg.requireForwardInput = defaults.requireForwardInput;
			a_cfg.disableWhenAttacking = defaults.disableWhenAttacking;
			a_cfg.disableWhenStaggered = defaults.disableWhenStaggered;
			a_cfg.debug = defaults.debug;
		}

		void ResetControlsDefaults(DIAGONAL::FakeDiagonalConfig& a_cfg)
		{
			const DIAGONAL::FakeDiagonalConfig defaults{};
			a_cfg.baseDriftSpeed = defaults.baseDriftSpeed;
			a_cfg.maxLateralSpeed = defaults.maxLateralSpeed;
			a_cfg.inputTau = defaults.inputTau;
			a_cfg.dirTau = defaults.dirTau;
		}

		void ResetScalingDefaults(DIAGONAL::FakeDiagonalConfig& a_cfg)
		{
			const DIAGONAL::FakeDiagonalConfig defaults{};
			a_cfg.speedScaling = defaults.speedScaling;
		}

		bool ConfigChanged(const DIAGONAL::FakeDiagonalConfig& a_lhs, const DIAGONAL::FakeDiagonalConfig& a_rhs)
		{
			return a_lhs.enabled != a_rhs.enabled ||
			       a_lhs.firstPersonOnly != a_rhs.firstPersonOnly ||
			       a_lhs.requireOnGround != a_rhs.requireOnGround ||
			       a_lhs.requireForwardInput != a_rhs.requireForwardInput ||
			       a_lhs.baseDriftSpeed != a_rhs.baseDriftSpeed ||
			       a_lhs.maxLateralSpeed != a_rhs.maxLateralSpeed ||
			       a_lhs.inputTau != a_rhs.inputTau ||
			       a_lhs.dirTau != a_rhs.dirTau ||
			       a_lhs.speedScaling.enabled != a_rhs.speedScaling.enabled ||
			       a_lhs.speedScaling.sprintSpeedRef != a_rhs.speedScaling.sprintSpeedRef ||
			       a_lhs.speedScaling.minScale != a_rhs.speedScaling.minScale ||
			       a_lhs.speedScaling.maxScale != a_rhs.speedScaling.maxScale ||
			       a_lhs.disableWhenAttacking != a_rhs.disableWhenAttacking ||
			       a_lhs.disableWhenStaggered != a_rhs.disableWhenStaggered ||
			       a_lhs.debug != a_rhs.debug;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("DiagonalSprint: SKSE Menu Framework not installed - MCM unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Diagonal Sprint");
		SKSEMenuFramework::AddSectionItem("Fake Diagonal Sprint", MainPanel::Render);
		LOG_INFO("DiagonalSprint: registered MCM panel");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = DIAGONAL::FakeDiagonalSprint::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;

			if (ImGui::BeginTabBar("DiagonalSprintTabs")) {
				if (ImGui::BeginTabItem("General")) {
					if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Checkbox("Enabled", &cfg.enabled);
						ImGui::Checkbox("First Person Only", &cfg.firstPersonOnly);
						ImGui::Checkbox("Require On Ground", &cfg.requireOnGround);
						ImGui::Checkbox("Require Forward Input", &cfg.requireForwardInput);
						ImGui::Checkbox("Disable While Attacking", &cfg.disableWhenAttacking);
						ImGui::Checkbox("Disable While Staggered", &cfg.disableWhenStaggered);
						ImGui::Checkbox("Debug Logging", &cfg.debug);
						ImGui::Spacing();
						if (ImGui::Button("Defaults##General")) {
							ResetGeneralDefaults(cfg);
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Controls")) {
					if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::SliderFloat("Base Drift Speed", &cfg.baseDriftSpeed, 0.0f, 250.0f, "%.1f");
						ImGui::SliderFloat("Max Lateral Speed", &cfg.maxLateralSpeed, 10.0f, 300.0f, "%.1f");
						ImGui::SliderFloat("Input Tau", &cfg.inputTau, kMinInputTau, kMaxInputTau, "%.3f");
						ImGui::SliderFloat("Direction Tau", &cfg.dirTau, kMinDirTau, kMaxDirTau, "%.3f");
						ImGui::Spacing();
						if (ImGui::Button("Defaults##Controls")) {
							ResetControlsDefaults(cfg);
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Scaling")) {
					if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Checkbox("Enable Speed Scaling", &cfg.speedScaling.enabled);
						ImGui::SliderFloat("Sprint Speed Ref", &cfg.speedScaling.sprintSpeedRef, 100.0f, 600.0f, "%.1f");
						ImGui::SliderFloat("Min Scale", &cfg.speedScaling.minScale, 0.1f, 2.0f, "%.2f");
						ImGui::SliderFloat("Max Scale", &cfg.speedScaling.maxScale, 0.1f, 2.0f, "%.2f");
						ImGui::Spacing();
						if (ImGui::Button("Defaults##Scaling")) {
							ResetScalingDefaults(cfg);
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			if (ConfigChanged(before, cfg)) {
				manager->SetConfig(cfg, true);
			}
		}
	}
}
