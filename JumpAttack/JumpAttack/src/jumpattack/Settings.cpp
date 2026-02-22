#include "jumpattack/Settings.h"

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>

namespace
{
    using json = nlohmann::json;

    json ToJson(const JA::SettingsData& a_data)
    {
        return {
            { "enable", a_data.enable },
            { "range", a_data.range },
            { "raysCount", a_data.raysCount },
            { "coneAngleDegrees", a_data.coneAngleDegrees },
            { "swingDelayMs", a_data.swingDelayMs },
            { "cooldownMs", a_data.cooldownMs },
            { "allowThroughActorsOnly", a_data.allowThroughActorsOnly },
            { "allowIfBowDrawn", a_data.allowIfBowDrawn },
            { "allowIfSpellEquipped", a_data.allowIfSpellEquipped },
            { "applyStagger", a_data.applyStagger },
            { "staggerMagnitude", a_data.staggerMagnitude },
            { "landingGraceMs", a_data.landingGraceMs },
            { "debugLog", a_data.debugLog }
        };
    }

    void FromJson(const json& a_json, JA::SettingsData& a_data)
    {
        a_data.enable = a_json.value("enable", a_data.enable);
        a_data.range = a_json.value("range", a_data.range);
        a_data.raysCount = a_json.value("raysCount", a_data.raysCount);
        a_data.coneAngleDegrees = a_json.value("coneAngleDegrees", a_data.coneAngleDegrees);
        a_data.swingDelayMs = a_json.value("swingDelayMs", a_data.swingDelayMs);
        a_data.cooldownMs = a_json.value("cooldownMs", a_data.cooldownMs);
        a_data.allowThroughActorsOnly = a_json.value("allowThroughActorsOnly", a_data.allowThroughActorsOnly);
        a_data.allowIfBowDrawn = a_json.value("allowIfBowDrawn", a_data.allowIfBowDrawn);
        a_data.allowIfSpellEquipped = a_json.value("allowIfSpellEquipped", a_data.allowIfSpellEquipped);
        a_data.applyStagger = a_json.value("applyStagger", a_data.applyStagger);
        a_data.staggerMagnitude = a_json.value("staggerMagnitude", a_data.staggerMagnitude);
        a_data.landingGraceMs = a_json.value("landingGraceMs", a_data.landingGraceMs);
        a_data.debugLog = a_json.value("debugLog", a_data.debugLog);
    }
}

namespace JA
{
    void Settings::Load()
    {
        SettingsData loaded{};
        LoadFromJson(loaded);
        Clamp(loaded);

        {
            std::unique_lock lk(_lock);
            _data = loaded;
        }

        ApplyLoggingLevel(loaded.debugLog);
    }

    void Settings::Set(const SettingsData& a_data, bool a_saveJson)
    {
        SettingsData updated = a_data;
        Clamp(updated);

        {
            std::unique_lock lk(_lock);
            _data = updated;
        }

        ApplyLoggingLevel(updated.debugLog);

        if (a_saveJson) {
            SaveToJson(updated);
        }
    }

    SettingsData Settings::Get() const
    {
        std::shared_lock lk(_lock);
        return _data;
    }

    bool Settings::IsDebugEnabled() const
    {
        std::shared_lock lk(_lock);
        return _data.debugLog;
    }

    void Settings::LoadFromJson(SettingsData& a_data)
    {
        const auto path = GetJsonPath();
        if (!std::filesystem::exists(path)) {
            SaveToJson(a_data);
            return;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_WARN("[JA] Failed to open JSON config '{}'", path.string());
            return;
        }

        json parsed{};
        try {
            file >> parsed;
            FromJson(parsed, a_data);
            LOG_INFO("[JA] Loaded JSON config '{}'", path.string());
        } catch (const std::exception& e) {
            LOG_WARN("[JA] Failed to parse JSON config '{}' ({})", path.string(), e.what());
        }
    }

    void Settings::SaveToJson(const SettingsData& a_data)
    {
        const auto path = GetJsonPath();
        std::error_code ec{};
        std::filesystem::create_directories(path.parent_path(), ec);

        std::ofstream file(path, std::ios::trunc);
        if (!file.is_open()) {
            LOG_WARN("[JA] Failed to open JSON config for write '{}'", path.string());
            return;
        }

        file << ToJson(a_data).dump(2);
    }

    std::filesystem::path Settings::GetJsonPath()
    {
        wchar_t dllPath[MAX_PATH]{};
        const auto handle = GetModuleHandleW(L"JumpAttack.dll");
        if (GetModuleFileNameW(handle, dllPath, MAX_PATH) == 0) {
            return std::filesystem::path("Data/SKSE/Plugins/JumpAttack.json");
        }
        return std::filesystem::path(dllPath).parent_path() / "JumpAttack.json";
    }

    void Settings::ApplyLoggingLevel(bool a_debugEnabled)
    {
        if (auto logger = spdlog::get(UTIL::LOGGER_NAME)) {
            const auto level = a_debugEnabled ? spdlog::level::info : spdlog::level::warn;
            logger->set_level(level);
            logger->flush_on(level);
        }
    }

    void Settings::Clamp(SettingsData& a_data)
    {
        a_data.range = std::clamp(a_data.range, 50.0f, 400.0f);
        a_data.raysCount = std::clamp<std::uint32_t>(a_data.raysCount, 1, 9);
        a_data.coneAngleDegrees = std::clamp(a_data.coneAngleDegrees, 0.0f, 25.0f);
        a_data.swingDelayMs = std::clamp<std::uint32_t>(a_data.swingDelayMs, 0, 500);
        a_data.cooldownMs = std::clamp<std::uint32_t>(a_data.cooldownMs, 50, 2000);
        a_data.staggerMagnitude = std::clamp(a_data.staggerMagnitude, 0.0f, 2.0f);
        a_data.landingGraceMs = std::clamp<std::uint32_t>(a_data.landingGraceMs, 0, 500);
    }

    void DebugLog(std::string_view a_message)
    {
        if (!Settings::GetSingleton()->IsDebugEnabled()) {
            return;
        }
        LOG_INFO("[JA] {}", a_message);
    }
}
