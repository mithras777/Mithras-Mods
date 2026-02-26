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
			{ QUICK_BUFF::TriggerID::kHealthBelow70, "Health Below 70%" },
			{ QUICK_BUFF::TriggerID::kHealthBelow50, "Health Below 50%" },
			{ QUICK_BUFF::TriggerID::kHealthBelow30, "Health Below 30%" },
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

		bool IsHealthTrigger(QUICK_BUFF::TriggerID a_id)
		{
			return a_id == QUICK_BUFF::TriggerID::kHealthBelow70 ||
			       a_id == QUICK_BUFF::TriggerID::kHealthBelow50 ||
			       a_id == QUICK_BUFF::TriggerID::kHealthBelow30;
		}

		bool GetTabEnabled(const QUICK_BUFF::Config& a_cfg, std::initializer_list<QUICK_BUFF::TriggerID> a_triggers)
		{
			for (const auto id : a_triggers) {
				if (a_cfg.Get(id).enabled) {
					return true;
				}
			}
			return false;
		}

		bool DrawTabEnable(QUICK_BUFF::Config& a_cfg, std::initializer_list<QUICK_BUFF::TriggerID> a_triggers, const char* a_label)
		{
			bool anyEnabled = GetTabEnabled(a_cfg, a_triggers);
			if (!ImGui::Checkbox(a_label, &anyEnabled)) {
				return false;
			}

			for (const auto id : a_triggers) {
				a_cfg.Get(id).enabled = anyEnabled;
			}
			return true;
		}

		bool DrawTabReset(QUICK_BUFF::Config& a_cfg, const QUICK_BUFF::Config& a_defaults, std::initializer_list<QUICK_BUFF::TriggerID> a_triggers, const char* a_label)
		{
			if (!ImGui::Button(a_label)) {
				return false;
			}
			for (const auto id : a_triggers) {
				a_cfg.Get(id) = a_defaults.Get(id);
			}
			return true;
		}

		bool DrawTriggerSection(
			QUICK_BUFF::Manager* a_manager,
			QUICK_BUFF::Config& a_cfg,
			const TriggerUIDef& a_def,
			const std::vector<QUICK_BUFF::KnownSpellOption>& a_spells)
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
			float magickaPercent = trigger.rules.requireMagickaPercentAbove * 100.0f;
			if (ImGui::SliderFloat(std::format("Require magicka above %%##{}_mag", a_def.title).c_str(), &magickaPercent, 0.0f, 100.0f, "%.0f%%")) {
				trigger.rules.requireMagickaPercentAbove = magickaPercent / 100.0f;
				changed = true;
			}

			if (IsHealthTrigger(a_def.id)) {
				float hysteresis = trigger.hysteresisMargin * 100.0f;
				if (ImGui::SliderFloat(std::format("Hysteresis %%##{}_hyst", a_def.title).c_str(), &hysteresis, 0.0f, 20.0f, "%.0f%%")) {
					trigger.hysteresisMargin = hysteresis / 100.0f;
					changed = true;
				}
			}

			if (a_def.id == QUICK_BUFF::TriggerID::kPowerAttackStart || a_def.id == QUICK_BUFF::TriggerID::kShoutStart) {
				changed |= ImGui::SliderFloat(std::format("Internal lockout (sec)##{}_lock", a_def.title).c_str(), &trigger.internalLockoutSec, 0.0f, 1.0f, "%.2f");
			}

			const bool isConcentration = !trigger.spellFormID.empty() && a_manager->IsSpellConcentration(trigger.spellFormID);
			if (isConcentration) {
				changed |= ImGui::SliderFloat(
					std::format("Concentration duration (sec)##{}_conc", a_def.title).c_str(),
					&trigger.concentrationDurationSec,
					1.0f,
					10.0f,
					"%.1f");
			}

			if (trigger.enabled && trigger.spellFormID.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "No spell selected");
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
			const auto defaults = QUICK_BUFF::Manager::GetDefaultConfig();
			bool changed = false;
			const auto spells = manager->GetKnownSpells();

			if (ImGui::BeginTabBar("QuickBuffTabs")) {
				if (ImGui::BeginTabItem("General")) {
					changed |= ImGui::Checkbox("Mod Enabled", &cfg.global.enabled);
					ImGui::Spacing();
					changed |= ImGui::Checkbox("First person only", &cfg.global.firstPersonOnlyDefault);
					changed |= ImGui::Checkbox("Weapon drawn only", &cfg.global.weaponDrawnOnlyDefault);
					changed |= ImGui::Checkbox("Block while in menus", &cfg.global.preventCastingInMenus);
					changed |= ImGui::Checkbox("Block while staggered", &cfg.global.preventCastingWhileStaggered);
					changed |= ImGui::Checkbox("Block while ragdoll", &cfg.global.preventCastingWhileRagdoll);
					changed |= ImGui::SliderFloat("Minimum time after load (sec)", &cfg.global.minTimeAfterLoadSeconds, 0.0f, 10.0f, "%.1f");
					if (ImGui::Button("Reset")) {
						cfg.global = defaults.global;
						changed = true;
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Combat")) {
					changed |= DrawTabEnable(cfg, { QUICK_BUFF::TriggerID::kCombatStart, QUICK_BUFF::TriggerID::kCombatEnd }, "Enable");
					ImGui::SameLine();
					changed |= DrawTabReset(cfg, defaults, { QUICK_BUFF::TriggerID::kCombatStart, QUICK_BUFF::TriggerID::kCombatEnd }, "Reset");
					ImGui::Separator();
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kCombatStart)], spells);
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kCombatEnd)], spells);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Health")) {
					changed |= DrawTabEnable(cfg, { QUICK_BUFF::TriggerID::kHealthBelow70, QUICK_BUFF::TriggerID::kHealthBelow50, QUICK_BUFF::TriggerID::kHealthBelow30 }, "Enable");
					ImGui::SameLine();
					changed |= DrawTabReset(cfg, defaults, { QUICK_BUFF::TriggerID::kHealthBelow70, QUICK_BUFF::TriggerID::kHealthBelow50, QUICK_BUFF::TriggerID::kHealthBelow30 }, "Reset");
					ImGui::Separator();
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kHealthBelow70)], spells);
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kHealthBelow50)], spells);
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kHealthBelow30)], spells);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Movement")) {
					changed |= DrawTabEnable(cfg, { QUICK_BUFF::TriggerID::kCrouchStart, QUICK_BUFF::TriggerID::kSprintStart }, "Enable");
					ImGui::SameLine();
					changed |= DrawTabReset(cfg, defaults, { QUICK_BUFF::TriggerID::kCrouchStart, QUICK_BUFF::TriggerID::kSprintStart }, "Reset");
					ImGui::Separator();
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kCrouchStart)], spells);
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kSprintStart)], spells);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Weapon")) {
					changed |= DrawTabEnable(cfg, { QUICK_BUFF::TriggerID::kWeaponDraw }, "Enable");
					ImGui::SameLine();
					changed |= DrawTabReset(cfg, defaults, { QUICK_BUFF::TriggerID::kWeaponDraw }, "Reset");
					ImGui::Separator();
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kWeaponDraw)], spells);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Power/Shout")) {
					changed |= DrawTabEnable(cfg, { QUICK_BUFF::TriggerID::kPowerAttackStart, QUICK_BUFF::TriggerID::kShoutStart }, "Enable");
					ImGui::SameLine();
					changed |= DrawTabReset(cfg, defaults, { QUICK_BUFF::TriggerID::kPowerAttackStart, QUICK_BUFF::TriggerID::kShoutStart }, "Reset");
					ImGui::Separator();
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kPowerAttackStart)], spells);
					changed |= DrawTriggerSection(manager, cfg, kTriggerDefs[static_cast<std::size_t>(QUICK_BUFF::TriggerID::kShoutStart)], spells);
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
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
