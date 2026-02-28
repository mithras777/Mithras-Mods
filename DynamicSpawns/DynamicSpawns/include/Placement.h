#pragma once

#include "ManagedRegistry.h"
#include "Settings.h"

#include <optional>

namespace DYNAMIC_SPAWNS
{
	struct SpawnPlacement
	{
		RE::NiPoint3      position{};
		RE::NiPoint3      rotation{};
		RE::TESObjectCELL* cell{ nullptr };
		RE::TESWorldSpace* worldspace{ nullptr };
	};

	class PlacementService final : public REX::Singleton<PlacementService>
	{
	public:
		[[nodiscard]] std::optional<SpawnPlacement> FindPlacement(
			RE::PlayerCharacter*      a_player,
			RE::TESObjectCELL*        a_cell,
			const SettingsData&       a_settings,
			const ManagedRegistry&    a_registry,
			bool                      a_allowInteriorRandom) const;

	private:
		[[nodiscard]] std::optional<SpawnPlacement> FindMarkerPlacement(
			RE::PlayerCharacter*   a_player,
			RE::TESObjectCELL*     a_cell,
			const SettingsData&    a_settings,
			const ManagedRegistry& a_registry) const;

		[[nodiscard]] std::optional<SpawnPlacement> FindRingPlacement(
			RE::PlayerCharacter*   a_player,
			RE::TESObjectCELL*     a_cell,
			const SettingsData&    a_settings,
			const ManagedRegistry& a_registry) const;

		[[nodiscard]] bool IsValidCandidate(
			RE::PlayerCharacter*   a_player,
			const RE::NiPoint3&    a_point,
			const SettingsData&    a_settings,
			const ManagedRegistry& a_registry) const;
	};
}
