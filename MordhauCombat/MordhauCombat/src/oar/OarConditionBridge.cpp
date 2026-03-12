#include "oar/OarConditionBridge.h"

#include "oar/OarConditions.h"
#include "util/LogUtil.h"

namespace MC::OAR
{
	namespace
	{
		template <typename T>
		void RegisterCondition()
		{
			switch (OAR_API::Conditions::AddCustomCondition<T>()) {
				case OAR_API::Conditions::APIResult::OK:
					LOG_INFO("MordhauCombat: registered OAR condition '{}'", T::CONDITION_NAME);
					break;
				case OAR_API::Conditions::APIResult::AlreadyRegistered:
					LOG_INFO("MordhauCombat: OAR condition '{}' already registered", T::CONDITION_NAME);
					break;
				case OAR_API::Conditions::APIResult::Invalid:
					LOG_WARN("MordhauCombat: OAR condition '{}' invalid", T::CONDITION_NAME);
					break;
				case OAR_API::Conditions::APIResult::Failed:
				default:
					LOG_WARN("MordhauCombat: failed to register OAR condition '{}'", T::CONDITION_NAME);
					break;
			}
		}
	}

	void RegisterConditions()
	{
		auto* api = OAR_API::Conditions::GetAPI(OAR_API::Conditions::InterfaceVersion::Latest);
		if (!api) {
			LOG_WARN("MordhauCombat: OpenAnimationReplacer API not available");
			return;
		}

		RegisterCondition<IsDirectionalAttackCondition>();
		RegisterCondition<IsPowerAttackCondition>();
		RegisterCondition<DirectionBucketCondition>();
	}
}
