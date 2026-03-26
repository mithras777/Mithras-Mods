#include "automcm/ProfileStore.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <nlohmann/json.hpp>

namespace
{
	using json = nlohmann::json;

	std::string ToString(AUTOMCM::Backend a_backend)
	{
		return a_backend == AUTOMCM::Backend::kMCMHelper ? "mcm_helper" : "skyui_generic";
	}

	AUTOMCM::Backend BackendFromString(std::string_view a_value)
	{
		return a_value == "mcm_helper" ? AUTOMCM::Backend::kMCMHelper : AUTOMCM::Backend::kGeneric;
	}

	std::string ToString(AUTOMCM::ValueType a_type)
	{
		switch (a_type) {
		case AUTOMCM::ValueType::kBool:
			return "bool";
		case AUTOMCM::ValueType::kInt:
			return "int";
		case AUTOMCM::ValueType::kFloat:
			return "float";
		case AUTOMCM::ValueType::kString:
			return "string";
		default:
			return "none";
		}
	}

	AUTOMCM::ValueType ValueTypeFromString(std::string_view a_value)
	{
		if (a_value == "bool") {
			return AUTOMCM::ValueType::kBool;
		}
		if (a_value == "int") {
			return AUTOMCM::ValueType::kInt;
		}
		if (a_value == "float") {
			return AUTOMCM::ValueType::kFloat;
		}
		if (a_value == "string") {
			return AUTOMCM::ValueType::kString;
		}
		return AUTOMCM::ValueType::kNone;
	}
}

namespace AUTOMCM
{
	ProfileStore& ProfileStore::GetSingleton()
	{
		static ProfileStore singleton;
		return singleton;
	}

	void ProfileStore::Load()
	{
		std::unique_lock lock(_lock);
		_desired.clear();
		_baseline.clear();

		const auto path = GetProfilePath();
		if (!std::filesystem::exists(path)) {
			return;
		}

		try {
			std::ifstream input(path, std::ios::binary);
			const auto root = json::parse(input);

			for (const auto& item : root.value("desired_entries", json::array())) {
				Entry entry{};
				entry.key = item.value("key", "");
				entry.backend = BackendFromString(item.value("backend", "skyui_generic"));
				entry.modName = item.value("mod_name", "");
				entry.pageName = item.value("page_name", "");
				entry.optionType = item.value("option_type", "");
				entry.optionId = item.value("option_id", -1);
				entry.settingId = item.value("setting_id", "");
				entry.stateName = item.value("state_name", "");
				entry.selector = item.value("selector", "");
				entry.selectorIndex = item.value("selector_index", -1);
				entry.side = item.value("side", "left");
				const auto& value = item.at("value");
				entry.value.type = ValueTypeFromString(value.value("type", "none"));
				entry.value.boolValue = value.value("bool", false);
				entry.value.intValue = value.value("int", 0);
				entry.value.floatValue = value.value("float", 0.0f);
				entry.value.stringValue = value.value("string", "");
				_desired[entry.key] = std::move(entry);
			}

			for (const auto& item : root.value("baseline_entries", json::array())) {
				Entry entry{};
				entry.key = item.value("key", "");
				entry.backend = BackendFromString(item.value("backend", "skyui_generic"));
				entry.modName = item.value("mod_name", "");
				entry.pageName = item.value("page_name", "");
				entry.optionType = item.value("option_type", "");
				entry.optionId = item.value("option_id", -1);
				entry.settingId = item.value("setting_id", "");
				entry.stateName = item.value("state_name", "");
				entry.selector = item.value("selector", "");
				entry.selectorIndex = item.value("selector_index", -1);
				entry.side = item.value("side", "left");
				const auto& value = item.at("value");
				entry.value.type = ValueTypeFromString(value.value("type", "none"));
				entry.value.boolValue = value.value("bool", false);
				entry.value.intValue = value.value("int", 0);
				entry.value.floatValue = value.value("float", 0.0f);
				entry.value.stringValue = value.value("string", "");
				_baseline[entry.key] = std::move(entry);
			}

			_status.lastReason = root.value("last_reason", "");
			_status.lastError = root.value("last_error", "");
			_status.lastApplySuccessCount = root.value("last_apply_success_count", 0);
			_status.lastApplyFailureCount = root.value("last_apply_failure_count", 0);
		}
		catch (const std::exception& e) {
			LOG_ERROR("Failed to load AutoMCM profile: {}", e.what());
		}
	}

	void ProfileStore::Save()
	{
		std::shared_lock lock(_lock);
		SaveLocked();
	}

	void ProfileStore::ClearDesired()
	{
		std::unique_lock lock(_lock);
		_desired.clear();
		SaveLocked();
	}

	void ProfileStore::ClearAll()
	{
		std::unique_lock lock(_lock);
		_desired.clear();
		_baseline.clear();
		_status = {};
		SaveLocked();
	}

	void ProfileStore::UpsertDesired(const Entry& a_entry)
	{
		std::unique_lock lock(_lock);
		_desired[a_entry.key] = a_entry;
		SaveLocked();
	}

	void ProfileStore::CaptureBaselineIfMissing(const Entry& a_entry)
	{
		std::unique_lock lock(_lock);
		if (!_baseline.contains(a_entry.key)) {
			_baseline[a_entry.key] = a_entry;
			SaveLocked();
		}
	}

	bool ProfileStore::TryGetDesired(std::string_view a_key, Entry& a_out) const
	{
		std::shared_lock lock(_lock);
		if (const auto it = _desired.find(std::string(a_key)); it != _desired.end()) {
			a_out = it->second;
			return true;
		}
		return false;
	}

	bool ProfileStore::TryGetBaseline(std::string_view a_key, Entry& a_out) const
	{
		std::shared_lock lock(_lock);
		if (const auto it = _baseline.find(std::string(a_key)); it != _baseline.end()) {
			a_out = it->second;
			return true;
		}
		return false;
	}

	std::vector<Entry> ProfileStore::GetDesiredEntries() const
	{
		std::shared_lock lock(_lock);
		std::vector<Entry> entries;
		for (const auto& [_, entry] : _desired) {
			entries.push_back(entry);
		}
		return entries;
	}

	std::vector<Entry> ProfileStore::GetBaselineEntries() const
	{
		std::shared_lock lock(_lock);
		std::vector<Entry> entries;
		for (const auto& [_, entry] : _baseline) {
			entries.push_back(entry);
		}
		return entries;
	}

	const SessionStatus& ProfileStore::GetStatus() const
	{
		return _status;
	}

	void ProfileStore::SetStatus(const SessionStatus& a_status)
	{
		std::unique_lock lock(_lock);
		_status = a_status;
		SaveLocked();
	}

	std::filesystem::path ProfileStore::GetProfilePath() const
	{
		auto pluginDir = std::filesystem::path(DLLMAIN::Plugin::GetSingleton()->Info().directory);
		std::filesystem::create_directories(pluginDir);
		return pluginDir / "AutoMCMProfile.json";
	}

	void ProfileStore::SaveLocked() const
	{
		try {
			nlohmann::json root = {
				{ "schema_version", 1 },
				{ "plugin", DLLMAIN::Plugin::GetSingleton()->Info().name },
				{ "plugin_version", DLLMAIN::Plugin::GetSingleton()->Info().version.string(".") },
				{ "last_reason", _status.lastReason },
				{ "last_error", _status.lastError },
				{ "last_apply_success_count", _status.lastApplySuccessCount },
				{ "last_apply_failure_count", _status.lastApplyFailureCount },
				{ "desired_entries", nlohmann::json::array() },
				{ "baseline_entries", nlohmann::json::array() }
			};

			for (const auto& [_, entry] : _desired) {
				root["desired_entries"].push_back({
					{ "key", entry.key },
					{ "backend", ToString(entry.backend) },
					{ "mod_name", entry.modName },
					{ "page_name", entry.pageName },
					{ "option_type", entry.optionType },
					{ "option_id", entry.optionId },
					{ "setting_id", entry.settingId },
					{ "state_name", entry.stateName },
					{ "selector", entry.selector },
					{ "selector_index", entry.selectorIndex },
					{ "side", entry.side },
					{ "value", {
						{ "type", ToString(entry.value.type) },
						{ "bool", entry.value.boolValue },
						{ "int", entry.value.intValue },
						{ "float", entry.value.floatValue },
						{ "string", entry.value.stringValue }
					} }
				});
			}

			for (const auto& [_, entry] : _baseline) {
				root["baseline_entries"].push_back({
					{ "key", entry.key },
					{ "backend", ToString(entry.backend) },
					{ "mod_name", entry.modName },
					{ "page_name", entry.pageName },
					{ "option_type", entry.optionType },
					{ "option_id", entry.optionId },
					{ "setting_id", entry.settingId },
					{ "state_name", entry.stateName },
					{ "selector", entry.selector },
					{ "selector_index", entry.selectorIndex },
					{ "side", entry.side },
					{ "value", {
						{ "type", ToString(entry.value.type) },
						{ "bool", entry.value.boolValue },
						{ "int", entry.value.intValue },
						{ "float", entry.value.floatValue },
						{ "string", entry.value.stringValue }
					} }
				});
			}

			std::ofstream output(GetProfilePath(), std::ios::binary | std::ios::trunc);
			output << root.dump(2);
		}
		catch (const std::exception& e) {
			LOG_ERROR("Failed to save AutoMCM profile: {}", e.what());
		}
	}
}
