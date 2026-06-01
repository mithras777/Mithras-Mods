#include "hook/RaceSexCameraHook.h"
#include "util/HookUtil.h"
#include "util/LogUtil.h"
#include "plugin.h"
#include <Windows.h>

namespace HOOK::RaceSexCamera {

	struct RaceSexCameraUpdateHook
	{
		static void thunk(RE::RaceSexCamera* a_this)
		{
			// Reset offset tracking if we have switched to a new camera instance
			if (s_lastInstance != a_this) {
				s_hasAppliedOffset = false;
				s_lastInstance = a_this;
			}

			// Revert the previous frame's offset before letting the original update run
			if (a_this && a_this->cameraRoot && s_hasAppliedOffset) {
				a_this->cameraRoot->local.translate += s_previousOffset;
				s_hasAppliedOffset = false;
			}

			// Call original function
			func(a_this);

			// Apply new offset
			if (a_this && a_this->cameraRoot) {
				auto& rotate = a_this->cameraRoot->local.rotate;
				
				// Get camera's forward vector (Y-axis / column 1 of local rotation matrix)
				RE::NiPoint3 forward = rotate.GetVectorY();
				
				// Read zoom amount from INI config (defaults to 2.0 notches)
				static float zoomAmount = []() {
					auto pluginName = DLLMAIN::Plugin::GetSingleton()->Info().name;
					auto iniPath = std::format("Data\\SKSE\\Plugins\\{}.ini", pluginName);
					
					char valueStr[64];
					GetPrivateProfileStringA("Camera", "fZoomAmount", "2.0", valueStr, sizeof(valueStr), iniPath.c_str());
					try {
						float val = std::stof(valueStr);
						// Convert notches to game units (1 notch = 25.0 units)
						return val * 25.0f;
					} catch (...) {
						return 2.0f * 25.0f;
					}
				}();
				
				// Calculate and apply offset backward along the forward vector
				s_previousOffset = forward * zoomAmount;
				a_this->cameraRoot->local.translate -= s_previousOffset;
				s_hasAppliedOffset = true;
			}
		}
		
		static inline REL::Relocation<decltype(thunk)> func;

	private:
		static inline bool s_hasAppliedOffset{ false };
		static inline RE::NiPoint3 s_previousOffset{ 0.f, 0.f, 0.f };
		static inline RE::RaceSexCamera* s_lastInstance{ nullptr };
	};

	void Install()
	{
		LOG_INFO("Registering RaceSexCamera Update virtual function hook...");
		stl::Hook::virtual_function<RE::RaceSexCamera, RaceSexCameraUpdateHook>(0, 2);
	}
}
