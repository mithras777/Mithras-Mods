#include "jumpattack/DamageUtil.h"

namespace
{
    float GetWeaponSkillLevel(RE::PlayerCharacter* a_player, RE::TESObjectWEAP* a_weapon)
    {
        if (!a_player || !a_weapon) {
            return 0.0f;
        }
        auto* avo = a_player->AsActorValueOwner();
        return avo ? avo->GetActorValue(a_weapon->weaponData.skill.get()) : 0.0f;
    }

    float ComputeDamage(RE::PlayerCharacter* a_player, RE::TESObjectWEAP* a_weapon, JA::DamageUtil::AttackKind a_kind)
    {
        if (!a_player) {
            return 0.0f;
        }

        float base = 0.0f;
        if (a_weapon) {
            const auto rawWeaponDamage = static_cast<float>(a_weapon->GetAttackDamage());
            const auto equippedDamage = a_player->GetEquippedWeaponsDamage();
            base = std::max(rawWeaponDamage, equippedDamage);
        } else {
            auto* avo = a_player->AsActorValueOwner();
            const float unarmed = avo ? avo->GetActorValue(RE::ActorValue::kUnarmedDamage) : 1.0f;
            base = std::max(1.0f, unarmed);
        }

        const float skill = GetWeaponSkillLevel(a_player, a_weapon);
        const float skillMult = 1.0f + (std::clamp(skill, 0.0f, 200.0f) * 0.003f);
        const float powerMult = (a_kind == JA::DamageUtil::AttackKind::kPower) ? 1.5f : 1.0f;

        return std::max(1.0f, base * skillMult * powerMult);
    }

    float ComputeStagger(RE::TESObjectWEAP* a_weapon, const JA::SettingsData& a_settings)
    {
        if (!a_settings.applyStagger) {
            return 0.0f;
        }
        const float weaponStagger = a_weapon ? a_weapon->GetStagger() : 0.25f;
        return std::max(0.0f, weaponStagger * a_settings.staggerMagnitude);
    }
}

namespace JA::DamageUtil
{
    DamageResult ApplyJumpHit(
        RE::PlayerCharacter* a_player,
        RE::Actor* a_target,
        const TraceUtil::HitResult& a_hit,
        AttackKind a_kind,
        const SettingsData& a_settings)
    {
        if (!a_player || !a_target || a_target->IsDead()) {
            return {};
        }

        auto weapon = a_player->GetEquippedObject(false) ? a_player->GetEquippedObject(false)->As<RE::TESObjectWEAP>() : nullptr;
        const float damage = ComputeDamage(a_player, weapon, a_kind);
        const float stagger = ComputeStagger(weapon, a_settings);

        // Engine damage path for actor-vs-actor health damage.
        a_target->HandleHealthDamage(a_player, damage);

        if (stagger > 0.0f) {
            if (auto* target3D = a_target->Get3D()) {
                RE::hkVector4 impulsePos(a_hit.hitPoint);
                const float impulseForce = 120.0f * stagger;
                RE::TESHavokUtilities::AddExplosionImpulse(target3D, impulsePos, impulseForce, nullptr);
            }
        }

        if (weapon && weapon->attackSound) {
            RE::BSSoundHandle sound{};
            a_target->PlayASound(sound, weapon->attackSound->GetFormID(), false, 0);
        }

        return { damage, stagger };
    }
}
