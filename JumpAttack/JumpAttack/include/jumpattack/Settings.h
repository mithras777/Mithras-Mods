#pragma once

#include <shared_mutex>
#include <format>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "util/LogUtil.h"

namespace JA
{
    struct SettingsData
    {
        bool enable{ true };
        float range{ 150.0f };
        std::uint32_t raysCount{ 3 };
        float coneAngleDegrees{ 6.0f };
        std::uint32_t swingDelayMs{ 80 };
        std::uint32_t cooldownMs{ 350 };
        bool allowThroughActorsOnly{ true };
        bool allowIfBowDrawn{ false };
        bool allowIfSpellEquipped{ false };
        bool applyStagger{ true };
        float staggerMagnitude{ 0.35f };
        std::uint32_t landingGraceMs{ 140 };
        bool debugLog{ false };
    };

    class Settings final : public REX::Singleton<Settings>
    {
    public:
        void Load();
        void Set(const SettingsData& a_data, bool a_saveJson = true);
        [[nodiscard]] SettingsData Get() const;
        [[nodiscard]] bool IsDebugEnabled() const;

    private:
        static void LoadFromIni(SettingsData& a_data);
        static void LoadFromJson(SettingsData& a_data);
        static void SaveToJson(const SettingsData& a_data);
        static std::filesystem::path GetJsonPath();
        static void ApplyLoggingLevel(bool a_debugEnabled);
        static void Clamp(SettingsData& a_data);
        static bool ParseBool(std::string a_value, bool a_fallback);

        mutable std::shared_mutex _lock;
        SettingsData _data{};
    };

    void DebugLog(std::string_view a_message);

    template <class... Args>
    void DebugLogFmt(std::format_string<Args...> a_fmt, Args&&... a_args)
    {
        if (!Settings::GetSingleton()->IsDebugEnabled()) {
            return;
        }
        LOG_INFO("[JA] {}", std::format(a_fmt, std::forward<Args>(a_args)...));
    }
}
