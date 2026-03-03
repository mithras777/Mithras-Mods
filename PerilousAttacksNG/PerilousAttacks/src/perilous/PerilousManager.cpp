#include "perilous/PerilousManager.h"

#include "perilous/AnimSpeedManager.h"
#include "perilous/PerilousConfig.h"
#include "perilous/PerilousForms.h"
#include "util/LogUtil.h"
#include <algorithm>
#include <mutex>

namespace
{
	float GetPerilousChance(RE::Actor* a_actor, float a_multiplier)
	{
		if (!a_actor) {
			return 0.0F;
		}

		auto* combatController = a_actor->GetActorRuntimeData().combatController;
		if (!combatController || !combatController->combatStyle) {
			return 0.0F;
		}

		return std::clamp(combatController->combatStyle->generalData.offensiveMult * a_multiplier, 0.0F, 1.0F);
	}

	void Stagger(RE::Actor* a_attacker, RE::Actor* a_victim, float a_magnitude)
	{
		if (!a_attacker || !a_victim) {
			return;
		}

		const float headingAngle = a_victim->GetHeadingAngle(a_attacker->GetPosition(), true);
		const float direction = headingAngle >= 0.0F ? headingAngle / 360.0F : (360.0F + headingAngle) / 360.0F;
		a_victim->SetGraphVariableFloat("staggerDirection", direction);
		a_victim->SetGraphVariableFloat("StaggerMagnitude", a_magnitude);
		a_victim->NotifyAnimationGraph("staggerStart");
	}
}

namespace PERILOUS
{
	void Manager::InitForms()
	{
		Forms::GetSingleton()->InitForms();
	}

	void Manager::SetPerilousFlag(RE::Actor* a_actor, Type a_type) const
	{
		if (a_actor) {
			a_actor->SetGraphVariableInt("val_perilous_attack_type", static_cast<std::int32_t>(a_type));
		}
	}

	void Manager::ClearPerilousFlag(RE::Actor* a_actor) const
	{
		if (a_actor) {
			a_actor->SetGraphVariableInt("val_perilous_attack_type", 0);
		}
	}

	bool Manager::TryStartRedPerilous(RE::Actor* a_attacker)
	{
		const auto cfg = ConfigStore::GetSingleton()->Get();
		if (!cfg.bPerilousAttack_Enable || !a_attacker || a_attacker->IsPlayerRef() || !a_attacker->IsInCombat()) {
			return false;
		}

		auto targetHandle = a_attacker->GetActorRuntimeData().currentCombatTarget;
		if (!targetHandle) {
			return false;
		}

		EndPerilous(a_attacker);

		const float chance = GetPerilousChance(a_attacker, cfg.fPerilousAttack_Chance_Mult);
		if (chance <= 0.0F) {
			return false;
		}

		std::mt19937 generator(_randomDevice());
		std::uniform_real_distribution<float> distribution(0.0F, 1.0F);
		if (distribution(generator) > chance) {
			return false;
		}

		bool canStart = true;
		{
			std::shared_lock lock(_stateLock);
			auto it = std::find_if(_targetCounters.begin(), _targetCounters.end(), [&](const auto& entry) { return entry.first == targetHandle; });
			if (it != _targetCounters.end() && it->second >= cfg.iPerilous_MaxAttackersPerTarget) {
				canStart = false;
			}
		}

		if (!canStart) {
			return false;
		}

		{
			std::unique_lock lock(_stateLock);
			auto attackerIt = std::find_if(_attackerToTarget.begin(), _attackerToTarget.end(), [&](const auto& entry) { return entry.first == a_attacker->GetHandle(); });
			if (attackerIt != _attackerToTarget.end()) {
				attackerIt->second = targetHandle;
			} else {
				_attackerToTarget.emplace_back(a_attacker->GetHandle(), targetHandle);
			}

			auto counterIt = std::find_if(_targetCounters.begin(), _targetCounters.end(), [&](const auto& entry) { return entry.first == targetHandle; });
			if (counterIt != _targetCounters.end()) {
				++counterIt->second;
			} else {
				_targetCounters.emplace_back(targetHandle, 1);
			}
		}

		SetPerilousFlag(a_attacker, Type::kRed);

		auto* forms = Forms::GetSingleton();
		if (cfg.bPerilous_VFX_Enable && forms->HasRedVfx()) {
			forms->PlayRedVfx(a_attacker);
		}
		if (cfg.bPerilous_SFX_Enable && forms->HasSfx()) {
			forms->PlaySfx(a_attacker);
		}

		if (cfg.bPerilousAttack_ChargeTime_Enable) {
			AnimSpeedManager::SetAnimSpeed(
				a_attacker->GetHandle(),
				cfg.fPerilousAttack_ChargeTime_Mult,
				cfg.fPerilousAttack_ChargeTime_Duration);
		}

		return true;
	}

	void Manager::EndPerilous(RE::Actor* a_attacker)
	{
		if (!a_attacker) {
			return;
		}

		RE::ActorHandle targetHandle;
		bool wasActive = false;
		{
			std::unique_lock lock(_stateLock);
			auto it = std::find_if(_attackerToTarget.begin(), _attackerToTarget.end(), [&](const auto& entry) { return entry.first == a_attacker->GetHandle(); });
			if (it != _attackerToTarget.end()) {
				targetHandle = it->second;
				_attackerToTarget.erase(it);
				wasActive = true;
			}

			if (wasActive) {
				auto targetIt = std::find_if(_targetCounters.begin(), _targetCounters.end(), [&](const auto& entry) { return entry.first == targetHandle; });
				if (targetIt != _targetCounters.end()) {
					if (targetIt->second > 1) {
						--targetIt->second;
					} else {
						_targetCounters.erase(targetIt);
					}
				}
			}
		}

		if (wasActive) {
			ClearPerilousFlag(a_attacker);
			AnimSpeedManager::RevertAnimSpeed(a_attacker->GetHandle());
		}
	}

	bool Manager::IsPerilousAttacking(RE::Actor* a_attacker, RE::ActorHandle* a_outTarget) const
	{
		if (!a_attacker) {
			return false;
		}

		std::shared_lock lock(_stateLock);
		auto it = std::find_if(_attackerToTarget.begin(), _attackerToTarget.end(), [&](const auto& entry) { return entry.first == a_attacker->GetHandle(); });
		if (it == _attackerToTarget.end()) {
			return false;
		}

		if (a_outTarget) {
			*a_outTarget = it->second;
		}
		return true;
	}

	void Manager::OnPostLoadGameClear()
	{
		{
			std::unique_lock lock(_stateLock);
			_attackerToTarget.clear();
			_targetCounters.clear();
		}
		AnimSpeedManager::RevertAllAnimSpeed();
	}

	void Manager::OnActorHit(RE::Actor* a_attacker, RE::Actor* a_victim)
	{
		if (!a_attacker || !a_victim) {
			return;
		}

		if (IsPerilousAttacking(a_attacker, nullptr)) {
			Stagger(a_attacker, a_victim, 1.0F);
		}
	}
}
