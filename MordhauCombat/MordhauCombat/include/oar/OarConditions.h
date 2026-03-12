#pragma once

#include "oar/OpenAnimationReplacerAPI-Conditions.h"

namespace MC::OAR
{
	class IsDirectionalAttackCondition : public Conditions::CustomCondition
	{
	public:
		constexpr static inline std::string_view CONDITION_NAME = "MordhauIsDirectionalAttack"sv;

		RE::BSString GetName() const override { return CONDITION_NAME.data(); }
		RE::BSString GetDescription() const override { return "True when Mordhau directional attack window is active."sv.data(); }
		constexpr REL::Version GetRequiredVersion() const override { return { 1, 0, 0 }; }

	protected:
		bool EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator* a_clipGenerator, void* a_subMod) const override;
	};

	class IsPowerAttackCondition : public Conditions::CustomCondition
	{
	public:
		constexpr static inline std::string_view CONDITION_NAME = "MordhauIsPowerAttack"sv;

		RE::BSString GetName() const override { return CONDITION_NAME.data(); }
		RE::BSString GetDescription() const override { return "True when the active Mordhau directional attack is power attack."sv.data(); }
		constexpr REL::Version GetRequiredVersion() const override { return { 1, 0, 0 }; }

	protected:
		bool EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator* a_clipGenerator, void* a_subMod) const override;
	};

	class DirectionBucketCondition : public Conditions::CustomCondition
	{
	public:
		constexpr static inline std::string_view CONDITION_NAME = "MordhauDirectionBucket"sv;

		DirectionBucketCondition();

		RE::BSString GetName() const override { return CONDITION_NAME.data(); }
		RE::BSString GetDescription() const override { return "Compares active Mordhau direction bucket against numeric value (0-8)."sv.data(); }
		constexpr REL::Version GetRequiredVersion() const override { return { 1, 0, 0 }; }
		RE::BSString GetArgument() const override;
		RE::BSString GetCurrent(RE::TESObjectREFR* a_refr) const override;

	protected:
		bool EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator* a_clipGenerator, void* a_subMod) const override;

	private:
		Conditions::IComparisonConditionComponent* comparisonComponent{ nullptr };
		Conditions::INumericConditionComponent* bucketComponent{ nullptr };
	};
}
