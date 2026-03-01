#include "serialization/Serialization.h"

#include "spellbinding/SpellBindingManager.h"
#include "util/LogUtil.h"

namespace SBIND::SERIALIZATION
{
	namespace
	{
		constexpr std::uint32_t kSerializationID = 'SBIN';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			SBIND::Manager::GetSingleton()->Serialize(a_intfc);
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			SBIND::Manager::GetSingleton()->Deserialize(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			SBIND::Manager::GetSingleton()->OnRevert();
		}
	}

	void Register()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			LOG_WARN("SpellBinding: serialization interface unavailable");
			return;
		}

		serialization->SetUniqueID(kSerializationID);
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		LOG_INFO("SpellBinding: serialization callbacks registered");
	}
}
