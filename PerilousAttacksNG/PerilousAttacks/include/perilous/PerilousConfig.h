#pragma once

#include "PCH.h"
#include <shared_mutex>

namespace PERILOUS
{
	struct Config
	{
		bool bPerilousAttack_Enable{ true };
		bool bPerilousAttack_ChargeTime_Enable{ true };
		float fPerilousAttack_ChargeTime_Mult{ 0.1F };
		float fPerilousAttack_ChargeTime_Duration{ 0.5F };
		float fPerilousAttack_Chance_Mult{ 1.0F };
		std::uint32_t iPerilous_MaxAttackersPerTarget{ 2 };
		bool bPerilous_VFX_Enable{ true };
		bool bPerilous_SFX_Enable{ true };
		std::string sPerilous_FormsPlugin{ "PerilousAttacks.esp" };
		RE::FormID iPerilous_RedVfxFormID{ 0 };
		RE::FormID iPerilous_SfxFormID{ 0 };
	};

	class ConfigStore final : public REX::Singleton<ConfigStore>
	{
	public:
		static constexpr auto SETTINGS_PATH = "Data\\SKSE\\Plugins\\PerilousAttacks\\Settings.ini";

		void Load();
		void Save() const;

		Config Get() const;
		void Set(const Config& a_config);

	private:
		void Sanitize(Config& a_config) const;

		mutable std::shared_mutex _lock;
		Config _config{};
	};
}
