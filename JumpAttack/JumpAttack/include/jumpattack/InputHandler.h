#pragma once

#include "jumpattack/DamageUtil.h"

#include <chrono>
#include <mutex>

namespace JA
{
    class InputHandler final :
        public REX::Singleton<InputHandler>,
        public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        void Install();

    private:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_event,
            RE::BSTEventSource<RE::InputEvent*>* a_source) override;

        [[nodiscard]] bool ShouldProcessPlayer(const SettingsData& a_settings, RE::PlayerCharacter* a_player) const;
        [[nodiscard]] bool IsAllowedEquipmentState(const SettingsData& a_settings, RE::PlayerCharacter* a_player, const RE::ActorState* a_state) const;
        [[nodiscard]] bool IsAttackStartEvent(const RE::ButtonEvent* a_button, DamageUtil::AttackKind& a_kindOut) const;
        [[nodiscard]] bool CanTriggerNow(DamageUtil::AttackKind a_kind, const SettingsData& a_settings, const std::chrono::steady_clock::time_point& a_now);
        void QueueDelayedHit(DamageUtil::AttackKind a_kind, const SettingsData& a_settings);
        void ExecuteDelayedHit(DamageUtil::AttackKind a_kind, std::chrono::steady_clock::time_point a_requestTime, std::uint32_t a_swingDelayMs);

        std::mutex _cooldownLock;
        std::chrono::steady_clock::time_point _lightCooldownUntil{};
        std::chrono::steady_clock::time_point _powerCooldownUntil{};
    };
}
