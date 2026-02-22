#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

	struct SpellKey
	{
		std::uint32_t formID{ 0 };
		SpellSchool school{ SpellSchool::kDestruction };
		std::string name{};

		[[nodiscard]] bool operator==(const SpellKey& a_rhs) const noexcept
		{
			return formID == a_rhs.formID && school == a_rhs.school && name == a_rhs.name;
		}
	};

	struct SpellKeyHash
	{
		[[nodiscard]] std::size_t operator()(const SpellKey& a_key) const noexcept
		{
			std::size_t hash = static_cast<std::size_t>(a_key.formID);
			hash ^= static_cast<std::size_t>(a_key.school) << 1;
			hash ^= std::hash<std::string>{}(a_key.name) << 2;
			return hash;
		}
	};

	struct MasteryStats
	{
		std::uint32_t kills{ 0 };
		std::uint32_t uses{ 0 };
		std::uint32_t summons{ 0 };
		std::uint32_t hits{ 0 };
		float equippedSeconds{ 0.0f };
		std::uint32_t level{ 0 };
	};

	struct MasteryBonuses
	{
		std::array<float, kSchoolCount> schoolSkillBonus{};
		std::array<float, kSchoolCount> schoolSkillAdvanceBonus{};
		std::array<float, kSchoolCount> schoolPowerBonus{};
		std::array<float, kSchoolCount> schoolCostReduction{};
		float magickaRateMult{ 0.0f };
		float magickaFlat{ 0.0f };
	};

	struct MasteryConfig
	{
		struct ProgressionTuning
		{
			bool gainFromKills{ false };
			bool gainFromUses{ false };
			bool gainFromSummons{ false };
			bool gainFromHits{ false };
			bool gainFromEquipTime{ false };
			float equipSecondsPerPoint{ 5.0f };
		};

		struct BonusTuning
		{
			float skillBonusPerLevel{ 2.0f };
			float skillBonusCap{ 40.0f };
			float skillAdvancePerLevel{ 1.0f };
			float skillAdvanceCap{ 50.0f };
			float powerBonusPerLevel{ 1.0f };
			float powerBonusCap{ 30.0f };
			float costReductionPerLevel{ 1.5f };
			float costReductionCap{ 75.0f };
			float magickaRatePerLevel{ 0.5f };
			float magickaRateCap{ 25.0f };
			float magickaFlatPerLevel{ 2.0f };
			float magickaFlatCap{ 100.0f };
		};

		struct SchoolConfig
		{
			ProgressionTuning progression{};
			bool useBonusOverride{ true };
			BonusTuning bonus{};
		};

		bool enabled{ true };
		float gainMultiplier{ 1.0f };
		std::vector<std::uint32_t> thresholds{ 10, 25, 50, 100, 200 };
		BonusTuning generalBonuses{};
		std::array<SchoolConfig, kSchoolCount> schools{};
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

	[[nodiscard]] inline RE::ActorValue SchoolToSkillAV(SpellSchool a_school)
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

	[[nodiscard]] inline RE::ActorValue SchoolToPowerAV(SpellSchool a_school)
	{
		switch (a_school) {
			case SpellSchool::kAlteration:
				return RE::ActorValue::kAlterationPowerModifier;
			case SpellSchool::kConjuration:
				return RE::ActorValue::kConjurationPowerModifier;
			case SpellSchool::kDestruction:
				return RE::ActorValue::kDestructionPowerModifier;
			case SpellSchool::kIllusion:
				return RE::ActorValue::kIllusionPowerModifier;
			case SpellSchool::kRestoration:
				return RE::ActorValue::kRestorationPowerModifier;
			default:
				return RE::ActorValue::kNone;
		}
	}

	[[nodiscard]] inline RE::ActorValue SchoolToSkillAdvanceAV(SpellSchool a_school)
	{
		switch (a_school) {
			case SpellSchool::kAlteration:
				return RE::ActorValue::kAlterationSkillAdvance;
			case SpellSchool::kConjuration:
				return RE::ActorValue::kConjurationSkillAdvance;
			case SpellSchool::kDestruction:
				return RE::ActorValue::kDestructionSkillAdvance;
			case SpellSchool::kIllusion:
				return RE::ActorValue::kIllusionSkillAdvance;
			case SpellSchool::kRestoration:
				return RE::ActorValue::kRestorationSkillAdvance;
			default:
				return RE::ActorValue::kNone;
		}
	}

	[[nodiscard]] inline RE::ActorValue SchoolToCostAV(SpellSchool a_school)
	{
		switch (a_school) {
			case SpellSchool::kAlteration:
				return RE::ActorValue::kAlterationModifier;
			case SpellSchool::kConjuration:
				return RE::ActorValue::kConjurationModifier;
			case SpellSchool::kDestruction:
				return RE::ActorValue::kDestructionModifier;
			case SpellSchool::kIllusion:
				return RE::ActorValue::kIllusionModifier;
			case SpellSchool::kRestoration:
				return RE::ActorValue::kRestorationModifier;
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

	[[nodiscard]] inline const RE::SpellItem* GetSpellFromForm(const RE::TESForm* a_form)
	{
		return a_form ? a_form->As<RE::SpellItem>() : nullptr;
	}

	[[nodiscard]] inline std::optional<SpellSchool> GetSpellSchoolFromForm(const RE::TESForm* a_form)
	{
		const auto* spell = GetSpellFromForm(a_form);
		if (!spell) {
			return std::nullopt;
		}
		return ActorValueToSchool(spell->GetAssociatedSkill());
	}

	[[nodiscard]] inline bool IsSummonLikeSpell(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}

		auto* mutableSpell = const_cast<RE::SpellItem*>(a_spell);
		return mutableSpell->HasEffect(RE::EffectArchetype::kSummonCreature) ||
		       mutableSpell->HasEffect(RE::EffectArchetype::kReanimate) ||
		       mutableSpell->HasEffect(RE::EffectArchetype::kCommandSummoned);
	}
}
