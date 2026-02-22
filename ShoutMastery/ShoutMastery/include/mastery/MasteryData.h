#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace MITHRAS::SHOUT_MASTERY
{
	struct ShoutKey
	{
		std::uint32_t formID{ 0 };
		std::string name{};

		[[nodiscard]] bool operator==(const ShoutKey& a_rhs) const noexcept
		{
			return formID == a_rhs.formID && name == a_rhs.name;
		}
	};

	struct ShoutKeyHash
	{
		[[nodiscard]] std::size_t operator()(const ShoutKey& a_key) const noexcept
		{
			return static_cast<std::size_t>(a_key.formID) ^ (std::hash<std::string>{}(a_key.name) << 1);
		}
	};

	struct MasteryStats
	{
		std::uint32_t uses{ 0 };
		std::uint32_t level{ 0 };
	};

	struct MasteryBonuses
	{
		float shoutRecoveryMult{ 0.0f };
		float shoutPower{ 0.0f };
		float voiceRate{ 0.0f };
		float voicePoints{ 0.0f };
		float staminaRateMult{ 0.0f };
	};

	struct MasteryConfig
	{
		struct BonusTuning
		{
			float shoutRecoveryPerLevel{ -1.5f };
			float shoutRecoveryCap{ -60.0f };
			float shoutPowerPerLevel{ 1.0f };
			float shoutPowerCap{ 60.0f };
			float voiceRatePerLevel{ 2.0f };
			float voiceRateCap{ 100.0f };
			float voicePointsPerLevel{ 1.0f };
			float voicePointsCap{ 60.0f };
			float staminaRatePerLevel{ 0.5f };
			float staminaRateCap{ 40.0f };
		};

		bool enabled{ true };
		float gainMultiplier{ 1.0f };
		bool gainFromUses{ true };
		std::vector<std::uint32_t> thresholds{ 10, 25, 50, 100, 200 };
		BonusTuning bonuses{};
	};

	[[nodiscard]] inline std::optional<ShoutKey> BuildKey(const RE::TESShout* a_shout)
	{
		if (!a_shout) {
			return std::nullopt;
		}

		ShoutKey key{};
		key.formID = a_shout->GetFormID();
		if (const char* name = a_shout->GetName(); name && name[0] != '\0') {
			key.name = name;
		} else {
			key.name = std::format("Shout {:08X}", key.formID);
		}
		return key;
	}
}
