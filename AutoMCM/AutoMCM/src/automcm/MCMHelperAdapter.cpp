#include "automcm/MCMHelperAdapter.h"

#include <SimpleIni.h>
#include <nlohmann/json.hpp>

namespace AUTOMCM
{
	MCMHelperAdapter& MCMHelperAdapter::GetSingleton()
	{
		static MCMHelperAdapter singleton;
		return singleton;
	}

	bool MCMHelperAdapter::ApplyEntry(const Entry& a_entry) const
	{
		if (a_entry.backend != Backend::kMCMHelper || a_entry.settingId.empty()) {
			return false;
		}

		switch (a_entry.value.type) {
		case ValueType::kBool:
			return DispatchSet("SetModSettingBool", a_entry);
		case ValueType::kInt:
			return DispatchSet("SetModSettingInt", a_entry);
		case ValueType::kFloat:
			return DispatchSet("SetModSettingFloat", a_entry);
		case ValueType::kString:
			return DispatchSet("SetModSettingString", a_entry);
		default:
			return false;
		}
	}

	bool MCMHelperAdapter::SupportsMod(std::string_view a_modName) const
	{
		auto configPath = std::filesystem::path("Data/MCM/Config") / std::string(a_modName) / "config.json";
		return std::filesystem::exists(configPath);
	}

	bool MCMHelperAdapter::TryGetDescriptor(
		std::string_view a_modName,
		std::string_view a_settingId,
		HelperSettingDescriptor& a_out) const
	{
		if (const auto* metadata = GetMetadata(a_modName)) {
			if (const auto it = metadata->find(std::string(a_settingId)); it != metadata->end()) {
				a_out = it->second;
				return true;
			}
		}
		return false;
	}

	bool MCMHelperAdapter::TryReadCurrentValue(
		std::string_view a_modName,
		std::string_view a_settingId,
		Value& a_out) const
	{
		HelperSettingDescriptor desc{};
		if (!TryGetDescriptor(a_modName, a_settingId, desc)) {
			return false;
		}

		const auto keyParts = SplitSettingKey(a_settingId);
		if (!keyParts) {
			return false;
		}

		const auto iniPath = std::filesystem::path("Data/MCM/Settings") / (std::string(a_modName) + ".ini");
		const auto defaultIniPath = std::filesystem::path("Data/MCM/Config") / std::string(a_modName) / "settings.ini";

		CSimpleIniA ini;
		ini.SetUnicode();
		if (std::filesystem::exists(iniPath)) {
			ini.LoadFile(iniPath.string().c_str());
		} else if (std::filesystem::exists(defaultIniPath)) {
			ini.LoadFile(defaultIniPath.string().c_str());
		} else {
			return false;
		}

		const auto rawValue = std::string(ini.GetValue(keyParts->second.c_str(), keyParts->first.c_str(), ""));
		if (rawValue.empty() && desc.valueType != ValueType::kString) {
			return false;
		}

		a_out.type = desc.valueType;
		switch (desc.valueType) {
		case ValueType::kBool:
			a_out.boolValue = ParseBool(rawValue);
			a_out.intValue = a_out.boolValue ? 1 : 0;
			a_out.floatValue = a_out.boolValue ? 1.0f : 0.0f;
			break;
		case ValueType::kInt:
			a_out.intValue = rawValue.empty() ? 0 : std::stoi(rawValue);
			a_out.floatValue = static_cast<float>(a_out.intValue);
			a_out.boolValue = a_out.intValue != 0;
			break;
		case ValueType::kFloat:
			a_out.floatValue = rawValue.empty() ? 0.0f : std::stof(rawValue);
			a_out.intValue = static_cast<std::int32_t>(a_out.floatValue);
			a_out.boolValue = a_out.floatValue != 0.0f;
			break;
		case ValueType::kString:
			a_out.stringValue = rawValue;
			break;
		default:
			return false;
		}

		return true;
	}

	std::unordered_map<std::string, Value> MCMHelperAdapter::SnapshotValues(std::string_view a_modName) const
	{
		std::unordered_map<std::string, Value> snapshot;
		if (const auto* metadata = GetMetadata(a_modName)) {
			for (const auto& [settingId, _] : *metadata) {
				Value value{};
				if (TryReadCurrentValue(a_modName, settingId, value)) {
					snapshot.emplace(settingId, std::move(value));
				}
			}
		}
		return snapshot;
	}

	bool MCMHelperAdapter::DispatchSet(std::string_view a_functionName, const Entry& a_entry) const
	{
		const auto skyrimVM = RE::SkyrimVM::GetSingleton();
		const auto vm = skyrimVM ? skyrimVM->impl : nullptr;
		if (!vm) {
			return false;
		}

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;

		if (a_functionName == "SetModSettingBool") {
			auto args = RE::MakeFunctionArguments(
				std::string(a_entry.modName),
				std::string(a_entry.settingId),
				static_cast<bool>(a_entry.value.boolValue));
			vm->DispatchStaticCall("MCM", a_functionName, args, callback);
			delete args;
			return true;
		}
		if (a_functionName == "SetModSettingInt") {
			auto args = RE::MakeFunctionArguments(
				std::string(a_entry.modName),
				std::string(a_entry.settingId),
				static_cast<std::int32_t>(a_entry.value.intValue));
			vm->DispatchStaticCall("MCM", a_functionName, args, callback);
			delete args;
			return true;
		}
		if (a_functionName == "SetModSettingFloat") {
			auto args = RE::MakeFunctionArguments(
				std::string(a_entry.modName),
				std::string(a_entry.settingId),
				static_cast<float>(a_entry.value.floatValue));
			vm->DispatchStaticCall("MCM", a_functionName, args, callback);
			delete args;
			return true;
		}
		if (a_functionName == "SetModSettingString") {
			auto args = RE::MakeFunctionArguments(
				std::string(a_entry.modName),
				std::string(a_entry.settingId),
				std::string(a_entry.value.stringValue));
			vm->DispatchStaticCall("MCM", a_functionName, args, callback);
			delete args;
			return true;
		}

		return false;
	}

	const std::unordered_map<std::string, HelperSettingDescriptor>* MCMHelperAdapter::GetMetadata(std::string_view a_modName) const
	{
		std::scoped_lock lock(_metadataLock);
		const auto key = std::string(a_modName);
		if (! _metadataCache.contains(key)) {
			_metadataCache.emplace(key, ParseMetadata(a_modName));
		}
		return std::addressof(_metadataCache.at(key));
	}

	std::unordered_map<std::string, HelperSettingDescriptor> MCMHelperAdapter::ParseMetadata(std::string_view a_modName) const
	{
		using json = nlohmann::json;

		std::unordered_map<std::string, HelperSettingDescriptor> metadata;
		const auto configPath = std::filesystem::path("Data/MCM/Config") / std::string(a_modName) / "config.json";
		if (!std::filesystem::exists(configPath)) {
			return metadata;
		}

		try {
			std::ifstream input(configPath, std::ios::binary);
			const auto root = json::parse(input);

			const auto visitContent = [&](const auto& self, const json& content) -> void {
				if (!content.is_array()) {
					return;
				}

				for (const auto& control : content) {
					if (!control.is_object() || !control.contains("id") || !control.contains("type")) {
						continue;
					}

					const auto id = control.value("id", "");
					const auto type = control.value("type", "");
					const auto sourceType = control.contains("valueOptions")
						? control["valueOptions"].value("sourceType", "")
						: "";

					HelperSettingDescriptor desc{};
					if (sourceType == "ModSettingBool") {
						desc.valueType = ValueType::kBool;
					} else if (sourceType == "ModSettingInt") {
						desc.valueType = ValueType::kInt;
					} else if (sourceType == "ModSettingFloat") {
						desc.valueType = ValueType::kFloat;
					} else if ((type == "text" || type == "menu" || type == "input") &&
						(sourceType.empty() || sourceType == "ModSettingString")) {
						desc.valueType = ValueType::kString;
					}

					if (!id.empty() && desc.valueType != ValueType::kNone) {
						metadata[id] = desc;
					}
				}
			};

			visitContent(visitContent, root.value("content", json::array()));
			for (const auto& page : root.value("pages", json::array())) {
				visitContent(visitContent, page.value("content", json::array()));
			}
		}
		catch (...) {
			return {};
		}

		return metadata;
	}

	std::optional<std::pair<std::string, std::string>> MCMHelperAdapter::SplitSettingKey(std::string_view a_settingId)
	{
		const auto delimiter = a_settingId.find(':');
		if (delimiter == std::string_view::npos) {
			return std::nullopt;
		}
		return std::make_pair(
			std::string(a_settingId.substr(0, delimiter)),
			std::string(a_settingId.substr(delimiter + 1)));
	}

	bool MCMHelperAdapter::ParseBool(std::string_view a_value)
	{
		return a_value == "1" || a_value == "true" || a_value == "True";
	}
}
