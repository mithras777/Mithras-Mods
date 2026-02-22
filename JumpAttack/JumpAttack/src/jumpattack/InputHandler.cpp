#include "jumpattack/InputHandler.h"

#include "jumpattack/Settings.h"
#include "jumpattack/TraceUtil.h"

#include <thread>

namespace JA
{
    void InputHandler::Install()
    {
        auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
        if (!inputManager) {
            LOG_ERROR("[JA] Failed to register input sink: BSInputDeviceManager is null");
            return;
        }

        inputManager->AddEventSink(this);
        LOG_INFO("[JA] Input sink registered");
    }

    RE::BSEventNotifyControl InputHandler::ProcessEvent(
        RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>*)
    {
        if (!a_event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto settings = Settings::GetSingleton()->Get();
        if (!settings.enable || !ShouldProcessPlayer(settings, player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        for (auto* event = *a_event; event; event = event->next) {
            const auto* button = event->AsButtonEvent();
            if (!button || !button->IsDown()) {
                continue;
            }

            DamageUtil::AttackKind kind{};
            if (!IsAttackStartEvent(button, kind)) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            if (!CanTriggerNow(kind, settings, now)) {
                continue;
            }

            QueueDelayedHit(kind, settings);
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    bool InputHandler::ShouldProcessPlayer(const SettingsData& a_settings, RE::PlayerCharacter* a_player) const
    {
        if (!a_player || a_player->IsDead() || a_player->IsInKillMove() || a_player->IsOnMount()) {
            return false;
        }

        const auto* state = a_player->AsActorState();
        if (!state) {
            return false;
        }

        if (state->IsSwimming() || !state->IsWeaponDrawn()) {
            return false;
        }

        if (!a_player->IsInMidair()) {
            return false;
        }

        auto* ui = RE::UI::GetSingleton();
        if (ui && (ui->GameIsPaused() || (ui->IsShowingMenus() && !ui->IsMenuOpen("HUD Menu")))) {
            return false;
        }

        auto* controlMap = RE::ControlMap::GetSingleton();
        if (!controlMap || !controlMap->IsFightingControlsEnabled()) {
            return false;
        }

        return IsAllowedEquipmentState(a_settings, a_player, state);
    }

    bool InputHandler::IsAllowedEquipmentState(const SettingsData& a_settings, RE::PlayerCharacter* a_player, const RE::ActorState* a_state) const
    {
        if (!a_player || !a_state) {
            return false;
        }

        if (!a_settings.allowIfBowDrawn) {
            switch (a_state->GetAttackState()) {
            case RE::ATTACK_STATE_ENUM::kBowDraw:
            case RE::ATTACK_STATE_ENUM::kBowAttached:
            case RE::ATTACK_STATE_ENUM::kBowDrawn:
                return false;
            default:
                break;
            }
        }

        if (!a_settings.allowIfSpellEquipped) {
            const auto* left = a_player->GetEquippedObject(true);
            const auto* right = a_player->GetEquippedObject(false);
            const bool leftSpell = left && left->GetFormType() == RE::FormType::Spell;
            const bool rightSpell = right && right->GetFormType() == RE::FormType::Spell;
            if (leftSpell || rightSpell) {
                return false;
            }
        }

        return true;
    }

    bool InputHandler::IsAttackStartEvent(const RE::ButtonEvent* a_button, DamageUtil::AttackKind& a_kindOut) const
    {
        if (!a_button) {
            return false;
        }

        const auto userEvent = a_button->QUserEvent();
        const auto* ue = RE::UserEvents::GetSingleton();
        if (!ue) {
            return false;
        }

        if (userEvent == ue->attackPowerStart) {
            a_kindOut = DamageUtil::AttackKind::kPower;
            return true;
        }
        if (userEvent == ue->attackStart || userEvent == ue->leftAttack || userEvent == ue->rightAttack) {
            a_kindOut = DamageUtil::AttackKind::kLight;
            return true;
        }
        return false;
    }

    bool InputHandler::CanTriggerNow(DamageUtil::AttackKind a_kind, const SettingsData& a_settings, const std::chrono::steady_clock::time_point& a_now)
    {
        std::lock_guard lk(_cooldownLock);
        auto& cooldown = (a_kind == DamageUtil::AttackKind::kPower) ? _powerCooldownUntil : _lightCooldownUntil;
        if (a_now < cooldown) {
            return false;
        }

        cooldown = a_now + std::chrono::milliseconds(a_settings.cooldownMs);
        return true;
    }

    void InputHandler::QueueDelayedHit(DamageUtil::AttackKind a_kind, const SettingsData& a_settings)
    {
        const auto requestTime = std::chrono::steady_clock::now();
        const auto delayMs = a_settings.swingDelayMs;

        std::thread([this, a_kind, requestTime, delayMs]() {
            if (delayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
            SKSE::GetTaskInterface()->AddTask([this, a_kind, requestTime, delayMs]() {
                ExecuteDelayedHit(a_kind, requestTime, delayMs);
            });
        }).detach();
    }

    void InputHandler::ExecuteDelayedHit(DamageUtil::AttackKind a_kind, std::chrono::steady_clock::time_point a_requestTime, std::uint32_t a_swingDelayMs)
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto settings = Settings::GetSingleton()->Get();
        if (!settings.enable || !player || player->IsDead()) {
            return;
        }

        const auto elapsedMs = static_cast<std::uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - a_requestTime).count());

        if (!player->IsInMidair()) {
            const auto allowedWindow = a_swingDelayMs + settings.landingGraceMs;
            if (elapsedMs > allowedWindow) {
                DebugLogFmt("Delayed hit canceled: landed and grace expired ({} ms)", elapsedMs);
                return;
            }
        }

        auto hit = TraceUtil::FindBestTarget(player, settings);
        if (!hit.has_value() || !hit->target) {
            DebugLog("No valid target found");
            return;
        }

        auto result = DamageUtil::ApplyJumpHit(player, hit->target, *hit, a_kind, settings);
        DebugLogFmt("Hit {:08X} | damage {:.2f} | stagger {:.2f}", hit->target->GetFormID(), result.damageApplied, result.staggerApplied);
    }
}
