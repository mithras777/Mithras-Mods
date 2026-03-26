#include "automcm/Runtime.h"

#include "automcm/MCMHelperAdapter.h"
#include "automcm/ProfileStore.h"
#include "script/ScriptObject.h"
#include "script/SkyUI.h"
#include "util/LogUtil.h"

namespace
{
	AUTOMCM::Entry MakeGenericEntry(
		std::string_view a_modName,
		std::string_view a_pageName,
		std::string_view a_optionType,
		std::int32_t a_optionId,
		std::string_view a_selector,
		std::string_view a_stateName,
		std::int32_t a_selectorIndex,
		const AUTOMCM::Value& a_value)
	{
		AUTOMCM::Entry entry{};
		entry.backend = AUTOMCM::Backend::kGeneric;
		entry.modName = a_modName;
		entry.pageName = a_pageName;
		entry.optionType = a_optionType;
		entry.optionId = a_optionId;
		entry.selector = a_selector;
		entry.stateName = a_stateName;
		entry.selectorIndex = a_selectorIndex;
		entry.value = a_value;
		entry.key = AUTOMCM::MakeGenericKey(a_modName, a_pageName, a_optionId, a_optionType, a_stateName);
		return entry;
	}

	void DispatchOptionAccept(const Script::ScriptObjectPtr& a_config, const AUTOMCM::Entry& a_entry)
	{
		using Script::Object;
		if (a_entry.optionType == "toggle" || a_entry.optionType == "text" || a_entry.optionType == "clickable") {
			Object::DispatchMethodCall(a_config, "OnOptionSelect", a_entry.optionId);
			return;
		}
		if (a_entry.optionType == "slider") {
			Object::DispatchMethodCall(a_config, "OnOptionSliderAccept", a_entry.optionId, a_entry.value.floatValue);
			return;
		}
		if (a_entry.optionType == "menu") {
			Object::DispatchMethodCall(a_config, "OnOptionMenuAccept", a_entry.optionId, a_entry.value.intValue);
			return;
		}
		if (a_entry.optionType == "keymap") {
			Object::DispatchMethodCall(a_config, "OnOptionKeyMapChange", a_entry.optionId, static_cast<std::uint32_t>(a_entry.value.intValue), std::string(""), std::string(""));
			return;
		}
		if (a_entry.optionType == "color") {
			Object::DispatchMethodCall(a_config, "OnOptionColorAccept", a_entry.optionId, static_cast<std::uint32_t>(a_entry.value.intValue));
			return;
		}
		if (a_entry.optionType == "input") {
			Object::DispatchMethodCall(a_config, "OnOptionInputAccept", a_entry.optionId, std::string(a_entry.value.stringValue));
			return;
		}
	}
}

namespace AUTOMCM
{
	Runtime& Runtime::GetSingleton()
	{
		static Runtime singleton;
		return singleton;
	}

	void Runtime::Initialize()
	{
		if (_initialized.exchange(true)) {
			return;
		}

		ProfileStore::GetSingleton().Load();
		LOG_INFO("AutoMCM runtime initialized");
	}

	void Runtime::OnDataLoaded()
	{
		Initialize();
	}

	void Runtime::OnNewGame()
	{
		ScheduleApply("new_game");
	}

	void Runtime::OnPostLoadGame()
	{
		ScheduleApply("post_load_game");
	}

	void Runtime::RecordGenericValue(
		std::string_view a_modName,
		std::string_view a_pageName,
		std::string_view a_optionType,
		std::int32_t a_optionId,
		std::string_view a_selector,
		std::string_view a_stateName,
		std::int32_t a_selectorIndex,
		const Value& a_value)
	{
		if (_applyInProgress.load() || _resetInProgress.load() || a_modName == "AutoMCM") {
			return;
		}

		auto entry = MakeGenericEntry(a_modName, a_pageName, a_optionType, a_optionId, a_selector, a_stateName, a_selectorIndex, a_value);

		auto& store = ProfileStore::GetSingleton();
		Entry existing{};
		if (!store.TryGetDesired(entry.key, existing)) {
			store.CaptureBaselineIfMissing(entry);
		}
		store.UpsertDesired(entry);
		LOG_INFO("AutoMCM stored generic desired entry: key='{}'", entry.key);
	}

	void Runtime::RecordHelperValue(
		std::string_view a_modName,
		std::string_view a_settingId,
		const Value& a_value)
	{
		if (_applyInProgress.load() || _resetInProgress.load() || a_modName == "AutoMCM") {
			return;
		}

		Entry entry{};
		entry.backend = Backend::kMCMHelper;
		entry.modName = a_modName;
		entry.settingId = a_settingId;
		entry.optionType = "helper";
		entry.value = a_value;
		entry.key = MakeHelperKey(a_modName, a_settingId);

		auto& store = ProfileStore::GetSingleton();
		Entry existing{};
		if (!store.TryGetDesired(entry.key, existing)) {
			store.CaptureBaselineIfMissing(entry);
		}
		store.UpsertDesired(entry);
	}

	void Runtime::SnapshotHelperState(std::string_view a_modName)
	{
		if (_applyInProgress.load() || _resetInProgress.load()) {
			return;
		}

		auto snapshot = MCMHelperAdapter::GetSingleton().SnapshotValues(a_modName);
		if (snapshot.empty()) {
			return;
		}

		std::scoped_lock lock(_helperSnapshotLock);
		_pendingHelperSnapshots[std::string(a_modName)] = std::move(snapshot);
	}

	void Runtime::RecordHelperSettingChange(std::string_view a_modName, std::string_view a_settingId)
	{
		if (_applyInProgress.load() || _resetInProgress.load()) {
			return;
		}

		Value current{};
		if (!MCMHelperAdapter::GetSingleton().TryReadCurrentValue(a_modName, a_settingId, current)) {
			return;
		}

		Entry entry{};
		entry.backend = Backend::kMCMHelper;
		entry.modName = a_modName;
		entry.settingId = a_settingId;
		entry.optionType = "helper";
		entry.value = current;
		entry.key = MakeHelperKey(a_modName, a_settingId);

		auto& store = ProfileStore::GetSingleton();
		Entry existing{};
		if (!store.TryGetDesired(entry.key, existing)) {
			std::scoped_lock lock(_helperSnapshotLock);
			if (const auto snapshotIt = _pendingHelperSnapshots.find(std::string(a_modName)); snapshotIt != _pendingHelperSnapshots.end()) {
				if (const auto valueIt = snapshotIt->second.find(std::string(a_settingId)); valueIt != snapshotIt->second.end()) {
					entry.value = valueIt->second;
					store.CaptureBaselineIfMissing(entry);
					entry.value = current;
				}
			}
		}

		store.UpsertDesired(entry);
		LOG_INFO("AutoMCM stored helper desired entry: key='{}'", entry.key);
	}

	void Runtime::ScheduleApply(std::string_view a_reason)
	{
		auto status = GetStatus();
		status.applyScheduled = true;
		status.lastReason = std::string(a_reason);
		SetStatus(status);

		StartRetryThread(std::string(a_reason));
	}

	void Runtime::ResetAll()
	{
		if (_resetInProgress.exchange(true)) {
			return;
		}

		for (const auto& entry : ProfileStore::GetSingleton().GetBaselineEntries()) {
			ApplyEntry(entry);
		}

		ProfileStore::GetSingleton().ClearDesired();

		auto status = GetStatus();
		status.lastReason = "reset_all";
		status.resetInProgress = false;
		status.lastError.clear();
		SetStatus(status);

		_resetInProgress.store(false);
	}

	void Runtime::ForceApplyNow()
	{
		TryApplyTick();
	}

	SessionStatus Runtime::GetStatus() const
	{
		return ProfileStore::GetSingleton().GetStatus();
	}

	bool Runtime::IsApplying() const
	{
		return _applyInProgress.load();
	}

	bool Runtime::IsResetting() const
	{
		return _resetInProgress.load();
	}

	void Runtime::StartRetryThread(std::string a_reason)
	{
		std::scoped_lock lock(_retryLock);
		_stopRetryThread.store(true);
		if (_retryThread.joinable()) {
			_retryThread.join();
		}
		_stopRetryThread.store(false);

		_retryThread = std::jthread([this, reason = std::move(a_reason)](std::stop_token) {
			for (int attempt = 0; attempt < 20 && !_stopRetryThread.load(); ++attempt) {
				std::this_thread::sleep_for(std::chrono::seconds(attempt == 0 ? 2 : 3));
				SKSE::GetTaskInterface()->AddTask([this]() { TryApplyTick(); });
				if (ProfileStore::GetSingleton().GetStatus().applyComplete) {
					return;
				}
			}
		});
	}

	void Runtime::TryApplyTick()
	{
		if (_applyInProgress.exchange(true) || _resetInProgress.load()) {
			return;
		}

		const auto configs = Script::SkyUI::ConfigManager::GetConfigs();
		if (configs.empty()) {
			_applyInProgress.store(false);
			return;
		}

		ApplyDesiredEntries();
		_applyInProgress.store(false);
	}

	void Runtime::ApplyDesiredEntries()
	{
		auto entries = ProfileStore::GetSingleton().GetDesiredEntries();
		std::ranges::sort(entries, [](const Entry& a_lhs, const Entry& a_rhs) {
			return static_cast<int>(a_lhs.backend) > static_cast<int>(a_rhs.backend);
		});

		int successes = 0;
		int failures = 0;
		std::string lastError;

		for (const auto& entry : entries) {
			if (ApplyEntry(entry)) {
				++successes;
			} else {
				++failures;
				lastError = std::format("Failed to apply {}", entry.key);
			}
		}

		auto status = GetStatus();
		status.applyScheduled = failures != 0;
		status.applyComplete = failures == 0;
		status.lastApplySuccessCount = successes;
		status.lastApplyFailureCount = failures;
		status.lastError = lastError;
		SetStatus(status);
	}

	bool Runtime::ApplyEntry(const Entry& a_entry)
	{
		if (a_entry.backend == Backend::kMCMHelper) {
			return MCMHelperAdapter::GetSingleton().ApplyEntry(a_entry);
		}
		return ApplyGenericEntry(a_entry);
	}

	bool Runtime::ApplyGenericEntry(const Entry& a_entry)
	{
		auto config = Script::SkyUI::ConfigManager::GetConfigByModName(a_entry.modName);
		if (!config) {
			return false;
		}

		Script::Object::DispatchMethodCall(config, "OpenConfig");

		if (!a_entry.pageName.empty()) {
			int pageIndex = 0;
			if (auto pages = Script::Object::GetArray(config, "Pages")) {
				for (std::uint32_t i = 0; i < pages->size(); ++i) {
					if (std::string_view(pages->data()[i].GetString()) == a_entry.pageName) {
						pageIndex = static_cast<int>(i);
						break;
					}
				}
			}

			Script::Object::DispatchMethodCall(config, "SetPage", std::string(a_entry.pageName), pageIndex);
		}

		DispatchOptionAccept(config, a_entry);
		return true;
	}

	void Runtime::SetStatus(const SessionStatus& a_status)
	{
		ProfileStore::GetSingleton().SetStatus(a_status);
	}
}
