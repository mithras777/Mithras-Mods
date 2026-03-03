#include "perilous/PerilousHooks.h"

#include "perilous/AnimSpeedManager.h"
#include "perilous/PerilousManager.h"
#include "util/LogUtil.h"

namespace
{
	constexpr std::uint32_t Hash(const char* a_data, std::size_t a_size) noexcept
	{
		std::uint32_t hash = 5381;
		for (const char* c = a_data; c < a_data + a_size; ++c) {
			hash = ((hash << 5) + hash) + static_cast<unsigned char>(*c);
		}
		return hash;
	}

	constexpr std::uint32_t operator"" _h(const char* a_str, std::size_t a_size) noexcept
	{
		return Hash(a_str, a_size);
	}

	bool IsPowerAttacking(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		auto* process = a_actor->GetActorRuntimeData().currentProcess;
		if (!process || !process->high || !process->high->attackData) {
			return false;
		}

		const auto flags = process->high->attackData->data.flags;
		return flags.any(RE::AttackData::AttackFlag::kPowerAttack);
	}
}

namespace PERILOUS
{
	void Hooks::Install()
	{
		AnimationEventHook::Install();
		MeleeHitHook::Install();
		AnimSpeedManager::InstallHooks();
	}

	void Hooks::AnimationEventHook::Install()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_Character[2] };
		_func = vtbl.write_vfunc(0x1, Thunk);
		LOG_INFO("Perilous hook: animation event");
	}

	RE::BSEventNotifyControl Hooks::AnimationEventHook::Thunk(
		RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
		RE::BSAnimationGraphEvent* a_event,
		RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		Process(a_event);
		return _func(a_sink, a_event, a_source);
	}

	void Hooks::AnimationEventHook::Process(RE::BSAnimationGraphEvent* a_event)
	{
		if (!a_event || !a_event->holder) {
			return;
		}

		auto* actor = const_cast<RE::TESObjectREFR*>(a_event->holder)->As<RE::Actor>();
		if (!actor) {
			return;
		}

		const std::string_view eventTag = a_event->tag.data();
		switch (Hash(eventTag.data(), eventTag.size())) {
		case "preHitFrame"_h:
			if (!actor->IsPlayerRef() && actor->IsInCombat() && IsPowerAttacking(actor)) {
				Manager::GetSingleton()->TryStartRedPerilous(actor);
			}
			break;
		case "attackStop"_h:
		case "bashStop"_h:
			Manager::GetSingleton()->EndPerilous(actor);
			break;
		default:
			break;
		}
	}

	void Hooks::MeleeHitHook::Install()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(37673, 38627) };
		_func = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x3C0, 0x4A8), Thunk);
		LOG_INFO("Perilous hook: melee hit");
	}

	void Hooks::MeleeHitHook::Thunk(RE::Actor* a_victim, RE::HitData& a_hitData)
	{
		if (auto attacker = a_hitData.aggressor.get().get()) {
			Manager::GetSingleton()->OnActorHit(attacker, a_victim);
		}
		_func(a_victim, a_hitData);
	}
}