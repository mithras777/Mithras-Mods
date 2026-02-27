#pragma once

#include "movement/Settings.h"

#include <array>
#include <string>
#include <unordered_map>

namespace MOVEMENT
{
	struct TargetPreview
	{
		bool resolved{ false };
		std::string displayName{};
		std::string pluginAndForm{};
	};

	class MovementPatcher final : public REX::Singleton<MovementPatcher>
	{
	public:
		void StartupApply(const SettingsData& a_settings);
		void OnSettingsChanged(const SettingsData& a_oldSettings, const SettingsData& a_newSettings);
		void ApplyNow(const SettingsData& a_settings);
		void Restore();
		void ShutdownRestoreIfNeeded(const SettingsData& a_settings);

		std::size_t DumpMovementTypesNow() const;
		TargetPreview PreviewTarget(const TargetEntry& a_target) const;
		std::string GetLogPathHint() const;

	private:
		struct ResolvedTarget
		{
			bool valid{ false };
			std::string formKey{};
			std::string name{};
			std::string pluginAndForm{};
			RE::BGSMovementType* form{ nullptr };
			bool originalsCaptured{ false };
			float originals[5][2]{};
		};

		enum class SpeedCategory : std::uint32_t
		{
			// v1 mapping assumption:
			//   indexOne = movement category (0..4)
			//   indexTwo = variant (0..1)
			// Update this enum + IndexFor() if dump analysis suggests a different map.
			kWalk = 0,
			kRun = 1,
			kSprint = 2,
			kSneakWalk = 3,
			kSneakRun = 4,
			kTotal = 5
		};

		static constexpr std::array<SpeedCategory, 5> kAllCategories{
			SpeedCategory::kWalk,
			SpeedCategory::kRun,
			SpeedCategory::kSprint,
			SpeedCategory::kSneakWalk,
			SpeedCategory::kSneakRun
		};

		void RestoreCurrentCache();
		void RebuildResolvedTargets(const SettingsData& a_settings);
		void CaptureOriginalsOnce();
		void ApplyResolved(const SettingsData& a_settings);

		static float MultiplierFor(const SettingsData& a_settings, SpeedCategory a_category);
		static const SpeedOverrides::Entry& OverrideFor(const SettingsData& a_settings, SpeedCategory a_category);
		static std::size_t IndexFor(SpeedCategory a_category);

		static std::string Trim(std::string a_text);
		static std::string CanonicalTargetKey(const std::string& a_formString);
		static std::string FormatPluginAndForm(const RE::TESForm* a_form);
		static TargetPreview BuildPreviewInternal(const TargetEntry& a_target);
		static ResolvedTarget ResolveTarget(const TargetEntry& a_target);

		std::unordered_map<std::string, ResolvedTarget> m_resolved{};
	};
}
