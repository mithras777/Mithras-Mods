#pragma once

#include "mastery/BonusApplier.h"
#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace MITHRAS::MASTERY
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

		[[nodiscard]] std::optional<ItemKey> GetCurrentWeaponKey() const;
		[[nodiscard]] std::optional<ItemKey> GetCurrentShieldKey() const;
		[[nodiscard]] std::string GetCurrentWeaponTypeName() const;
		[[nodiscard]] MasteryStats GetStats(const ItemKey& a_key) const;
		[[nodiscard]] std::string GetItemName(const ItemKey& a_key) const;
		[[nodiscard]] std::size_t GetDatabaseSize() const;

		[[nodiscard]] static MasteryConfig DefaultConfig();
		void ResetAllConfigToDefault(bool a_writeJson = true);
		void ResetEquippedItemMastery();
		void ClearDatabase();
		void SaveConfigToJson() const;

	private:
		using Clock = std::chrono::steady_clock;

		struct EquippedState
		{
			std::optional<ItemKey> key{};
			Clock::time_point equippedAt{};
		};

		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;
		void RefreshEquippedFromPlayer();
		void ReapplyBonuses();
		void UpdateEquippedTimeLocked(EquippedState& a_slot);
		void SetEquippedKeyLocked(EquippedState& a_slot, std::optional<ItemKey> a_key);
		[[nodiscard]] std::optional<RE::WEAPON_TYPE> GetCurrentWeaponTypeLocked() const;
		void RefreshLevelLocked(MasteryStats& a_stats) const;
		[[nodiscard]] MasteryBonuses ComputeBonusesLocked(const MasteryStats& a_stats, bool a_isShield) const;

		[[nodiscard]] bool IsTrackedWeapon(const RE::TESForm* a_form) const;
		[[nodiscard]] bool IsShield(const RE::TESForm* a_form) const;

		mutable std::mutex m_lock;
		MasteryConfig m_config{};
		std::unordered_map<ItemKey, MasteryStats, ItemKeyHash> m_mastery{};
		EquippedState m_weapon{};
		EquippedState m_shield{};
		BonusApplier m_bonusApplier{};
	};
}
