#include "perilous/AnimSpeedManager.h"

#include "util/LogUtil.h"
#include <algorithm>
#include <mutex>

namespace PERILOUS
{
	void AnimSpeedManager::InstallHooks()
	{
		OnUpdateAnimationInternal::Install();
		OnUpdateAnimationPlayer::Install();
	}

	void AnimSpeedManager::SetAnimSpeed(RE::ActorHandle a_actorHandle, float a_speedMult, float a_duration)
	{
		std::unique_lock lock(_trackedLock);
		auto it = std::find_if(_tracked.begin(), _tracked.end(), [&](const auto& entry) { return entry.first == a_actorHandle; });
		if (it != _tracked.end()) {
			it->second = AnimSpeedData{ a_speedMult, a_duration };
		} else {
			_tracked.emplace_back(a_actorHandle, AnimSpeedData{ a_speedMult, a_duration });
		}
	}

	void AnimSpeedManager::RevertAnimSpeed(RE::ActorHandle a_actorHandle)
	{
		std::unique_lock lock(_trackedLock);
		_tracked.erase(
			std::remove_if(_tracked.begin(), _tracked.end(), [&](const auto& entry) { return entry.first == a_actorHandle; }),
			_tracked.end());
	}

	void AnimSpeedManager::RevertAllAnimSpeed()
	{
		std::unique_lock lock(_trackedLock);
		_tracked.clear();
	}

	void AnimSpeedManager::Update(RE::ActorHandle a_actorHandle, float& a_deltaTime)
	{
		if (a_deltaTime <= 0.0F) {
			return;
		}

		std::unique_lock lock(_trackedLock);
		auto it = std::find_if(_tracked.begin(), _tracked.end(), [&](const auto& entry) { return entry.first == a_actorHandle; });
		if (it == _tracked.end()) {
			return;
		}

		auto& data = it->second;
		const float nextRemaining = data.remainingDuration - a_deltaTime;
		float blend = 1.0F;
		if (nextRemaining <= 0.0F) {
			blend = (a_deltaTime + nextRemaining) / a_deltaTime;
		}

		data.remainingDuration = nextRemaining;
		a_deltaTime *= data.speedMult + ((1.0F - data.speedMult) * (1.0F - blend));

		if (nextRemaining <= 0.0F) {
			_tracked.erase(it);
		}
	}

	void AnimSpeedManager::OnUpdateAnimationInternal::Install()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(40436, 41453) };
		_func = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x74, 0x74), Thunk);
		LOG_INFO("Perilous hook: update animation internal");
	}

	void AnimSpeedManager::OnUpdateAnimationInternal::Thunk(RE::Actor* a_this, float a_deltaTime)
	{
		Update(a_this->GetHandle(), a_deltaTime);
		_func(a_this, a_deltaTime);
	}

	void AnimSpeedManager::OnUpdateAnimationPlayer::Install()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
		_func = vtbl.write_vfunc(0x7D, Thunk);
		LOG_INFO("Perilous hook: update animation player");
	}

	void AnimSpeedManager::OnUpdateAnimationPlayer::Thunk(RE::PlayerCharacter* a_this, float a_deltaTime)
	{
		Update(a_this->GetHandle(), a_deltaTime);
		_func(a_this, a_deltaTime);
	}
}
