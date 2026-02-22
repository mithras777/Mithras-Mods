#pragma once

#include "mastery/BonusApplier.h"
#include <array>
#include <filesystem>
#include <mutex>

namespace MITHRAS::SPELL_MASTERY
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();
		void OnRevert();
		void OnEquipEvent(const RE::TESEquipEvent& a_event);
		void OnDeathEvent(const RE::TESDeathEvent& a_event);

		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);

		[[nodiscard]] MasteryConfig GetConfig() const;
		void SetConfig(const MasteryConfig& a_config, bool a_writeJson = true);
		[[nodiscard]] MasteryStats GetStats(SpellSchool a_school) const;
		[[nodiscard]] bool IsSchoolActive(SpellSchool a_school) const;
		[[nodiscard]] const std::array<MasteryStats, kSchoolCount>& GetMasteryData() const { return m_mastery; }

		[[nodiscard]] static MasteryConfig DefaultConfig();
		void ResetAllConfigToDefault(bool a_writeJson = true);
		void ClearDatabase();
		void SaveConfigToJson() const;

	private:
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;
		void RefreshActiveSchoolsLocked();
		void ReapplyBonusesLocked();
		void RefreshLevelLocked(MasteryStats& a_stats) const;
		[[nodiscard]] MasteryBonuses ComputeBonusesLocked() const;
		[[nodiscard]] static bool IsTrackedSpellForm(const RE::TESForm* a_form);

		mutable std::mutex m_lock;
		MasteryConfig m_config{};
		std::array<MasteryStats, kSchoolCount> m_mastery{};
		std::array<bool, kSchoolCount> m_activeSchools{};
		BonusApplier m_bonusApplier{};
	};
}
