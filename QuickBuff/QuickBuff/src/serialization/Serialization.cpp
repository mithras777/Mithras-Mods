#include "serialization/Serialization.h"

#include "buff/QuickBuffManager.h"
#include "util/LogUtil.h"

namespace QUICK_BUFF::SERIALIZATION
{
	namespace
	{
		constexpr std::uint32_t kSerializationID = 'QBUF';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			QUICK_BUFF::Manager::GetSingleton()->Serialize(a_intfc);
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			QUICK_BUFF::Manager::GetSingleton()->Deserialize(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			QUICK_BUFF::Manager::GetSingleton()->OnRevert();
		}
	}

	void Register()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("QuickBuff: serialization interface unavailable");
			return;
		}

		serialization->SetUniqueID(kSerializationID);
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		LOG_INFO("QuickBuff: registered SKSE serialization callbacks");
	}
}
