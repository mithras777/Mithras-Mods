#include "event/mastery/MasteryEventSinks.h"

#include "mastery_shout/MasteryManager.h"
#include "mastery_spell/MasteryManager.h"
#include "mastery_weapon/MasteryManager.h"
#include "util/LogUtil.h"

namespace SBO::EVENT::MASTERY
{
	namespace
	{
		struct SpellCastSink final : RE::BSTEventSink<RE::TESSpellCastEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				SBO::MASTERY_SPELL::Manager::GetSingleton()->OnSpellCastEvent(*a_event);
				SBO::MASTERY_SHOUT::Manager::GetSingleton()->OnSpellCastEvent(*a_event);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		struct HitSink final : RE::BSTEventSink<RE::TESHitEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				SBO::MASTERY_SPELL::Manager::GetSingleton()->OnHitEvent(*a_event);
				SBO::MASTERY_SHOUT::Manager::GetSingleton()->OnHitEvent(*a_event);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		struct DeathSink final : RE::BSTEventSink<RE::TESDeathEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				SBO::MASTERY_SPELL::Manager::GetSingleton()->OnDeathEvent(*a_event);
				SBO::MASTERY_WEAPON::Manager::GetSingleton()->OnDeathEvent(*a_event);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		struct EquipSink final : RE::BSTEventSink<RE::TESEquipEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				SBO::MASTERY_SPELL::Manager::GetSingleton()->OnEquipEvent(*a_event);
				SBO::MASTERY_WEAPON::Manager::GetSingleton()->OnEquipEvent(*a_event);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		struct ShoutAttackSink final : RE::BSTEventSink<RE::ShoutAttack::Event>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::ShoutAttack::Event* a_event, RE::BSTEventSource<RE::ShoutAttack::Event>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				SBO::MASTERY_SHOUT::Manager::GetSingleton()->OnShoutAttack(*a_event);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		SpellCastSink g_spellCastSink{};
		HitSink g_hitSink{};
		DeathSink g_deathSink{};
		EquipSink g_equipSink{};
		ShoutAttackSink g_shoutAttackSink{};
	}

	void Register()
	{
		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			LOG_WARN("SpellbladeOverhaul: ScriptEventSourceHolder unavailable for mastery event sinks");
			return;
		}
		holder->AddEventSink<RE::TESSpellCastEvent>(&g_spellCastSink);
		holder->AddEventSink<RE::TESHitEvent>(&g_hitSink);
		holder->AddEventSink<RE::TESDeathEvent>(&g_deathSink);
		holder->AddEventSink<RE::TESEquipEvent>(&g_equipSink);

		if (auto* shoutSource = RE::ShoutAttack::GetEventSource(); shoutSource) {
			shoutSource->AddEventSink(&g_shoutAttackSink);
		}
		LOG_INFO("SpellbladeOverhaul: mastery event sinks registered");
	}
}
