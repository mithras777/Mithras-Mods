#pragma once

#include "PCH.h"

namespace PERILOUS
{
	class Hooks
	{
	public:
		static void Install();

	private:
		class AnimationEventHook
		{
		public:
			static void Install();

		private:
			static RE::BSEventNotifyControl Thunk(
				RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
				RE::BSAnimationGraphEvent* a_event,
				RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);

			static void Process(RE::BSAnimationGraphEvent* a_event);
			static inline REL::Relocation<decltype(Thunk)> _func;
		};

		class MeleeHitHook
		{
		public:
			static void Install();

		private:
			static void Thunk(RE::Actor* a_victim, RE::HitData& a_hitData);
			static inline REL::Relocation<decltype(Thunk)> _func;
		};
	};
}