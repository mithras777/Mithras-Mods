#include "ui/UI.h"

#include "smartcast/SmartCastController.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <format>

#if SMARTCAST_HAS_MENU_FRAMEWORK
namespace UI
{
	namespace
	{
		using Controller = SMART_CAST::Controller;

		bool DrawSpellCombo(const char* a_label, std::string& a_selectedKey, const std::vector<SMART_CAST::KnownSpellOption>& a_spells)
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
					const bool selected = (spell.formKey == a_selectedKey);
					if (ImGui::Selectable(spell.name.c_str(), selected) && !selected) {
						a_selectedKey = spell.formKey;
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
			return changed;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("SmartCast: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("SmartCast SKSE");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* controller = Controller::GetSingleton();
			auto cfg = controller->GetConfig();
			const auto spells = controller->GetKnownSpells();
			bool changed = false;
			static int selectedChain = 0;
			selectedChain = std::clamp(selectedChain, 0, std::max(0, static_cast<int>(cfg.chains.size()) - 1));

			if (ImGui::BeginTabBar("SmartCastTabs")) {
				if (ImGui::BeginTabItem("General")) {
					changed |= ImGui::Checkbox("Enabled", &cfg.global.enabled);
					changed |= ImGui::Checkbox("First person only", &cfg.global.firstPersonOnly);
					changed |= ImGui::Checkbox("Prevent in menus", &cfg.global.preventInMenus);
					changed |= ImGui::Checkbox("Prevent while staggered", &cfg.global.preventWhileStaggered);
					changed |= ImGui::Checkbox("Prevent while ragdoll", &cfg.global.preventWhileRagdoll);
					changed |= ImGui::SliderFloat("Min time after load (sec)", &cfg.global.minTimeAfterLoadSeconds, 0.0f, 10.0f, "%.1f");
					changed |= ImGui::SliderInt("Max chains", &cfg.global.maxChains, 1, 10);
					changed |= ImGui::SliderInt("Max steps per chain", &cfg.global.maxStepsPerChain, 1, 30);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Hotkeys")) {
					static constexpr std::array<const char*, 30> keys{
						"None","R","G","Q","E","F","Z","X","C","V","B","T","Y","H","J","K","L","M","N","P","U","I","O","Mouse4","Mouse5","F1","F2","F3","F4","F5"
					};
					auto drawKeyCombo = [&](const char* label, std::string& value) {
						bool c = false;
						if (ImGui::BeginCombo(label, value.c_str())) {
							for (const auto* k : keys) {
								const bool selected = value == k;
								if (ImGui::Selectable(k, selected) && !selected) {
									value = k;
									c = true;
								}
							}
							ImGui::EndCombo();
						}
						return c;
					};
					changed |= drawKeyCombo("Record key", cfg.global.record.toggleKey);
					changed |= drawKeyCombo("Record cancel key", cfg.global.record.cancelKey);
					changed |= drawKeyCombo("Play key", cfg.global.playback.playKey);
					changed |= drawKeyCombo("Play cancel key", cfg.global.playback.cancelKey);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Record")) {
					changed |= ImGui::SliderFloat("Max idle (sec)", &cfg.global.record.maxIdleSec, 0.5f, 30.0f, "%.1f");
					changed |= ImGui::Checkbox("Record only successful casts", &cfg.global.record.recordOnlySuccessfulCasts);
					changed |= ImGui::Checkbox("Ignore powers", &cfg.global.record.ignorePowers);
					changed |= ImGui::Checkbox("Ignore shouts", &cfg.global.record.ignoreShouts);
					changed |= ImGui::Checkbox("Ignore scrolls", &cfg.global.record.ignoreScrolls);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Playback")) {
					changed |= ImGui::SliderInt("Default chain index", &cfg.global.playback.defaultChainIndex, 1, std::max(1, cfg.global.maxChains));
					changed |= ImGui::SliderFloat("Step delay (sec)", &cfg.global.playback.stepDelaySec, 0.0f, 1.0f, "%.2f");
					changed |= ImGui::Checkbox("Abort on fail", &cfg.global.playback.abortOnFail);
					changed |= ImGui::Checkbox("Skip on fail", &cfg.global.playback.skipOnFail);
					changed |= ImGui::Checkbox("Require weapon sheathed", &cfg.global.playback.requireWeaponSheathed);
					changed |= ImGui::Checkbox("Auto-sheath during playback", &cfg.global.playback.autoSheathDuringPlayback);
					changed |= ImGui::Checkbox("Stop if player hit", &cfg.global.playback.stopIfPlayerHit);
					changed |= ImGui::Checkbox("Stop if attack pressed", &cfg.global.playback.stopIfAttackPressed);
					changed |= ImGui::Checkbox("Stop if block pressed", &cfg.global.playback.stopIfBlockPressed);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Concentration")) {
					changed |= ImGui::SliderFloat("Min hold (sec)", &cfg.global.concentration.minHoldSec, 0.05f, 10.0f, "%.2f");
					changed |= ImGui::SliderFloat("Max hold (sec)", &cfg.global.concentration.maxHoldSec, 0.05f, 10.0f, "%.2f");
					changed |= ImGui::SliderFloat("Sample granularity (sec)", &cfg.global.concentration.sampleGranularitySec, 0.01f, 0.5f, "%.2f");
					changed |= ImGui::SliderFloat("Release padding (sec)", &cfg.global.concentration.releasePaddingSec, 0.0f, 1.0f, "%.2f");
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Targeting")) {
					const char* modes[] = { "selfOnly", "crosshairOnly", "hybrid" };
					int modeIndex = 2;
					if (cfg.global.targeting.mode == "selfOnly") modeIndex = 0;
					else if (cfg.global.targeting.mode == "crosshairOnly") modeIndex = 1;
					if (ImGui::Combo("Mode", &modeIndex, modes, static_cast<int>(std::size(modes)))) {
						cfg.global.targeting.mode = modes[modeIndex];
						changed = true;
					}
					changed |= ImGui::Checkbox("Prefer crosshair actor", &cfg.global.targeting.preferCrosshairActor);
					changed |= ImGui::Checkbox("Fallback to self if no target", &cfg.global.targeting.fallbackToSelfIfNoTarget);
					ImGui::TextDisabled("Targeting uses crosshair target only (no raycast).");
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Chains")) {
					if (!cfg.chains.empty()) {
						ImGui::SliderInt("Selected chain", &selectedChain, 0, static_cast<int>(cfg.chains.size()) - 1, "Chain %d");
						auto& chain = cfg.chains[static_cast<std::size_t>(selectedChain)];
						changed |= ImGui::Checkbox("Enabled", &chain.enabled);
						char nameBuf[128]{};
						std::snprintf(nameBuf, sizeof(nameBuf), "%s", chain.name.c_str());
						if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
							chain.name = nameBuf;
							changed = true;
						}
						static constexpr std::array<const char*, 30> keys{
							"None","R","G","Q","E","F","Z","X","C","V","B","T","Y","H","J","K","L","M","N","P","U","I","O","Mouse4","Mouse5","F1","F2","F3","F4","F5"
						};
						if (ImGui::BeginCombo("Chain hotkey", chain.hotkey.c_str())) {
							for (const auto* k : keys) {
								const bool selected = (chain.hotkey == k);
								if (ImGui::Selectable(k, selected) && !selected) {
									chain.hotkey = k;
									changed = true;
								}
							}
							ImGui::EndCombo();
						}

						if (ImGui::Button("Start Recording to this Chain")) {
							controller->SetConfig(cfg, true);
							controller->StartRecording(selectedChain + 1);
						}
						ImGui::SameLine();
						if (ImGui::Button("Play this Chain")) {
							controller->SetConfig(cfg, true);
							controller->StartPlayback(selectedChain + 1);
						}
						ImGui::SameLine();
						if (ImGui::Button("Clear Chain")) {
							chain.steps.clear();
							changed = true;
						}

						ImGui::Separator();
						for (std::size_t i = 0; i < chain.steps.size(); ++i) {
							auto& step = chain.steps[i];
							ImGui::PushID(static_cast<int>(i));
							ImGui::Text("Step %d", static_cast<int>(i + 1));
							changed |= DrawSpellCombo("Spell", step.spellFormID, spells);

							int type = (step.type == SMART_CAST::StepType::kConcentration) ? 1 : 0;
							if (ImGui::Combo("Type", &type, "fireAndForget\0concentration\0")) {
								step.type = (type == 1) ? SMART_CAST::StepType::kConcentration : SMART_CAST::StepType::kFireAndForget;
								changed = true;
							}

							int castOn = static_cast<int>(step.castOn);
							if (ImGui::Combo("Cast On", &castOn, "self\0crosshair\0aimed\0")) {
								step.castOn = static_cast<SMART_CAST::CastOn>(castOn);
								changed = true;
							}

							if (step.type == SMART_CAST::StepType::kConcentration) {
								changed |= ImGui::SliderFloat("Hold (sec)", &step.holdSec, 0.05f, 10.0f, "%.2f");
							}

							if (ImGui::Button("Up") && i > 0) {
								std::swap(chain.steps[i], chain.steps[i - 1]);
								changed = true;
							}
							ImGui::SameLine();
							if (ImGui::Button("Down") && i + 1 < chain.steps.size()) {
								std::swap(chain.steps[i], chain.steps[i + 1]);
								changed = true;
							}
							ImGui::SameLine();
							if (ImGui::Button("Delete")) {
								chain.steps.erase(chain.steps.begin() + static_cast<std::ptrdiff_t>(i));
								changed = true;
								ImGui::PopID();
								break;
							}
							ImGui::Separator();
							ImGui::PopID();
						}

						if (ImGui::Button("Add Empty Step") && static_cast<int>(chain.steps.size()) < cfg.global.maxStepsPerChain) {
							chain.steps.emplace_back();
							changed = true;
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			if (changed) {
				controller->SetConfig(cfg, true);
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
