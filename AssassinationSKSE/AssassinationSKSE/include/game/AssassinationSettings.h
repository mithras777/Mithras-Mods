#pragma once

#include <mutex>
#include <string>

namespace GAME::ASSASSINATION {
	struct Config
	{
		int version{ 1 };
		bool enabled{ true };
		int maxLevelDifference{ 100 };
		int assassinationChance{ 100 };
		bool melee{ true };
		bool unarmed{ true };
		bool bowsCrossbows{ true };
		bool excludeBosses{ false };
		bool excludeDragons{ false };
		bool excludeGiants{ false };
	};

	class Settings final : public REX::Singleton<Settings>
	{
	public:
		void Initialize();
		Config Get() const;
		Config GetDefaults() const;
		void Set(const Config& a_settings, bool a_writeJson);
		void Save();
		void ResetToDefaults(bool a_writeJson);
		std::string GetConfigPathString() const;

	private:
		void LoadFromDisk();
		static Config Defaults();
		static void Clamp(Config& a_settings);

		mutable std::mutex m_lock;
		Config m_settings{};
	};

	[[nodiscard]] Config GetConfig();
	void                 SetConfig(const Config& a_config);
	void                 Register();
}
