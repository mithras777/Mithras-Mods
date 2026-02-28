#include "ui/UI.h"

#include "Settings.h"
#include "util/LogUtil.h"

#if DYNAMIC_SPAWNS_HAS_MENU_FRAMEWORK
namespace UI
{
	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("DynamicSpawns: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Dynamic Spawns");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* settings = DYNAMIC_SPAWNS::Settings::GetSingleton();
			auto cfg = settings->Get();
			bool changed = false;

			if (ImGui::BeginTabBar("DynamicSpawnsTabs")) {
				if (ImGui::BeginTabItem("General")) {
					changed |= ImGui::Checkbox("Enabled", &cfg.enabled);
					changed |= ImGui::SliderInt("Global max alive", &cfg.limits.globalMaxAlive, 1, 128);
					changed |= ImGui::SliderInt("Max per cell", &cfg.limits.maxPerCell, 1, 32);
					changed |= ImGui::SliderInt("Max per event", &cfg.limits.maxPerEvent, 1, 16);
					changed |= ImGui::SliderFloat("Global cooldown (sec)", &cfg.timing.cooldownSecondsGlobal, 0.0f, 30.0f, "%.1f");
					changed |= ImGui::SliderFloat("Same-cell cooldown (sec)", &cfg.timing.cooldownSecondsSameCell, 0.0f, 900.0f, "%.1f");
					changed |= ImGui::Checkbox("Skip interior", &cfg.filters.skipIfInterior);
					changed |= ImGui::Checkbox("Skip exterior", &cfg.filters.skipIfExterior);
					changed |= ImGui::Checkbox("Debug notifications", &cfg.debug.notify);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Genesis Parity")) {
					changed |= ImGui::Checkbox("Skip if hostiles nearby", &cfg.genesis.skipIfHostilesNearby);
					changed |= ImGui::SliderFloat("Hostile scan radius", &cfg.genesis.hostileScanRadius, 1000.0f, 20000.0f, "%.0f");
					changed |= ImGui::SliderInt("Hostile scan max refs", &cfg.genesis.hostileScanMaxRefs, 50, 500);
					changed |= ImGui::Checkbox("Unlevel spawned NPCs", &cfg.genesis.unlevelNPCs);
					changed |= ImGui::SliderFloat("Health min %", &cfg.genesis.healthPctMin, 10.0f, 1000.0f, "%.0f");
					changed |= ImGui::SliderFloat("Health max %", &cfg.genesis.healthPctMax, cfg.genesis.healthPctMin, 1000.0f, "%.0f");
					changed |= ImGui::SliderFloat("Stamina min %", &cfg.genesis.staminaPctMin, 10.0f, 1000.0f, "%.0f");
					changed |= ImGui::SliderFloat("Stamina max %", &cfg.genesis.staminaPctMax, cfg.genesis.staminaPctMin, 1000.0f, "%.0f");
					changed |= ImGui::SliderFloat("Magicka min %", &cfg.genesis.magickaPctMin, 10.0f, 1000.0f, "%.0f");
					changed |= ImGui::SliderFloat("Magicka max %", &cfg.genesis.magickaPctMax, cfg.genesis.magickaPctMin, 1000.0f, "%.0f");
					changed |= ImGui::Checkbox("Give spawned NPC potions", &cfg.genesis.giveSpawnPotions);
					changed |= ImGui::SliderInt("Potion chance %", &cfg.genesis.potionChancePct, 0, 100);
					changed |= ImGui::SliderInt("Potion count min", &cfg.genesis.potionCountMin, 1, 10);
					changed |= ImGui::SliderInt("Potion count max", &cfg.genesis.potionCountMax, cfg.genesis.potionCountMin, 10);
					changed |= ImGui::Checkbox("Inject bonus loot into containers", &cfg.genesis.lootInjectionEnabled);
					changed |= ImGui::SliderInt("Container loot chance %", &cfg.genesis.lootChancePct, 0, 100);

					ImGui::TextDisabled("Loot/potion pools are edited in settings.json (genesis.*Pool arrays).");
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			if (changed) {
				settings->UpdateAndSave(cfg);
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
