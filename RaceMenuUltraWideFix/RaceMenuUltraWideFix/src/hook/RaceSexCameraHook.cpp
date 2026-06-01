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
			// Call original function first
			func(a_this);

			// Adjust camera translation along its look direction to zoom out
			if (a_this && a_this->cameraRoot) {
				auto& rotate = a_this->cameraRoot->local.rotate;
				
				// Get camera's forward vector (Y-axis / column 1 of local rotation matrix)
				RE::NiPoint3 forward = rotate.GetVectorY();
				
				// Read zoom amount from INI config (defaults to 50.0 units)
				static float zoomAmount = []() {
					auto pluginName = DLLMAIN::Plugin::GetSingleton()->Info().name;
					auto iniPath = std::format("Data\\SKSE\\Plugins\\{}.ini", pluginName);
					
					char valueStr[64];
					GetPrivateProfileStringA("Camera", "fZoomAmount", "50.0", valueStr, sizeof(valueStr), iniPath.c_str());
					try {
						return std::stof(valueStr);
					} catch (...) {
						return 50.0f;
					}
				}();
				
				// Move camera backwards away from the player character
				a_this->cameraRoot->local.translate -= forward * zoomAmount;
			}
		}
		
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		LOG_INFO("Registering RaceSexCamera Update virtual function hook...");
		stl::Hook::virtual_function<RE::RaceSexCamera, RaceSexCameraUpdateHook>(0, 2);
	}
}
