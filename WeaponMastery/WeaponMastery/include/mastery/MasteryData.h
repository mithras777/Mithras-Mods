#pragma once
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <array>
#include <vector>

namespace MITHRAS::MASTERY
{
	inline constexpr std::size_t kWeaponTypeCount = static_cast<std::size_t>(RE::WEAPON_TYPE::kTotal);

	struct ItemKey
	{
		RE::FormID baseFormID{ 0 };
		std::uint16_t uniqueID{ 0 };
		bool leftHand{ false };
		bool shield{ false };

		[[nodiscard]] bool operator==(const ItemKey& a_rhs) const noexcept
		{
			return baseFormID == a_rhs.baseFormID && uniqueID == a_rhs.uniqueID && leftHand == a_rhs.leftHand && shield == a_rhs.shield;
		}
	};

	struct ItemKeyHash
	{
		[[nodiscard]] std::size_t operator()(const ItemKey& a_key) const noexcept
		{
			std::size_t hash = static_cast<std::size_t>(a_key.baseFormID);
			hash ^= static_cast<std::size_t>(a_key.uniqueID) << 1;
			hash ^= static_cast<std::size_t>(a_key.leftHand) << 20;
			hash ^= static_cast<std::size_t>(a_key.shield) << 21;
			return hash;
		}
	};

	struct MasteryStats
	{
		std::uint32_t kills{ 0 };
		std::uint32_t hits{ 0 };
		std::uint32_t powerHits{ 0 };
		std::uint32_t blocks{ 0 };
		float secondsEquipped{ 0.0f };
		std::uint32_t level{ 0 };
	};

	struct MasteryBonuses
	{
		float dmgMult{ 0.0f };
		float atkSpeed{ 0.0f };
		float critChance{ 0.0f };
		float staminaPowerCostMult{ 1.0f };
		float blockStaminaMult{ 1.0f };
	};

	struct MasteryConfig
	{
		struct BonusTuning
		{
			float damagePerLevel{ 0.02f };
			float attackSpeedPerLevel{ 0.01f };
			float critChancePerLevel{ 0.005f };
			float powerAttackStaminaReductionPerLevel{ 0.02f };
			float blockStaminaReductionPerLevel{ 0.02f };

			float damageCap{ 0.35f };
			float attackSpeedCap{ 0.25f };
			float critChanceCap{ 0.20f };
			float powerAttackStaminaFloor{ 0.60f };
			float blockStaminaFloor{ 0.60f };
		};

		struct WeaponTypeBonusConfig
		{
			bool enabled{ false };
			BonusTuning tuning{};
		};

		bool enabled{ true };
		float gainMultiplier{ 1.0f };
		std::vector<std::uint32_t> thresholds{ 10, 25, 50, 100, 200 };

		bool gainFromKills{ true };
		bool gainFromHits{ false };
		bool gainFromPowerHits{ false };
		bool gainFromBlocks{ false };
		bool timeBasedLeveling{ false };

		BonusTuning generalBonuses{};
		std::array<WeaponTypeBonusConfig, kWeaponTypeCount> weaponTypeBonuses{};
	};

	[[nodiscard]] inline std::string ToString(const ItemKey& a_key)
	{
		return std::format("{:08X}:{:04X}:{}:{}", a_key.baseFormID, a_key.uniqueID, a_key.leftHand ? "L" : "R", a_key.shield ? "S" : "W");
	}

	[[nodiscard]] inline std::string_view WeaponTypeName(RE::WEAPON_TYPE a_type)
	{
		switch (a_type) {
			case RE::WEAPON_TYPE::kHandToHandMelee:
				return "Unarmed";
			case RE::WEAPON_TYPE::kOneHandSword:
				return "One-Handed Sword";
			case RE::WEAPON_TYPE::kOneHandDagger:
				return "One-Handed Dagger";
			case RE::WEAPON_TYPE::kOneHandAxe:
				return "One-Handed Axe";
			case RE::WEAPON_TYPE::kOneHandMace:
				return "One-Handed Mace";
			case RE::WEAPON_TYPE::kTwoHandSword:
				return "Two-Handed Sword";
			case RE::WEAPON_TYPE::kTwoHandAxe:
				return "Two-Handed Axe";
			case RE::WEAPON_TYPE::kBow:
				return "Bow";
			case RE::WEAPON_TYPE::kStaff:
				return "Staff";
			case RE::WEAPON_TYPE::kCrossbow:
				return "Crossbow";
			default:
				return "Unknown";
		}
	}

	[[nodiscard]] inline std::optional<RE::WEAPON_TYPE> GetWeaponTypeFromForm(const RE::TESForm* a_form)
	{
		const auto* weapon = a_form ? a_form->As<RE::TESObjectWEAP>() : nullptr;
		if (!weapon) {
			return std::nullopt;
		}
		return weapon->GetWeaponType();
	}
}
