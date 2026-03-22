#include "oar/OarConditions.h"

#include "combat/DirectionalController.h"

#include <format>

namespace MC::OAR
{
	namespace
	{
		bool IsPlayerContext(RE::TESObjectREFR* a_refr)
		{
			const auto* player = RE::PlayerCharacter::GetSingleton();
			return player && a_refr == player;
		}
	}

	bool IsDirectionalAttackCondition::EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator*, void*) const
	{
		if (!IsPlayerContext(a_refr)) {
			return false;
		}

		// Keep directional routing active while the gameplay context is valid.
		return MC::DIRECTIONAL::Controller::GetSingleton()->ShouldShowOverlay();
	}

	bool IsPowerAttackCondition::EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator*, void*) const
	{
		if (!IsPlayerContext(a_refr)) {
			return false;
		}

		const auto latched = MC::DIRECTIONAL::Controller::GetSingleton()->GetLatchedState();
		return latched.active && latched.power;
	}

	DirectionBucketCondition::DirectionBucketCondition()
	{
		comparisonComponent = static_cast<Conditions::IComparisonConditionComponent*>(
			AddBaseComponent(Conditions::ConditionComponentType::kComparison, "Comparison"));
		bucketComponent = static_cast<Conditions::INumericConditionComponent*>(
			AddBaseComponent(Conditions::ConditionComponentType::kNumeric, "Bucket"));
	}

	RE::BSString DirectionBucketCondition::GetArgument() const
	{
		return std::format("bucket {} {}", comparisonComponent->GetArgument().data(), bucketComponent->GetArgument().data()).data();
	}

	RE::BSString DirectionBucketCondition::GetCurrent(RE::TESObjectREFR*) const
	{
		const auto cursor = MC::DIRECTIONAL::Controller::GetSingleton()->GetCursorState();
		return std::to_string(static_cast<int>(cursor.bucket)).data();
	}

	bool DirectionBucketCondition::EvaluateImpl(RE::TESObjectREFR* a_refr, RE::hkbClipGenerator*, void*) const
	{
		if (!IsPlayerContext(a_refr)) {
			return false;
		}
		if (!MC::DIRECTIONAL::Controller::GetSingleton()->ShouldShowOverlay()) {
			return false;
		}

		const auto cursor = MC::DIRECTIONAL::Controller::GetSingleton()->GetCursorState();
		const float currentBucket = static_cast<float>(static_cast<int>(cursor.bucket));
		const float requiredBucket = bucketComponent->GetNumericValue(a_refr);
		return comparisonComponent->GetComparisonResult(currentBucket, requiredBucket);
	}
}
