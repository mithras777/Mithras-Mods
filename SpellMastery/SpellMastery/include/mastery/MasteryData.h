#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace MITHRAS::SPELL_MASTERY
{
	enum class SpellSchool : std::uint32_t
	{
		kAlteration = 0,
		kConjuration = 1,
		kDestruction = 2,
		kIllusion = 3,
		kRestoration = 4,
		kCount = 5
	};

	inline constexpr std::size_t kSchoolCount = static_cast<std::size_t>(SpellSchool::kCount);

	struct MasteryStats
	{
		std::uint32_t kills{ 0 };
		std::uint32_t level{ 0 };
	};

	struct MasteryBonuses
	{
		std::array<float, kSchoolCount> schoolSkillBonus{};
		float magickaRateMult{ 0.0f };
	};

	struct MasteryConfig
	{
		struct BonusTuning
		{
			float skillBonusPerLevel{ 2.0f };
			float skillBonusCap{ 40.0f };
			float magickaRatePerLevel{ 0.5f };
			float magickaRateCap{ 25.0f };
		};

		struct SchoolBonusConfig
		{
			bool enabled{ true };
			BonusTuning tuning{};
		};

		bool enabled{ true };
		float gainMultiplier{ 1.0f };
		std::vector<std::uint32_t> thresholds{ 10, 25, 50, 100, 200 };
		bool gainFromKills{ true };
		BonusTuning generalBonuses{};
		std::array<SchoolBonusConfig, kSchoolCount> schoolBonuses{};
	};

	[[nodiscard]] inline std::string_view SchoolName(SpellSchool a_school)
	{
		switch (a_school) {
			case SpellSchool::kAlteration:
				return "Alteration";
			case SpellSchool::kConjuration:
				return "Conjuration";
			case SpellSchool::kDestruction:
				return "Destruction";
			case SpellSchool::kIllusion:
				return "Illusion";
			case SpellSchool::kRestoration:
				return "Restoration";
			default:
				return "Unknown";
		}
	}

	[[nodiscard]] inline RE::ActorValue SchoolToActorValue(SpellSchool a_school)
	{
		switch (a_school) {
			case SpellSchool::kAlteration:
				return RE::ActorValue::kAlteration;
			case SpellSchool::kConjuration:
				return RE::ActorValue::kConjuration;
			case SpellSchool::kDestruction:
				return RE::ActorValue::kDestruction;
			case SpellSchool::kIllusion:
				return RE::ActorValue::kIllusion;
			case SpellSchool::kRestoration:
				return RE::ActorValue::kRestoration;
			default:
				return RE::ActorValue::kNone;
		}
	}

	[[nodiscard]] inline std::optional<SpellSchool> ActorValueToSchool(RE::ActorValue a_av)
	{
		switch (a_av) {
			case RE::ActorValue::kAlteration:
				return SpellSchool::kAlteration;
			case RE::ActorValue::kConjuration:
				return SpellSchool::kConjuration;
			case RE::ActorValue::kDestruction:
				return SpellSchool::kDestruction;
			case RE::ActorValue::kIllusion:
				return SpellSchool::kIllusion;
			case RE::ActorValue::kRestoration:
				return SpellSchool::kRestoration;
			default:
				return std::nullopt;
		}
	}

	[[nodiscard]] inline std::optional<SpellSchool> GetSpellSchoolFromForm(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return std::nullopt;
		}

		if (const auto* spell = a_form->As<RE::SpellItem>(); spell) {
			return ActorValueToSchool(spell->GetAssociatedSkill());
		}

		return std::nullopt;
	}
}
