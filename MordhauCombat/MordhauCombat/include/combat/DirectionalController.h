#pragma once

#include "combat/DirectionalTypes.h"

#include <atomic>
#include <string_view>

namespace MC::DIRECTIONAL
{
	class Controller
	{
	public:
		static Controller* GetSingleton();

		void OnMouseDelta(int a_dx, int a_dy);
		void OnAttackPressed(bool a_power);
		void OnAnimationEvent(std::string_view a_tag, std::string_view a_payload);
		void Update(float a_delta);

		void SetEnabled(bool a_enabled);
		bool IsEnabled() const;
		void ToggleEnabled();

		CursorVectorState GetCursorState() const;
		LatchedAttackState GetLatchedState() const;
		float GetDragScalar() const;
		bool ShouldShowOverlay() const;

	private:
		Controller();

		void UpdateBuckets();
		void PublishGraphVariables(RE::PlayerCharacter* a_player);
		void ApplyWeaponSpeedScalar(RE::PlayerCharacter* a_player, float a_scalar);
		void ClearAttackRuntime(RE::PlayerCharacter* a_player);
		bool IsContextValid(RE::PlayerCharacter* a_player) const;
		bool IsMenuBlocked() const;

		CursorVectorState m_cursor{};
		LatchedAttackState m_latched{};
		float m_dragScalar{ 0.0f };
		float m_targetInputX{ 0.0f };
		float m_targetInputY{ 0.0f };
		float m_smoothedInputX{ 0.0f };
		float m_smoothedInputY{ 0.0f };
		float m_lastNormInputX{ 0.0f };
		float m_lastNormInputY{ 0.0f };
		float m_appliedWeaponSpeedMod{ 0.0f };
		std::atomic_bool m_enabled{ true };
	};
}
