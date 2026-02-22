#pragma once

#include "mastery/MasteryData.h"

namespace MITHRAS::SHOUT_MASTERY
{
	class BonusApplier
	{
	public:
		void Apply(RE::PlayerCharacter* a_player, const MasteryBonuses& a_bonuses);
		void Clear(RE::PlayerCharacter* a_player);

	private:
		MasteryBonuses m_applied{};
		bool m_active{ false };
	};
}
