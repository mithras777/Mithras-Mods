#pragma once

#include "PCH.h"
#include <shared_mutex>
#include <vector>

namespace PERILOUS
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		enum class Type : std::int32_t
		{
			kNone = 0,
			kRed = 2
		};

		void InitForms();
		bool TryStartRedPerilous(RE::Actor* a_attacker);
		void EndPerilous(RE::Actor* a_attacker);
		bool IsPerilousAttacking(RE::Actor* a_attacker, RE::ActorHandle* a_outTarget = nullptr) const;
		void OnPostLoadGameClear();
		void OnActorHit(RE::Actor* a_attacker, RE::Actor* a_victim);

	private:
		void SetPerilousFlag(RE::Actor* a_actor, Type a_type) const;
		void ClearPerilousFlag(RE::Actor* a_actor) const;

		mutable std::shared_mutex _stateLock;
		std::vector<std::pair<RE::ActorHandle, RE::ActorHandle>> _attackerToTarget;
		std::vector<std::pair<RE::ActorHandle, std::uint32_t>> _targetCounters;
		std::random_device _randomDevice;
	};
}
