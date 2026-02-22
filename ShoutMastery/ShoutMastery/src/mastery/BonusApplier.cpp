#include "mastery/BonusApplier.h"

namespace MITHRAS::SHOUT_MASTERY
{
	namespace
	{
		void ModAV(RE::PlayerCharacter* a_player, RE::ActorValue a_av, float a_delta)
		{
			if (!a_player || a_delta == 0.0f || a_av == RE::ActorValue::kNone) {
				return;
			}
			a_player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, a_av, a_delta);
		}
	}

	void BonusApplier::Apply(RE::PlayerCharacter* a_player, const MasteryBonuses& a_bonuses)
	{
		if (!a_player) {
			return;
		}

		Clear(a_player);
		m_applied = a_bonuses;
		m_active = true;

		ModAV(a_player, RE::ActorValue::kShoutRecoveryMult, m_applied.shoutRecoveryMult);
		ModAV(a_player, RE::ActorValue::kVoiceRate, m_applied.voiceRate);
		ModAV(a_player, RE::ActorValue::kVoicePoints, m_applied.voicePoints);
		ModAV(a_player, RE::ActorValue::kStaminaRateMult, m_applied.staminaRateMult);
	}

	void BonusApplier::Clear(RE::PlayerCharacter* a_player)
	{
		if (!a_player || !m_active) {
			return;
		}

		ModAV(a_player, RE::ActorValue::kShoutRecoveryMult, -m_applied.shoutRecoveryMult);
		ModAV(a_player, RE::ActorValue::kVoiceRate, -m_applied.voiceRate);
		ModAV(a_player, RE::ActorValue::kVoicePoints, -m_applied.voicePoints);
		ModAV(a_player, RE::ActorValue::kStaminaRateMult, -m_applied.staminaRateMult);

		m_applied = {};
		m_active = false;
	}
}
