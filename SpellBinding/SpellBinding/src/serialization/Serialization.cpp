#include "serialization/Serialization.h"

#include "overhaul/SpellbladeOverhaulManager.h"
#include "util/LogUtil.h"

namespace SBIND::SERIALIZATION
{
	namespace
	{
		constexpr std::uint32_t kSerializationID = 'SBOV';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			SB_OVERHAUL::Manager::GetSingleton()->Serialize(a_intfc);
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			SB_OVERHAUL::Manager::GetSingleton()->Deserialize(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			SB_OVERHAUL::Manager::GetSingleton()->OnRevert();
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
