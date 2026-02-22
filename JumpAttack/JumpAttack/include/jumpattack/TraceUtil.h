#pragma once

#include "jumpattack/Settings.h"

#include <limits>
#include <optional>

namespace JA::TraceUtil
{
    struct HitResult
    {
        RE::Actor* target{ nullptr };
        RE::NiPoint3 hitPoint{};
        float distance{ std::numeric_limits<float>::max() };
    };

    [[nodiscard]] bool GetCameraRay(RE::NiPoint3& a_originOut, RE::NiPoint3& a_forwardOut);
    [[nodiscard]] std::optional<HitResult> FindBestTarget(RE::PlayerCharacter* a_player, const SettingsData& a_settings);
}
