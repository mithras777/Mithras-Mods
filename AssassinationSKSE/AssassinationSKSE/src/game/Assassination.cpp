#include "game/AssassinationSettings.h"

#include "util/LogUtil.h"

#include "RE/A/Actor.h"
#include "RE/B/BSTEvent.h"
#include "RE/S/ScriptEventSourceHolder.h"
#include "RE/T/TESHitEvent.h"
#include "RE/T/TESForm.h"
#include "RE/T/TESObjectWEAP.h"

#include <algorithm>
#include <random>
#include <string_view>

namespace GAME::ASSASSINATION {
	namespace {
		enum class AttackKind
		{
			kUnknown,
			kMelee,
			kUnarmed,
			kBowCrossbow
		};

		using weapon_type_t = RE::WEAPON_TYPE;

		[[nodiscard]] AttackKind ClassifyAttack(RE::Actor* a_attacker, const RE::TESHitEvent& a_event)
		{
			const auto* source = RE::TESForm::LookupByID(a_event.source);
			if (const auto* weapon = source ? source->As<RE::TESObjectWEAP>() : nullptr) {
				const auto type = weapon->GetWeaponType();
				if (type == weapon_type_t::kBow || type == weapon_type_t::kCrossbow) {
					return AttackKind::kBowCrossbow;
				}

				if (type == weapon_type_t::kHandToHandMelee) {
					return AttackKind::kUnarmed;
				}

				if (type == weapon_type_t::kStaff) {
					return AttackKind::kUnknown;
				}

				return AttackKind::kMelee;
			}

			if (!a_attacker) {
				return AttackKind::kUnknown;
			}

			const auto* rightHand = a_attacker->GetEquippedObject(false);
			const auto* leftHand = a_attacker->GetEquippedObject(true);
			const auto* weapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
			if (!weapon && leftHand) {
				weapon = leftHand->As<RE::TESObjectWEAP>();
			}

			if (weapon) {
				switch (weapon->GetWeaponType()) {
				case weapon_type_t::kBow:
				case weapon_type_t::kCrossbow:
					return AttackKind::kBowCrossbow;
				case weapon_type_t::kHandToHandMelee:
					return AttackKind::kUnarmed;
				case weapon_type_t::kStaff:
					return AttackKind::kUnknown;
				default:
					return AttackKind::kMelee;
				}
			}

			return AttackKind::kUnarmed;
		}

		[[nodiscard]] bool IsAttackEnabled(AttackKind a_kind, const Config& a_config)
		{
			if (!a_config.enabled) {
				return false;
			}

			switch (a_kind) {
			case AttackKind::kMelee:
				return a_config.melee;
			case AttackKind::kUnarmed:
				return a_config.unarmed;
			case AttackKind::kBowCrossbow:
				return a_config.bowsCrossbows;
			default:
				return false;
			}
		}

		[[nodiscard]] bool HasActorKeyword(RE::Actor* a_actor, std::string_view a_keyword)
		{
			if (!a_actor) {
				return false;
			}

			return a_actor->HasKeywordString(a_keyword);
		}

		[[nodiscard]] bool IsDragon(RE::Actor* a_actor)
		{
			if (!a_actor) {
				return false;
			}

			if (HasActorKeyword(a_actor, "ActorTypeDragon")) {
				return true;
			}

			const auto* race = a_actor->GetRace();
			return race && race->HasKeywordString("ActorTypeDragon");
		}

		[[nodiscard]] bool IsGiant(RE::Actor* a_actor)
		{
			if (!a_actor) {
				return false;
			}

			if (HasActorKeyword(a_actor, "ActorTypeGiant")) {
				return true;
			}

			const auto* race = a_actor->GetRace();
			return race && race->HasKeywordString("ActorTypeGiant");
		}

		[[nodiscard]] bool IsBossLike(RE::Actor* a_actor)
		{
			if (!a_actor) {
				return false;
			}

			if (HasActorKeyword(a_actor, "ActorTypeBoss")) {
				return true;
			}

			const auto* actorBase = a_actor->GetActorBase();
			if (!actorBase) {
				return false;
			}

			return actorBase->IsUnique() || actorBase->IsEssential() || actorBase->IsProtected();
		}

		[[nodiscard]] bool IsTargetExcluded(RE::Actor* a_victim, const Config& a_config)
		{
			if (!a_victim) {
				return false;
			}

			if (a_config.excludeDragons && IsDragon(a_victim)) {
				return true;
			}

			if (a_config.excludeGiants && IsGiant(a_victim)) {
				return true;
			}

			if (a_config.excludeBosses && IsBossLike(a_victim)) {
				return true;
			}

			return false;
		}

		[[nodiscard]] bool RollAssassinationChance(const Config& a_config)
		{
			const auto chance = std::clamp(a_config.assassinationChance, 0, 100);
			if (chance <= 0) {
				return false;
			}

			if (chance >= 100) {
				return true;
			}

			static thread_local std::mt19937 rng{ std::random_device{}() };
			std::uniform_int_distribution<int> dist(1, 100);
			return dist(rng) <= chance;
		}

		bool IsUnseenByVictim(RE::Actor* a_attacker, RE::Actor* a_victim, const RE::TESHitEvent& a_event)
		{
			if (!a_attacker || !a_victim) {
				return false;
			}

			if (a_event.flags.any(RE::TESHitEvent::Flag::kSneakAttack)) {
				return true;
			}

			bool hasLOS = false;
			const bool victimSeesAttacker = a_victim->HasLineOfSight(a_attacker, hasLOS);
			return !victimSeesAttacker && !hasLOS;
		}

		[[nodiscard]] bool IsWithinLevelDifference(RE::Actor* a_attacker, RE::Actor* a_victim, const Config& a_config)
		{
			if (!a_attacker || !a_victim) {
				return false;
			}

			const auto attackerLevel = static_cast<int>(a_attacker->GetLevel());
			const auto victimLevel = static_cast<int>(a_victim->GetLevel());
			const auto maxDifference = a_config.maxLevelDifference < 0 ? 0 : a_config.maxLevelDifference;
			return victimLevel <= attackerLevel + maxDifference;
		}

		void ForceDeath(RE::Actor* a_victim, RE::Actor* a_attacker)
		{
			if (!a_victim) {
				return;
			}

			constexpr float lethalDamage = 100000.0f;
			LOG_INFO("[Assassination] Applying lethal damage to {} via {}", a_victim->GetName(), lethalDamage);
			a_victim->DoDamage(lethalDamage, a_attacker, true);
		}

		bool ShouldAssassinate(RE::Actor* a_attacker, RE::Actor* a_victim, const RE::TESHitEvent& a_event)
		{
			if (!a_attacker || !a_victim) {
				return false;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || a_attacker != player) {
				return false;
			}

			if (!a_attacker->IsSneaking() || a_attacker->IsDead() || a_attacker->IsInKillMove()) {
				return false;
			}

			if (a_victim->IsDead() || a_victim->IsInKillMove() || a_victim->IsOnMount()) {
				return false;
			}

			if (a_event.flags.any(RE::TESHitEvent::Flag::kBashAttack) || a_event.flags.any(RE::TESHitEvent::Flag::kHitBlocked)) {
				return false;
			}

			const auto config = Settings::GetSingleton()->Get();

			const auto attackKind = ClassifyAttack(a_attacker, a_event);
			if (!IsAttackEnabled(attackKind, config)) {
				return false;
			}

			if (!a_attacker->IsSneaking() || !IsUnseenByVictim(a_attacker, a_victim, a_event)) {
				return false;
			}

			if (!IsWithinLevelDifference(a_attacker, a_victim, config)) {
				return false;
			}

			if (IsTargetExcluded(a_victim, config)) {
				return false;
			}

			return RollAssassinationChance(config);
		}

		class HitManager final : public RE::BSTEventSink<RE::TESHitEvent> {
		public:
			void Register()
			{
				auto* source = RE::ScriptEventSourceHolder::GetSingleton();
				if (source) {
					source->AddEventSink<RE::TESHitEvent>(this);
				}
			}

		private:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override
			{
				if (!a_event || !a_event->target || !a_event->cause) {
					return RE::BSEventNotifyControl::kContinue;
				}

				auto* attacker = a_event->cause.get()->As<RE::Actor>();
				auto* victim = a_event->target.get()->As<RE::Actor>();
				if (!ShouldAssassinate(attacker, victim, *a_event)) {
					return RE::BSEventNotifyControl::kContinue;
				}

				LOG_DEBUG("[Assassination] Sneak assassination on {}", victim->GetName());
				ForceDeath(victim, attacker);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		HitManager& GetManager()
		{
			static HitManager manager{};
			return manager;
		}
	}

	void Register()
	{
		GetManager().Register();
		LOG_INFO("Assassination hit listener registered");
	}
}
