#include "ManagedRegistry.h"

#include "util/LogUtil.h"

namespace DYNAMIC_SPAWNS
{
	namespace
	{
		float GetGameHours()
		{
			if (const auto* calendar = RE::Calendar::GetSingleton()) {
				return calendar->GetHoursPassed();
			}
			return 0.0F;
		}
	}

	void ManagedRegistry::AddSpawn(RE::Actor* a_actor, RE::TESObjectCELL* a_cell)
	{
		if (!a_actor) {
			return;
		}

		ManagedSpawn entry{};
		entry.actorID = a_actor->GetFormID();
		entry.cellID = a_cell ? a_cell->GetFormID() : 0;
		entry.spawnGameTimeHours = GetGameHours();
		entry.persistent = a_actor->IsPersistent();

		std::scoped_lock lk(m_lock);
		m_spawns.push_back(entry);
	}

	void ManagedRegistry::ClearAll()
	{
		std::scoped_lock lk(m_lock);
		for (const auto& spawn : m_spawns) {
			if (auto* actor = RE::TESForm::LookupByID<RE::Actor>(spawn.actorID)) {
				DespawnActor(actor);
			}
		}
		m_spawns.clear();
	}

	void ManagedRegistry::ClearCell(RE::FormID a_cellID)
	{
		std::scoped_lock lk(m_lock);
		for (auto it = m_spawns.begin(); it != m_spawns.end();) {
			if (it->cellID != a_cellID) {
				++it;
				continue;
			}

			if (auto* actor = RE::TESForm::LookupByID<RE::Actor>(it->actorID)) {
				DespawnActor(actor);
			}
			it = m_spawns.erase(it);
		}
	}

	void ManagedRegistry::PruneToLimit(std::int32_t a_maxManagedRefs)
	{
		std::scoped_lock lk(m_lock);
		if (a_maxManagedRefs < 1) {
			a_maxManagedRefs = 1;
		}

		auto removeDeadFirst = [&]() {
			for (auto it = m_spawns.begin(); it != m_spawns.end();) {
				auto* actor = RE::TESForm::LookupByID<RE::Actor>(it->actorID);
				if (!actor || actor->IsDead() || actor->IsDisabled()) {
					if (actor) {
						DespawnActor(actor);
					}
					it = m_spawns.erase(it);
					if (static_cast<std::int32_t>(m_spawns.size()) <= a_maxManagedRefs) {
						return;
					}
				} else {
					++it;
				}
			}
		};

		removeDeadFirst();

		while (static_cast<std::int32_t>(m_spawns.size()) > a_maxManagedRefs) {
			const auto oldest = m_spawns.front();
			m_spawns.pop_front();
			if (auto* actor = RE::TESForm::LookupByID<RE::Actor>(oldest.actorID)) {
				DespawnActor(actor);
			}
		}
	}

	void ManagedRegistry::Cleanup(const LimitsSettings& a_limits, RE::Actor* a_player, bool a_forceDistanceCheck)
	{
		std::scoped_lock lk(m_lock);
		const auto playerPos = a_player ? a_player->GetPosition() : RE::NiPoint3{};

		for (auto it = m_spawns.begin(); it != m_spawns.end();) {
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(it->actorID);
			if (!actor) {
				it = m_spawns.erase(it);
				continue;
			}

			bool remove = actor->IsDead() || actor->IsDisabled();
			if (!remove && a_player && a_forceDistanceCheck) {
				const auto dist = actor->GetPosition().GetDistance(playerPos);
				if (dist > a_limits.despawnDistance) {
					remove = true;
				}
			}

			if (remove) {
				DespawnActor(actor);
				it = m_spawns.erase(it);
			} else {
				++it;
			}
		}
	}

	std::int32_t ManagedRegistry::CountAlive() const
	{
		std::scoped_lock lk(m_lock);
		std::int32_t count = 0;

		for (const auto& spawn : m_spawns) {
			const auto* actor = RE::TESForm::LookupByID<RE::Actor>(spawn.actorID);
			if (actor && !actor->IsDead() && !actor->IsDisabled()) {
				++count;
			}
		}
		return count;
	}

	std::int32_t ManagedRegistry::CountInCell(RE::FormID a_cellID) const
	{
		std::scoped_lock lk(m_lock);
		std::int32_t count = 0;

		for (const auto& spawn : m_spawns) {
			if (spawn.cellID != a_cellID) {
				continue;
			}
			const auto* actor = RE::TESForm::LookupByID<RE::Actor>(spawn.actorID);
			if (actor && !actor->IsDead() && !actor->IsDisabled()) {
				++count;
			}
		}
		return count;
	}

	bool ManagedRegistry::HasSpawnNear(const RE::NiPoint3& a_point, float a_minDistance) const
	{
		std::scoped_lock lk(m_lock);
		for (const auto& spawn : m_spawns) {
			const auto* actor = RE::TESForm::LookupByID<RE::Actor>(spawn.actorID);
			if (!actor || actor->IsDead() || actor->IsDisabled()) {
				continue;
			}
			if (actor->GetPosition().GetDistance(a_point) < a_minDistance) {
				return true;
			}
		}
		return false;
	}

	bool ManagedRegistry::DespawnActor(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		if (!a_actor->IsDisabled()) {
			a_actor->Disable();
		}
		a_actor->SetDelete(true);
		return true;
	}
}
