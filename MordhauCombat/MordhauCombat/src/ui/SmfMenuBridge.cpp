#include "ui/SmfMenuBridge.h"

#include "combat/DirectionalConfig.h"
#include "combat/DirectionalController.h"
#include "util/LogUtil.h"

#include <Windows.h>

#include <array>
#include <cstdio>

namespace MC::UI
{
	namespace
	{
		using RenderFn = void(__stdcall*)();
		using AddSectionItemFn = void (*)(const char*, RenderFn);
		using CheckboxFn = bool (*)(const char*, bool*);
		using SliderFloatFn = bool (*)(const char*, float*, float, float, const char*, int);
		using SliderIntFn = bool (*)(const char*, int*, int, int, const char*, int);
		using TextUnformattedFn = void (*)(const char*, const char*);
		using SeparatorTextFn = void (*)(const char*);

		HMODULE g_smfModule{ nullptr };
		bool g_registered = false;

		AddSectionItemFn g_addSectionItem{ nullptr };
		CheckboxFn g_checkbox{ nullptr };
		SliderFloatFn g_sliderFloat{ nullptr };
		SliderIntFn g_sliderInt{ nullptr };
		TextUnformattedFn g_textUnformatted{ nullptr };
		SeparatorTextFn g_separatorText{ nullptr };

		bool ResolveApis()
		{
			if (!g_smfModule) {
				g_smfModule = ::GetModuleHandleA("SKSEMenuFramework");
			}
			if (!g_smfModule) {
				g_smfModule = ::LoadLibraryA("Data\\SKSE\\Plugins\\SKSEMenuFramework.dll");
			}
			if (!g_smfModule) {
				return false;
			}

			g_addSectionItem = reinterpret_cast<AddSectionItemFn>(::GetProcAddress(g_smfModule, "AddSectionItem"));
			g_checkbox = reinterpret_cast<CheckboxFn>(::GetProcAddress(g_smfModule, "igCheckbox"));
			g_sliderFloat = reinterpret_cast<SliderFloatFn>(::GetProcAddress(g_smfModule, "igSliderFloat"));
			g_sliderInt = reinterpret_cast<SliderIntFn>(::GetProcAddress(g_smfModule, "igSliderInt"));
			g_textUnformatted = reinterpret_cast<TextUnformattedFn>(::GetProcAddress(g_smfModule, "igTextUnformatted"));
			g_separatorText = reinterpret_cast<SeparatorTextFn>(::GetProcAddress(g_smfModule, "igSeparatorText"));

			return g_addSectionItem && g_checkbox && g_sliderFloat && g_sliderInt && g_textUnformatted && g_separatorText;
		}

		void TextLine(const char* a_text)
		{
			if (g_textUnformatted) {
				g_textUnformatted(a_text, nullptr);
			}
		}

		void __stdcall RenderGeneral()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			auto* controller = MC::DIRECTIONAL::Controller::GetSingleton();

			bool changed = false;
			changed |= g_checkbox("Enable Directional Combat", &data.enabled);

			int hotkey = static_cast<int>(data.toggleKey);
			if (g_sliderInt("Toggle Key (DIK)", &hotkey, 1, 255, "%d", 0)) {
				data.toggleKey = static_cast<unsigned int>(hotkey);
				changed = true;
			}

			if (changed) {
				controller->SetEnabled(data.enabled);
				cfg->Save();
			}
		}

		void __stdcall RenderCombat()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			bool changed = false;

			changed |= g_sliderFloat("Mouse Sensitivity", &data.mouseSensitivity, 0.05f, 5.0f, "%.2f", 0);
			changed |= g_sliderFloat("Deadzone", &data.deadzone, 0.00f, 0.95f, "%.2f", 0);
			changed |= g_sliderFloat("Smoothing", &data.smoothing, 0.00f, 0.95f, "%.2f", 0);
			changed |= g_sliderFloat("Drag Max Accel", &data.dragMaxAccel, 0.00f, 1.00f, "%.2f", 0);
			changed |= g_sliderFloat("Drag Max Slow", &data.dragMaxSlow, -1.00f, 0.00f, "%.2f", 0);
			changed |= g_sliderFloat("Drag Smoothing", &data.dragSmoothing, 0.01f, 0.95f, "%.2f", 0);
			changed |= g_sliderFloat("Weapon Speed Scale", &data.weaponSpeedMultScale, 0.00f, 3.00f, "%.2f", 0);

			if (changed) {
				cfg->Save();
			}
		}

		void __stdcall RenderOverlay()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			if (g_sliderFloat("Vector Scale", &data.overlayScale, 0.20f, 5.00f, "%.2f", 0)) {
				cfg->Save();
			}

			TextLine("Style: short white center vector");
		}

		void __stdcall RenderDebug()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			if (g_checkbox("Debug Mode", &data.debugMode)) {
				cfg->Save();
			}

			if (g_separatorText) {
				g_separatorText("Runtime");
			}
			auto* controller = MC::DIRECTIONAL::Controller::GetSingleton();
			auto cursor = controller->GetCursorState();
			auto latched = controller->GetLatchedState();

			char line[128]{};
			std::snprintf(line, sizeof(line), "Dir XY: %.2f, %.2f", cursor.x, cursor.y);
			TextLine(line);
			std::snprintf(line, sizeof(line), "Dir Bucket: %d", static_cast<int>(cursor.bucket));
			TextLine(line);
			std::snprintf(line, sizeof(line), "Latched Bucket: %d", static_cast<int>(latched.bucket));
			TextLine(line);
			std::snprintf(line, sizeof(line), "Drag Scalar: %.3f", controller->GetDragScalar());
			TextLine(line);
		}
	}

	void SmfMenuBridge::Register()
	{
		if (g_registered) {
			return;
		}

		if (!ResolveApis()) {
			LOG_WARN("MordhauCombat: SKSEMenuFramework not available or missing exports; menu not registered");
			return;
		}

		g_addSectionItem("MordhauCombat/General", &RenderGeneral);
		g_addSectionItem("MordhauCombat/Combat", &RenderCombat);
		g_addSectionItem("MordhauCombat/Overlay", &RenderOverlay);
		g_addSectionItem("MordhauCombat/Debug", &RenderDebug);
		g_registered = true;
		LOG_INFO("MordhauCombat: SMF sections registered");
	}
}
