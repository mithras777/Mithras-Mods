#pragma once

#include "jumpattack/Settings.h"
#include "jumpattack/TraceUtil.h"

namespace JA::DamageUtil
{
    enum class AttackKind
    {
        kLight,
        kPower
    };

    struct DamageResult
    {
        float damageApplied{ 0.0f };
        float staggerApplied{ 0.0f };
    };

    [[nodiscard]] DamageResult ApplyJumpHit(
        RE::PlayerCharacter* a_player,
        RE::Actor* a_target,
        const TraceUtil::HitResult& a_hit,
        AttackKind a_kind,
        const SettingsData& a_settings);
}
