#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace MOVEMENT
{
	struct GeneralPolicy
	{
		bool useMultipliers{ true };
		bool useOverrides{ false };
	};

	struct GeneralSettings
	{
		bool enabled{ true };
		bool restoreOnDisable{ true };
		bool dumpOnStartup{ false };
		GeneralPolicy policy{};
	};

	struct SpeedMultipliers
	{
		float walk{ 1.0f };
		float run{ 1.0f };
		float sprint{ 1.0f };
		float sneakWalk{ 1.0f };
		float sneakRun{ 1.0f };
	};

	struct SpeedOverrides
	{
		struct Entry
		{
			bool enabled{ false };
			float value{ 0.0f };
		};

		Entry walk{};
		Entry run{};
		Entry sprint{};
		Entry sneakWalk{};
		Entry sneakRun{};
	};

	struct TargetEntry
	{
		std::string name{};
		std::string form{};  // "Plugin|0xID"
		bool enabled{ true };
		bool originalsCaptured{ false };
		float originals[5][2]{};
	};

	struct SettingsData
	{
		int version{ 1 };
		GeneralSettings general{};
		SpeedMultipliers multipliers{};
		SpeedOverrides overrides{};
		std::vector<TargetEntry> targets{};
	};

	class Settings final : public REX::Singleton<Settings>
	{
	public:
		void Initialize();
		SettingsData Get() const;
		void Set(const SettingsData& a_settings, bool a_writeJson);
		void Save();
		void ResetToDefaults(bool a_writeJson);
		std::string GetConfigPathString() const;

	private:
		void LoadFromDisk();
		static SettingsData Defaults();
		static void Clamp(SettingsData& a_settings);

		mutable std::mutex m_lock;
		SettingsData m_settings{};
	};
}
