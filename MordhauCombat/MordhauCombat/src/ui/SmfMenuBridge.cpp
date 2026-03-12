#include "ui/SmfMenuBridge.h"

#include "combat/DirectionalConfig.h"
#include "combat/DirectionalController.h"
#include "util/LogUtil.h"

#include <Windows.h>

#include <cstdio>

namespace MC::UI
{
	namespace
	{
		using RenderFn = void(__stdcall*)();
		using SetSectionFn = void (*)(const char*);
		using AddSectionItemFn = void (*)(const char*, RenderFn);
		using CheckboxFn = bool (*)(const char*, bool*);
		using SliderFloatFn = bool (*)(const char*, float*, float, float, const char*, int);
		using SliderIntFn = bool (*)(const char*, int*, int, int, const char*, int);
		using TextUnformattedFn = void (*)(const char*, const char*);
		using TextFn = void (*)(const char*, ...);
		using SeparatorTextFn = void (*)(const char*);
		using BeginTabBarFn = bool (*)(const char*, int);
		using EndTabBarFn = void (*)();
		using BeginTabItemFn = bool (*)(const char*, bool*, int);
		using EndTabItemFn = void (*)();

		HMODULE g_smfModule{ nullptr };
		bool g_registered = false;

		SetSectionFn g_setSection{ nullptr };
		AddSectionItemFn g_addSectionItem{ nullptr };
		CheckboxFn g_checkbox{ nullptr };
		SliderFloatFn g_sliderFloat{ nullptr };
		SliderIntFn g_sliderInt{ nullptr };
		TextUnformattedFn g_textUnformatted{ nullptr };
		TextFn g_text{ nullptr };
		SeparatorTextFn g_separatorText{ nullptr };
		BeginTabBarFn g_beginTabBar{ nullptr };
		EndTabBarFn g_endTabBar{ nullptr };
		BeginTabItemFn g_beginTabItem{ nullptr };
		EndTabItemFn g_endTabItem{ nullptr };

		bool ResolveApis()
		{
			if (!g_smfModule) {
				g_smfModule = ::GetModuleHandleA("SKSEMenuFramework.dll");
			}
			if (!g_smfModule) {
				g_smfModule = ::GetModuleHandleA("SKSEMenuFramework");
			}
			if (!g_smfModule) {
				g_smfModule = ::LoadLibraryA("Data\\SKSE\\Plugins\\SKSEMenuFramework.dll");
			}
			if (!g_smfModule) {
				return false;
			}

			g_setSection = reinterpret_cast<SetSectionFn>(::GetProcAddress(g_smfModule, "SetSection"));
			g_addSectionItem = reinterpret_cast<AddSectionItemFn>(::GetProcAddress(g_smfModule, "AddSectionItem"));
			g_checkbox = reinterpret_cast<CheckboxFn>(::GetProcAddress(g_smfModule, "igCheckbox"));
			g_sliderFloat = reinterpret_cast<SliderFloatFn>(::GetProcAddress(g_smfModule, "igSliderFloat"));
			g_sliderInt = reinterpret_cast<SliderIntFn>(::GetProcAddress(g_smfModule, "igSliderInt"));
			g_textUnformatted = reinterpret_cast<TextUnformattedFn>(::GetProcAddress(g_smfModule, "igTextUnformatted"));
			g_text = reinterpret_cast<TextFn>(::GetProcAddress(g_smfModule, "igText"));
			g_separatorText = reinterpret_cast<SeparatorTextFn>(::GetProcAddress(g_smfModule, "igSeparatorText"));
			g_beginTabBar = reinterpret_cast<BeginTabBarFn>(::GetProcAddress(g_smfModule, "igBeginTabBar"));
			g_endTabBar = reinterpret_cast<EndTabBarFn>(::GetProcAddress(g_smfModule, "igEndTabBar"));
			g_beginTabItem = reinterpret_cast<BeginTabItemFn>(::GetProcAddress(g_smfModule, "igBeginTabItem"));
			g_endTabItem = reinterpret_cast<EndTabItemFn>(::GetProcAddress(g_smfModule, "igEndTabItem"));

			return g_addSectionItem != nullptr;
		}

		void TextLine(const char* a_text)
		{
			if (g_textUnformatted) {
				g_textUnformatted(a_text, nullptr);
			} else if (g_text) {
				g_text("%s", a_text);
			}
		}

		bool UiCheckbox(const char* a_label, bool* a_value)
		{
			return g_checkbox ? g_checkbox(a_label, a_value) : false;
		}

		bool UiSliderFloat(const char* a_label, float* a_value, float a_min, float a_max, const char* a_format)
		{
			return g_sliderFloat ? g_sliderFloat(a_label, a_value, a_min, a_max, a_format, 0) : false;
		}

		bool UiSliderInt(const char* a_label, int* a_value, int a_min, int a_max, const char* a_format)
		{
			return g_sliderInt ? g_sliderInt(a_label, a_value, a_min, a_max, a_format, 0) : false;
		}

		void __stdcall RenderGeneral();
		void __stdcall RenderCombat();
		void __stdcall RenderOverlay();
		void __stdcall RenderDebug();
		void __stdcall RenderSettings();

		void __stdcall RenderGeneral()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			auto* controller = MC::DIRECTIONAL::Controller::GetSingleton();

			bool changed = false;
			changed |= UiCheckbox("Enable Directional Combat", &data.enabled);

			int hotkey = static_cast<int>(data.toggleKey);
			if (UiSliderInt("Toggle Key (DIK)", &hotkey, 1, 255, "%d")) {
				data.toggleKey = static_cast<unsigned int>(hotkey);
				changed = true;
			}

			if (changed) {
				controller->SetEnabled(data.enabled);
				cfg->Save();
			}
		}

		void __stdcall RenderSettings()
		{
			if (g_beginTabBar && g_endTabBar && g_beginTabItem && g_endTabItem) {
				if (g_beginTabBar("MordhauCombatTabs", 0)) {
					if (g_beginTabItem("General", nullptr, 0)) {
						RenderGeneral();
						g_endTabItem();
					}
					if (g_beginTabItem("Combat", nullptr, 0)) {
						RenderCombat();
						g_endTabItem();
					}
					if (g_beginTabItem("Overlay", nullptr, 0)) {
						RenderOverlay();
						g_endTabItem();
					}
					if (g_beginTabItem("Debug", nullptr, 0)) {
						RenderDebug();
						g_endTabItem();
					}
					g_endTabBar();
				}
			} else {
				// Fallback when tab exports are unavailable.
				RenderGeneral();
				RenderCombat();
				RenderOverlay();
				RenderDebug();
			}
		}

		void __stdcall RenderCombat()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			bool changed = false;

			changed |= UiSliderFloat("Mouse Sensitivity", &data.mouseSensitivity, 0.05f, 5.0f, "%.2f");
			changed |= UiSliderFloat("Deadzone", &data.deadzone, 0.00f, 0.95f, "%.2f");
			changed |= UiSliderFloat("Smoothing", &data.smoothing, 0.00f, 0.95f, "%.2f");
			changed |= UiSliderFloat("Drag Max Accel", &data.dragMaxAccel, 0.00f, 1.00f, "%.2f");
			changed |= UiSliderFloat("Drag Max Slow", &data.dragMaxSlow, -1.00f, 0.00f, "%.2f");
			changed |= UiSliderFloat("Drag Smoothing", &data.dragSmoothing, 0.01f, 0.95f, "%.2f");
			changed |= UiSliderFloat("Weapon Speed Scale", &data.weaponSpeedMultScale, 0.00f, 3.00f, "%.2f");

			if (changed) {
				cfg->Save();
			}
		}

		void __stdcall RenderOverlay()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			if (UiSliderFloat("Vector Scale", &data.overlayScale, 0.20f, 5.00f, "%.2f")) {
				cfg->Save();
			}

			TextLine("Style: short white center vector");
		}

		void __stdcall RenderDebug()
		{
			auto* cfg = MC::DIRECTIONAL::Config::GetSingleton();
			auto& data = cfg->GetMutableData();
			if (UiCheckbox("Debug Mode", &data.debugMode)) {
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
			LOG_WARN("MordhauCombat: SKSEMenuFramework not available or AddSectionItem export missing; menu not registered");
			return;
		}

		if (g_setSection) {
			g_setSection("MordhauCombat");
			g_addSectionItem("Settings", &RenderSettings);
		} else {
			// Fallback for older SKSEMenuFramework builds without SetSection export.
			g_addSectionItem("MordhauCombat/Settings", &RenderSettings);
		}

		g_registered = true;
		LOG_INFO("MordhauCombat: SMF sections registered");
	}
}
