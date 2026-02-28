#pragma once

#include "Settings.h"

#include <deque>
#include <mutex>

namespace DYNAMIC_SPAWNS
{
	struct ManagedSpawn
	{
		RE::FormID actorID{ 0 };
		RE::FormID cellID{ 0 };
		float      spawnGameTimeHours{ 0.0F };
		bool       persistent{ false };
	};

	class ManagedRegistry final : public REX::Singleton<ManagedRegistry>
	{
	public:
		void AddSpawn(RE::Actor* a_actor, RE::TESObjectCELL* a_cell);
		void ClearAll();
		void ClearCell(RE::FormID a_cellID);
		void PruneToLimit(std::int32_t a_maxManagedRefs);
		void Cleanup(const LimitsSettings& a_limits, RE::Actor* a_player, bool a_forceDistanceCheck);

		[[nodiscard]] std::int32_t CountAlive() const;
		[[nodiscard]] std::int32_t CountInCell(RE::FormID a_cellID) const;
		[[nodiscard]] bool HasSpawnNear(const RE::NiPoint3& a_point, float a_minDistance) const;

	private:
		static bool DespawnActor(RE::Actor* a_actor);

		mutable std::mutex            m_lock;
		std::deque<ManagedSpawn>      m_spawns;
	};
}
