#include "serialization/Serialization.h"

#include "smartcast/SmartCastController.h"
#include "util/LogUtil.h"

namespace SMART_CAST::SERIALIZATION
{
	namespace
	{
		constexpr std::uint32_t kSerializationID = 'SMCT';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			SMART_CAST::Controller::GetSingleton()->Serialize(a_intfc);
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			SMART_CAST::Controller::GetSingleton()->Deserialize(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			SMART_CAST::Controller::GetSingleton()->OnRevert();
		}
	}

	void Register()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("SmartCast: serialization interface unavailable");
			return;
		}

		serialization->SetUniqueID(kSerializationID);
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		LOG_INFO("SmartCast: registered SKSE serialization callbacks");
	}
}
