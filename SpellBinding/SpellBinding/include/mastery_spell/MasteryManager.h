#pragma once

#include "mastery_spell/BonusApplier.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace SBO::MASTERY_SPELL
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();
		void OnRevert();
		void OnEquipEvent(const RE::TESEquipEvent& a_event);
		void OnSpellCastEvent(const RE::TESSpellCastEvent& a_event);
		void OnDirectSpellCast(const RE::SpellItem* a_spell);
		void OnHitEvent(const RE::TESHitEvent& a_event);
		void OnDeathEvent(const RE::TESDeathEvent& a_event);

		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);

		[[nodiscard]] MasteryConfig GetConfig() const;
		void SetConfig(const MasteryConfig& a_config, bool a_writeJson = true);
		[[nodiscard]] MasteryStats GetStats(const SpellKey& a_key) const;
		[[nodiscard]] std::uint32_t GetProgressCount(const SpellKey& a_key) const;
		[[nodiscard]] const std::unordered_map<SpellKey, MasteryStats, SpellKeyHash>& GetMasteryData() const { return m_mastery; }
		[[nodiscard]] std::vector<SpellKey> GetEquippedSpellKeys() const;

		[[nodiscard]] static MasteryConfig DefaultConfig();
		void ResetAllConfigToDefault(bool a_writeJson = true);
		void ClearDatabase();
		void SaveConfigToJson() const;

	private:
		using Clock = std::chrono::steady_clock;

		struct EquippedState
		{
			std::optional<SpellKey> key{};
			Clock::time_point equippedAt{};
		};

		struct LastCastState
		{
			std::optional<SpellKey> key{};
			Clock::time_point castAt{};
		};

		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;
		void RefreshEquippedFromPlayerLocked();
		void ReapplyBonusesLocked();
		void UpdateEquippedTimeLocked(EquippedState& a_slot);
		void SetEquippedKeyLocked(EquippedState& a_slot, std::optional<SpellKey> a_key);
		void RefreshLevelLocked(const SpellKey& a_key, MasteryStats& a_stats) const;
		[[nodiscard]] std::uint32_t ComputeProgressCountLocked(const SpellKey& a_key, const MasteryStats& a_stats) const;
		[[nodiscard]] MasteryBonuses ComputeBonusesLocked() const;
		[[nodiscard]] static std::optional<SpellKey> BuildSpellKey(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsTrackedSpellForm(const RE::TESForm* a_form);

		mutable std::mutex m_lock;
		MasteryConfig m_config{};
		std::unordered_map<SpellKey, MasteryStats, SpellKeyHash> m_mastery{};
		EquippedState m_leftSpell{};
		EquippedState m_rightSpell{};
		std::array<LastCastState, kSchoolCount> m_lastCast{};
		BonusApplier m_bonusApplier{};
	};
}
