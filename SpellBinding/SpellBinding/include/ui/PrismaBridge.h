#pragma once

#if __has_include("PrismaUI/PrismaUI_API.h")
	#include "PrismaUI/PrismaUI_API.h"
#else
	#include <cstdint>
	#include <Windows.h>

	using PrismaView = std::uint64_t;

	namespace PRISMA_UI_API
	{
		enum class InterfaceVersion : std::uint32_t
		{
			V1 = 0,
			V2 = 1
		};

		typedef void (*OnDomReadyCallback)(PrismaView view);
		typedef void (*JSCallback)(const char* result);
		typedef void (*JSListenerCallback)(const char* argument);

		enum class ConsoleMessageLevel : std::uint8_t
		{
			Log = 0,
			Warning,
			Error,
			Debug,
			Info
		};

		typedef void (*ConsoleMessageCallback)(PrismaView view, ConsoleMessageLevel level, const char* message);

		class IVPrismaUI1
		{
		protected:
			~IVPrismaUI1() = default;

		public:
			virtual PrismaView CreateView(const char* htmlPath,
				OnDomReadyCallback onDomReadyCallback = nullptr) noexcept = 0;
			virtual void Invoke(PrismaView view, const char* script, JSCallback callback = nullptr) noexcept = 0;
			virtual void InteropCall(PrismaView view, const char* functionName, const char* argument) noexcept = 0;
			virtual void RegisterJSListener(PrismaView view, const char* functionName,
				JSListenerCallback callback) noexcept = 0;
			virtual bool HasFocus(PrismaView view) noexcept = 0;
			virtual bool Focus(PrismaView view, bool pauseGame = false, bool disableFocusMenu = false) noexcept = 0;
			virtual void Unfocus(PrismaView view) noexcept = 0;
			virtual void Show(PrismaView view) noexcept = 0;
			virtual void Hide(PrismaView view) noexcept = 0;
			virtual bool IsHidden(PrismaView view) noexcept = 0;
			virtual int GetScrollingPixelSize(PrismaView view) noexcept = 0;
			virtual void SetScrollingPixelSize(PrismaView view, int pixelSize) noexcept = 0;
			virtual bool IsValid(PrismaView view) noexcept = 0;
			virtual void Destroy(PrismaView view) noexcept = 0;
			virtual void SetOrder(PrismaView view, int order) noexcept = 0;
			virtual int GetOrder(PrismaView view) noexcept = 0;
			virtual void CreateInspectorView(PrismaView view) noexcept = 0;
			virtual void SetInspectorVisibility(PrismaView view, bool visible) noexcept = 0;
			virtual bool IsInspectorVisible(PrismaView view) noexcept = 0;
			virtual void SetInspectorBounds(PrismaView view, float topLeftX, float topLeftY, unsigned int width,
				unsigned int height) noexcept = 0;
			virtual bool HasAnyActiveFocus() noexcept = 0;
		};

		class IVPrismaUI2 : public IVPrismaUI1
		{
		protected:
			~IVPrismaUI2() = default;

		public:
			virtual void RegisterConsoleCallback(PrismaView view, ConsoleMessageCallback callback) noexcept = 0;
		};

		template <typename T>
		struct InterfaceVersionMap;

		template <>
		struct InterfaceVersionMap<IVPrismaUI1>
		{
			static constexpr InterfaceVersion version = InterfaceVersion::V1;
		};

		template <>
		struct InterfaceVersionMap<IVPrismaUI2>
		{
			static constexpr InterfaceVersion version = InterfaceVersion::V2;
		};

		inline void* RequestPluginAPI(InterfaceVersion a_interfaceVersion)
		{
			using RequestPluginAPIFunc = void* (*)(InterfaceVersion);
			static RequestPluginAPIFunc s_request = []() -> RequestPluginAPIFunc {
				HMODULE mod = GetModuleHandleA("PrismaUI.dll");
				if (!mod) {
					mod = GetModuleHandleA("PrismaUI");
				}
				if (!mod) {
					mod = LoadLibraryA("Data\\SKSE\\Plugins\\PrismaUI.dll");
				}
				if (!mod) {
					return nullptr;
				}
				return reinterpret_cast<RequestPluginAPIFunc>(GetProcAddress(mod, "RequestPluginAPI"));
			}();
			return s_request ? s_request(a_interfaceVersion) : nullptr;
		}

		template <typename T>
		[[nodiscard]] inline T* RequestPluginAPI()
		{
			return static_cast<T*>(RequestPluginAPI(InterfaceVersionMap<T>::version));
		}
	}
#endif

#include <string>

namespace UI::PRISMA
{
	class Bridge final : public REX::Singleton<Bridge>
	{
	public:
		void Initialize();
		void Toggle();
		void PushSnapshot(const std::string& a_json);
		void PushHUDSnapshot(const std::string& a_json);
		void ShowToast(const std::string& a_text);
		void SendEscapeToMenu();
		void SendCloseRequestToMenu();
		void SetFocusState(bool a_focused);
		[[nodiscard]] bool IsMenuOpen() const;
		[[nodiscard]] bool IsMenuFocused() const;

		[[nodiscard]] bool IsAvailable() const noexcept { return m_api != nullptr; }

	private:
		void EnsureViewCreated();
		void EnsureHUDViewCreated();
		void RegisterListeners(PrismaView a_view);
		void RegisterHUDListeners(PrismaView a_view);
		void PushSnapshot(PrismaView a_view, const std::string& a_json);
		void PushHUDSnapshot(PrismaView a_view, const std::string& a_json);

	private:
		PRISMA_UI_API::IVPrismaUI1* m_api{ nullptr };
		PrismaView m_view{ 0 };
		PrismaView m_hudView{ 0 };
		bool m_listenersRegistered{ false };
		bool m_hudListenersRegistered{ false };
		bool m_viewReady{ false };
		bool m_hudViewReady{ false };
	};
}
