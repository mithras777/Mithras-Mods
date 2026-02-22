#include "serialization/Serialization.h"

#include "mastery/MasteryManager.h"
#include "util/LogUtil.h"

namespace MITHRAS::SERIALIZATION
{
	namespace
	{
		constexpr std::uint32_t kSerializationID = 'SHOT';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			MITHRAS::SHOUT_MASTERY::Manager::GetSingleton()->Serialize(a_intfc);
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			MITHRAS::SHOUT_MASTERY::Manager::GetSingleton()->Deserialize(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			MITHRAS::SHOUT_MASTERY::Manager::GetSingleton()->OnRevert();
		}
	}

	void Register()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("Serialization interface unavailable");
			return;
		}

		serialization->SetUniqueID(kSerializationID);
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		LOG_INFO("Registered SKSE serialization callbacks");
	}
}
