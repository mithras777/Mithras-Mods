#pragma once

#if __has_include("PrismaUI/PrismaUI_API.h")
	#include "PrismaUI/PrismaUI_API.h"
#else
	#include <cstdint>

	using PrismaView = std::uint32_t;

	namespace PRISMA_UI_API
	{
		enum class InterfaceVersion : std::uint32_t
		{
			V1 = 1
		};

		class IVPrismaUI1
		{
		public:
			virtual ~IVPrismaUI1() = default;
			virtual bool IsValid(PrismaView) = 0;
			virtual PrismaView CreateView(const char*, void(*)(PrismaView)) = 0;
			virtual void Show(PrismaView) = 0;
			virtual void Hide(PrismaView) = 0;
			virtual bool IsHidden(PrismaView) = 0;
			virtual void SetOrder(PrismaView, std::int32_t) = 0;
			virtual bool Focus(PrismaView, bool, bool) = 0;
			virtual bool HasFocus(PrismaView) = 0;
			virtual void InteropCall(PrismaView, const char*, const char*) = 0;
			virtual void RegisterJSListener(PrismaView, const char*, void(*)(const char*)) = 0;
		};

		inline void* RequestPluginAPI(InterfaceVersion)
		{
			return nullptr;
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
		void RegisterListeners();
		void RegisterHUDListeners();

	private:
		PRISMA_UI_API::IVPrismaUI1* m_api{ nullptr };
		PrismaView m_view{ 0 };
		PrismaView m_hudView{ 0 };
		bool m_listenersRegistered{ false };
		bool m_hudListenersRegistered{ false };
	};
}
