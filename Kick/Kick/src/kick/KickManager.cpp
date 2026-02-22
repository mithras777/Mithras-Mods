#include "kick/KickManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace MITHRAS::KICK
{
	namespace
	{
		constexpr float kMinVectorLen = 1e-4f;
		constexpr float kMinRange = 32.0f;
		constexpr float kMaxRange = 700.0f;
		constexpr float kMinForce = 1.0f;
		constexpr float kMaxForce = 4000.0f;

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
	}

	void Manager::Initialize()
	{
		std::scoped_lock lock(m_lock);
		m_lastKick = {};
		m_captureHotkey = false;
	}

	void Manager::SetConfig(const KickConfig& a_config)
	{
		std::scoped_lock lock(m_lock);
		m_config = a_config;
		m_config.range = std::clamp(m_config.range, kMinRange, kMaxRange);
		m_config.force = std::clamp(m_config.force, kMinForce, kMaxForce);
		m_config.selfBoostForce = std::clamp(m_config.selfBoostForce, 0.0f, kMaxForce);
		m_config.upwardBias = std::clamp(m_config.upwardBias, 0.0f, 1.0f);
		m_config.selfBoostUpwardMin = std::clamp(m_config.selfBoostUpwardMin, 0.0f, 1.0f);
		m_config.downwardKickThreshold = std::clamp(m_config.downwardKickThreshold, 0.0f, 0.95f);
		m_config.cooldownSeconds = std::clamp(m_config.cooldownSeconds, 0.0f, 5.0f);
		m_config.raySpread = std::clamp(m_config.raySpread, 0.0f, 1.0f);
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
		std::scoped_lock lock(m_lock);
		if (!m_captureHotkey) {
			return;
		}
		m_captureHotkey = false;
		m_config.hotkey = a_keyCode;
		LOG_INFO("Kick: hotkey updated to {}", a_keyCode);
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
		auto debug = [&](std::string_view a_msg) {
			if (cfg.debugLogging) {
				LOG_INFO("Kick Debug: {}", a_msg);
			}
		};

		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			const auto now = std::chrono::steady_clock::now();
			if (cfg.cooldownSeconds > 0.0f) {
				const auto cooldown = std::chrono::duration<float>(cfg.cooldownSeconds);
				if (now - m_lastKick < cooldown) {
					debug("blocked by cooldown");
					return;
				}
			}
			m_lastKick = now;
		}

		debug("kick attempt started");

		if (!cfg.enabled || IsGameplayBlocked()) {
			if (!cfg.enabled) {
				debug("blocked: mod disabled");
			} else {
				debug("blocked: gameplay/menu state");
			}
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || player->IsDead()) {
			debug("blocked: no player or player dead");
			return;
		}

		const auto hit = AcquireKickTarget(player, cfg);
		if (!hit.has_value() || !hit->hasHit) {
			debug("no valid raycast hit target");
			return;
		}

		LOG_INFO("Kick Debug: target candidate distance={:.2f} z={:.2f}", hit->distance, hit->hitPoint.z);

		if (hit->reference) {
			const bool applied = ApplyKickImpulse(hit->reference, hit->direction, cfg.force, cfg.upwardBias);
			LOG_INFO("Kick Debug: impulse {} on ref {:08X}", applied ? "applied" : "failed", hit->reference->GetFormID());
		} else {
			debug("raycast hit had no resolved reference");
		}

		const bool downwardKick = hit->direction.z < -cfg.downwardKickThreshold;
		if (downwardKick && hit->hitPoint.z < player->GetPositionZ() + 16.0f) {
			ApplySelfBoost(player, hit->direction, cfg);
			debug("self boost applied");
		} else {
			debug("self boost not applied");
		}
	}

	void Manager::DebugForceKick()
	{
		auto cfg = GetConfig();
		cfg.debugLogging = true;
		SetConfig(cfg);
		LOG_INFO("Kick Debug: force kick requested from console");
		TryKick();
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

	std::optional<Manager::RaycastResult> Manager::AcquireKickTarget(RE::PlayerCharacter* a_player, const KickConfig& a_cfg)
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
					if (target && target != a_player && !target->IsDisabled()) {
						const auto hitPoint = ResolveRefCenter(target);
						const auto delta = hitPoint - origin;
						const auto dist = delta.Length();
						if (dist > kMinVectorLen && dist <= a_cfg.range) {
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
			const auto dir = NormalizeOrZero(forward + right * (offset.x * a_cfg.raySpread) + up * (offset.y * a_cfg.raySpread));
			if (dir.SqrLength() <= kMinVectorLen) {
				continue;
			}

			const auto hit = RaycastFrom(a_player, origin, dir, a_cfg.range);
			if (!hit.has_value()) {
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
			tes->ForEachReferenceInRange(a_player, a_cfg.range, [&](RE::TESObjectREFR* a_ref) {
				if (!a_ref || a_ref == a_player || a_ref->IsDisabled()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (auto* actor = a_ref->As<RE::Actor>(); actor && actor->IsDead()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const auto center = ResolveRefCenter(a_ref);
				const auto delta = center - origin;
				const auto dist = delta.Length();
				if (dist <= kMinVectorLen || dist > a_cfg.range) {
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

	bool Manager::ApplyKickImpulse(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_dir, float a_force, float a_upwardBias)
	{
		if (!a_ref || a_ref->IsDisabled()) {
			return false;
		}

		auto impulseDir = NormalizeOrZero(a_dir + RE::NiPoint3(0.0f, 0.0f, a_upwardBias));
		if (impulseDir.SqrLength() <= kMinVectorLen) {
			return false;
		}

		if (auto* actor = a_ref->As<RE::Actor>(); actor && !actor->IsDead()) {
			actor->NotifyAnimationGraph("RagdollInstant");
		}

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

		const RE::hkVector4 impulse(impulseDir.x * a_force, impulseDir.y * a_force, impulseDir.z * a_force, 0.0f);
		rigidBody->SetLinearImpulse(impulse);
		return true;
	}

	void Manager::ApplySelfBoost(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_kickDir, const KickConfig& a_cfg)
	{
		if (!a_player || a_cfg.selfBoostForce <= 0.0f) {
			return;
		}

		auto boostDir = NormalizeOrZero(RE::NiPoint3(-a_kickDir.x, -a_kickDir.y, std::max(a_cfg.selfBoostUpwardMin, -a_kickDir.z)));
		if (boostDir.SqrLength() <= kMinVectorLen) {
			return;
		}

		const RE::hkVector4 current(boostDir.x * a_cfg.selfBoostForce, boostDir.y * a_cfg.selfBoostForce, boostDir.z * a_cfg.selfBoostForce, 0.0f);
		a_player->ApplyCurrent(0.15f, current);
	}
}
