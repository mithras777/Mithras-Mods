#pragma once

#include "FormResolver.h"
#include "ManagedRegistry.h"
#include "Placement.h"
#include "Settings.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace DYNAMIC_SPAWNS
{
	class SpawnManager final :
		public REX::Singleton<SpawnManager>,
		public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>,
		public RE::BSTEventSink<RE::TESOpenCloseEvent>
	{
	public:
		void Initialize();
		void Shutdown();
		void OnDataLoaded();
		void OnPostLoadGame();
		void OnNewGame();

		void ForceEvaluateNow();
		void ClearManagedNow();

	private:
		struct PendingSpawn
		{
			RE::FormID                                  cellID{ 0 };
			std::chrono::steady_clock::time_point       executeAt{};
			std::int32_t                                count{ 0 };
			SpawnRule                                   rule{};
		};

		RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* a_source) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESOpenCloseEvent* a_event, RE::BSTEventSource<RE::TESOpenCloseEvent>* a_source) override;

		void StartUpdateLoop();
		void Tick();
		void EvaluateCurrentCell(bool a_forced);
		void HandleCellChanged(RE::TESObjectCELL* a_newCell);
		void TryQueueSpawn(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, bool a_forced);
		void RunPendingSpawn();
		void ExecuteSpawnEvent(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SpawnRule& a_rule, std::int32_t a_requestedCount);

		[[nodiscard]] bool IsSpawnAllowedByGlobalState(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell) const;
		[[nodiscard]] bool PassesFilters(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SettingsData& a_settings) const;
		[[nodiscard]] bool RuleMatches(const SpawnRule& a_rule, RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, RE::BGSLocation* a_location) const;
		[[nodiscard]] std::optional<SpawnRule> FindMatchingRule(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell) const;
		[[nodiscard]] RE::Actor* SpawnOne(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SpawnRule& a_rule) const;
		[[nodiscard]] RE::TESBoundObject* ResolveSpawnBaseObject(const SpawnRule& a_rule) const;
		[[nodiscard]] bool HasNearbyHostile(RE::PlayerCharacter* a_player, float a_radius, std::int32_t a_maxRefs) const;
		void ApplyPostSpawnGenesisCompatibility(RE::Actor* a_actor, RE::PlayerCharacter* a_player, const SettingsData& a_settings) const;
		void TryInjectLoot(const RE::TESOpenCloseEvent* a_event);

		mutable std::mutex                            m_lock;
		std::optional<PendingSpawn>                  m_pendingSpawn;
		RE::FormID                                   m_lastCellID{ 0 };
		std::chrono::steady_clock::time_point        m_lastTick{};
		std::chrono::steady_clock::time_point        m_lastCleanup{};
		std::chrono::steady_clock::time_point        m_lastGlobalSpawn{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> m_lastCellSpawn{};
		std::unordered_set<RE::FormID>                m_lootProcessedContainers{};
		bool                                         m_initialized{ false };
		bool                                         m_running{ false };
	};
}
