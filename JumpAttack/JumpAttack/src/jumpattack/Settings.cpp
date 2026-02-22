#include "jumpattack/Settings.h"

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace
{
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
}

namespace JA
{
    void Settings::Load()
    {
        SettingsData loaded{};

        std::array<std::filesystem::path, 2> paths{
            std::filesystem::path("Data/SKSE/Plugins/JumpAttacks.ini"),
            std::filesystem::path("Data/SKSE/Plugins/JumpAttacksNoBehaviors.ini")
        };

        std::filesystem::path usedPath{};
        for (const auto& path : paths) {
            if (std::filesystem::exists(path)) {
                usedPath = path;
                break;
            }
        }

        if (usedPath.empty()) {
            LOG_WARN("[JA] INI not found, using defaults");
            std::unique_lock lk(_lock);
            _data = loaded;
            return;
        }

        std::ifstream file(usedPath);
        if (!file.is_open()) {
            LOG_WARN("[JA] Could not open INI '{}', using defaults", usedPath.string());
            std::unique_lock lk(_lock);
            _data = loaded;
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

            auto key = ToLower(Trim(line.substr(0, sep)));
            auto value = Trim(line.substr(sep + 1));

            if (key == "enable") {
                loaded.enable = ParseBool(value, loaded.enable);
            } else if (key == "range") {
                loaded.range = parseFloat(value, loaded.range);
            } else if (key == "rayscount") {
                loaded.raysCount = parseUInt(value, loaded.raysCount);
            } else if (key == "coneangledegrees") {
                loaded.coneAngleDegrees = parseFloat(value, loaded.coneAngleDegrees);
            } else if (key == "swingdelayms") {
                loaded.swingDelayMs = parseUInt(value, loaded.swingDelayMs);
            } else if (key == "cooldownms") {
                loaded.cooldownMs = parseUInt(value, loaded.cooldownMs);
            } else if (key == "allowthroughactorsonly") {
                loaded.allowThroughActorsOnly = ParseBool(value, loaded.allowThroughActorsOnly);
            } else if (key == "allowifbowdrawn") {
                loaded.allowIfBowDrawn = ParseBool(value, loaded.allowIfBowDrawn);
            } else if (key == "allowifspellequipped") {
                loaded.allowIfSpellEquipped = ParseBool(value, loaded.allowIfSpellEquipped);
            } else if (key == "debuglog") {
                loaded.debugLog = ParseBool(value, loaded.debugLog);
            } else if (key == "applystagger") {
                loaded.applyStagger = ParseBool(value, loaded.applyStagger);
            } else if (key == "staggermagnitude") {
                loaded.staggerMagnitude = parseFloat(value, loaded.staggerMagnitude);
            } else if (key == "landinggracems") {
                loaded.landingGraceMs = parseUInt(value, loaded.landingGraceMs);
            }
        }

        Clamp(loaded);

        {
            std::unique_lock lk(_lock);
            _data = loaded;
        }

        LOG_INFO("[JA] Loaded settings from '{}'", usedPath.string());
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
