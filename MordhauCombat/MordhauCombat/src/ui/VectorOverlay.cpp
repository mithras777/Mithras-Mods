#include "ui/VectorOverlay.h"

#include "combat/DirectionalConfig.h"
#include "combat/DirectionalController.h"
#include "util/HookUtil.h"
#include "util/LogUtil.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <cmath>
#include <dxgi.h>

namespace MC::UI
{
	namespace
	{
		struct OverlayState
		{
			bool initialized{ false };
			bool installed{ false };
			HWND hwnd{ nullptr };
			REX::W32::ID3D11Device* device{ nullptr };
			REX::W32::ID3D11DeviceContext* context{ nullptr };
		};

		OverlayState g_state{};

		void DrawVector()
		{
			auto* controller = MC::DIRECTIONAL::Controller::GetSingleton();
			if (!controller->ShouldShowOverlay()) {
				return;
			}

			auto cursor = controller->GetCursorState();
			if (cursor.magnitude <= 0.01f) {
				return;
			}

			const float len = 15.0f * MC::DIRECTIONAL::Config::GetSingleton()->GetData().overlayScale;
			ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

			float nx = cursor.x;
			float ny = cursor.y;
			const float mag = std::sqrt((nx * nx) + (ny * ny));
			if (mag <= 0.0001f) {
				return;
			}
			nx /= mag;
			ny /= mag;

			ImVec2 end(center.x + (nx * len), center.y + (ny * len));
			ImDrawList* drawList = ImGui::GetBackgroundDrawList();
			const ImU32 color = IM_COL32(245, 245, 245, 235);
			drawList->AddLine(center, end, color, 2.6f);
			drawList->AddCircleFilled(end, 2.25f, color, 12);
		}

		struct D3DInitHook
		{
			static void thunk()
			{
				func();

				if (g_state.initialized) {
					return;
				}

				auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
				if (!renderer) {
					LOG_WARN("MordhauCombat: renderer unavailable for vector overlay");
					return;
				}

				auto& data = renderer->GetRuntimeData();
				auto* swapChain = data.renderWindows[0].swapChain;
				if (!swapChain || !data.forwarder || !data.context) {
					LOG_WARN("MordhauCombat: swapchain/device unavailable for vector overlay");
					return;
				}

				REX::W32::DXGI_SWAP_CHAIN_DESC desc{};
				if (swapChain->GetDesc(&desc) < 0 || !desc.outputWindow) {
					LOG_WARN("MordhauCombat: failed to query swapchain desc for vector overlay");
					return;
				}

				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				if (!ImGui_ImplWin32_Init(reinterpret_cast<HWND>(desc.outputWindow))) {
					LOG_WARN("MordhauCombat: ImGui Win32 init failed for vector overlay");
					return;
				}
				if (!ImGui_ImplDX11_Init(reinterpret_cast<ID3D11Device*>(data.forwarder),
				                         reinterpret_cast<ID3D11DeviceContext*>(data.context))) {
					LOG_WARN("MordhauCombat: ImGui DX11 init failed for vector overlay");
					return;
				}

				g_state.hwnd = reinterpret_cast<HWND>(desc.outputWindow);
				g_state.device = data.forwarder;
				g_state.context = data.context;
				g_state.initialized = true;
				LOG_INFO("MordhauCombat: vector overlay initialized");
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct DXGIPresentHook
		{
			static void thunk(std::uint32_t a_p1)
			{
				func(a_p1);

				if (!g_state.initialized) {
					return;
				}

				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				DrawVector();

				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void VectorOverlay::Install()
	{
		if (g_state.installed) {
			return;
		}

		stl::Hook::call<D3DInitHook>(RELOCATION_ID(75595, 77226).address() + RELOCATION_OFFSET(0x9, 0x275));
		stl::Hook::call<DXGIPresentHook>(RELOCATION_ID(75461, 77246).address() + RELOCATION_OFFSET(0x9, 0x9));
		g_state.installed = true;
	}

	void VectorOverlay::Update(float)
	{
		// Runtime data comes from DirectionalController; rendering is done in DXGI present hook.
	}
}
