#include "ui/UI.h"

#include "perilous/PerilousConfig.h"
#include "perilous/PerilousManager.h"
#include "util/LogUtil.h"
#include <algorithm>
#include <cstdio>

namespace UI
{
	namespace
	{
		bool Changed(const PERILOUS::Config& a_lhs, const PERILOUS::Config& a_rhs)
		{
			return a_lhs.bPerilousAttack_Enable != a_rhs.bPerilousAttack_Enable ||
			       a_lhs.bPerilousAttack_ChargeTime_Enable != a_rhs.bPerilousAttack_ChargeTime_Enable ||
			       a_lhs.fPerilousAttack_ChargeTime_Mult != a_rhs.fPerilousAttack_ChargeTime_Mult ||
			       a_lhs.fPerilousAttack_ChargeTime_Duration != a_rhs.fPerilousAttack_ChargeTime_Duration ||
			       a_lhs.fPerilousAttack_Chance_Mult != a_rhs.fPerilousAttack_Chance_Mult ||
			       a_lhs.iPerilous_MaxAttackersPerTarget != a_rhs.iPerilous_MaxAttackersPerTarget ||
			       a_lhs.bPerilous_VFX_Enable != a_rhs.bPerilous_VFX_Enable ||
			       a_lhs.bPerilous_SFX_Enable != a_rhs.bPerilous_SFX_Enable ||
			       a_lhs.sPerilous_FormsPlugin != a_rhs.sPerilous_FormsPlugin ||
			       a_lhs.iPerilous_RedVfxFormID != a_rhs.iPerilous_RedVfxFormID ||
			       a_lhs.iPerilous_SfxFormID != a_rhs.iPerilous_SfxFormID;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("PerilousAttacks: SKSE Menu Framework not installed - settings UI unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Perilous Attacks");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("PerilousAttacks: registered SKSE Menu Framework panel");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* store = PERILOUS::ConfigStore::GetSingleton();
			auto cfg = store->Get();
			const auto before = cfg;

			ImGui::Checkbox("Enable Perilous Attacks", &cfg.bPerilousAttack_Enable);
			ImGui::Checkbox("Enable Charge Time", &cfg.bPerilousAttack_ChargeTime_Enable);
			ImGui::SliderFloat("Charge Mult", &cfg.fPerilousAttack_ChargeTime_Mult, 0.0F, 5.0F, "%.2f");
			ImGui::SliderFloat("Charge Duration", &cfg.fPerilousAttack_ChargeTime_Duration, 0.0F, 10.0F, "%.2f s");
			ImGui::SliderFloat("Chance Mult", &cfg.fPerilousAttack_Chance_Mult, 0.0F, 10.0F, "%.2f");

			int maxAttackers = static_cast<int>(cfg.iPerilous_MaxAttackersPerTarget);
			if (ImGui::SliderInt("Max Attackers / Target", &maxAttackers, 1, 10)) {
				cfg.iPerilous_MaxAttackersPerTarget = static_cast<std::uint32_t>(maxAttackers);
			}

			ImGui::Separator();
			ImGui::Checkbox("Enable VFX", &cfg.bPerilous_VFX_Enable);
			ImGui::Checkbox("Enable SFX", &cfg.bPerilous_SFX_Enable);

			char pluginBuffer[260]{};
			std::snprintf(pluginBuffer, std::size(pluginBuffer), "%s", cfg.sPerilous_FormsPlugin.c_str());
			if (ImGui::InputText("Forms Plugin", pluginBuffer, std::size(pluginBuffer))) {
				cfg.sPerilous_FormsPlugin = pluginBuffer;
			}

			int redVfxId = static_cast<int>(cfg.iPerilous_RedVfxFormID);
			if (ImGui::InputInt("Red VFX FormID", &redVfxId, 1, 100, ImGuiInputTextFlags_CharsHexadecimal)) {
				cfg.iPerilous_RedVfxFormID = static_cast<RE::FormID>(std::max(redVfxId, 0));
			}

			int sfxId = static_cast<int>(cfg.iPerilous_SfxFormID);
			if (ImGui::InputInt("SFX FormID", &sfxId, 1, 100, ImGuiInputTextFlags_CharsHexadecimal)) {
				cfg.iPerilous_SfxFormID = static_cast<RE::FormID>(std::max(sfxId, 0));
			}

			if (Changed(before, cfg)) {
				store->Set(cfg);
				store->Save();
				PERILOUS::Manager::GetSingleton()->InitForms();
			}
		}
	}
}
