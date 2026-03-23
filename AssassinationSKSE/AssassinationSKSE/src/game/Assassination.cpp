#include "game/Assassination.h"

#include "util/LogUtil.h"

#include "RE/A/Actor.h"
#include "RE/B/BSTEvent.h"
#include "RE/S/ScriptEventSourceHolder.h"
#include "RE/T/TESHitEvent.h"
#include "RE/T/TESForm.h"
#include "RE/T/TESObjectWEAP.h"

namespace GAME::ASSASSINATION {
	namespace {
		enum class AttackKind
		{
			kUnknown,
			kMelee,
			kUnarmed,
			kBowCrossbow
		};

		Config g_config{};

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

		void ForceDeath(RE::Actor* a_victim, RE::Actor* a_attacker)
		{
			if (!a_victim) {
				return;
			}

			a_victim->KillImpl(a_attacker, 100000.0f, true, false);
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

			const auto attackKind = ClassifyAttack(a_attacker, a_event);
			if (!IsAttackEnabled(attackKind, g_config)) {
				return false;
			}

			return IsUnseenByVictim(a_attacker, a_victim, a_event);
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

	Config GetConfig()
	{
		return g_config;
	}

	void SetConfig(const Config& a_config)
	{
		g_config = a_config;
	}

	void Register()
	{
		GetManager().Register();
		LOG_INFO("Assassination hit listener registered");
	}
}
