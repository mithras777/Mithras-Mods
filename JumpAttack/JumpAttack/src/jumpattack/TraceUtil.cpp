#include "jumpattack/TraceUtil.h"

namespace
{
    constexpr auto kForward = RE::NiPoint3(0.0f, 1.0f, 0.0f);
    constexpr float kPi = 3.14159265358979323846f;

    bool Normalize(RE::NiPoint3& a_vec)
    {
        return a_vec.Unitize() > 1e-4f;
    }

    std::vector<RE::NiPoint3> BuildRayDirections(const RE::NiPoint3& a_forward, std::uint32_t a_count, float a_angleDegrees)
    {
        std::vector<RE::NiPoint3> result{};
        result.reserve(a_count);
        result.push_back(a_forward);

        if (a_count <= 1 || a_angleDegrees <= 0.001f) {
            return result;
        }

        auto camera = RE::PlayerCamera::GetSingleton();
        if (!camera || !camera->cameraRoot) {
            return result;
        }

        auto right = camera->cameraRoot->world.rotate.GetVectorX();
        auto up = camera->cameraRoot->world.rotate.GetVectorZ();
        if (!Normalize(right) || !Normalize(up)) {
            return result;
        }

        const float cone = a_angleDegrees * (kPi / 180.0f);
        const float cosA = std::cos(cone);
        const float sinA = std::sin(cone);

        for (std::uint32_t i = 1; i < a_count; i++) {
            const float phase = (2.0f * kPi * static_cast<float>(i - 1)) / static_cast<float>(a_count - 1);
            auto radial = (right * std::cos(phase)) + (up * std::sin(phase));
            if (!Normalize(radial)) {
                continue;
            }

            auto dir = (a_forward * cosA) + (radial * sinA);
            if (Normalize(dir)) {
                result.push_back(dir);
            }
        }

        return result;
    }

    std::optional<RE::TESObjectREFR*> RaycastFirstRef(RE::Actor* a_player, const RE::NiPoint3& a_from, const RE::NiPoint3& a_to, float* a_hitFractionOut = nullptr)
    {
        if (!a_player) {
            return std::nullopt;
        }

        auto tes = RE::TES::GetSingleton();
        if (!tes) {
            return std::nullopt;
        }

        RE::bhkPickData pick{};
        pick.rayInput.from = RE::hkVector4(a_from);
        pick.rayInput.to = RE::hkVector4(a_to);
        pick.rayInput.enableShapeCollectionFilter = true;
        a_player->GetCollisionFilterInfo(pick.rayInput.filterInfo);

        auto* pickedNode = tes->Pick(pick);
        if (!pick.rayOutput.HasHit()) {
            return std::nullopt;
        }

        if (a_hitFractionOut) {
            *a_hitFractionOut = pick.rayOutput.hitFraction;
        }

        RE::TESObjectREFR* hitRef = nullptr;
        if (pick.rayOutput.rootCollidable) {
            hitRef = RE::TESHavokUtilities::FindCollidableRef(*pick.rayOutput.rootCollidable);
        }
        if (!hitRef && pickedNode) {
            hitRef = pickedNode->GetUserData();
        }
        return hitRef;
    }
}

namespace JA::TraceUtil
{
    bool GetCameraRay(RE::NiPoint3& a_originOut, RE::NiPoint3& a_forwardOut)
    {
        auto camera = RE::PlayerCamera::GetSingleton();
        if (!camera || !camera->cameraRoot) {
            return false;
        }

        a_originOut = camera->cameraRoot->world.translate;
        a_forwardOut = camera->cameraRoot->world.rotate * kForward;
        return Normalize(a_forwardOut);
    }

    std::optional<HitResult> FindBestTarget(RE::PlayerCharacter* a_player, const SettingsData& a_settings)
    {
        if (!a_player) {
            return std::nullopt;
        }

        RE::NiPoint3 origin{};
        RE::NiPoint3 forward{};
        if (!GetCameraRay(origin, forward)) {
            return std::nullopt;
        }

        const auto directions = BuildRayDirections(forward, a_settings.raysCount, a_settings.coneAngleDegrees);
        if (directions.empty()) {
            return std::nullopt;
        }

        std::optional<HitResult> best{};
        auto tes = RE::TES::GetSingleton();
        if (!tes) {
            return std::nullopt;
        }

        tes->ForEachReferenceInRange(a_player, a_settings.range + 32.0f, [&](RE::TESObjectREFR* a_ref) {
            auto actor = a_ref ? a_ref->As<RE::Actor>() : nullptr;
            if (!actor || actor == a_player || actor->IsDead()) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

            auto targetPoint = actor->GetLookingAtLocation();
            auto toTarget = targetPoint - origin;
            const float distance = toTarget.Length();
            if (distance <= 1e-3f || distance > a_settings.range) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

            auto dirToTarget = toTarget / distance;
            const float radius = std::max(actor->GetBoundRadius() * 0.70f, 20.0f);

            bool passesCone = false;
            for (const auto& rayDir : directions) {
                const float dot = std::clamp(rayDir.Dot(dirToTarget), -1.0f, 1.0f);
                if (dot <= 0.0f) {
                    continue;
                }
                const float lateral = std::sqrt(std::max(0.0f, 1.0f - (dot * dot))) * distance;
                if (lateral <= radius) {
                    passesCone = true;
                    break;
                }
            }
            if (!passesCone) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

            if (!a_settings.allowThroughActorsOnly) {
                float hitFraction = 1.0f;
                auto hitRef = RaycastFirstRef(a_player, origin, targetPoint, &hitFraction);
                if (hitRef.has_value()) {
                    auto hitActor = (*hitRef) ? (*hitRef)->As<RE::Actor>() : nullptr;
                    if (!hitActor || hitActor != actor) {
                        return RE::BSContainer::ForEachResult::kContinue;
                    }
                }
            }

            if (!best.has_value() || distance < best->distance) {
                best = HitResult{ actor, targetPoint, distance };
            }

            return RE::BSContainer::ForEachResult::kContinue;
        });

        return best;
    }
}
