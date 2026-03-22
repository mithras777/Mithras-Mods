#include "spellbinding/SpellBindingManager.h"

#include "event/AttackAnimationEventSink.h"
#include "overhaul/SpellbladeOverhaulManager.h"
#include "smartcast/SmartCastController.h"
#include "ui/PrismaBridge.h"
#include "util/LogUtil.h"

#include "RE/C/Calendar.h"
#include "RE/M/MagicMenu.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>

namespace SBIND
{
	using json = nlohmann::json;

	namespace
	{
		constexpr std::uint32_t kSerializationVersion = 4;
		constexpr std::uint32_t kBindingsRecord = 'SBND';
		constexpr std::uint32_t kConfigVersion = 2;
		constexpr float kMinWindowWidth = 960.0f;
		constexpr float kMinWindowHeight = 620.0f;

		constexpr float kDefaultWeaponCooldown = 1.5f;
		constexpr bool kDefaultOnlyInCombat = false;
		constexpr float kPendingLightDelaySec = 0.30f;

		AttackSlot ParseAttackSlot(std::uint32_t a_slot)
		{
			switch (a_slot) {
				case 1: return AttackSlot::kPower;
				case 2: return AttackSlot::kBash;
				default: return AttackSlot::kLight;
			}
		}
	}

	void Manager::Initialize()
	{
		std::scoped_lock lock(m_lock);
		m_config = {};
		m_config.version = kConfigVersion;
		m_runtime = {};
		m_runtime.unarmedKey = BuildUnarmedKey();
		LoadConfig();
		ClampConfig(m_config);
	}

	void Manager::OnRevert()
	{
		std::scoped_lock lock(m_lock);
		m_bindings.clear();
		m_runtime = {};
		m_runtime.unarmedKey = BuildUnarmedKey();
		LoadConfig();
		ClampConfig(m_config);
	}

	void Manager::Serialize(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc) {
			return;
		}

		std::scoped_lock lock(m_lock);
		if (!a_intfc->OpenRecord(kBindingsRecord, kSerializationVersion)) {
			return;
		}

		const auto writeString = [a_intfc](const std::string& a_value) {
			const std::uint32_t length = static_cast<std::uint32_t>(a_value.size());
			a_intfc->WriteRecordData(length);
			for (const char ch : a_value) {
				a_intfc->WriteRecordData(ch);
			}
		};
		const auto writeSlot = [a_intfc, &writeString](const SlotBinding& a_slot) {
			writeString(a_slot.spellFormKey);
			writeString(a_slot.displayName);
			a_intfc->WriteRecordData(a_slot.lastKnownCost);
			a_intfc->WriteRecordData(a_slot.spellType);
			a_intfc->WriteRecordData(a_slot.castHoldSec);
			a_intfc->WriteRecordData(a_slot.castIntervalSec);
			a_intfc->WriteRecordData(a_slot.castCount);
			a_intfc->WriteRecordData(a_slot.enabled);
		};

		const std::uint32_t count = static_cast<std::uint32_t>(m_bindings.size());
		a_intfc->WriteRecordData(count);
		for (const auto& [key, profile] : m_bindings) {
			writeString(key.pluginName);
			a_intfc->WriteRecordData(key.localFormID);
			a_intfc->WriteRecordData(key.uniqueID);
			const auto slot = static_cast<std::uint32_t>(key.handSlot);
			a_intfc->WriteRecordData(slot);
			a_intfc->WriteRecordData(key.isUnarmed);
			writeString(key.displayName);

			writeSlot(profile.light);
			writeSlot(profile.power);
			writeSlot(profile.bash);
			a_intfc->WriteRecordData(profile.triggerCooldownSec);
			a_intfc->WriteRecordData(profile.onlyInCombat);
		}
	}

	void Manager::Deserialize(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		std::unordered_map<BindingKey, WeaponBindingProfile, BindingKeyHash> loaded{};

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kBindingsRecord || version > kSerializationVersion) {
				continue;
			}

			const auto readString = [a_intfc](std::string& a_value) {
				std::uint32_t size = 0;
				a_intfc->ReadRecordData(size);
				a_value.clear();
				a_value.reserve(size);
				for (std::uint32_t i = 0; i < size; ++i) {
					char ch = '\0';
					a_intfc->ReadRecordData(ch);
					a_value.push_back(ch);
				}
			};
			const auto readSlot = [a_intfc, &readString](SlotBinding& a_slot) {
				readString(a_slot.spellFormKey);
				readString(a_slot.displayName);
				a_intfc->ReadRecordData(a_slot.lastKnownCost);
				a_intfc->ReadRecordData(a_slot.spellType);
				a_intfc->ReadRecordData(a_slot.enabled);
			};
			const auto readSlotV4 = [a_intfc, &readString](SlotBinding& a_slot) {
				readString(a_slot.spellFormKey);
				readString(a_slot.displayName);
				a_intfc->ReadRecordData(a_slot.lastKnownCost);
				a_intfc->ReadRecordData(a_slot.spellType);
				a_intfc->ReadRecordData(a_slot.castHoldSec);
				a_intfc->ReadRecordData(a_slot.castIntervalSec);
				a_intfc->ReadRecordData(a_slot.castCount);
				a_intfc->ReadRecordData(a_slot.enabled);
			};

			std::uint32_t count = 0;
			a_intfc->ReadRecordData(count);
			for (std::uint32_t i = 0; i < count; ++i) {
				BindingKey key{};
				std::uint32_t handSlot = 0;
				readString(key.pluginName);
				a_intfc->ReadRecordData(key.localFormID);
				a_intfc->ReadRecordData(key.uniqueID);
				a_intfc->ReadRecordData(handSlot);
				a_intfc->ReadRecordData(key.isUnarmed);
				readString(key.displayName);
				key.handSlot = static_cast<HandSlot>(handSlot);

				WeaponBindingProfile profile{};
				profile.triggerCooldownSec = kDefaultWeaponCooldown;
				profile.onlyInCombat = kDefaultOnlyInCombat;

				if (version >= 4) {
					readSlotV4(profile.light);
					readSlotV4(profile.power);
					readSlotV4(profile.bash);
					a_intfc->ReadRecordData(profile.triggerCooldownSec);
					a_intfc->ReadRecordData(profile.onlyInCombat);
				} else if (version >= 3) {
					readSlot(profile.light);
					readSlot(profile.power);
					readSlot(profile.bash);
					a_intfc->ReadRecordData(profile.triggerCooldownSec);
					a_intfc->ReadRecordData(profile.onlyInCombat);
				} else {
					readString(profile.light.spellFormKey);
					readString(profile.light.displayName);
					a_intfc->ReadRecordData(profile.light.lastKnownCost);
					if (version >= 2) {
						a_intfc->ReadRecordData(profile.light.spellType);
					} else if (auto* resolved = ResolveSpell(profile.light.spellFormKey)) {
						profile.light.spellType = static_cast<std::uint32_t>(resolved->GetSpellType());
					}
					profile.light.enabled = !profile.light.spellFormKey.empty();
				}

				profile.triggerCooldownSec = std::clamp(profile.triggerCooldownSec, 0.5f, 5.0f);
				auto clampSlot = [](SlotBinding& slot) {
					slot.castHoldSec = std::clamp(slot.castHoldSec, 0.30f, 10.0f);
					slot.castIntervalSec = std::clamp(slot.castIntervalSec, 0.0f, 1.0f);
					slot.castCount = std::clamp(slot.castCount, 1u, 10u);
				};
				clampSlot(profile.light);
				clampSlot(profile.power);
				clampSlot(profile.bash);
				loaded[key] = std::move(profile);
			}
		}

		std::scoped_lock lock(m_lock);
		m_bindings = std::move(loaded);
		m_runtime.unarmedKey = BuildUnarmedKey();
	}

	SpellBindingConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetConfig(const SpellBindingConfig& a_config, bool a_save)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			ClampConfig(m_config);
		}
		if (a_save) {
			SaveConfig();
		}
		PushUISnapshot();
		PushHUDSnapshot();
	}
	void Manager::SetSettingFromJson(const std::string& a_payload)
	{
		if (a_payload.empty()) {
			return;
		}

		try {
			const auto parsed = json::parse(a_payload);
			auto config = GetConfig();
			const auto id = parsed.value("id", std::string{});
			if (id == "enabled") {
				config.enabled = parsed.value("value", config.enabled);
			} else if (id == "uiToggleKey") {
				config.uiToggleKey = parsed.value("value", config.uiToggleKey);
			} else if (id == "bindKey") {
				config.bindKey = parsed.value("value", config.bindKey);
			} else if (id == "cycleSlotModifierKey") {
				config.cycleSlotModifierKey = parsed.value("value", config.cycleSlotModifierKey);
			} else if (id == "showHudNotifications") {
				config.showHudNotifications = parsed.value("value", config.showHudNotifications);
			} else if (id == "enableSoundCues") {
				config.enableSoundCues = parsed.value("value", config.enableSoundCues);
			} else if (id == "soundCueVolume") {
				config.soundCueVolume = parsed.value("value", config.soundCueVolume);
			} else if (id == "hudDonutEnabled") {
				config.hudDonutEnabled = parsed.value("value", config.hudDonutEnabled);
			} else if (id == "hudDonutOnlyUnsheathed") {
				config.hudDonutOnlyUnsheathed = parsed.value("value", config.hudDonutOnlyUnsheathed);
			} else if (id == "hudShowCooldownSeconds") {
				config.hudShowCooldownSeconds = parsed.value("value", config.hudShowCooldownSeconds);
			} else if (id == "hudDonutSize") {
				config.hudDonutSize = parsed.value("value", config.hudDonutSize);
			} else if (id == "hudCycleSize") {
				config.hudCycleSize = parsed.value("value", config.hudCycleSize);
			} else if (id == "hudChainSize") {
				config.hudChainSize = parsed.value("value", config.hudChainSize);
			} else if (id == "hudChainAlwaysShowInCombat") {
				config.hudChainAlwaysShowInCombat = parsed.value("value", config.hudChainAlwaysShowInCombat);
			} else if (id == "blacklistEnabled") {
				config.blacklistEnabled = parsed.value("value", config.blacklistEnabled);
			} else if (id == "debugMode") {
				config.debugMode = parsed.value("value", config.debugMode);
			} else if (id == "currentBindSlotMode") {
				config.currentBindSlotMode = ParseAttackSlot(parsed.value("value", static_cast<std::uint32_t>(config.currentBindSlotMode)));
			}
			SetConfig(config, true);
		} catch (...) {}
	}

	void Manager::SetWeaponSettingFromJson(const std::string& a_payload)
	{
		if (a_payload.empty()) {
			return;
		}

		try {
			const auto parsed = json::parse(a_payload);
			BindingKey key{};
			auto keyJson = parsed["key"];
			key.pluginName = keyJson.value("pluginName", "");
			key.localFormID = keyJson.value("localFormID", 0u);
			key.uniqueID = static_cast<std::uint16_t>(keyJson.value("uniqueID", 0u));
			key.handSlot = static_cast<HandSlot>(keyJson.value("handSlot", 0u));
			key.isUnarmed = keyJson.value("isUnarmed", false);
			key.displayName = keyJson.value("displayName", "");
			const auto id = parsed.value("id", std::string{});

			{
				std::scoped_lock lock(m_lock);
				auto& profile = EnsureProfileLocked(key);
				if (id == "triggerCooldownSec") {
					profile.triggerCooldownSec = std::clamp(parsed.value("value", profile.triggerCooldownSec), 0.5f, 5.0f);
				} else if (id == "onlyInCombat") {
					profile.onlyInCombat = parsed.value("value", profile.onlyInCombat);
				} else if (id == "castHoldSec" || id == "castIntervalSec" || id == "castCount") {
					const auto slot = ParseAttackSlot(parsed.value("slot", 0u));
					auto* slotBinding = GetSlotBinding(profile, slot);
					if (slotBinding) {
						if (id == "castHoldSec") {
							slotBinding->castHoldSec = std::clamp(parsed.value("value", slotBinding->castHoldSec), 0.30f, 10.0f);
						} else if (id == "castIntervalSec") {
							slotBinding->castIntervalSec = std::clamp(parsed.value("value", slotBinding->castIntervalSec), 0.0f, 1.0f);
						} else {
							slotBinding->castCount = std::clamp(parsed.value("value", slotBinding->castCount), 1u, 10u);
						}
					}
				}
			}
			PushUISnapshot();
		} catch (...) {}
	}

	void Manager::SetBlacklistFromJson(const std::string& a_payload)
	{
		if (a_payload.empty()) {
			return;
		}
		try {
			const auto parsed = json::parse(a_payload);
			auto config = GetConfig();
			config.blacklistedSpellKeys.clear();
			for (const auto& it : parsed["items"]) {
				if (it.is_string()) {
					config.blacklistedSpellKeys.push_back(it.get<std::string>());
				}
			}
			SetConfig(config, true);
		} catch (...) {}
	}

	void Manager::SetUIWindowFromJson(const std::string& a_payload)
	{
		if (a_payload.empty()) {
			return;
		}

		try {
			const auto parsed = json::parse(a_payload);
			if (parsed.value("id", std::string{}) != "spellBindingWindow" || !parsed.contains("value") || !parsed["value"].is_object()) {
				return;
			}

			UIWindowConfig next{};
			{
				std::scoped_lock lock(m_lock);
				next = m_uiWindow;
			}
			const auto& value = parsed["value"];
			next.x = value.value("x", next.x);
			next.y = value.value("y", next.y);
			next.width = value.value("width", next.width);
			next.height = value.value("height", next.height);
			next.isFullscreen = value.value("isFullscreen", next.isFullscreen);
			next.hasSaved = true;
			ClampWindowConfig(next);

			{
				std::scoped_lock lock(m_lock);
				m_uiWindow = next;
			}
			SaveConfig();
		} catch (...) {}
	}

	void Manager::Update(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		if (!a_player || a_deltaTime <= 0.0f) {
			return;
		}

		const auto config = GetConfig();
		const bool debugMode = config.debugMode;
		bool powerAttackActive = false;
		{
			std::scoped_lock lock(m_lock);
			m_runtime.worldTimeSec += a_deltaTime;
			m_runtime.wasAttacking = a_player->IsAttacking();
			powerAttackActive = IsPowerAttackActive(a_player);
			if (debugMode && powerAttackActive != m_runtime.wasPowerAttackActive) {
				LOG_INFO("SpellBinding Debug: power attack state changed -> {}", powerAttackActive ? "active" : "inactive");
			}
			if (!powerAttackActive) {
				m_runtime.powerAttackLatched = false;
			}
			m_runtime.wasPowerAttackActive = powerAttackActive;
			if (!m_runtime.wasAttacking) {
				m_runtime.attackChainActive = false;
				m_runtime.pendingLightAttack = false;
			}
		}

		if (!config.enabled || a_player->IsDead()) {
			std::scoped_lock lock(m_lock);
			StopActiveConcentration(a_player, true);
			PushHUDSnapshot();
			return;
		}

		bool triggeredPendingLight = false;
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(a_player);
			UpdateActiveConcentration(a_player, a_deltaTime);
			if (m_runtime.pendingLightAttack && m_runtime.worldTimeSec >= m_runtime.pendingLightReadyAtWorldTimeSec) {
				m_runtime.pendingLightAttack = false;
				const bool shouldPromotePower = powerAttackActive && !m_runtime.powerAttackLatched;
				if (shouldPromotePower) {
					triggeredPendingLight = TryTriggerAttackSlotLocked(a_player, AttackSlot::kPower, true);
					if (triggeredPendingLight) {
						m_runtime.powerAttackLatched = true;
					}
				} else if (!powerAttackActive) {
					triggeredPendingLight = TryTriggerAttackSlotLocked(a_player, AttackSlot::kLight, false);
				}
				if (debugMode) {
					LOG_INFO("SpellBinding Debug: pending attack resolved -> powerActive={}, promotedPower={}, triggered={}", powerAttackActive, shouldPromotePower, triggeredPendingLight);
				}
			}
		}
		if (triggeredPendingLight) {
			PushUISnapshot();
		}
		PushHUDSnapshot();
	}

	void Manager::OnEquipChanged()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(player);
		}
		PushUISnapshot();
		PushHUDSnapshot();
	}

	void Manager::OnMenuStateChanged(bool a_blockingMenuOpen)
	{
		std::scoped_lock lock(m_lock);
		m_runtime.menuBlocked = a_blockingMenuOpen;
		if (m_config.debugMode) {
			LOG_INFO("SpellBinding Debug: menu blocked -> {}", a_blockingMenuOpen);
		}
	}

	void Manager::OnAttackAnimationEvent(std::string_view a_tag, std::string_view a_payload)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		const auto config = GetConfig();
		if (!config.enabled || player->IsDead()) {
			return;
		}
		const bool debugMode = config.debugMode;

		auto toLower = [](std::string_view text) {
			std::string out(text);
			std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			return out;
		};
		const auto tag = toLower(a_tag);
		const auto payload = toLower(a_payload);
		if (debugMode) {
			LOG_INFO("SpellBinding Debug: attack event tag='{}' payload='{}'", tag, payload);
		}

		const bool hasAttackWord =
			tag.find("attack") != std::string::npos ||
			tag.find("swing") != std::string::npos ||
			tag.find("bash") != std::string::npos ||
			payload.find("attack") != std::string::npos ||
			payload.find("swing") != std::string::npos ||
			payload.find("bash") != std::string::npos;
		if (!hasAttackWord) {
			return;
		}

		const bool isEndLike =
			tag.find("stop") != std::string::npos ||
			tag.find("end") != std::string::npos ||
			tag.find("release") != std::string::npos ||
			payload.find("stop") != std::string::npos ||
			payload.find("end") != std::string::npos ||
			payload.find("release") != std::string::npos;
		if (isEndLike) {
			std::scoped_lock lock(m_lock);
			m_runtime.pendingLightAttack = false;
			// End markers delimit attack windows; allow the next attack in a combo chain.
			m_runtime.attackChainActive = false;
			if (debugMode) {
				LOG_INFO("SpellBinding Debug: end-like attack event ignored");
			}
			return;
		}

		const bool isStartLike =
			tag.find("start") != std::string::npos ||
			tag.find("swing") != std::string::npos ||
			tag.find("bash") != std::string::npos ||
			tag.find("begin") != std::string::npos ||
			payload.find("start") != std::string::npos ||
			payload.find("swing") != std::string::npos ||
			payload.find("bash") != std::string::npos ||
			payload.find("begin") != std::string::npos;
		if (!isStartLike) {
			return;
		}

		const bool hasAttackStart =
			tag.find("attackstart") != std::string::npos ||
			payload.find("attackstart") != std::string::npos;
		const bool hasBashStart =
			tag.find("bash") != std::string::npos ||
			payload.find("bash") != std::string::npos;

		bool attemptedTrigger = false;
		{
			std::scoped_lock lock(m_lock);
			const bool powerAttackActive = IsPowerAttackActive(player);
			const bool powerAttackEdge = powerAttackActive && !m_runtime.wasPowerAttackActive;
			m_runtime.wasPowerAttackActive = powerAttackActive;
			if (powerAttackEdge) {
				// Permit light -> power transitions inside a continuous combo.
				m_runtime.attackChainActive = false;
			}

			if (hasBashStart) {
				m_runtime.pendingLightAttack = false;
				attemptedTrigger = TryTriggerAttackSlotLocked(player, AttackSlot::kBash, false);
			} else if (hasAttackStart) {
				if (powerAttackEdge && !m_runtime.powerAttackLatched) {
					m_runtime.pendingLightAttack = false;
					attemptedTrigger = TryTriggerAttackSlotLocked(player, AttackSlot::kPower, true);
					if (attemptedTrigger) {
						m_runtime.powerAttackLatched = true;
					}
				} else {
					m_runtime.pendingLightAttack = true;
					m_runtime.pendingLightReadyAtWorldTimeSec = m_runtime.worldTimeSec + kPendingLightDelaySec;
					if (debugMode) {
						LOG_INFO("SpellBinding Debug: queued pending light (powerActive={}, powerEdge={}, latched={})", powerAttackActive, powerAttackEdge, m_runtime.powerAttackLatched);
					}
					return;
				}
			} else {
				const AttackSlot attackSlot = ResolveAttackSlot(tag, payload, player);
				if (attackSlot == AttackSlot::kPower) {
					if (powerAttackEdge && !m_runtime.powerAttackLatched) {
						m_runtime.pendingLightAttack = false;
						attemptedTrigger = TryTriggerAttackSlotLocked(player, AttackSlot::kPower, true);
						if (attemptedTrigger) {
							m_runtime.powerAttackLatched = true;
						}
					} else {
						m_runtime.pendingLightAttack = true;
						m_runtime.pendingLightReadyAtWorldTimeSec = m_runtime.worldTimeSec + kPendingLightDelaySec;
						if (debugMode) {
							LOG_INFO("SpellBinding Debug: deferred power slot (powerActive={}, powerEdge={}, latched={})", powerAttackActive, powerAttackEdge, m_runtime.powerAttackLatched);
						}
						return;
					}
				} else {
					m_runtime.pendingLightAttack = false;
					attemptedTrigger = TryTriggerAttackSlotLocked(player, attackSlot, false);
				}
			}
			if (debugMode) {
				LOG_INFO("SpellBinding Debug: event resolved (powerActive={}, powerEdge={}, latched={}, triggered={})", powerAttackActive, powerAttackEdge, m_runtime.powerAttackLatched, attemptedTrigger);
			}
		}
		if (attemptedTrigger) {
			PushUISnapshot();
		}
		PushHUDSnapshot();
	}

	bool Manager::TryTriggerAttackSlotLocked(RE::PlayerCharacter* a_player, AttackSlot a_slot, bool a_isPowerAttack)
	{
		if (!a_player) {
			return false;
		}
		if (m_runtime.menuBlocked ||
		    IsMountedBlocked(a_player) ||
		    IsKillmoveBlocked(a_player) ||
		    IsDialogueBlocked()) {
			return false;
		}
		if (m_runtime.attackChainActive) {
			return false;
		}
		RefreshEquippedKeysLocked(a_player);
		std::optional<BindingKey> keyOpt;
		if (m_runtime.rightWeapon.has_value() && m_runtime.leftWeapon.has_value()) {
			if (IsLikelyLeftHandAttack(a_player)) {
				keyOpt = m_runtime.leftWeapon;
			} else {
				keyOpt = m_runtime.rightWeapon;
			}
		} else if (m_runtime.rightWeapon.has_value()) {
			keyOpt = m_runtime.rightWeapon;
		} else if (m_runtime.leftWeapon.has_value()) {
			keyOpt = m_runtime.leftWeapon;
		} else {
			keyOpt = m_runtime.unarmedKey;
		}
		if (!keyOpt.has_value()) {
			return false;
		}

		m_runtime.lastResolvedAttackSlot = a_slot;
		const bool triggered = TryTriggerBoundSpellLocked(a_player, *keyOpt, a_slot, a_isPowerAttack);
		if (triggered) {
			m_runtime.attackChainActive = true;
			m_runtime.lastAttackTriggerWorldTimeSec = m_runtime.worldTimeSec;
		}
		return triggered;
	}

	void Manager::ToggleUI()
	{
		auto* bridge = UI::PRISMA::Bridge::GetSingleton();
		bridge->Initialize();
		if (!bridge->IsAvailable()) {
			Notify("SpellBinding: Prisma UI is not installed");
			return;
		}
		bridge->Toggle();
	}

	void Manager::PushUISnapshot()
	{
		SB_OVERHAUL::Manager::GetSingleton()->PushUISnapshot();
	}

	void Manager::PushHUDSnapshot()
	{
		UI::PRISMA::Bridge::GetSingleton()->PushHUDSnapshot(GetHUDSnapshot());
	}

	ManagerSnapshot Manager::GetSnapshot() const
	{
		std::scoped_lock lock(m_lock);
		return { BuildSnapshotJsonLocked() };
	}

	std::string Manager::GetHUDSnapshot() const
	{
		std::scoped_lock lock(m_lock);
		return BuildHUDSnapshotJsonLocked();
	}
	bool Manager::TryBindSelectedMagicMenuSpell()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		const auto selectedFormID = ResolveSelectedSpellFormIDFromMagicMenu();
		if (!selectedFormID.has_value()) {
			Notify("SpellBinding: no selected spell in Magic Menu");
			return false;
		}

		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*selectedFormID);
		if (!spell || !IsSupportedSpell(spell)) {
			Notify("SpellBinding: selected spell type is not supported");
			return false;
		}
		if (IsBoundWeaponSpell(spell)) {
			{
				std::scoped_lock lock(m_lock);
				m_runtime.cycleHudIsError = true;
				m_runtime.cycleHudErrorText = "Blocked: Bound spell";
				m_runtime.lastCycleSwitchWorldTimeSec = m_runtime.worldTimeSec;
				m_runtime.lastError = "Bound weapon spells cannot be slotted";
			}
			PushHUDSnapshot();
			return false;
		}

		const std::string formKey = BuildFormKey(spell);
		if (formKey.empty()) {
			Notify("SpellBinding: failed to resolve spell form key");
			return false;
		}
		if (IsBlacklisted(formKey)) {
			Notify("SpellBinding: spell is blacklisted");
			return false;
		}

		BindingKey bindingKey{};
		AttackSlot slot = AttackSlot::kLight;
		{
			std::scoped_lock lock(m_lock);
			RefreshEquippedKeysLocked(player);
			const auto keyOpt = ResolveCurrentBindingKeyLocked(player);
			if (!keyOpt.has_value()) {
				Notify("SpellBinding: no equipped weapon/unarmed key available");
				return false;
			}
			bindingKey = *keyOpt;
			slot = m_config.currentBindSlotMode;
			auto& profile = EnsureProfileLocked(bindingKey);
			auto* slotBinding = GetSlotBinding(profile, slot);
			if (!slotBinding) {
				return false;
			}
			*slotBinding = SlotBinding{
				formKey,
				spell->GetName() ? spell->GetName() : "",
				spell->CalculateMagickaCost(player),
				static_cast<std::uint32_t>(spell->GetSpellType()),
				1.5f,
				0.0f,
				1u,
				true
			};
			m_runtime.lastCycleSwitchWorldTimeSec = m_runtime.worldTimeSec;
			m_runtime.cycleHudIsError = false;
			m_runtime.cycleHudErrorText.clear();
			m_runtime.lastError.clear();
		}

		Notify(std::format("SpellBinding: bound '{}' to {} ({})", spell->GetName(), bindingKey.displayName, AttackSlotLabel(slot)));
		PushUISnapshot();
		return true;
	}

	void Manager::CycleBindSlotMode()
	{
		auto config = GetConfig();
		const auto next = (static_cast<std::uint32_t>(config.currentBindSlotMode) + 1u) % 3u;
		config.currentBindSlotMode = ParseAttackSlot(next);
		SetConfig(config, true);
	{
		std::scoped_lock lock(m_lock);
		m_runtime.lastCycleSwitchWorldTimeSec = m_runtime.worldTimeSec;
		m_runtime.cycleHudIsError = false;
		m_runtime.cycleHudErrorText.clear();
	}
	}

	bool Manager::BindSpellForSlotFromJson(const std::string& a_payload)
	{
		try {
			const auto parsed = json::parse(a_payload);
			BindingKey key{};
			const auto keyJson = parsed["key"];
			key.pluginName = keyJson.value("pluginName", "");
			key.localFormID = keyJson.value("localFormID", 0u);
			key.uniqueID = static_cast<std::uint16_t>(keyJson.value("uniqueID", 0u));
			key.handSlot = static_cast<HandSlot>(keyJson.value("handSlot", 0u));
			key.isUnarmed = keyJson.value("isUnarmed", false);
			key.displayName = keyJson.value("displayName", "");

			const auto slot = ParseAttackSlot(parsed.value("slot", 0u));
			const std::string spellFormKey = parsed.value("spellFormKey", "");
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* spell = ResolveSpell(spellFormKey);
			if (!player || !spell || !IsSupportedSpell(spell)) {
				return false;
			}
			if (IsBoundWeaponSpell(spell)) {
				{
					std::scoped_lock lock(m_lock);
					m_runtime.cycleHudIsError = true;
					m_runtime.cycleHudErrorText = "Blocked: Bound spell";
					m_runtime.lastCycleSwitchWorldTimeSec = m_runtime.worldTimeSec;
					m_runtime.lastError = "Bound weapon spells cannot be slotted";
				}
				PushHUDSnapshot();
				return false;
			}
			if (IsBlacklisted(spellFormKey)) {
				Notify("SpellBinding: spell is blacklisted");
				return false;
			}

			{
				std::scoped_lock lock(m_lock);
				auto& profile = EnsureProfileLocked(key);
				auto* slotBinding = GetSlotBinding(profile, slot);
				if (!slotBinding) {
					return false;
				}
				*slotBinding = SlotBinding{
					spellFormKey,
					spell->GetName() ? spell->GetName() : "",
					spell->CalculateMagickaCost(player),
					static_cast<std::uint32_t>(spell->GetSpellType()),
					1.5f,
					0.0f,
					1u,
					true
				};
				m_runtime.lastCycleSwitchWorldTimeSec = m_runtime.worldTimeSec;
				m_runtime.cycleHudIsError = false;
				m_runtime.cycleHudErrorText.clear();
			}
			PushUISnapshot();
			PushHUDSnapshot();
			return true;
		} catch (...) {
			return false;
		}
	}

	bool Manager::UnbindSlotFromJson(const std::string& a_payload)
	{
		try {
			const auto parsed = json::parse(a_payload);
			BindingKey key{};
			const auto keyJson = parsed["key"];
			key.pluginName = keyJson.value("pluginName", "");
			key.localFormID = keyJson.value("localFormID", 0u);
			key.uniqueID = static_cast<std::uint16_t>(keyJson.value("uniqueID", 0u));
			key.handSlot = static_cast<HandSlot>(keyJson.value("handSlot", 0u));
			key.isUnarmed = keyJson.value("isUnarmed", false);
			key.displayName = keyJson.value("displayName", "");
			const auto slot = ParseAttackSlot(parsed.value("slot", 0u));

			{
				std::scoped_lock lock(m_lock);
				auto it = m_bindings.find(key);
				if (it == m_bindings.end()) {
					return false;
				}
				auto* slotBinding = GetSlotBinding(it->second, slot);
				if (!slotBinding) {
					return false;
				}
				*slotBinding = {};
			}
			PushUISnapshot();
			return true;
		} catch (...) {
			return false;
		}
	}

	bool Manager::UnbindWeaponFromSerializedKey(const std::string& a_key)
	{
		json payload;
		try {
			payload = json::parse(a_key);
		} catch (...) {
			return false;
		}

		BindingKey key{};
		key.pluginName = payload.value("pluginName", "");
		key.localFormID = payload.value("localFormID", 0u);
		key.uniqueID = static_cast<std::uint16_t>(payload.value("uniqueID", 0u));
		key.handSlot = static_cast<HandSlot>(payload.value("handSlot", 0u));
		key.isUnarmed = payload.value("isUnarmed", false);
		key.displayName = payload.value("displayName", "");

		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			removed = m_bindings.erase(key) > 0;
			m_runtime.weaponCooldownReadyAt.erase(key);
		}

		if (removed) {
			Notify(std::format("SpellBinding: unbound {}", key.displayName));
			PushUISnapshot();
			PushHUDSnapshot();
		}
		return removed;
	}

	void Manager::EnterHudDragMode()
	{
		std::scoped_lock lock(m_lock);
		m_runtime.hudDragModeActive = true;
	}

	void Manager::SaveHudPositionFromJson(const std::string& a_payload)
	{
		try {
			const auto parsed = json::parse(a_payload);
			auto cfg = GetConfig();
			const auto target = parsed.value("target", std::string{ "donut" });
			const auto x = parsed.value("x", cfg.hudPosX);
			const auto y = parsed.value("y", cfg.hudPosY);
			if (target == "cycle") {
				cfg.hudCyclePosX = x;
				cfg.hudCyclePosY = y;
			} else if (target == "chain") {
				cfg.hudChainPosX = x;
				cfg.hudChainPosY = y;
			} else {
				cfg.hudPosX = x;
				cfg.hudPosY = y;
			}
			SetConfig(cfg, true);
			const bool commit = parsed.value("commit", false);
			if (commit) {
				std::scoped_lock lock(m_lock);
				m_runtime.hudDragModeActive = false;
			}
			PushHUDSnapshot();
		} catch (...) {}
	}

	void Manager::NotifyChainSwitch(std::int32_t a_chainIndex1Based, std::string_view a_chainName)
	{
		std::scoped_lock lock(m_lock);
		m_runtime.lastChainSwitchWorldTimeSec = m_runtime.worldTimeSec;
		const auto chainName = std::string(a_chainName);
		if (!chainName.empty()) {
			m_runtime.lastChainHudText = chainName;
		} else {
			const auto safeIndex = std::max(1, a_chainIndex1Based);
			m_runtime.lastChainHudText = std::format("Chain {}", safeIndex);
		}
	}

	void Manager::NotifyChainRecordingState(std::int32_t a_chainIndex1Based, std::string_view a_chainName, bool a_recording)
	{
		std::scoped_lock lock(m_lock);
		const auto chainName = std::string(a_chainName);
		if (!chainName.empty()) {
			m_runtime.lastChainHudText = chainName;
		} else {
			const auto safeIndex = std::max(1, a_chainIndex1Based);
			m_runtime.lastChainHudText = std::format("Chain {}", safeIndex);
		}
		m_runtime.chainRecordingActive = a_recording;
		if (!a_recording) {
			m_runtime.lastChainSwitchWorldTimeSec = m_runtime.worldTimeSec;
		}
	}

	void Manager::NotifyChainPlayingState(std::int32_t a_chainIndex1Based, std::string_view a_chainName, bool a_playing)
	{
		std::scoped_lock lock(m_lock);
		const auto chainName = std::string(a_chainName);
		if (!chainName.empty()) {
			m_runtime.lastChainHudText = chainName;
		} else {
			const auto safeIndex = std::max(1, a_chainIndex1Based);
			m_runtime.lastChainHudText = std::format("Chain {}", safeIndex);
		}
		m_runtime.chainPlayingActive = a_playing;
		m_runtime.lastChainPlayingWorldTimeSec = m_runtime.worldTimeSec;
		if (!a_playing) {
			m_runtime.lastChainSwitchWorldTimeSec = m_runtime.worldTimeSec;
		}
	}

	std::string Manager::GetUIHotkeyName() const
	{
		std::scoped_lock lock(m_lock);
		return GetKeyboardKeyName(m_config.uiToggleKey);
	}

	std::string Manager::GetBindHotkeyName() const
	{
		std::scoped_lock lock(m_lock);
		return GetKeyboardKeyName(m_config.bindKey);
	}

	void Manager::LoadConfig()
	{
		std::ifstream file(std::string(kConfigPath), std::ios::in);
		if (!file.is_open()) {
			SaveConfig();
			return;
		}

		json root;
		try {
			file >> root;
		} catch (...) {
			return;
		}
		const auto cfgNode = root.contains("spellBinding") && root["spellBinding"].is_object() ? root["spellBinding"] : root;

		m_config.version = cfgNode.value("version", m_config.version);
		m_config.enabled = cfgNode.value("enabled", m_config.enabled);
		m_config.uiToggleKey = cfgNode.value("uiToggleKey", m_config.uiToggleKey);
		m_config.bindKey = cfgNode.value("bindKey", m_config.bindKey);
		m_config.showHudNotifications = cfgNode.value("showHudNotifications", m_config.showHudNotifications);
		m_config.cycleSlotModifierKey = cfgNode.value("cycleSlotModifierKey", m_config.cycleSlotModifierKey);
		m_config.enableSoundCues = cfgNode.value("enableSoundCues", m_config.enableSoundCues);
		m_config.soundCueVolume = cfgNode.value("soundCueVolume", m_config.soundCueVolume);
		m_config.hudDonutEnabled = cfgNode.value("hudDonutEnabled", m_config.hudDonutEnabled);
		m_config.hudDonutOnlyUnsheathed = cfgNode.value("hudDonutOnlyUnsheathed", m_config.hudDonutOnlyUnsheathed);
		m_config.hudShowCooldownSeconds = cfgNode.value("hudShowCooldownSeconds", m_config.hudShowCooldownSeconds);
		m_config.hudAnchor = cfgNode.value("hudAnchor", m_config.hudAnchor);
		m_config.hudPosX = cfgNode.value("hudPosX", m_config.hudPosX);
		m_config.hudPosY = cfgNode.value("hudPosY", m_config.hudPosY);
		m_config.hudDonutSize = cfgNode.value("hudDonutSize", m_config.hudDonutSize);
		m_config.hudCyclePosX = cfgNode.value("hudCyclePosX", m_config.hudCyclePosX);
		m_config.hudCyclePosY = cfgNode.value("hudCyclePosY", m_config.hudCyclePosY);
		m_config.hudCycleSize = cfgNode.value("hudCycleSize", m_config.hudCycleSize);
		m_config.hudChainPosX = cfgNode.value("hudChainPosX", m_config.hudChainPosX);
		m_config.hudChainPosY = cfgNode.value("hudChainPosY", m_config.hudChainPosY);
		m_config.hudChainSize = cfgNode.value("hudChainSize", m_config.hudChainSize);
		m_config.hudChainAlwaysShowInCombat = cfgNode.value("hudChainAlwaysShowInCombat", m_config.hudChainAlwaysShowInCombat);
		m_config.blacklistEnabled = cfgNode.value("blacklistEnabled", m_config.blacklistEnabled);
		m_config.blacklistedSpellKeys = cfgNode.value("blacklistedSpellKeys", m_config.blacklistedSpellKeys);
		m_config.debugMode = cfgNode.value("debugMode", m_config.debugMode);
		m_config.currentBindSlotMode = ParseAttackSlot(cfgNode.value("currentBindSlotMode", static_cast<std::uint32_t>(m_config.currentBindSlotMode)));

		if (root.contains("ui") && root["ui"].is_object()) {
			const auto& uiNode = root["ui"];
			if (uiNode.contains("spellBindingWindow") && uiNode["spellBindingWindow"].is_object()) {
				const auto& windowNode = uiNode["spellBindingWindow"];
				m_uiWindow.x = windowNode.value("x", m_uiWindow.x);
				m_uiWindow.y = windowNode.value("y", m_uiWindow.y);
				m_uiWindow.width = windowNode.value("width", m_uiWindow.width);
				m_uiWindow.height = windowNode.value("height", m_uiWindow.height);
				m_uiWindow.isFullscreen = windowNode.value("isFullscreen", m_uiWindow.isFullscreen);
				m_uiWindow.hasSaved = true;
			}
		}
		ClampWindowConfig(m_uiWindow);
	}

	void Manager::SaveConfig() const
	{
		SpellBindingConfig cfgCopy{};
		UIWindowConfig windowCopy{};
		{
			std::scoped_lock lock(m_lock);
			cfgCopy = m_config;
			windowCopy = m_uiWindow;
		}

		const json node = {
			{ "version", cfgCopy.version },
			{ "enabled", cfgCopy.enabled },
			{ "uiToggleKey", cfgCopy.uiToggleKey },
			{ "bindKey", cfgCopy.bindKey },
			{ "showHudNotifications", cfgCopy.showHudNotifications },
			{ "cycleSlotModifierKey", cfgCopy.cycleSlotModifierKey },
			{ "enableSoundCues", cfgCopy.enableSoundCues },
			{ "soundCueVolume", cfgCopy.soundCueVolume },
			{ "hudDonutEnabled", cfgCopy.hudDonutEnabled },
			{ "hudDonutOnlyUnsheathed", cfgCopy.hudDonutOnlyUnsheathed },
			{ "hudShowCooldownSeconds", cfgCopy.hudShowCooldownSeconds },
			{ "hudAnchor", cfgCopy.hudAnchor },
			{ "hudPosX", cfgCopy.hudPosX },
			{ "hudPosY", cfgCopy.hudPosY },
			{ "hudDonutSize", cfgCopy.hudDonutSize },
			{ "hudCyclePosX", cfgCopy.hudCyclePosX },
			{ "hudCyclePosY", cfgCopy.hudCyclePosY },
			{ "hudCycleSize", cfgCopy.hudCycleSize },
			{ "hudChainPosX", cfgCopy.hudChainPosX },
			{ "hudChainPosY", cfgCopy.hudChainPosY },
			{ "hudChainSize", cfgCopy.hudChainSize },
			{ "hudChainAlwaysShowInCombat", cfgCopy.hudChainAlwaysShowInCombat },
			{ "blacklistEnabled", cfgCopy.blacklistEnabled },
			{ "blacklistedSpellKeys", cfgCopy.blacklistedSpellKeys },
			{ "debugMode", cfgCopy.debugMode },
			{ "currentBindSlotMode", static_cast<std::uint32_t>(cfgCopy.currentBindSlotMode) }
		};
		const json windowNode = {
			{ "x", windowCopy.x },
			{ "y", windowCopy.y },
			{ "width", windowCopy.width },
			{ "height", windowCopy.height },
			{ "isFullscreen", windowCopy.isFullscreen }
		};

		try {
			const std::filesystem::path path{ std::string(kConfigPath) };
			std::filesystem::create_directories(path.parent_path());
			json root = json::object();
			if (std::ifstream in(path, std::ios::in); in.is_open()) {
				try {
					in >> root;
				} catch (...) {
					root = json::object();
				}
			}
			root["spellBinding"] = node;
			if (!root.contains("ui") || !root["ui"].is_object()) {
				root["ui"] = json::object();
			}
			root["ui"]["spellBindingWindow"] = windowNode;
			std::ofstream out(path, std::ios::out | std::ios::trunc);
			out << root.dump(2);
		} catch (...) {}
	}

	void Manager::ClampConfig(SpellBindingConfig& a_config) const
	{
		a_config.version = kConfigVersion;
		a_config.soundCueVolume = std::clamp(a_config.soundCueVolume, 0.0f, 1.0f);
		a_config.hudDonutSize = std::clamp(a_config.hudDonutSize, 48.0f, 220.0f);
		a_config.hudCycleSize = std::clamp(a_config.hudCycleSize, 36.0f, 200.0f);
		a_config.hudChainSize = std::clamp(a_config.hudChainSize, 36.0f, 200.0f);
		if (a_config.hudAnchor.empty()) {
			a_config.hudAnchor = "top-right";
		}
	}

	void Manager::ClampWindowConfig(UIWindowConfig& a_config) const
	{
		a_config.width = std::clamp(a_config.width, kMinWindowWidth, 4096.0f);
		a_config.height = std::clamp(a_config.height, kMinWindowHeight, 4096.0f);
		if (!std::isfinite(a_config.x)) {
			a_config.x = 0.0f;
		}
		if (!std::isfinite(a_config.y)) {
			a_config.y = 0.0f;
		}
	}
	void Manager::RefreshEquippedKeysLocked(RE::PlayerCharacter* a_player)
	{
		const auto* rightEntry = a_player->GetEquippedEntryData(false);
		const auto* leftEntry = a_player->GetEquippedEntryData(true);

		m_runtime.rightWeapon = BuildWeaponKeyFromEntry(rightEntry, HandSlot::kRight);
		m_runtime.leftWeapon = BuildWeaponKeyFromEntry(leftEntry, HandSlot::kLeft);
		m_runtime.unarmedKey = BuildUnarmedKey();
	}

	std::optional<BindingKey> Manager::ResolveCurrentBindingKeyLocked(RE::PlayerCharacter* a_player) const
	{
		(void)a_player;
		if (m_runtime.rightWeapon.has_value()) {
			return m_runtime.rightWeapon;
		}
		if (m_runtime.leftWeapon.has_value()) {
			return m_runtime.leftWeapon;
		}
		return m_runtime.unarmedKey;
	}

	std::optional<BindingKey> Manager::BuildWeaponKeyFromEntry(const RE::InventoryEntryData* a_entry, HandSlot a_slot)
	{
		if (!a_entry || !a_entry->object) {
			return std::nullopt;
		}

		auto* weapon = a_entry->object->As<RE::TESObjectWEAP>();
		if (!weapon) {
			return std::nullopt;
		}

		auto* file = weapon->GetFile(0);
		if (!file) {
			return std::nullopt;
		}

		std::uint16_t uniqueID = 0;
		if (a_entry->extraLists) {
			for (const auto& xList : *a_entry->extraLists) {
				if (!xList) {
					continue;
				}
				const auto* id = xList->GetByType<RE::ExtraUniqueID>();
				if (id) {
					uniqueID = static_cast<std::uint16_t>(id->uniqueID);
					break;
				}
			}
		}

		BindingKey key{};
		key.pluginName = file->GetFilename();
		key.localFormID = weapon->GetLocalFormID();
		key.uniqueID = uniqueID;
		key.handSlot = a_slot;
		key.isUnarmed = false;
		key.displayName = weapon->GetName() ? weapon->GetName() : "";
		if (key.displayName.empty()) {
			key.displayName = std::format("{}|0x{:08X}", key.pluginName, key.localFormID);
		}
		return key;
	}

	BindingKey Manager::BuildUnarmedKey()
	{
		BindingKey key{};
		key.handSlot = HandSlot::kUnarmed;
		key.isUnarmed = true;
		key.displayName = "Unarmed";
		return key;
	}

	std::string Manager::BuildFormKey(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return {};
		}
		const auto* file = a_form->GetFile(0);
		if (!file) {
			return {};
		}
		const auto filename = file->GetFilename();
		if (filename.empty()) {
			return {};
		}
		return std::format("{}|0x{:08X}", filename, a_form->GetLocalFormID());
	}

	bool Manager::ParseFormKey(std::string_view a_formKey, std::string& a_pluginName, RE::FormID& a_localFormID)
	{
		const auto split = a_formKey.find('|');
		if (split == std::string_view::npos) {
			return false;
		}

		a_pluginName = std::string(a_formKey.substr(0, split));
		auto text = std::string(a_formKey.substr(split + 1));
		if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) {
			text = text.substr(2);
		}

		std::uint32_t parsed = 0;
		const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed, 16);
		if (ec != std::errc{} || ptr != text.data() + text.size()) {
			return false;
		}

		a_localFormID = parsed;
		return true;
	}

	RE::SpellItem* Manager::ResolveSpell(std::string_view a_formKey)
	{
		std::string pluginName{};
		RE::FormID localFormID = 0;
		if (!ParseFormKey(a_formKey, pluginName, localFormID)) {
			return nullptr;
		}

		auto* data = RE::TESDataHandler::GetSingleton();
		if (!data) {
			return nullptr;
		}

		auto* form = data->LookupForm(localFormID, pluginName);
		return form ? form->As<RE::SpellItem>() : nullptr;
	}

	std::optional<RE::FormID> Manager::ResolveSelectedSpellFormIDFromMagicMenu()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			return std::nullopt;
		}

		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return std::nullopt;
		}

		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return std::nullopt;
		}

		if (auto* selected = runtime.itemList->GetSelectedItem(); selected && selected->data.baseForm) {
			auto* spell = selected->data.baseForm->As<RE::SpellItem>();
			if (spell) {
				return spell->GetFormID();
			}
		}
		return std::nullopt;
	}

	AttackSlot Manager::ResolveAttackSlot(std::string_view a_tag, std::string_view a_payload, RE::PlayerCharacter* a_player) const
	{
		auto contains = [](std::string_view text, std::string_view token) {
			return text.find(token) != std::string_view::npos;
		};
		if (IsPowerAttackActive(a_player, a_tag, a_payload)) {
			return AttackSlot::kPower;
		}
		if (contains(a_tag, "bash") || contains(a_payload, "bash")) {
			return AttackSlot::kBash;
		}
		return AttackSlot::kLight;
	}

bool Manager::IsPowerAttackActive(RE::PlayerCharacter* a_player, std::string_view a_tag, std::string_view a_payload) const
{
		(void)a_tag;
		(void)a_payload;

		if (!a_player) {
			return false;
		}

		if (auto* high = a_player->GetHighProcess(); high && high->attackData) {
			const auto flags = high->attackData->data.flags;
			if (flags.all(RE::AttackData::AttackFlag::kPowerAttack) || flags.all(RE::AttackData::AttackFlag::kChargeAttack)) {
				return true;
			}
		}
		return false;
}

	void Manager::StopActiveConcentration(RE::PlayerCharacter* a_player, bool a_interruptCast)
	{
		if (m_runtime.activeConcentrationSpellKey.empty()) {
			return;
		}

		if (a_interruptCast && a_player) {
			if (auto* caster = a_player->GetMagicCaster(m_runtime.concentrationSource)) {
				caster->InterruptCast(false);
			}
		}

		m_runtime.activeConcentrationSpellKey.clear();
		m_runtime.concentrationRemainingSec = 0.0f;
		m_runtime.concentrationTickTimer = 0.0f;
		m_runtime.concentrationCastOnSelf = true;
		m_runtime.concentrationTargetHandle = RE::ObjectRefHandle{};
	}

	void Manager::UpdateActiveConcentration(RE::PlayerCharacter* a_player, float a_deltaTime)
	{
		if (m_runtime.activeConcentrationSpellKey.empty()) {
			return;
		}
		const bool debugMode = m_config.debugMode;

		if (!a_player ||
		    m_runtime.menuBlocked ||
		    IsMountedBlocked(a_player) ||
		    IsKillmoveBlocked(a_player) ||
		    IsDialogueBlocked()) {
			StopActiveConcentration(a_player, true);
			return;
		}

		m_runtime.concentrationRemainingSec = std::max(0.0f, m_runtime.concentrationRemainingSec - a_deltaTime);
		m_runtime.concentrationTickTimer = std::max(0.0f, m_runtime.concentrationTickTimer - a_deltaTime);
		if (m_runtime.concentrationRemainingSec <= 0.0f) {
			StopActiveConcentration(a_player, true);
			return;
		}
		if (m_runtime.concentrationTickTimer > 0.0f) {
			return;
		}

		auto* spell = ResolveSpell(m_runtime.activeConcentrationSpellKey);
		if (!spell || !a_player->HasSpell(spell) || spell->GetCastingType() != RE::MagicSystem::CastingType::kConcentration) {
			StopActiveConcentration(a_player, true);
			return;
		}

		auto* caster = a_player->GetMagicCaster(m_runtime.concentrationSource);
		if (!caster) {
			StopActiveConcentration(a_player, false);
			return;
		}
		auto* avOwner = a_player->AsActorValueOwner();
		if (!avOwner) {
			StopActiveConcentration(a_player, false);
			return;
		}

		auto combatTarget = a_player->GetActorRuntimeData().currentCombatTarget.get();
		RE::TESObjectREFR* target = m_runtime.concentrationCastOnSelf ? static_cast<RE::TESObjectREFR*>(a_player) : combatTarget.get();

		const float magickaPerSec = std::max(0.0f, spell->CalculateMagickaCost(a_player));
		const float tickCost = magickaPerSec * 0.30f;
		const float magickaBefore = avOwner->GetActorValue(RE::ActorValue::kMagicka);
		if (debugMode) {
			LOG_INFO("SpellBinding Debug: magicka check [concentration tick] spell='{}' current={:.2f} tickCost={:.2f} mps={:.2f}",
				spell->GetName() ? spell->GetName() : "<unknown>",
				magickaBefore,
				tickCost,
				magickaPerSec);
		}
		if (tickCost > 0.0f && magickaBefore + 0.001f < tickCost) {
			if (debugMode) {
				LOG_INFO("SpellBinding Debug: magicka blocked [concentration tick] current={:.2f} required={:.2f}", magickaBefore, tickCost);
			}
			StopActiveConcentration(a_player, true);
			return;
		}

		caster->currentSpellCost = magickaPerSec;
		caster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, m_runtime.concentrationCastOnSelf ? nullptr : a_player);
		if (tickCost > 0.0f) {
			avOwner->DamageActorValue(RE::ActorValue::kMagicka, tickCost);
			m_runtime.lastMagickaCost = tickCost;
			if (debugMode) {
				const float magickaAfter = avOwner->GetActorValue(RE::ActorValue::kMagicka);
				LOG_INFO("SpellBinding Debug: magicka spent [concentration tick] amount={:.2f} before={:.2f} after={:.2f}",
					tickCost,
					magickaBefore,
					magickaAfter);
			}
		} else if (debugMode) {
			LOG_INFO("SpellBinding Debug: magicka spent [concentration tick] amount=0.00 (zero-cost tick)");
		}
		m_runtime.concentrationTickTimer = 0.30f;
	}

	bool Manager::TryTriggerBoundSpellLocked(RE::PlayerCharacter* a_player, const BindingKey& a_key, AttackSlot a_slot, bool a_isPowerAttack)
	{
		const auto profileIt = m_bindings.find(a_key);
		if (profileIt == m_bindings.end()) {
			return false;
		}
		auto* slotBinding = GetSlotBinding(profileIt->second, a_slot);
		if (!slotBinding || !slotBinding->enabled || slotBinding->spellFormKey.empty()) {
			return false;
		}
		if (profileIt->second.onlyInCombat && !a_player->IsInCombat()) {
			SetLastErrorLocked("Only in combat");
			PlayCueBlocked();
			return false;
		}
		if (IsBlacklisted(slotBinding->spellFormKey)) {
			SetLastErrorLocked("Spell is blacklisted");
			PlayCueBlocked();
			return false;
		}

		auto* spell = ResolveSpell(slotBinding->spellFormKey);
		if (!spell || !a_player->HasSpell(spell)) {
			SetLastErrorLocked("Missing or invalid bound spell");
			PlayCueBlocked();
			return false;
		}
		const auto spellType = spell->GetSpellType();
		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		const bool debugMode = m_config.debugMode;
		if (!m_runtime.activeConcentrationSpellKey.empty()) {
			StopActiveConcentration(a_player, true);
		}
		if (spellType == RE::MagicSystem::SpellType::kPower || spellType == RE::MagicSystem::SpellType::kVoicePower) {
			// SpellBinding no longer enforces power/shout cooldown gating.
		}

		auto* avOwner = a_player->AsActorValueOwner();
		if (!avOwner) {
			SetLastErrorLocked("No actor value owner");
			PlayCueBlocked();
			return false;
		}

		const float magickaCost = std::max(0.0f, spell->CalculateMagickaCost(a_player));
		const auto castCount = isConcentration ? 1u : 1u;
		const float concentrationTickCost = magickaCost * 0.30f;
		const float requiredMagicka = isConcentration ? concentrationTickCost : (magickaCost * static_cast<float>(castCount));
		const float magickaBefore = avOwner->GetActorValue(RE::ActorValue::kMagicka);
		if (debugMode) {
			LOG_INFO("SpellBinding Debug: magicka check [cast] spell='{}' slot={} concentration={} type={} current={:.2f} baseCost={:.2f} required={:.2f}",
				spell->GetName() ? spell->GetName() : "<unknown>",
				AttackSlotLabel(a_slot),
				isConcentration,
				static_cast<std::uint32_t>(spellType),
				magickaBefore,
				magickaCost,
				requiredMagicka);
		}
		if (requiredMagicka > 0.0f && magickaBefore + 0.001f < requiredMagicka) {
			if (debugMode) {
				LOG_INFO("SpellBinding Debug: magicka blocked [cast] current={:.2f} required={:.2f}", magickaBefore, requiredMagicka);
			}
			SetLastErrorLocked("Not enough magicka");
			PlayCueBlocked();
			return false;
		}

		auto source = RE::MagicSystem::CastingSource::kInstant;
		auto* caster = a_player->GetMagicCaster(source);
		if (!caster) {
			SetLastErrorLocked("No valid magic caster");
			PlayCueBlocked();
			return false;
		}

		const bool castOnSelf = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
		auto combatTarget = a_player->GetActorRuntimeData().currentCombatTarget.get();
		RE::TESObjectREFR* target = castOnSelf ? static_cast<RE::TESObjectREFR*>(a_player) : combatTarget.get();
		for (std::uint32_t i = 0; i < castCount; ++i) {
			if (isConcentration) {
				caster->currentSpellCost = magickaCost;
			}
			caster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, castOnSelf ? nullptr : a_player);
			if (spell->GetDelivery() == RE::MagicSystem::Delivery::kTargetLocation && !isConcentration && a_player->IsCasting(spell)) {
				a_player->InterruptCast(false);
				SetLastErrorLocked("Target location cast failed");
				PlayCueBlocked();
				return false;
			}
		}

		float spentMagicka = 0.0f;
		if (isConcentration) {
			if (concentrationTickCost > 0.0f) {
				avOwner->DamageActorValue(RE::ActorValue::kMagicka, concentrationTickCost);
				spentMagicka = concentrationTickCost;
			}
		} else {
			const float totalCost = magickaCost * static_cast<float>(castCount);
			if (totalCost > 0.0f) {
				avOwner->DamageActorValue(RE::ActorValue::kMagicka, totalCost);
				spentMagicka = totalCost;
			}
		}
		if (debugMode) {
			const float magickaAfter = avOwner->GetActorValue(RE::ActorValue::kMagicka);
			LOG_INFO("SpellBinding Debug: magicka spent [cast] spell='{}' amount={:.2f} before={:.2f} after={:.2f} concentration={} casts={}",
				spell->GetName() ? spell->GetName() : "<unknown>",
				spentMagicka,
				magickaBefore,
				magickaAfter,
				isConcentration,
				castCount);
		}

		slotBinding->spellType = static_cast<std::uint32_t>(spellType);
		if (isConcentration) {
			m_runtime.activeConcentrationSpellKey = slotBinding->spellFormKey;
			m_runtime.concentrationCastOnSelf = castOnSelf;
			m_runtime.concentrationTargetHandle = (!castOnSelf && target) ? target->CreateRefHandle() : RE::ObjectRefHandle{};
			m_runtime.concentrationSource = source;
			m_runtime.concentrationRemainingSec = std::max(0.30f, slotBinding->castHoldSec);
			m_runtime.concentrationTickTimer = 0.30f;
		}

		m_runtime.lastTriggerWeapon = a_key.displayName;
		m_runtime.lastTriggerSpell = slotBinding->displayName;
		m_runtime.lastMagickaCost = spentMagicka;
		m_runtime.lastWasPowerAttack = a_isPowerAttack;
		m_runtime.lastError.clear();
		PlayCueSuccess();
		return true;
	}
	WeaponBindingProfile& Manager::EnsureProfileLocked(const BindingKey& a_key)
	{
		auto [it, _] = m_bindings.emplace(a_key, WeaponBindingProfile{});
		it->second.triggerCooldownSec = std::clamp(it->second.triggerCooldownSec, 0.5f, 5.0f);
		return it->second;
	}

	SlotBinding* Manager::GetSlotBinding(WeaponBindingProfile& a_profile, AttackSlot a_slot)
	{
		switch (a_slot) {
			case AttackSlot::kPower: return &a_profile.power;
			case AttackSlot::kBash: return &a_profile.bash;
			default: return &a_profile.light;
		}
	}

	const SlotBinding* Manager::GetSlotBinding(const WeaponBindingProfile& a_profile, AttackSlot a_slot) const
	{
		switch (a_slot) {
			case AttackSlot::kPower: return &a_profile.power;
			case AttackSlot::kBash: return &a_profile.bash;
			default: return &a_profile.light;
		}
	}

	bool Manager::IsMountedBlocked(RE::PlayerCharacter* a_player) const
	{
		return a_player && a_player->IsOnMount();
	}

	bool Manager::IsKillmoveBlocked(RE::PlayerCharacter* a_player) const
	{
		return a_player && a_player->IsInKillMove();
	}

	bool Manager::IsDialogueBlocked() const
	{
		auto* ui = RE::UI::GetSingleton();
		return ui && ui->IsMenuOpen("Dialogue Menu");
	}

	bool Manager::IsBlacklisted(std::string_view a_spellFormKey) const
	{
		if (!m_config.blacklistEnabled || a_spellFormKey.empty()) {
			return false;
		}
		return std::find(m_config.blacklistedSpellKeys.begin(), m_config.blacklistedSpellKeys.end(), a_spellFormKey) != m_config.blacklistedSpellKeys.end();
	}

	void Manager::PlayCueSuccess() const
	{
		(void)m_config;
	}

	void Manager::PlayCueBlocked() const
	{
		(void)m_config;
	}

bool Manager::IsSupportedSpell(const RE::SpellItem* a_spell)
{
	if (!a_spell) {
		return false;
	}
		const auto type = a_spell->GetSpellType();
		return type == RE::MagicSystem::SpellType::kSpell ||
		       type == RE::MagicSystem::SpellType::kLesserPower ||
	       type == RE::MagicSystem::SpellType::kPower ||
	       type == RE::MagicSystem::SpellType::kVoicePower;
}

bool Manager::IsBoundWeaponSpell(const RE::SpellItem* a_spell)
{
	if (!a_spell) {
		return false;
	}

	const char* rawName = a_spell->GetName();
	if (!rawName) {
		return false;
	}

	std::string name(rawName);
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return name.rfind("bound ", 0) == 0;
}

bool Manager::IsDestructionSpell(const RE::SpellItem* a_spell)
{
	return a_spell && a_spell->GetAssociatedSkill() == RE::ActorValue::kDestruction;
}

bool Manager::SupportsCastCount(const RE::SpellItem* a_spell)
{
	return a_spell &&
	       a_spell->GetCastingType() != RE::MagicSystem::CastingType::kConcentration &&
	       IsDestructionSpell(a_spell);
}

	float Manager::GetGameDaysPassed()
	{
		const auto* calendar = RE::Calendar::GetSingleton();
		return calendar ? calendar->GetDaysPassed() : 0.0f;
	}

	std::string Manager::BuildSpellMetricLocked(const SlotBinding& a_spellData, RE::SpellItem* a_spell, RE::PlayerCharacter* a_player) const
	{
		if (!a_spell || !a_player) {
			return "N/A";
		}
		const auto type = a_spell->GetSpellType();
		if (type == RE::MagicSystem::SpellType::kPower || type == RE::MagicSystem::SpellType::kVoicePower) {
			if (type == RE::MagicSystem::SpellType::kVoicePower) {
				float remaining = 0.0f;
				if (const auto cdIt = m_runtime.powerCooldownUntil.find(a_spellData.spellFormKey); cdIt != m_runtime.powerCooldownUntil.end()) {
					remaining = std::max(0.0f, cdIt->second - m_runtime.worldTimeSec);
				}
				remaining = std::max(remaining, std::max(0.0f, a_player->GetVoiceRecoveryTime()));
				return remaining > 0.01f ? std::format("Cooldown: {:.1f}s", remaining) : "Cooldown: Ready";
			}
			const float nowDays = GetGameDaysPassed();
			if (const auto dayIt = m_runtime.powerCooldownGameDayUntil.find(a_spellData.spellFormKey);
			    dayIt != m_runtime.powerCooldownGameDayUntil.end() && nowDays < dayIt->second) {
				const float hoursRemaining = (dayIt->second - nowDays) * 24.0f;
				return std::format("Cooldown: {:.1f}h", std::max(0.0f, hoursRemaining));
			}
			if (a_player->IsInCastPowerList(a_spell)) {
				return "Cooldown: 1/day used";
			}
			return "Cooldown: Ready";
		}
		return std::format("Cost: {:.1f}", std::max(0.0f, a_spell->CalculateMagickaCost(a_player)));
	}

	std::string Manager::SpellTypeLabel(std::uint32_t a_spellType)
	{
		switch (static_cast<RE::MagicSystem::SpellType>(a_spellType)) {
			case RE::MagicSystem::SpellType::kSpell: return "Spell";
			case RE::MagicSystem::SpellType::kLesserPower: return "Lesser Power";
			case RE::MagicSystem::SpellType::kPower: return "Power";
			case RE::MagicSystem::SpellType::kVoicePower: return "Shout";
			default: return "Magic";
		}
	}

	std::string Manager::AttackSlotLabel(AttackSlot a_slot)
	{
		switch (a_slot) {
			case AttackSlot::kPower: return "Power";
			case AttackSlot::kBash: return "Bash";
			default: return "Light";
		}
	}

	std::string Manager::GetKeyboardKeyName(std::uint32_t a_keyCode)
	{
		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		if (!inputMgr) {
			return std::format("Key {}", a_keyCode);
		}
		RE::BSFixedString name{};
		if (inputMgr->GetButtonNameFromID(RE::INPUT_DEVICE::kKeyboard, static_cast<std::int32_t>(a_keyCode), name)) {
			if (const auto* value = name.c_str(); value && value[0] != '\0') {
				return value;
			}
		}
		return std::format("Key {}", a_keyCode);
	}

	bool Manager::IsLikelyLeftHandAttack(RE::PlayerCharacter* a_player)
	{
		bool value = false;
		static constexpr std::array<std::string_view, 4> candidates{ "IsLeftAttack", "IsLeftAttacking", "AttackLeft", "IsAttackingLeft" };
		for (const auto& candidate : candidates) {
			if (a_player->GetGraphVariableBool(RE::BSFixedString(candidate.data()), value) && value) {
				return true;
			}
		}
		return false;
	}

	std::string Manager::BuildSnapshotJsonLocked() const
	{
		json root{};
		auto* player = RE::PlayerCharacter::GetSingleton();
		root["config"] = {
			{ "enabled", m_config.enabled },
			{ "uiToggleKey", GetKeyboardKeyName(m_config.uiToggleKey) },
			{ "uiToggleKeyCode", m_config.uiToggleKey },
			{ "bindKey", GetKeyboardKeyName(m_config.bindKey) },
			{ "bindKeyCode", m_config.bindKey },
			{ "cycleSlotModifierKey", m_config.cycleSlotModifierKey },
			{ "showHudNotifications", m_config.showHudNotifications },
			{ "enableSoundCues", m_config.enableSoundCues },
			{ "soundCueVolume", m_config.soundCueVolume },
			{ "hudDonutEnabled", m_config.hudDonutEnabled },
			{ "hudDonutOnlyUnsheathed", m_config.hudDonutOnlyUnsheathed },
			{ "hudShowCooldownSeconds", m_config.hudShowCooldownSeconds },
			{ "hudPosX", m_config.hudPosX },
			{ "hudPosY", m_config.hudPosY },
			{ "hudDonutSize", m_config.hudDonutSize },
			{ "hudCyclePosX", m_config.hudCyclePosX },
			{ "hudCyclePosY", m_config.hudCyclePosY },
			{ "hudCycleSize", m_config.hudCycleSize },
			{ "hudChainPosX", m_config.hudChainPosX },
			{ "hudChainPosY", m_config.hudChainPosY },
			{ "hudChainSize", m_config.hudChainSize },
			{ "hudChainAlwaysShowInCombat", m_config.hudChainAlwaysShowInCombat },
			{ "blacklistEnabled", m_config.blacklistEnabled },
			{ "debugMode", m_config.debugMode }
		};
		root["bindMode"] = {
			{ "slot", static_cast<std::uint32_t>(m_config.currentBindSlotMode) },
			{ "label", AttackSlotLabel(m_config.currentBindSlotMode) }
		};

		auto current = m_runtime.rightWeapon.has_value() ? m_runtime.rightWeapon : (m_runtime.leftWeapon.has_value() ? m_runtime.leftWeapon : m_runtime.unarmedKey);
		if (current.has_value()) {
			root["currentWeapon"] = {
				{ "key", {
					{ "pluginName", current->pluginName },
					{ "localFormID", current->localFormID },
					{ "uniqueID", current->uniqueID },
					{ "handSlot", static_cast<std::uint32_t>(current->handSlot) },
					{ "isUnarmed", current->isUnarmed },
					{ "displayName", current->displayName }
				} },
				{ "displayName", current->displayName },
				{ "handSlot", static_cast<std::uint32_t>(current->handSlot) },
				{ "isUnarmed", current->isUnarmed }
			};

			if (const auto it = m_bindings.find(*current); it != m_bindings.end()) {
				const auto slotJson = [this, player](const SlotBinding& slot) {
					json out = {
						{ "enabled", slot.enabled },
						{ "displayName", slot.displayName },
						{ "spellFormKey", slot.spellFormKey },
						{ "spellType", SpellTypeLabel(slot.spellType) },
						{ "metric", "N/A" },
						{ "castHoldSec", slot.castHoldSec },
						{ "castIntervalSec", 0.0f },
						{ "castCount", 1u },
						{ "school", "Other" },
						{ "castingType", "FireAndForget" },
						{ "baseCost", 0.0f },
						{ "costPerSecond", 0.0f },
						{ "netCost", 0.0f },
						{ "supportsCastCount", false }
					};
					if (auto* resolved = ResolveSpell(slot.spellFormKey)) {
						out["metric"] = BuildSpellMetricLocked(slot, resolved, player);
						const auto school = resolved->GetAssociatedSkill();
						if (school == RE::ActorValue::kDestruction) out["school"] = "Destruction";
						else if (school == RE::ActorValue::kConjuration) out["school"] = "Conjuration";
						else if (school == RE::ActorValue::kRestoration) out["school"] = "Restoration";
						else if (school == RE::ActorValue::kIllusion) out["school"] = "Illusion";
						else if (school == RE::ActorValue::kAlteration) out["school"] = "Alteration";
						const bool conc = resolved->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
						const float baseCost = player ? std::max(0.0f, resolved->CalculateMagickaCost(player)) : 0.0f;
						out["castingType"] = conc ? "Concentration" : "FireAndForget";
						out["baseCost"] = baseCost;
						out["costPerSecond"] = conc ? baseCost : 0.0f;
						out["netCost"] = baseCost;
						out["castIntervalSec"] = 0.0f;
						out["castCount"] = 1u;
						out["supportsCastCount"] = false;
					}
					return out;
				};
				root["currentWeapon"]["slots"] = {
					{ "light", slotJson(it->second.light) },
					{ "power", slotJson(it->second.power) },
					{ "bash", slotJson(it->second.bash) }
				};
				root["currentWeapon"]["triggerCooldownSec"] = it->second.triggerCooldownSec;
				root["currentWeapon"]["onlyInCombat"] = it->second.onlyInCombat;
				root["currentWeapon"]["cooldownRemainingSec"] = 0.0f;
			}
		}

		root["bindings"] = json::array();
		for (const auto& [key, profile] : m_bindings) {
			root["bindings"].push_back({
				{ "key", {
					{ "pluginName", key.pluginName },
					{ "localFormID", key.localFormID },
					{ "uniqueID", key.uniqueID },
					{ "handSlot", static_cast<std::uint32_t>(key.handSlot) },
					{ "isUnarmed", key.isUnarmed },
					{ "displayName", key.displayName }
				} },
				{ "summary", {
					{ "light", profile.light.enabled ? profile.light.displayName : "" },
					{ "power", profile.power.enabled ? profile.power.displayName : "" },
					{ "bash", profile.bash.enabled ? profile.bash.displayName : "" }
				} }
			});
		}
		root["knownSpells"] = json::array();
		if (player) {
			struct Visitor final : RE::Actor::ForEachSpellVisitor {
				explicit Visitor(json& a_out, const Manager* a_mgr) : out(a_out), mgr(a_mgr) {}
				RE::BSContainer::ForEachResult Visit(RE::SpellItem* a_spell) override
				{
					if (!a_spell || !Manager::IsSupportedSpell(a_spell)) {
						return RE::BSContainer::ForEachResult::kContinue;
					}
					const auto key = Manager::BuildFormKey(a_spell);
					if (key.empty()) {
						return RE::BSContainer::ForEachResult::kContinue;
					}
					out.push_back({
						{ "spellFormKey", key },
						{ "displayName", a_spell->GetName() ? a_spell->GetName() : "" },
						{ "spellType", Manager::SpellTypeLabel(static_cast<std::uint32_t>(a_spell->GetSpellType())) },
						{ "blacklisted", mgr->IsBlacklisted(key) }
					});
					return RE::BSContainer::ForEachResult::kContinue;
				}
				json& out;
				const Manager* mgr;
			} visitor(root["knownSpells"], this);
			player->VisitSpells(visitor);
		}
		root["blacklist"] = m_config.blacklistedSpellKeys;
		root["status"] = {
			{ "lastTriggerWeapon", m_runtime.lastTriggerWeapon },
			{ "lastTriggerSpell", m_runtime.lastTriggerSpell },
			{ "lastMagickaCost", m_runtime.lastMagickaCost },
			{ "lastWasPowerAttack", m_runtime.lastWasPowerAttack },
			{ "lastError", m_runtime.lastError }
		};
		root["ui"] = {
			{ "spellBindingWindow", {
				{ "x", m_uiWindow.x },
				{ "y", m_uiWindow.y },
				{ "width", m_uiWindow.width },
				{ "height", m_uiWindow.height },
				{ "isFullscreen", m_uiWindow.isFullscreen },
				{ "hasSaved", m_uiWindow.hasSaved }
			} }
		};
		return root.dump();
	}

	std::string Manager::BuildHUDSnapshotJsonLocked() const
	{
		json root{};
		auto* player = RE::PlayerCharacter::GetSingleton();
		std::string cycleSpellName = "None";

		const auto current = m_runtime.rightWeapon.has_value() ? m_runtime.rightWeapon : (m_runtime.leftWeapon.has_value() ? m_runtime.leftWeapon : m_runtime.unarmedKey);
		if (current.has_value()) {
			if (const auto pit = m_bindings.find(*current); pit != m_bindings.end()) {
				if (const auto* slot = GetSlotBinding(pit->second, m_config.currentBindSlotMode); slot && slot->enabled) {
					if (!slot->displayName.empty()) {
						cycleSpellName = slot->displayName;
					}
				}
			}
		}

		const bool isMagicMenuOpen = player && RE::UI::GetSingleton() && RE::UI::GetSingleton()->IsMenuOpen(RE::MagicMenu::MENU_NAME);
		const bool cycleVisible = isMagicMenuOpen || (m_runtime.worldTimeSec - m_runtime.lastCycleSwitchWorldTimeSec) <= 2.0f;
		const bool inCombat = player ? player->IsInCombat() : false;
		const bool chainPopupVisible = (m_runtime.worldTimeSec - m_runtime.lastChainSwitchWorldTimeSec) <= 2.0f;
		const bool chainPlayingRecently = (m_runtime.worldTimeSec - m_runtime.lastChainPlayingWorldTimeSec) <= 2.0f;
		const bool chainVisible = m_runtime.chainRecordingActive || m_runtime.chainPlayingActive || chainPlayingRecently || chainPopupVisible || (m_config.hudChainAlwaysShowInCombat && inCombat);
		std::string chainCurrentSpell{};
		float chainStepProgress = 0.0f;
		const bool hasPlaybackHudState = SMART_CAST::Controller::GetSingleton()->GetPlaybackHudState(chainCurrentSpell, chainStepProgress);
		(void)hasPlaybackHudState;

		root["cycleHud"] = {
			{ "visible", cycleVisible },
			{ "text", AttackSlotLabel(m_config.currentBindSlotMode) },
			{ "subtext", m_runtime.cycleHudIsError ? m_runtime.cycleHudErrorText : cycleSpellName },
			{ "isError", m_runtime.cycleHudIsError },
			{ "x", m_config.hudCyclePosX },
			{ "y", m_config.hudCyclePosY },
			{ "size", m_config.hudCycleSize },
			{ "dragMode", m_runtime.hudDragModeActive }
		};
		root["chainHud"] = {
			{ "visible", chainVisible },
			{ "text", m_runtime.lastChainHudText.empty() ? "Chain 1" : m_runtime.lastChainHudText },
			{ "currentSpell", chainCurrentSpell },
			{ "stepProgress", chainStepProgress },
			{ "x", m_config.hudChainPosX },
			{ "y", m_config.hudChainPosY },
			{ "size", m_config.hudChainSize },
			{ "dragMode", m_runtime.hudDragModeActive },
			{ "alwaysShowInCombat", m_config.hudChainAlwaysShowInCombat },
			{ "isRecording", m_runtime.chainRecordingActive },
			{ "recordingChainText", m_runtime.lastChainHudText.empty() ? "Chain 1" : m_runtime.lastChainHudText },
			{ "isPlaying", m_runtime.chainPlayingActive },
			{ "playingChainText", m_runtime.lastChainHudText.empty() ? "Chain 1" : m_runtime.lastChainHudText }
		};

		// Legacy fields kept inert for compatibility.
		root["visible"] = false;
		root["progress"] = 0.0f;
		root["remainingSec"] = 0.0f;
		root["x"] = m_config.hudPosX;
		root["y"] = m_config.hudPosY;
		root["size"] = m_config.hudDonutSize;
		root["anchor"] = m_config.hudAnchor;
		root["showSeconds"] = false;
		root["dragMode"] = m_runtime.hudDragModeActive;
		return root.dump();
	}

void Manager::Notify(const std::string& a_message) const
{
		(void)a_message;
}

	void Manager::SetLastErrorLocked(std::string a_error)
	{
		m_runtime.lastError = std::move(a_error);
	}
}
