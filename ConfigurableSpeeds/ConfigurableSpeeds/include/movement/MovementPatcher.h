#pragma once

#include "movement/Settings.h"

#include <string>
#include <unordered_map>

namespace RE
{
	class PlayerCharacter;
}

namespace MOVEMENT
{
	class MovementPatcher final : public REX::Singleton<MovementPatcher>
	{
	public:
		void StartupApply(const SettingsData& a_settings);
		void OnSettingsChanged(const SettingsData& a_oldSettings, const SettingsData& a_newSettings);
		void ApplyNow(const SettingsData& a_settings);
		void Restore();
		void ShutdownRestoreIfNeeded(const SettingsData& a_settings);
		float GetPlayerSpeedMultiplier(const SettingsData& a_settings, RE::PlayerCharacter& a_player) const;

	private:
		struct ResolvedTarget
		{
			bool valid{ false };
			std::string formKey{};
			RE::BGSMovementType* form{ nullptr };
			bool originalsCaptured{ false };
			float originals[5][2]{};
		};

		void RestoreCurrentCache();
		void RebuildResolvedTargets(const SettingsData& a_settings);
		void CaptureOriginalsOnce();
		void ApplyResolved(const SettingsData& a_settings);

		static std::string CanonicalTargetKey(const std::string& a_formString);
		static std::string Trim(std::string a_text);
		static ResolvedTarget ResolveTarget(const MovementEntry& a_entry);

		std::unordered_map<std::string, ResolvedTarget> m_resolved{};
	};
}
