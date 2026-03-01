#pragma once

#include "PrismaUI/PrismaUI_API.h"

#include <string>

namespace UI::PRISMA
{
	class Bridge final : public REX::Singleton<Bridge>
	{
	public:
		void Initialize();
		void Toggle();
		void PushSnapshot(const std::string& a_json);
		void ShowToast(const std::string& a_text);
		void SetFocusState(bool a_focused);

		[[nodiscard]] bool IsAvailable() const noexcept { return m_api != nullptr; }

	private:
		void EnsureViewCreated();
		void RegisterListeners();

	private:
		PRISMA_UI_API::IVPrismaUI1* m_api{ nullptr };
		PrismaView m_view{ 0 };
		bool m_listenersRegistered{ false };
	};
}
