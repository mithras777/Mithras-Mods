#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace MOVEMENT
{
	struct GeneralSettings
	{
		bool enabled{ true };
		bool restoreOnDisable{ true };
	};

	struct MovementEntry
	{
		std::string name{};
		std::string form{};  // "Plugin|0xID"
		std::string group{}; // Default / Combat / Horses
		bool enabled{ true };
		float speeds[5][2]{};
	};

	struct SettingsData
	{
		int version{ 7 };
		GeneralSettings general{};
		std::vector<MovementEntry> entries{};
	};

	class Settings final : public REX::Singleton<Settings>
	{
	public:
		void Initialize();
		SettingsData Get() const;
		SettingsData GetDefaults() const;
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
