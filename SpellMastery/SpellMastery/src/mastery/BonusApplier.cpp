#include "mastery/BonusApplier.h"

namespace MITHRAS::SPELL_MASTERY
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

		for (std::size_t i = 0; i < kSchoolCount; ++i) {
			const auto school = static_cast<SpellSchool>(i);
			ModAV(a_player, SchoolToSkillAV(school), m_applied.schoolSkillBonus[i]);
			ModAV(a_player, SchoolToSkillAdvanceAV(school), m_applied.schoolSkillAdvanceBonus[i]);
			ModAV(a_player, SchoolToPowerAV(school), m_applied.schoolPowerBonus[i]);
			ModAV(a_player, SchoolToCostAV(school), m_applied.schoolCostReduction[i]);
		}
		ModAV(a_player, RE::ActorValue::kMagickaRateMult, m_applied.magickaRateMult);
		ModAV(a_player, RE::ActorValue::kMagicka, m_applied.magickaFlat);
	}

	void BonusApplier::Clear(RE::PlayerCharacter* a_player)
	{
		if (!a_player || !m_active) {
			return;
		}

		for (std::size_t i = 0; i < kSchoolCount; ++i) {
			const auto school = static_cast<SpellSchool>(i);
			ModAV(a_player, SchoolToSkillAV(school), -m_applied.schoolSkillBonus[i]);
			ModAV(a_player, SchoolToSkillAdvanceAV(school), -m_applied.schoolSkillAdvanceBonus[i]);
			ModAV(a_player, SchoolToPowerAV(school), -m_applied.schoolPowerBonus[i]);
			ModAV(a_player, SchoolToCostAV(school), -m_applied.schoolCostReduction[i]);
		}
		ModAV(a_player, RE::ActorValue::kMagickaRateMult, -m_applied.magickaRateMult);
		ModAV(a_player, RE::ActorValue::kMagicka, -m_applied.magickaFlat);

		m_applied = {};
		m_active = false;
	}
}
