#include "ui/UI.h"

#include "game/Assassination.h"
#include "util/LogUtil.h"

#include "SKSEMenuFramework/SKSEMenuFramework.h"

namespace UI {
	namespace {
		[[nodiscard]] bool ConfigChanged(const GAME::ASSASSINATION::Config& a_lhs, const GAME::ASSASSINATION::Config& a_rhs)
		{
			return a_lhs.enabled != a_rhs.enabled ||
			       a_lhs.maxLevelDifference != a_rhs.maxLevelDifference ||
			       a_lhs.melee != a_rhs.melee ||
			       a_lhs.unarmed != a_rhs.unarmed ||
			       a_lhs.bowsCrossbows != a_rhs.bowsCrossbows;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("AssassinationSKSE: SKSE Menu Framework not installed - MCM unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Assassination SKSE");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("AssassinationSKSE: registered MCM panel");
	}

	namespace MainPanel {
		void __stdcall Render()
		{
			auto cfg = GAME::ASSASSINATION::GetConfig();
			const auto before = cfg;

			if (ImGui::BeginTabBar("AssassinationTabs")) {
				if (ImGui::BeginTabItem("General")) {
					if (ImGui::CollapsingHeader("Assassination", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Checkbox("Enabled", &cfg.enabled);
						ImGui::TextDisabled("When disabled, the hit listener remains active but will never execute targets.");
						ImGui::SliderInt("Max Level Difference", &cfg.maxLevelDifference, 0, 100);
						ImGui::TextDisabled("0 = target must be your level or lower. 100 = target can be up to 100 levels above you.");
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Attack Types")) {
					if (ImGui::CollapsingHeader("Allowed attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Checkbox("Melee", &cfg.melee);
						ImGui::Checkbox("Unarmed", &cfg.unarmed);
						ImGui::Checkbox("Bows / Crossbows", &cfg.bowsCrossbows);
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			if (ConfigChanged(before, cfg)) {
				GAME::ASSASSINATION::SetConfig(cfg);
			}
		}
	}
}
