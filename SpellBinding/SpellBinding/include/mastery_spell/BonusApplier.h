#pragma once

#include "mastery_spell/MasteryData.h"

namespace SBO::MASTERY_SPELL
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
