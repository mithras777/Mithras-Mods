#include "ui/UI.h"

#include "buff/QuickBuffManager.h"
#include "util/LogUtil.h"

#include <array>
#include <format>

#if QUICKBUFF_HAS_MENU_FRAMEWORK
namespace UI
{
	namespace
	{
		struct TriggerUIDef
		{
			QUICK_BUFF::TriggerID id;
			const char* title;
		};

		constexpr std::array<TriggerUIDef, static_cast<std::size_t>(QUICK_BUFF::TriggerID::kTotal)> kTriggerDefs{ {
			{ QUICK_BUFF::TriggerID::kCombatStart, "Combat Start" },
			{ QUICK_BUFF::TriggerID::kCombatEnd, "Combat End" },
			{ QUICK_BUFF::TriggerID::kHealthBelow, "Health Drops Below Threshold" },
			{ QUICK_BUFF::TriggerID::kCrouchStart, "Crouch Start" },
			{ QUICK_BUFF::TriggerID::kSprintStart, "Sprint Start" },
			{ QUICK_BUFF::TriggerID::kWeaponDraw, "Weapon Draw" },
			{ QUICK_BUFF::TriggerID::kPowerAttackStart, "Power Attack Start" },
			{ QUICK_BUFF::TriggerID::kShoutStart, "Shout Start" }
		} };

		bool DrawSpellCombo(const char* a_label, std::string& a_selectedKey, const std::vector<QUICK_BUFF::KnownSpellOption>& a_spells)
		{
			std::string currentLabel = "<None>";
			for (const auto& spell : a_spells) {
				if (spell.formKey == a_selectedKey) {
					currentLabel = spell.name;
					break;
				}
			}

			bool changed = false;
			if (ImGui::BeginCombo(a_label, currentLabel.c_str())) {
				const bool selectedNone = a_selectedKey.empty();
				if (ImGui::Selectable("<None>", selectedNone) && !selectedNone) {
					a_selectedKey.clear();
					changed = true;
				}

				for (const auto& spell : a_spells) {
					const bool isSelected = (spell.formKey == a_selectedKey);
					if (ImGui::Selectable(spell.name.c_str(), isSelected) && !isSelected) {
						a_selectedKey = spell.formKey;
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
			return changed;
		}

		bool DrawTriggerSection(QUICK_BUFF::Config& a_cfg, const TriggerUIDef& a_def, const std::vector<QUICK_BUFF::KnownSpellOption>& a_spells)
		{
			auto& trigger = a_cfg.Get(a_def.id);
			bool changed = false;
			if (!ImGui::CollapsingHeader(a_def.title, ImGuiTreeNodeFlags_DefaultOpen)) {
				return false;
			}

			changed |= ImGui::Checkbox(std::format("Enabled##{}_enabled", a_def.title).c_str(), &trigger.enabled);
			changed |= DrawSpellCombo(std::format("Spell##{}_spell", a_def.title).c_str(), trigger.spellFormID, a_spells);
			changed |= ImGui::SliderFloat(std::format("Cooldown (sec)##{}_cd", a_def.title).c_str(), &trigger.cooldownSec, 0.0f, 120.0f, "%.1f");

			ImGui::SeparatorText(std::format("Conditions##{}_cond", a_def.title).c_str());
			changed |= ImGui::Checkbox(std::format("First person only##{}_fp", a_def.title).c_str(), &trigger.conditions.firstPersonOnly);
			changed |= ImGui::Checkbox(std::format("Weapon drawn only##{}_wd", a_def.title).c_str(), &trigger.conditions.weaponDrawnOnly);
			changed |= ImGui::Checkbox(std::format("Require in combat##{}_ic", a_def.title).c_str(), &trigger.conditions.requireInCombat);
			changed |= ImGui::Checkbox(std::format("Require out of combat##{}_ooc", a_def.title).c_str(), &trigger.conditions.requireOutOfCombat);

			ImGui::SeparatorText(std::format("Rules##{}_rules", a_def.title).c_str());
			changed |= ImGui::Checkbox(std::format("Skip if already active##{}_active", a_def.title).c_str(), &trigger.rules.skipIfEffectAlreadyActive);
			float magickaPercent = trigger.rules.requireMagickaPercentAbove * 100.0f;
			if (ImGui::SliderFloat(std::format("Require magicka above %%##{}_mag", a_def.title).c_str(), &magickaPercent, 0.0f, 100.0f, "%.0f%%")) {
				trigger.rules.requireMagickaPercentAbove = magickaPercent / 100.0f;
				changed = true;
			}

			if (a_def.id == QUICK_BUFF::TriggerID::kHealthBelow) {
				float threshold = trigger.thresholdHealthPercent * 100.0f;
				float hysteresis = trigger.hysteresisMargin * 100.0f;
				if (ImGui::SliderFloat(std::format("Threshold %%##{}_hp", a_def.title).c_str(), &threshold, 1.0f, 99.0f, "%.0f%%")) {
					trigger.thresholdHealthPercent = threshold / 100.0f;
					changed = true;
				}
				if (ImGui::SliderFloat(std::format("Hysteresis %%##{}_hyst", a_def.title).c_str(), &hysteresis, 0.0f, 20.0f, "%.0f%%")) {
					trigger.hysteresisMargin = hysteresis / 100.0f;
					changed = true;
				}
			}

			if (a_def.id == QUICK_BUFF::TriggerID::kPowerAttackStart || a_def.id == QUICK_BUFF::TriggerID::kShoutStart) {
				changed |= ImGui::SliderFloat(std::format("Internal lockout (sec)##{}_lock", a_def.title).c_str(), &trigger.internalLockoutSec, 0.0f, 1.0f, "%.2f");
			}

			if (trigger.enabled && trigger.spellFormID.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "No spell selected");
			}

			if (ImGui::Button(std::format("Test Cast##{}_test", a_def.title).c_str())) {
				(void)QUICK_BUFF::Manager::GetSingleton()->TryTestCast(a_def.id);
			}
			ImGui::Spacing();
			return changed;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("QuickBuff: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Quick Buff");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = QUICK_BUFF::Manager::GetSingleton();
			auto cfg = manager->GetConfig();
			bool changed = false;

			changed |= ImGui::Checkbox("Mod Enabled", &cfg.global.enabled);

			if (ImGui::CollapsingHeader("Global Defaults", ImGuiTreeNodeFlags_DefaultOpen)) {
				changed |= ImGui::Checkbox("Default: First person only", &cfg.global.firstPersonOnlyDefault);
				changed |= ImGui::Checkbox("Default: Weapon drawn only", &cfg.global.weaponDrawnOnlyDefault);
				changed |= ImGui::Checkbox("Block while in menus", &cfg.global.preventCastingInMenus);
				changed |= ImGui::Checkbox("Block while staggered", &cfg.global.preventCastingWhileStaggered);
				changed |= ImGui::Checkbox("Block while ragdoll", &cfg.global.preventCastingWhileRagdoll);
				changed |= ImGui::SliderFloat("Minimum time after load (sec)", &cfg.global.minTimeAfterLoadSeconds, 0.0f, 10.0f, "%.1f");
			}

			const auto spells = manager->GetKnownSelfSpells();
			for (const auto& triggerDef : kTriggerDefs) {
				changed |= DrawTriggerSection(cfg, triggerDef, spells);
			}

			if (changed) {
				manager->SetConfig(cfg, true);
			}
		}
	}
}
#else
namespace UI
{
	void Register() {}

	namespace MainPanel
	{
		void __stdcall Render() {}
	}
}
#endif
