#include "jumpattack/Settings.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>

namespace
{
    using json = nlohmann::json;

    std::string Trim(std::string a_value)
    {
        auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

        while (!a_value.empty() && isSpace(static_cast<unsigned char>(a_value.front()))) {
            a_value.erase(a_value.begin());
        }
        while (!a_value.empty() && isSpace(static_cast<unsigned char>(a_value.back()))) {
            a_value.pop_back();
        }
        return a_value;
    }

    std::string ToLower(std::string a_value)
    {
        std::transform(a_value.begin(), a_value.end(), a_value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return a_value;
    }

    bool ParseIniBool(std::string a_value, bool a_fallback)
    {
        auto lowered = ToLower(Trim(std::move(a_value)));
        if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
            return true;
        }
        if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
            return false;
        }
        return a_fallback;
    }

    void ReadIniInto(const std::filesystem::path& a_path, JA::SettingsData& a_data)
    {
        std::ifstream file(a_path);
        if (!file.is_open()) {
            return;
        }

        std::string line{};
        auto parseFloat = [](const std::string& a_text, float a_fallback) {
            try {
                return std::stof(a_text);
            } catch (...) {
                return a_fallback;
            }
        };
        auto parseUInt = [](const std::string& a_text, std::uint32_t a_fallback) {
            try {
                return static_cast<std::uint32_t>(std::stoul(a_text));
            } catch (...) {
                return a_fallback;
            }
        };

        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty() || line.starts_with('#') || line.starts_with(';') || line.starts_with('[')) {
                continue;
            }

            const auto sep = line.find('=');
            if (sep == std::string::npos) {
                continue;
            }

            const auto key = ToLower(Trim(line.substr(0, sep)));
            const auto value = Trim(line.substr(sep + 1));

            if (key == "enable") {
                a_data.enable = ParseIniBool(value, a_data.enable);
            } else if (key == "range") {
                a_data.range = parseFloat(value, a_data.range);
            } else if (key == "rayscount") {
                a_data.raysCount = parseUInt(value, a_data.raysCount);
            } else if (key == "coneangledegrees") {
                a_data.coneAngleDegrees = parseFloat(value, a_data.coneAngleDegrees);
            } else if (key == "swingdelayms") {
                a_data.swingDelayMs = parseUInt(value, a_data.swingDelayMs);
            } else if (key == "cooldownms") {
                a_data.cooldownMs = parseUInt(value, a_data.cooldownMs);
            } else if (key == "allowthroughactorsonly") {
                a_data.allowThroughActorsOnly = ParseIniBool(value, a_data.allowThroughActorsOnly);
            } else if (key == "allowifbowdrawn") {
                a_data.allowIfBowDrawn = ParseIniBool(value, a_data.allowIfBowDrawn);
            } else if (key == "allowifspellequipped") {
                a_data.allowIfSpellEquipped = ParseIniBool(value, a_data.allowIfSpellEquipped);
            } else if (key == "debuglog") {
                a_data.debugLog = ParseIniBool(value, a_data.debugLog);
            } else if (key == "applystagger") {
                a_data.applyStagger = ParseIniBool(value, a_data.applyStagger);
            } else if (key == "staggermagnitude") {
                a_data.staggerMagnitude = parseFloat(value, a_data.staggerMagnitude);
            } else if (key == "landinggracems") {
                a_data.landingGraceMs = parseUInt(value, a_data.landingGraceMs);
            }
        }
    }

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
        LoadFromIni(loaded);
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

    void Settings::LoadFromIni(SettingsData& a_data)
    {
        std::array<std::filesystem::path, 2> paths{
            std::filesystem::path("Data/SKSE/Plugins/JumpAttacks.ini"),
            std::filesystem::path("Data/SKSE/Plugins/JumpAttacksNoBehaviors.ini")
        };

        for (const auto& path : paths) {
            if (!std::filesystem::exists(path)) {
                continue;
            }

            ReadIniInto(path, a_data);
            LOG_INFO("[JA] Loaded INI defaults from '{}'", path.string());
            return;
        }

        LOG_WARN("[JA] INI not found, using built-in defaults");
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
        const auto handle = GetModuleHandleW(L"JumpAttacksNoBehaviors.dll");
        if (GetModuleFileNameW(handle, dllPath, MAX_PATH) == 0) {
            return std::filesystem::path("Data/SKSE/Plugins/JumpAttacksNoBehaviors.json");
        }
        return std::filesystem::path(dllPath).parent_path() / "JumpAttacksNoBehaviors.json";
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

    bool Settings::ParseBool(std::string a_value, bool a_fallback)
    {
        auto lowered = ToLower(Trim(std::move(a_value)));
        if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
            return true;
        }
        if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
            return false;
        }
        return a_fallback;
    }

    void DebugLog(std::string_view a_message)
    {
        if (!Settings::GetSingleton()->IsDebugEnabled()) {
            return;
        }
        LOG_INFO("[JA] {}", a_message);
    }
}
