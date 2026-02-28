#include "SpawnManager.h"

#include "Utils.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <random>

namespace DYNAMIC_SPAWNS
{
	namespace
	{
		bool HasAnyLocationKeyword(RE::BGSLocation* a_location, const std::vector<std::string>& a_keywordEditorIDs)
		{
			if (!a_location) {
				return false;
			}

			auto* resolver = FormResolver::GetSingleton();
			for (const auto& id : a_keywordEditorIDs) {
				if (auto* keyword = resolver->ResolveKeywordByEditorID(id); keyword && a_location->HasKeyword(keyword)) {
					return true;
				}
			}
			return false;
		}
	}

	void SpawnManager::Initialize()
	{
		if (m_initialized) {
			return;
		}

		if (auto* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton()) {
			sourceHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
			sourceHolder->AddEventSink<RE::TESOpenCloseEvent>(this);
		}

		m_initialized = true;
		m_running = true;
		StartUpdateLoop();
		LOG_INFO("SpawnManager initialized");
	}

	void SpawnManager::Shutdown()
	{
		m_running = false;
		m_initialized = false;
		ClearManagedNow();
	}

	void SpawnManager::OnDataLoaded()
	{
		Settings::GetSingleton()->LoadOrCreate();
		Initialize();
	}

	void SpawnManager::OnPostLoadGame()
	{
		ClearManagedNow();
		m_lastCellSpawn.clear();
		m_lootProcessedContainers.clear();
		m_lastGlobalSpawn = {};
		m_pendingSpawn.reset();
	}

	void SpawnManager::OnNewGame()
	{
		OnPostLoadGame();
	}

	void SpawnManager::ForceEvaluateNow()
	{
		EvaluateCurrentCell(true);
	}

	void SpawnManager::ClearManagedNow()
	{
		ManagedRegistry::GetSingleton()->ClearAll();
	}

	RE::BSEventNotifyControl SpawnManager::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
	{
		if (!a_event || !a_event->cell || !m_running) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || player->GetParentCell() != a_event->cell) {
			return RE::BSEventNotifyControl::kContinue;
		}

		HandleCellChanged(a_event->cell);
		TryQueueSpawn(player, a_event->cell, false);
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl SpawnManager::ProcessEvent(const RE::TESOpenCloseEvent* a_event, RE::BSTEventSource<RE::TESOpenCloseEvent>*)
	{
		if (m_running) {
			TryInjectLoot(a_event);
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void SpawnManager::StartUpdateLoop()
	{
		SKSE::GetTaskInterface()->AddTask([this]() {
			if (!m_running) {
				return;
			}

			Tick();
			StartUpdateLoop();
		});
	}

	void SpawnManager::Tick()
	{
		const auto now = std::chrono::steady_clock::now();
		if (m_lastTick.time_since_epoch().count() != 0) {
			const auto delta = now - m_lastTick;
			if (delta < std::chrono::milliseconds(200)) {
				RunPendingSpawn();
				return;
			}
		}
		m_lastTick = now;

		EvaluateCurrentCell(false);
		RunPendingSpawn();

		if (m_lastCleanup.time_since_epoch().count() == 0 || (now - m_lastCleanup) >= std::chrono::seconds(1)) {
			m_lastCleanup = now;
			auto* player = RE::PlayerCharacter::GetSingleton();
			const auto& limits = Settings::GetSingleton()->Get().limits;
			ManagedRegistry::GetSingleton()->Cleanup(limits, player, true);
			ManagedRegistry::GetSingleton()->PruneToLimit(limits.maxManagedRefs);
		}
	}

	void SpawnManager::EvaluateCurrentCell(bool a_forced)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* cell = player->GetParentCell();
		if (!cell) {
			return;
		}

		if (cell->GetFormID() != m_lastCellID) {
			HandleCellChanged(cell);
		}

		TryQueueSpawn(player, cell, a_forced);
	}

	void SpawnManager::HandleCellChanged(RE::TESObjectCELL* a_newCell)
	{
		if (!a_newCell) {
			return;
		}

		const auto oldCellID = m_lastCellID;
		m_lastCellID = a_newCell->GetFormID();

		const auto& settings = Settings::GetSingleton()->Get();
		if (settings.limits.despawnOnCellDetach && oldCellID != 0 && oldCellID != m_lastCellID) {
			ManagedRegistry::GetSingleton()->ClearCell(oldCellID);
		}
	}

	void SpawnManager::TryQueueSpawn(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, bool a_forced)
	{
		const auto& settings = Settings::GetSingleton()->Get();
		if (!settings.enabled || !IsSpawnAllowedByGlobalState(a_player, a_cell)) {
			return;
		}

		if (!PassesFilters(a_player, a_cell, settings)) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		if (!a_forced) {
			if (m_lastGlobalSpawn.time_since_epoch().count() != 0 &&
			    (now - m_lastGlobalSpawn) < std::chrono::duration<float>(settings.timing.cooldownSecondsGlobal)) {
				return;
			}

			if (const auto it = m_lastCellSpawn.find(a_cell->GetFormID()); it != m_lastCellSpawn.end()) {
				if ((now - it->second) < std::chrono::duration<float>(settings.timing.cooldownSecondsSameCell)) {
					return;
				}
			}
		}

		const auto rule = FindMatchingRule(a_player, a_cell);
		if (!rule || !rule->enabled) {
			return;
		}

		std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<double> chanceDist(0.0, 1.0);
		if (!a_forced && chanceDist(rng) > rule->budget.chance) {
			return;
		}

		const auto alive = ManagedRegistry::GetSingleton()->CountAlive();
		const auto inCell = ManagedRegistry::GetSingleton()->CountInCell(a_cell->GetFormID());
		if (alive >= settings.limits.globalMaxAlive || inCell >= settings.limits.maxPerCell) {
			return;
		}

		std::uniform_int_distribution<std::int32_t> countDist(rule->budget.minToSpawn, rule->budget.maxToSpawn);
		std::int32_t count = a_forced ? std::max(1, rule->budget.minToSpawn) : countDist(rng);
		count = std::min(count, settings.limits.maxPerEvent);
		count = std::min(count, settings.limits.globalMaxAlive - alive);
		count = std::min(count, settings.limits.maxPerCell - inCell);
		if (count <= 0) {
			return;
		}

		PendingSpawn pending{};
		pending.cellID = a_cell->GetFormID();
		pending.count = count;
		pending.rule = *rule;
		pending.executeAt = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<float>(settings.timing.deferSpawnSeconds));

		{
			std::scoped_lock lk(m_lock);
			m_pendingSpawn = std::move(pending);
		}

		m_lastGlobalSpawn = now;
		m_lastCellSpawn[a_cell->GetFormID()] = now;
		LOG_DEBUG("Queued spawn in cell {:08X}, rule '{}', count {}", a_cell->GetFormID(), rule->name, count);
	}

	void SpawnManager::RunPendingSpawn()
	{
		std::optional<PendingSpawn> pending{};
		{
			std::scoped_lock lk(m_lock);
			if (!m_pendingSpawn) {
				return;
			}
			if (std::chrono::steady_clock::now() < m_pendingSpawn->executeAt) {
				return;
			}
			pending = m_pendingSpawn;
			m_pendingSpawn.reset();
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* cell = player->GetParentCell();
		if (!cell || cell->GetFormID() != pending->cellID) {
			return;
		}

		ExecuteSpawnEvent(player, cell, pending->rule, pending->count);
	}

	void SpawnManager::ExecuteSpawnEvent(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SpawnRule& a_rule, std::int32_t a_requestedCount)
	{
		if (!a_player || !a_cell || a_requestedCount <= 0) {
			return;
		}

		const auto& settings = Settings::GetSingleton()->Get();
		std::int32_t spawned = 0;
		for (std::int32_t i = 0; i < a_requestedCount; ++i) {
			const auto alive = ManagedRegistry::GetSingleton()->CountAlive();
			const auto inCell = ManagedRegistry::GetSingleton()->CountInCell(a_cell->GetFormID());
			if (alive >= settings.limits.globalMaxAlive || inCell >= settings.limits.maxPerCell) {
				break;
			}

			if (auto* actor = SpawnOne(a_player, a_cell, a_rule)) {
				ApplyPostSpawnGenesisCompatibility(actor, a_player, settings);
				ManagedRegistry::GetSingleton()->AddSpawn(actor, a_cell);
				++spawned;
			}
		}

		if (spawned > 0) {
			ManagedRegistry::GetSingleton()->PruneToLimit(settings.limits.maxManagedRefs);
			if (settings.debug.notify) {
				if (auto* console = RE::ConsoleLog::GetSingleton()) {
					const auto cellName = UTILS::GetCellName(a_cell);
					console->Print("DynamicSpawns: spawned %d in %s", spawned, cellName.empty() ? "<unnamed>" : cellName.c_str());
				}
			}
			LOG_INFO("Spawned {} actor(s) in cell {:08X} via rule '{}'", spawned, a_cell->GetFormID(), a_rule.name);
		}
	}

	bool SpawnManager::IsSpawnAllowedByGlobalState(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell) const
	{
		if (!a_player || !a_cell) {
			return false;
		}
		if (UTILS::IsGameplayBlocked()) {
			return false;
		}
		return true;
	}

	bool SpawnManager::PassesFilters(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SettingsData& a_settings) const
	{
		const auto& filters = a_settings.filters;
		if (filters.skipIfInCombat && a_player->IsInCombat()) {
			return false;
		}
		if (filters.skipIfSneaking && a_player->IsSneaking()) {
			return false;
		}
		if (filters.skipIfInterior && a_cell->IsInteriorCell()) {
			return false;
		}
		if (filters.skipIfExterior && a_cell->IsExteriorCell()) {
			return false;
		}

		const auto worldEditorID = UTILS::ToLower(UTILS::GetWorldspaceEditorID(a_player->GetWorldspace()));
		for (const auto& blocked : filters.skipWorldspacesLower) {
			if (!blocked.empty() && worldEditorID == blocked) {
				return false;
			}
		}

		if (UTILS::IsCellNameBlacklisted(UTILS::GetCellName(a_cell), filters.skipCellsByNameContainsLower)) {
			return false;
		}

		auto* location = UTILS::GetCurrentLocation(a_player);
		if (HasAnyLocationKeyword(location, filters.skipLocationsByKeyword)) {
			return false;
		}

		if (filters.onlySpawnIfEncounterZoneExists && a_player->GetEncounterZone() == nullptr) {
			return false;
		}

		if (a_settings.genesis.skipIfHostilesNearby &&
			HasNearbyHostile(a_player, a_settings.genesis.hostileScanRadius, a_settings.genesis.hostileScanMaxRefs)) {
			return false;
		}

		if (!filters.skipIfCellHasKeyword.empty() && HasAnyLocationKeyword(a_cell->GetLocation(), filters.skipIfCellHasKeyword)) {
			return false;
		}

		return true;
	}

	bool SpawnManager::RuleMatches(const SpawnRule& a_rule, RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, RE::BGSLocation* a_location) const
	{
		if (!a_rule.enabled) {
			return false;
		}

		const auto& match = a_rule.match;
		if (!match.worldspace.empty()) {
			const auto worldspace = UTILS::ToLower(UTILS::GetWorldspaceEditorID(a_player->GetWorldspace()));
			if (worldspace != UTILS::ToLower(match.worldspace)) {
				return false;
			}
		}

		if (match.cellType == CellType::kInterior && !a_cell->IsInteriorCell()) {
			return false;
		}
		if (match.cellType == CellType::kExterior && !a_cell->IsExteriorCell()) {
			return false;
		}

		if (match.timeOfDay != TimeOfDay::kAny && match.timeOfDay != UTILS::GetTimeOfDay()) {
			return false;
		}

		if (!match.locationKeywordAny.empty() && !HasAnyLocationKeyword(a_location, match.locationKeywordAny)) {
			return false;
		}

		return true;
	}

	std::optional<SpawnRule> SpawnManager::FindMatchingRule(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell) const
	{
		const auto& settings = Settings::GetSingleton()->Get();
		auto* location = UTILS::GetCurrentLocation(a_player);

		for (const auto& rule : settings.spawnRules) {
			if (RuleMatches(rule, a_player, a_cell, location)) {
				LOG_DEBUG("Matched spawn rule '{}'", rule.name);
				return rule;
			}
		}
		return std::nullopt;
	}

	RE::Actor* SpawnManager::SpawnOne(RE::PlayerCharacter* a_player, RE::TESObjectCELL* a_cell, const SpawnRule& a_rule) const
	{
		const auto& settings = Settings::GetSingleton()->Get();
		const bool allowInteriorRandom = false;
		const auto placement = PlacementService::GetSingleton()->FindPlacement(
			a_player,
			a_cell,
			settings,
			*ManagedRegistry::GetSingleton(),
			allowInteriorRandom);
		if (!placement) {
			return nullptr;
		}

		auto* baseToSpawn = ResolveSpawnBaseObject(a_rule);
		if (!baseToSpawn) {
			return nullptr;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return nullptr;
		}

		const RE::ObjectRefHandle linked{};
		const auto handle = dataHandler->CreateReferenceAtLocation(
			baseToSpawn,
			placement->position,
			placement->rotation,
			placement->cell,
			placement->worldspace,
			nullptr,
			nullptr,
			linked,
			false,
			true);

		auto refr = RE::TESObjectREFR::LookupByHandle(handle.native_handle());
		auto* actor = refr ? refr->As<RE::Actor>() : nullptr;
		if (!actor) {
			return nullptr;
		}

		if (a_rule.factions.inheritFromEncounterZone) {
			if (auto* zone = a_player->GetEncounterZone()) {
				actor->SetEncounterZone(zone);
			}
		}

		return actor;
	}

	RE::TESBoundObject* SpawnManager::ResolveSpawnBaseObject(const SpawnRule& a_rule) const
	{
		auto* resolver = FormResolver::GetSingleton();
		const auto resolved = resolver->ResolveSpawnForm(a_rule.what.form);

		if (a_rule.what.method == SpawnMethod::kActorBase) {
			return resolved.kind == ResolvedSpawnForm::Kind::kActorBase ? resolved.boundObject : nullptr;
		}
		if (a_rule.what.method == SpawnMethod::kLeveledList) {
			return resolved.kind == ResolvedSpawnForm::Kind::kLeveledList ? resolved.boundObject : nullptr;
		}
		if (a_rule.what.method == SpawnMethod::kFormList) {
			if (resolved.kind != ResolvedSpawnForm::Kind::kFormList || !resolved.formList) {
				return nullptr;
			}

			std::vector<RE::TESBoundObject*> choices{};
			resolved.formList->ForEachForm([&](RE::TESForm* a_form) {
				if (!a_form) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				if (auto* npc = a_form->As<RE::TESNPC>()) {
					choices.push_back(npc);
				} else if (auto* lev = a_form->As<RE::TESLevCharacter>()) {
					choices.push_back(lev);
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});

			if (choices.empty()) {
				return nullptr;
			}

			std::mt19937 rng{ std::random_device{}() };
			std::uniform_int_distribution<std::size_t> dist(0, choices.size() - 1);
			return choices[dist(rng)];
		}

		return nullptr;
	}

	bool SpawnManager::HasNearbyHostile(RE::PlayerCharacter* a_player, float a_radius, std::int32_t a_maxRefs) const
	{
		auto* cell = a_player ? a_player->GetParentCell() : nullptr;
		if (!a_player || !cell) {
			return false;
		}

		const auto origin = a_player->GetPosition();
		std::int32_t scanned = 0;
		bool found = false;

		cell->ForEachReferenceInRange(origin, a_radius, [&](RE::TESObjectREFR* a_ref) {
			if (!a_ref || found) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			if (++scanned > a_maxRefs) {
				return RE::BSContainer::ForEachResult::kStop;
			}
			auto* actor = a_ref->As<RE::Actor>();
			if (!actor || actor == a_player || actor->IsDead()) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			if (actor->IsHostileToActor(a_player)) {
				found = true;
				return RE::BSContainer::ForEachResult::kStop;
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		return found;
	}

	void SpawnManager::ApplyPostSpawnGenesisCompatibility(RE::Actor* a_actor, RE::PlayerCharacter* a_player, const SettingsData& a_settings) const
	{
		if (!a_actor || !a_player) {
			return;
		}
		auto* actorAV = a_actor->AsActorValueOwner();
		auto* playerAV = a_player->AsActorValueOwner();
		if (!actorAV || !playerAV) {
			return;
		}

		const auto& g = a_settings.genesis;
		std::mt19937 rng{ std::random_device{}() };

		if (g.unlevelNPCs) {
			auto rollPct = [&](float minPct, float maxPct) {
				std::uniform_real_distribution<float> dist(minPct, maxPct);
				return dist(rng) / 100.0F;
			};
			const float hp = std::max(1.0F, playerAV->GetBaseActorValue(RE::ActorValue::kHealth) * rollPct(g.healthPctMin, g.healthPctMax));
			const float mp = std::max(1.0F, playerAV->GetBaseActorValue(RE::ActorValue::kMagicka) * rollPct(g.magickaPctMin, g.magickaPctMax));
			const float sp = std::max(1.0F, playerAV->GetBaseActorValue(RE::ActorValue::kStamina) * rollPct(g.staminaPctMin, g.staminaPctMax));
			actorAV->SetActorValue(RE::ActorValue::kHealth, hp);
			actorAV->SetActorValue(RE::ActorValue::kMagicka, mp);
			actorAV->SetActorValue(RE::ActorValue::kStamina, sp);
		}

		// Best effort equivalent for immediate hostility.
		actorAV->SetActorValue(RE::ActorValue::kAggression, 3.0F);

		if (g.giveSpawnPotions) {
			std::uniform_int_distribution<std::int32_t> chance(1, 100);
			if (chance(rng) <= g.potionChancePct) {
				std::uniform_int_distribution<std::int32_t> countDist(g.potionCountMin, g.potionCountMax);
				const auto count = countDist(rng);
				for (std::int32_t i = 0; i < count; ++i) {
					if (auto* item = FormResolver::GetSingleton()->ResolveRandomBoundObject(g.spawnPotionPool)) {
						a_actor->AddObjectToContainer(item, nullptr, 1, nullptr);
					}
				}
			}
		}
	}

	void SpawnManager::TryInjectLoot(const RE::TESOpenCloseEvent* a_event)
	{
		if (!a_event || !a_event->opened || !a_event->ref || !a_event->activeRef) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || a_event->activeRef.get() != player) {
			return;
		}

		const auto& g = Settings::GetSingleton()->Get().genesis;
		if (!g.lootInjectionEnabled) {
			return;
		}

		auto* containerRef = a_event->ref.get();
		if (!containerRef) {
			return;
		}

		auto* base = containerRef->GetBaseObject();
		if (!base || base->GetFormType() != RE::FormType::Container) {
			return;
		}

		const auto id = containerRef->GetFormID();
		{
			std::scoped_lock lk(m_lock);
			if (m_lootProcessedContainers.contains(id)) {
				return;
			}
		}

		std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<std::int32_t> chance(1, 100);
		if (chance(rng) > g.lootChancePct) {
			return;
		}

		auto* resolver = FormResolver::GetSingleton();
		auto addOne = [&](const std::vector<std::string>& pool) {
			if (auto* item = resolver->ResolveRandomBoundObject(pool)) {
				containerRef->AddObjectToContainer(item, nullptr, 1, nullptr);
			}
		};

		addOne(g.lootPotionPool);
		addOne(g.lootScrollPool);
		addOne(g.lootMiscPool);
		addOne(g.lootSackPool);

		{
			std::scoped_lock lk(m_lock);
			m_lootProcessedContainers.insert(id);
		}
	}
}
