#pragma once

#include "mastery/MasteryData.h"

namespace MITHRAS::MASTERY
{
	class BonusApplier
	{
	public:
		void Apply(RE::PlayerCharacter* a_player, const MasteryBonuses& a_bonuses, bool a_hasShield);
		void Clear(RE::PlayerCharacter* a_player);

	private:
		MasteryBonuses m_applied{};
		bool m_hasShield{ false };
		bool m_active{ false };
	};
}
