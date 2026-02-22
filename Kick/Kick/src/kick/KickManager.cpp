#include "kick/KickManager.h"

#include "PROXY/Proxies.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <format>
#include <limits>
#include <nlohmann/json.hpp>

namespace MITHRAS::KICK
{
	namespace
	{
		using json = nlohmann::json;

		constexpr float kMinVectorLen = 1e-4f;
		constexpr float kMinRange = 32.0f;
		constexpr float kMaxRange = 700.0f;
		constexpr float kMinForce = 1.0f;
		constexpr float kMaxForce = 4000.0f;

		bool ApplyActorKnockExplosion(RE::Actor* a_target, RE::Actor* a_source, float a_force)
		{
			if (!a_target || a_target->IsDead()) {
				return false;
			}

			auto* source = a_source ? a_source : RE::PlayerCharacter::GetSingleton();
			auto* process = source ? PROXY::Actor::Data(source)->currentProcess : nullptr;
			if (!process) {
				process = PROXY::Actor::Data(a_target)->currentProcess;
			}

			if (!process) {
				return false;
			}

			const auto sourcePos = source ? source->GetPosition() : a_target->GetPosition();
			process->KnockExplosion(a_target, sourcePos, std::max(0.1f, a_force / 160.0f));
			return true;
		}

		bool IsGameplayBlocked()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return true;
			}

			return ui->GameIsPaused() ||
			       ui->IsMenuOpen(RE::MainMenu::MENU_NAME) ||
			       ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME);
		}

		std::optional<RE::TESObjectREFR*> ResolveReference(RE::NiAVObject* a_hitObject)
		{
			if (!a_hitObject) {
				return std::nullopt;
			}

			if (auto* ref = a_hitObject->GetUserData()) {
				return ref;
			}

			if (auto* ref = RE::TESObjectREFR::FindReferenceFor3D(a_hitObject)) {
				return ref;
			}

			return std::nullopt;
		}

		RE::NiPoint3 ResolveRefCenter(RE::TESObjectREFR* a_ref)
		{
			if (!a_ref) {
				return {};
			}

			if (auto* obj3D = a_ref->Get3D()) {
				const auto world = obj3D->worldBound.center;
				return RE::NiPoint3(world.x, world.y, world.z);
			}
			return a_ref->GetPosition();
		}

		bool IsDesiredTargetType(RE::TESObjectREFR* a_ref, bool a_wantActor)
		{
			if (!a_ref) {
				return false;
			}

			const bool isActor = (a_ref->As<RE::Actor>() != nullptr);
			return isActor == a_wantActor;
		}

		json ToJson(const KickConfig& a_config)
		{
			return {
				{ "enabled", a_config.enabled },
				{ "hotkey", a_config.hotkey },
				{ "objectRange", a_config.objectRange },
				{ "objectForce", a_config.objectForce },
				{ "objectUpwardBias", a_config.objectUpwardBias },
				{ "objectCooldownSeconds", a_config.objectCooldownSeconds },
				{ "objectRaySpread", a_config.objectRaySpread },
				{ "npcRange", a_config.npcRange },
				{ "npcForce", a_config.npcForce },
				{ "npcUpwardBias", a_config.npcUpwardBias },
				{ "npcCooldownSeconds", a_config.npcCooldownSeconds },
				{ "npcRaySpread", a_config.npcRaySpread }
			};
		}

		void FromJson(const json& a_json, KickConfig& a_config)
		{
			a_config.enabled = a_json.value("enabled", a_config.enabled);
			a_config.hotkey = a_json.value("hotkey", a_config.hotkey);
			a_config.objectRange = a_json.value("objectRange", a_config.objectRange);
			a_config.objectForce = a_json.value("objectForce", a_config.objectForce);
			a_config.objectUpwardBias = a_json.value("objectUpwardBias", a_config.objectUpwardBias);
			a_config.objectCooldownSeconds = a_json.value("objectCooldownSeconds", a_config.objectCooldownSeconds);
			a_config.objectRaySpread = a_json.value("objectRaySpread", a_config.objectRaySpread);
			a_config.npcRange = a_json.value("npcRange", a_config.npcRange);
			a_config.npcForce = a_json.value("npcForce", a_config.npcForce);
			a_config.npcUpwardBias = a_json.value("npcUpwardBias", a_config.npcUpwardBias);
			a_config.npcCooldownSeconds = a_json.value("npcCooldownSeconds", a_config.npcCooldownSeconds);
			a_config.npcRaySpread = a_json.value("npcRaySpread", a_config.npcRaySpread);
		}
	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_lastObjectKick = {};
			m_lastNPCKick = {};
			m_captureHotkey = false;
		}

		LoadConfigFromJson();
	}

	void Manager::SetConfig(const KickConfig& a_config)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			m_config.objectRange = std::clamp(m_config.objectRange, kMinRange, kMaxRange);
			m_config.objectForce = std::clamp(m_config.objectForce, kMinForce, kMaxForce);
			m_config.objectUpwardBias = std::clamp(m_config.objectUpwardBias, 0.0f, 1.0f);
			m_config.objectCooldownSeconds = std::clamp(m_config.objectCooldownSeconds, 0.0f, 5.0f);
			m_config.objectRaySpread = std::clamp(m_config.objectRaySpread, 0.0f, 1.0f);
			m_config.npcRange = std::clamp(m_config.npcRange, kMinRange, kMaxRange);
			m_config.npcForce = std::clamp(m_config.npcForce, kMinForce, kMaxForce);
			m_config.npcUpwardBias = std::clamp(m_config.npcUpwardBias, 0.0f, 1.0f);
			m_config.npcCooldownSeconds = std::clamp(m_config.npcCooldownSeconds, 0.0f, 5.0f);
			m_config.npcRaySpread = std::clamp(m_config.npcRaySpread, 0.0f, 1.0f);
		}
		SaveConfigToJson();
	}

	KickConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetCaptureHotkey(bool a_capture)
	{
		std::scoped_lock lock(m_lock);
		m_captureHotkey = a_capture;
	}

	bool Manager::IsCapturingHotkey() const
	{
		std::scoped_lock lock(m_lock);
		return m_captureHotkey;
	}

	void Manager::OnHotkeyPressed(std::uint32_t a_keyCode)
	{
		bool updated = false;
		{
			std::scoped_lock lock(m_lock);
			if (!m_captureHotkey) {
				return;
			}
			m_captureHotkey = false;
			m_config.hotkey = a_keyCode;
			updated = true;
			LOG_INFO("Kick: hotkey updated to {}", a_keyCode);
		}

		if (updated) {
			SaveConfigToJson();
		}
	}

	bool Manager::PollCaptureHotkey()
	{
		return false;
	}

	std::string Manager::GetKeyboardKeyName(std::uint32_t a_keyCode)
	{
		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		if (!inputMgr) {
			return "Unknown";
		}

		RE::BSFixedString name{};
		if (inputMgr->GetButtonNameFromID(RE::INPUT_DEVICE::kKeyboard, static_cast<std::int32_t>(a_keyCode), name)) {
			if (const auto* c = name.c_str(); c && c[0] != '\0') {
				return c;
			}
		}
		return std::format("Key {}", a_keyCode);
	}

	void Manager::TryKick()
	{
		KickConfig cfg{};

		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		if (!cfg.enabled || IsGameplayBlocked()) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || player->IsDead()) {
			return;
		}

		const auto objectHit = AcquireKickTarget(player, cfg.objectRange, cfg.objectRaySpread, false);
		const auto npcHit = AcquireKickTarget(player, cfg.npcRange, cfg.npcRaySpread, true);

		std::optional<RaycastResult> hit{};
		if (objectHit.has_value() && objectHit->hasHit) {
			hit = objectHit;
		}
		if (npcHit.has_value() && npcHit->hasHit) {
			if (!hit.has_value() || npcHit->distance < hit->distance) {
				hit = npcHit;
			}
		}

		if (!hit.has_value()) {
			return;
		}

		if (hit->reference) {
			const bool isActor = (hit->reference->As<RE::Actor>() != nullptr);
			const auto now = std::chrono::steady_clock::now();

			{
				std::scoped_lock lock(m_lock);
				if (isActor) {
					if (cfg.npcCooldownSeconds > 0.0f) {
						const auto cooldown = std::chrono::duration<float>(cfg.npcCooldownSeconds);
						if (now - m_lastNPCKick < cooldown) {
							return;
						}
					}
					m_lastNPCKick = now;
				} else {
					if (cfg.objectCooldownSeconds > 0.0f) {
						const auto cooldown = std::chrono::duration<float>(cfg.objectCooldownSeconds);
						if (now - m_lastObjectKick < cooldown) {
							return;
						}
					}
					m_lastObjectKick = now;
				}
			}

			ApplyKickImpulse(hit->reference, player, hit->direction, cfg);
		}
	}

	RE::NiPoint3 Manager::NormalizeOrZero(const RE::NiPoint3& a_vector)
	{
		const float len = a_vector.Length();
		if (len <= kMinVectorLen) {
			return {};
		}
		return a_vector / len;
	}

	RE::NiPoint3 Manager::BuildLookDirection(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_origin)
	{
		if (!a_player) {
			return {};
		}

		const auto lookAt = a_player->GetLookingAtLocation();
		auto direction = NormalizeOrZero(lookAt - a_origin);
		if (direction.SqrLength() > kMinVectorLen) {
			return direction;
		}

		const float yaw = a_player->GetAngleZ();
		const float pitch = a_player->GetAngleX();
		const float cp = std::cos(pitch);
		return NormalizeOrZero({
			std::sin(yaw) * cp,
			std::cos(yaw) * cp,
			-std::sin(pitch)
		});
	}

	std::optional<Manager::RaycastResult> Manager::RaycastFrom(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_origin, const RE::NiPoint3& a_direction, float a_range)
	{
		if (!a_player) {
			return std::nullopt;
		}

		RE::bhkPickData pickData{};
		pickData.rayInput.from = RE::hkVector4(a_origin);
		pickData.rayInput.to = RE::hkVector4(a_origin + a_direction * a_range);

		auto* tes = RE::TES::GetSingleton();
		if (!tes) {
			return std::nullopt;
		}

		RE::NiAVObject* hitObject = tes->Pick(pickData);
		if (!pickData.rayOutput.HasHit()) {
			return std::nullopt;
		}

		const float hitFraction = std::clamp(pickData.rayOutput.hitFraction, 0.0f, 1.0f);
		const auto hitPoint = a_origin + (a_direction * (a_range * hitFraction));
		const auto refOpt = ResolveReference(hitObject);
		if (refOpt.has_value() && *refOpt == a_player) {
			return std::nullopt;
		}

		RaycastResult result{};
		result.reference = refOpt.value_or(nullptr);
		result.origin = a_origin;
		result.hitPoint = hitPoint;
		result.direction = a_direction;
		result.distance = a_range * hitFraction;
		result.hasHit = true;
		return result;
	}

	std::optional<Manager::RaycastResult> Manager::AcquireKickTarget(RE::PlayerCharacter* a_player, float a_range, float a_raySpread, bool a_wantActor)
	{
		if (!a_player) {
			return std::nullopt;
		}

		const auto playerPos = a_player->GetPosition();
		const auto origin = playerPos + RE::NiPoint3(0.0f, 0.0f, a_player->GetHeight() * 0.55f);
		const auto forward = BuildLookDirection(a_player, origin);
		if (forward.SqrLength() <= kMinVectorLen) {
			return std::nullopt;
		}

		// Primary path: use crosshair pick data (most reliable for player-facing target selection).
		if (auto* crosshair = RE::CrosshairPickData::GetSingleton(); crosshair) {
#if defined(EXCLUSIVE_SKYRIM_FLAT)
			const auto targetHandle = crosshair->target;
#else
			const auto targetHandle = crosshair->target[RE::VR_DEVICE::kHeadset];
#endif
			if (targetHandle) {
				if (const auto targetPtr = targetHandle.get()) {
					auto* target = targetPtr.get();
					if (target && target != a_player && !target->IsDisabled() && IsDesiredTargetType(target, a_wantActor)) {
						const auto hitPoint = ResolveRefCenter(target);
						const auto delta = hitPoint - origin;
						const auto dist = delta.Length();
						if (dist > kMinVectorLen && dist <= a_range) {
							RaycastResult result{};
							result.reference = target;
							result.origin = origin;
							result.hitPoint = hitPoint;
							result.direction = NormalizeOrZero(delta);
							result.distance = dist;
							result.hasHit = true;
							return result;
						}
					}
				}
			}
		}

		const RE::NiPoint3 worldUp(0.0f, 0.0f, 1.0f);
		auto right = NormalizeOrZero(forward.Cross(worldUp));
		if (right.SqrLength() <= kMinVectorLen) {
			right = RE::NiPoint3(1.0f, 0.0f, 0.0f);
		}
		const auto up = NormalizeOrZero(right.Cross(forward));

		const std::array<RE::NiPoint2, 9> offsets{
			RE::NiPoint2(0.0f, 0.0f),
			RE::NiPoint2(1.0f, 0.0f),
			RE::NiPoint2(-1.0f, 0.0f),
			RE::NiPoint2(0.0f, 1.0f),
			RE::NiPoint2(0.0f, -1.0f),
			RE::NiPoint2(0.7f, 0.7f),
			RE::NiPoint2(-0.7f, 0.7f),
			RE::NiPoint2(0.7f, -0.7f),
			RE::NiPoint2(-0.7f, -0.7f)
		};

		std::optional<RaycastResult> best{};
		for (const auto& offset : offsets) {
			const auto dir = NormalizeOrZero(forward + right * (offset.x * a_raySpread) + up * (offset.y * a_raySpread));
			if (dir.SqrLength() <= kMinVectorLen) {
				continue;
			}

			const auto hit = RaycastFrom(a_player, origin, dir, a_range);
			if (!hit.has_value()) {
				continue;
			}
			if (!hit->reference || !IsDesiredTargetType(hit->reference, a_wantActor)) {
				continue;
			}

			if (!best.has_value() || hit->distance < best->distance) {
				best = hit;
			}
		}

		if (best.has_value()) {
			return best;
		}

		// Fallback: forward cone scan of references in range.
		if (auto* tes = RE::TES::GetSingleton(); tes) {
			float bestScore = std::numeric_limits<float>::max();
			tes->ForEachReferenceInRange(a_player, a_range, [&](RE::TESObjectREFR* a_ref) {
				if (!a_ref || a_ref == a_player || a_ref->IsDisabled()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				if (!IsDesiredTargetType(a_ref, a_wantActor)) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (auto* actor = a_ref->As<RE::Actor>(); actor && actor->IsDead()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const auto center = ResolveRefCenter(a_ref);
				const auto delta = center - origin;
				const auto dist = delta.Length();
				if (dist <= kMinVectorLen || dist > a_range) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const auto dir = delta / dist;
				const float dot = forward.Dot(dir);
				if (dot < 0.50f) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const float score = dist - (dot * 120.0f);
				if (score < bestScore) {
					bestScore = score;
					RaycastResult fallback{};
					fallback.reference = a_ref;
					fallback.origin = origin;
					fallback.hitPoint = center;
					fallback.direction = dir;
					fallback.distance = dist;
					fallback.hasHit = true;
					best = fallback;
				}

				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

		return best;
	}

	bool Manager::ApplyKickImpulse(RE::TESObjectREFR* a_ref, RE::Actor* a_source, const RE::NiPoint3& a_dir, const KickConfig& a_cfg)
	{
		if (!a_ref || a_ref->IsDisabled()) {
			return false;
		}

		if (auto* actor = a_ref->As<RE::Actor>(); actor && !actor->IsDead()) {
			auto impulseDir = NormalizeOrZero(a_dir + RE::NiPoint3(0.0f, 0.0f, a_cfg.npcUpwardBias));
			if (impulseDir.SqrLength() <= kMinVectorLen) {
				return false;
			}

			auto* source = a_source ? a_source : RE::PlayerCharacter::GetSingleton();
			const bool hadKnock = ApplyActorKnockExplosion(actor, source, a_cfg.npcForce);

			// Extra velocity helps when target is already ragdolled.
			bool hadCurrent = false;
			if (actor->IsInRagdollState()) {
				const RE::hkVector4 impulse(impulseDir.x * a_cfg.npcForce, impulseDir.y * a_cfg.npcForce, impulseDir.z * a_cfg.npcForce, 0.0f);
				hadCurrent = actor->ApplyCurrent(0.2f, impulse);
				actor->PotentiallyFixRagdollState();
			}

			return hadCurrent || hadKnock;
		}

		auto impulseDir = NormalizeOrZero(a_dir + RE::NiPoint3(0.0f, 0.0f, a_cfg.objectUpwardBias));
		if (impulseDir.SqrLength() <= kMinVectorLen) {
			return false;
		}

		// Non-actors/regular objects: use rigid-body impulse.
		auto* root = a_ref->Get3D();
		if (!root) {
			return false;
		}
		root->SetMotionType(RE::hkpMotion::MotionType::kDynamic, true, false, true);

		auto* collision = root->GetCollisionObject();
		if (!collision) {
			return false;
		}

		auto* bhkCollision = collision->AsBhkNiCollisionObject();
		if (!bhkCollision || !bhkCollision->body) {
			return false;
		}

		auto* rigidBody = bhkCollision->body->AsBhkRigidBody();
		if (!rigidBody) {
			return false;
		}

		const RE::hkVector4 impulse(impulseDir.x * a_cfg.objectForce, impulseDir.y * a_cfg.objectForce, impulseDir.z * a_cfg.objectForce, 0.0f);
		rigidBody->SetLinearImpulse(impulse);
		return true;
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		KickConfig cfg{};
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}

		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("Kick: failed to open config for write: {}", path.string());
			return;
		}

		file << ToJson(cfg).dump(2);
	}

	void Manager::LoadConfigFromJson()
	{
		const auto path = GetConfigPath();
		if (!std::filesystem::exists(path)) {
			SaveConfigToJson();
			return;
		}

		std::ifstream file(path);
		if (!file.is_open()) {
			LOG_WARN("Kick: failed to open config for read: {}", path.string());
			return;
		}

		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("Kick: failed parsing JSON config ({}), rewriting defaults", e.what());
			SaveConfigToJson();
			return;
		}

		KickConfig loaded{};
		{
			std::scoped_lock lock(m_lock);
			loaded = m_config;
		}
		FromJson(parsed, loaded);

		std::scoped_lock lock(m_lock);
		m_config = loaded;
		m_config.objectRange = std::clamp(m_config.objectRange, kMinRange, kMaxRange);
		m_config.objectForce = std::clamp(m_config.objectForce, kMinForce, kMaxForce);
		m_config.objectUpwardBias = std::clamp(m_config.objectUpwardBias, 0.0f, 1.0f);
		m_config.objectCooldownSeconds = std::clamp(m_config.objectCooldownSeconds, 0.0f, 5.0f);
		m_config.objectRaySpread = std::clamp(m_config.objectRaySpread, 0.0f, 1.0f);
		m_config.npcRange = std::clamp(m_config.npcRange, kMinRange, kMaxRange);
		m_config.npcForce = std::clamp(m_config.npcForce, kMinForce, kMaxForce);
		m_config.npcUpwardBias = std::clamp(m_config.npcUpwardBias, 0.0f, 1.0f);
		m_config.npcCooldownSeconds = std::clamp(m_config.npcCooldownSeconds, 0.0f, 5.0f);
		m_config.npcRaySpread = std::clamp(m_config.npcRaySpread, 0.0f, 1.0f);
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"Kick.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "Kick.json";
	}
}
