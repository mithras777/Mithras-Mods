#include "movement/MovementPatcher.h"

#include "RE/A/Actor.h"
#include "RE/M/MagicCaster.h"
#include "RE/M/MagicSystem.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESObjectWEAP.h"

#include <algorithm>
#include <cctype>
#include <array>
#include <string_view>

namespace MOVEMENT
{
	namespace
	{
		constexpr auto kNPCDefaultForm = "Skyrim.esm|0x0003580D";
		constexpr auto kNPC1HMForm = "Skyrim.esm|0x00069CD8";
		constexpr auto kNPC2HMForm = "Skyrim.esm|0x00069CD9";

		constexpr std::array<const char*, 5> kPlayerEntryNames{
			"Player_Default_MT",
			"Player_Blocking_MT",
			"Player_BowDrawn_MT",
			"Player_MagicCasting_MT",
			"Player_Sneaking_MT"
		};

		bool IsSprintEntry(std::string_view a_name)
		{
			return a_name.find("Sprinting") != std::string_view::npos ||
			       a_name.find("Sprint") != std::string_view::npos;
		}

		const MovementEntry* FindEntryByName(const SettingsData& a_settings, std::string_view a_name)
		{
			for (const auto& entry : a_settings.entries) {
				if (entry.name == a_name) {
					return &entry;
				}
			}
			return nullptr;
		}

		bool IsBowEquipped(RE::PlayerCharacter& a_player)
		{
			const auto* right = skyrim_cast<RE::TESObjectWEAP*>(a_player.GetEquippedObject(false));
			const auto* left = skyrim_cast<RE::TESObjectWEAP*>(a_player.GetEquippedObject(true));
			const auto* weapon = right ? right : left;
			if (!weapon) {
				return false;
			}
			return weapon->IsBow() || weapon->IsCrossbow();
		}

		bool IsMagicActive(RE::PlayerCharacter& a_player)
		{
			for (const auto source : { RE::MagicSystem::CastingSource::kLeftHand, RE::MagicSystem::CastingSource::kRightHand, RE::MagicSystem::CastingSource::kOther, RE::MagicSystem::CastingSource::kInstant }) {
				const auto* caster = a_player.GetMagicCaster(source);
				if (!caster) {
					continue;
				}
				if (caster->state != RE::MagicCaster::State::kNone) {
					return true;
				}
				if (caster->currentSpell) {
					return true;
				}
			}
			return false;
		}

		std::string_view ResolvePlayerEntryName(RE::PlayerCharacter& a_player)
		{
			const auto* actorState = a_player.AsActorState();
			if (!actorState) {
				return kPlayerEntryNames[0];
			}

			if (actorState->actorState1.sneaking) {
				return kPlayerEntryNames[4];
			}

			if (actorState->actorState2.wantBlocking || a_player.IsBlocking()) {
				return kPlayerEntryNames[1];
			}

			if (IsBowEquipped(a_player)) {
				if (actorState->IsWeaponDrawn()) {
					return kPlayerEntryNames[2];
				}
			}

			if (IsMagicActive(a_player)) {
				return kPlayerEntryNames[3];
			}

			return kPlayerEntryNames[0];
		}

		void RefreshActiveActorMovement()
		{
			// Intentionally no-op: do not force movement refresh pulses.
		}
	}

	void MovementPatcher::StartupApply(const SettingsData& a_settings)
	{
		RestoreCurrentCache();
		RebuildResolvedTargets(a_settings);
		CaptureOriginalsOnce();
		if (a_settings.general.enabled) {
			ApplyResolved(a_settings);
			RefreshActiveActorMovement();
		}
	}

	void MovementPatcher::OnSettingsChanged(const SettingsData& a_oldSettings, const SettingsData& a_newSettings)
	{
		if (a_newSettings.general.enabled) {
			RestoreCurrentCache();
			RebuildResolvedTargets(a_newSettings);
			CaptureOriginalsOnce();
			ApplyResolved(a_newSettings);
			RefreshActiveActorMovement();
			return;
		}

		if (a_oldSettings.general.enabled && a_newSettings.general.restoreOnDisable) {
			RestoreCurrentCache();
			RefreshActiveActorMovement();
		}

		RebuildResolvedTargets(a_newSettings);
		CaptureOriginalsOnce();
	}

	void MovementPatcher::ApplyNow(const SettingsData& a_settings)
	{
		RestoreCurrentCache();
		RebuildResolvedTargets(a_settings);
		CaptureOriginalsOnce();
		if (a_settings.general.enabled) {
			ApplyResolved(a_settings);
			RefreshActiveActorMovement();
		}
	}

	void MovementPatcher::Restore()
	{
		RestoreCurrentCache();
		RefreshActiveActorMovement();
	}

	void MovementPatcher::ShutdownRestoreIfNeeded(const SettingsData& a_settings)
	{
		if (a_settings.general.restoreOnDisable) {
			RestoreCurrentCache();
			RefreshActiveActorMovement();
		}
	}

	float MovementPatcher::GetPlayerSpeedMultiplier(const SettingsData& a_settings, RE::PlayerCharacter& a_player) const
	{
		const auto playerEntryName = ResolvePlayerEntryName(a_player);
		const auto* entry = FindEntryByName(a_settings, playerEntryName);
		if (!entry || !entry->playerOnly || !entry->enabled) {
			return 100.0f;
		}

		return entry->playerSpeedMult;
	}

	void MovementPatcher::RestoreCurrentCache()
	{
		for (auto& [_, entry] : m_resolved) {
			if (!entry.form || !entry.originalsCaptured) {
				continue;
			}
			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					entry.form->movementTypeData.defaultData.speeds[i][j] = entry.originals[i][j];
				}
			}
		}
	}

	void MovementPatcher::RebuildResolvedTargets(const SettingsData& a_settings)
	{
		auto previous = m_resolved;
		m_resolved.clear();
		m_resolved.reserve(a_settings.entries.size() + 2);

		auto addResolved = [this, &previous](const MovementEntry& a_entry) {
			auto resolved = ResolveTarget(a_entry);
			if (!resolved.valid) {
				return;
			}

			if (const auto it = previous.find(resolved.formKey); it != previous.end() && it->second.originalsCaptured) {
				resolved.originalsCaptured = true;
				for (std::size_t i = 0; i < 5; ++i) {
					for (std::size_t j = 0; j < 2; ++j) {
						resolved.originals[i][j] = it->second.originals[i][j];
					}
				}
			}

			m_resolved[resolved.formKey] = std::move(resolved);
		};

		for (const auto& entry : a_settings.entries) {
			addResolved(entry);
		}

		MovementEntry npc1HM{};
		npc1HM.form = kNPC1HMForm;
		addResolved(npc1HM);

		MovementEntry npc2HM{};
		npc2HM.form = kNPC2HMForm;
		addResolved(npc2HM);
	}

	void MovementPatcher::CaptureOriginalsOnce()
	{
		for (auto& [_, entry] : m_resolved) {
			if (!entry.form || entry.originalsCaptured) {
				continue;
			}
			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					entry.originals[i][j] = entry.form->movementTypeData.defaultData.speeds[i][j];
				}
			}
			entry.originalsCaptured = true;
		}
	}

	void MovementPatcher::ApplyResolved(const SettingsData& a_settings)
	{
		// Movement::SPEED_DIRECTIONS::kRotations (index 4) controls turning/rotation behavior.
		// We intentionally do not patch it so camera/turn speed remains untouched.
		constexpr std::size_t kPatchedDirections = 4;
		const auto npcDefaultKey = CanonicalTargetKey(kNPCDefaultForm);
		const auto npc1HMKey = CanonicalTargetKey(kNPC1HMForm);
		const auto npc2HMKey = CanonicalTargetKey(kNPC2HMForm);
		float npcDefaultRun[kPatchedDirections]{};
		bool hasNPCDefaultRun = false;

		for (const auto& setting : a_settings.entries) {
			if (!setting.enabled) {
				continue;
			}

			const auto key = CanonicalTargetKey(setting.form);
			const auto it = m_resolved.find(key);
			if (it == m_resolved.end() || !it->second.form) {
				continue;
			}

			if (IsSprintEntry(setting.name)) {
				it->second.form->movementTypeData.defaultData.speeds[2][1] = setting.speeds[2][1];
				continue;
			}

			if (setting.group == "Horses") {
				// Horses are forward-only for run tuning.
				it->second.form->movementTypeData.defaultData.speeds[2][1] = setting.speeds[2][1];
				continue;
			}

			for (std::size_t i = 0; i < kPatchedDirections; ++i) {
				it->second.form->movementTypeData.defaultData.speeds[i][1] = setting.speeds[i][1];
				if (key == npcDefaultKey) {
					npcDefaultRun[i] = setting.speeds[i][1];
					hasNPCDefaultRun = true;
				}
			}
		}

		if (hasNPCDefaultRun) {
			for (const auto& targetKey : { npc1HMKey, npc2HMKey }) {
				const auto target = m_resolved.find(targetKey);
				if (target == m_resolved.end() || !target->second.form) {
					continue;
				}
				for (std::size_t i = 0; i < kPatchedDirections; ++i) {
					target->second.form->movementTypeData.defaultData.speeds[i][1] = npcDefaultRun[i];
				}
			}
		}
	}

	std::string MovementPatcher::Trim(std::string a_text)
	{
		while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front())) != 0) {
			a_text.erase(a_text.begin());
		}
		while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back())) != 0) {
			a_text.pop_back();
		}
		return a_text;
	}

	std::string MovementPatcher::CanonicalTargetKey(const std::string& a_formString)
	{
		std::string key = Trim(a_formString);
		std::transform(
			key.begin(),
			key.end(),
			key.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return key;
	}

	MovementPatcher::ResolvedTarget MovementPatcher::ResolveTarget(const MovementEntry& a_entry)
	{
		ResolvedTarget out{};
		out.formKey = CanonicalTargetKey(a_entry.form);
		if (out.formKey.empty()) {
			return out;
		}

		const auto separator = a_entry.form.find('|');
		if (separator == std::string::npos) {
			return out;
		}

		const std::string plugin = Trim(a_entry.form.substr(0, separator));
		const std::string idText = Trim(a_entry.form.substr(separator + 1));
		if (plugin.empty() || idText.empty()) {
			return out;
		}

		std::uint32_t parsedID = 0;
		try {
			parsedID = static_cast<std::uint32_t>(std::stoul(idText, nullptr, 0));
		} catch (...) {
			return out;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return out;
		}

		auto* movementType = dataHandler->LookupForm<RE::BGSMovementType>(parsedID, plugin);
		if (!movementType) {
			movementType = dataHandler->LookupFormRaw<RE::BGSMovementType>(parsedID, plugin);
		}
		if (!movementType) {
			return out;
		}

		out.valid = true;
		out.form = movementType;
		return out;
	}
}
