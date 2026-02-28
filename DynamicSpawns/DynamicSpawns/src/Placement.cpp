#include "Placement.h"

#include <algorithm>
#include <numbers>
#include <random>
#include <vector>

namespace DYNAMIC_SPAWNS
{
	std::optional<SpawnPlacement> PlacementService::FindPlacement(
		RE::PlayerCharacter*   a_player,
		RE::TESObjectCELL*     a_cell,
		const SettingsData&    a_settings,
		const ManagedRegistry& a_registry,
		bool                   a_allowInteriorRandom) const
	{
		if (!a_player || !a_cell) {
			return std::nullopt;
		}

		if (auto marker = FindMarkerPlacement(a_player, a_cell, a_settings, a_registry)) {
			return marker;
		}

		if (a_cell->IsInteriorCell() && !a_allowInteriorRandom) {
			return std::nullopt;
		}

		return FindRingPlacement(a_player, a_cell, a_settings, a_registry);
	}

	std::optional<SpawnPlacement> PlacementService::FindMarkerPlacement(
		RE::PlayerCharacter*   a_player,
		RE::TESObjectCELL*     a_cell,
		const SettingsData&    a_settings,
		const ManagedRegistry& a_registry) const
	{
		const auto origin = a_player->GetPosition();
		std::vector<RE::TESObjectREFR*> candidates{};
		candidates.reserve(64);
		std::int32_t scanned = 0;

		a_cell->ForEachReferenceInRange(origin, 4500.0F, [&](RE::TESObjectREFR* a_ref) {
			++scanned;
			if (scanned > 200 || !a_ref) {
				return RE::BSContainer::ForEachResult::kStop;
			}

			auto* base = a_ref->GetBaseObject();
			if (!base) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			const auto* editorID = base->GetFormEditorID();
			const std::string markerEditor = editorID ? editorID : "";
			const bool isHeadingMarker = base->IsHeadingMarker() ||
			                             markerEditor.contains("XMarker") ||
			                             markerEditor.contains("Patrol");
			if (isHeadingMarker) {
				candidates.push_back(a_ref);
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		if (candidates.empty()) {
			return std::nullopt;
		}

		std::shuffle(candidates.begin(), candidates.end(), std::mt19937{ std::random_device{}() });
		for (auto* candidate : candidates) {
			const auto pos = candidate->GetPosition();
			if (!IsValidCandidate(a_player, pos, a_settings, a_registry)) {
				continue;
			}

			SpawnPlacement out{};
			out.position = pos;
			out.rotation = candidate->GetAngle();
			out.cell = a_cell;
			out.worldspace = a_player->GetWorldspace();
			return out;
		}

		return std::nullopt;
	}

	std::optional<SpawnPlacement> PlacementService::FindRingPlacement(
		RE::PlayerCharacter*   a_player,
		RE::TESObjectCELL*     a_cell,
		const SettingsData&    a_settings,
		const ManagedRegistry& a_registry) const
	{
		auto* tes = RE::TES::GetSingleton();
		if (!tes) {
			return std::nullopt;
		}

		const auto origin = a_player->GetPosition();
		const float minRadius = a_settings.limits.minDistanceFromPlayer;
		const float maxRadius = a_settings.limits.minDistanceFromPlayer + 2000.0F;

		std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<float> angleDist(0.0F, std::numbers::pi_v<float> * 2.0F);
		std::uniform_real_distribution<float> radiusDist(minRadius, maxRadius);

		for (std::int32_t i = 0; i < 24; ++i) {
			const float angle = angleDist(rng);
			const float radius = radiusDist(rng);

			RE::NiPoint3 point{};
			point.x = origin.x + std::cos(angle) * radius;
			point.y = origin.y + std::sin(angle) * radius;
			point.z = origin.z + 1200.0F;

			float groundZ = 0.0F;
			if (!tes->GetLandHeight(point, groundZ)) {
				continue;
			}
			point.z = groundZ;

			if (!IsValidCandidate(a_player, point, a_settings, a_registry)) {
				continue;
			}

			SpawnPlacement out{};
			out.position = point;
			out.rotation = RE::NiPoint3{ 0.0F, 0.0F, angle };
			out.cell = a_cell;
			out.worldspace = a_player->GetWorldspace();
			return out;
		}

		return std::nullopt;
	}

	bool PlacementService::IsValidCandidate(
		RE::PlayerCharacter*   a_player,
		const RE::NiPoint3&    a_point,
		const SettingsData&    a_settings,
		const ManagedRegistry& a_registry) const
	{
		if (!a_player) {
			return false;
		}

		if (a_player->GetPosition().GetDistance(a_point) < a_settings.limits.minDistanceFromPlayer) {
			return false;
		}
		if (a_registry.HasSpawnNear(a_point, a_settings.limits.minDistanceFromOtherSpawn)) {
			return false;
		}

		auto* cell = a_player->GetParentCell();
		if (cell) {
			float waterHeight = 0.0F;
			if (cell->GetWaterHeight(a_point, waterHeight) && waterHeight > a_point.z + 32.0F) {
				return false;
			}
		}

		return true;
	}
}
