#pragma once

#include "PCH.h"
#include <shared_mutex>
#include <vector>

namespace PERILOUS
{
	class AnimSpeedManager
	{
	public:
		static void InstallHooks();

		static void SetAnimSpeed(RE::ActorHandle a_actorHandle, float a_speedMult, float a_duration);
		static void RevertAnimSpeed(RE::ActorHandle a_actorHandle);
		static void RevertAllAnimSpeed();

	private:
		struct AnimSpeedData
		{
			float speedMult{ 1.0F };
			float remainingDuration{ 0.0F };
		};

		static void Update(RE::ActorHandle a_actorHandle, float& a_deltaTime);

		class OnUpdateAnimationInternal
		{
		public:
			static void Install();

		private:
			static void Thunk(RE::Actor* a_this, float a_deltaTime);
			static inline REL::Relocation<decltype(Thunk)> _func;
		};

		class OnUpdateAnimationPlayer
		{
		public:
			static void Install();

		private:
			static void Thunk(RE::PlayerCharacter* a_this, float a_deltaTime);
			static inline REL::Relocation<decltype(Thunk)> _func;
		};

		static inline std::vector<std::pair<RE::ActorHandle, AnimSpeedData>> _tracked;
		static inline std::shared_mutex _trackedLock;
	};
}
