#pragma once

namespace GAME::ASSASSINATION {
	struct Config
	{
		bool enabled{ true };
		int maxLevelDifference{ 100 };
		bool melee{ true };
		bool unarmed{ true };
		bool bowsCrossbows{ true };
	};

	[[nodiscard]] Config GetConfig();
	void                 SetConfig(const Config& a_config);
	void Register();
}
