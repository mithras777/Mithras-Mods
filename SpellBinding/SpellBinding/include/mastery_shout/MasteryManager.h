#pragma once

#include "mastery_shout/BonusApplier.h"
#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace SBO::MASTERY_SHOUT
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();
		void OnRevert();
		void OnSpellCastEvent(const RE::TESSpellCastEvent& a_event);
		void OnHitEvent(const RE::TESHitEvent& a_event);
		void OnShoutAttack(const RE::ShoutAttack::Event& a_event);

		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);

		[[nodiscard]] MasteryConfig GetConfig() const;
		void SetConfig(const MasteryConfig& a_config, bool a_writeJson = true);
		[[nodiscard]] std::optional<ShoutKey> GetCurrentShoutKey() const;
		[[nodiscard]] MasteryStats GetStats(const ShoutKey& a_key) const;
		[[nodiscard]] const std::unordered_map<ShoutKey, MasteryStats, ShoutKeyHash>& GetMasteryData() const { return m_mastery; }

		[[nodiscard]] static MasteryConfig DefaultConfig();
		void ResetAllConfigToDefault(bool a_writeJson = true);
		void ClearDatabase();
		void SaveConfigToJson() const;

	private:
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;
		void RefreshCurrentShoutLocked();
		void ReapplyBonusesLocked();
		void RefreshLevelLocked(MasteryStats& a_stats) const;
		[[nodiscard]] MasteryBonuses ComputeBonusesLocked() const;
		void GainUseLocked(const ShoutKey& a_key);
		[[nodiscard]] std::optional<ShoutKey> TryMapSpellToCurrentShoutLocked(RE::FormID a_spellFormID) const;

		mutable std::mutex m_lock;
		MasteryConfig m_config{};
		std::unordered_map<ShoutKey, MasteryStats, ShoutKeyHash> m_mastery{};
		std::optional<ShoutKey> m_currentShout{};
		BonusApplier m_bonusApplier{};
	};
}
