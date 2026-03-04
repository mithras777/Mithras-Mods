#include "mastery_weapon/BonusApplier.h"

#include "util/LogUtil.h"

namespace SBO::MASTERY_WEAPON
{
	namespace
	{
		void ModAV(RE::PlayerCharacter* a_player, RE::ActorValue a_av, float a_delta)
		{
			if (!a_player || a_delta == 0.0f) {
				return;
			}
			a_player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, a_av, a_delta);
		}
	}

	void BonusApplier::Apply(RE::PlayerCharacter* a_player, const MasteryBonuses& a_bonuses, bool a_hasShield)
	{
		if (!a_player) {
			return;
		}

		Clear(a_player);

		m_applied = a_bonuses;
		m_hasShield = a_hasShield;
		m_active = true;

		ModAV(a_player, RE::ActorValue::kAttackDamageMult, m_applied.dmgMult);
		ModAV(a_player, RE::ActorValue::kWeaponSpeedMult, m_applied.atkSpeed);
		ModAV(a_player, RE::ActorValue::kCriticalChance, m_applied.critChance);

		if (m_applied.staminaPowerCostMult < 1.0f) {
			const float staminaRegenBump = (1.0f - m_applied.staminaPowerCostMult) * 5.0f;
			ModAV(a_player, RE::ActorValue::kStaminaRateMult, staminaRegenBump);
		}

		if (m_hasShield && m_applied.blockStaminaMult < 1.0f) {
			const float blockPower = (1.0f - m_applied.blockStaminaMult) * 20.0f;
			ModAV(a_player, RE::ActorValue::kBlockPowerModifier, blockPower);
		}
	}

	void BonusApplier::Clear(RE::PlayerCharacter* a_player)
	{
		if (!a_player || !m_active) {
			return;
		}

		ModAV(a_player, RE::ActorValue::kAttackDamageMult, -m_applied.dmgMult);
		ModAV(a_player, RE::ActorValue::kWeaponSpeedMult, -m_applied.atkSpeed);
		ModAV(a_player, RE::ActorValue::kCriticalChance, -m_applied.critChance);

		if (m_applied.staminaPowerCostMult < 1.0f) {
			const float staminaRegenBump = (1.0f - m_applied.staminaPowerCostMult) * 5.0f;
			ModAV(a_player, RE::ActorValue::kStaminaRateMult, -staminaRegenBump);
		}

		if (m_hasShield && m_applied.blockStaminaMult < 1.0f) {
			const float blockPower = (1.0f - m_applied.blockStaminaMult) * 20.0f;
			ModAV(a_player, RE::ActorValue::kBlockPowerModifier, -blockPower);
		}

		m_applied = {};
		m_hasShield = false;
		m_active = false;
	}
}
