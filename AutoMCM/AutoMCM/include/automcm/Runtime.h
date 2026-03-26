#pragma once

#include "automcm/Types.h"

namespace AUTOMCM
{
	class Runtime
	{
	public:
		static Runtime& GetSingleton();

		void Initialize();
		void OnDataLoaded();
		void OnNewGame();
		void OnPostLoadGame();

		void RecordGenericValue(
			std::string_view a_modName,
			std::string_view a_pageName,
			std::string_view a_optionType,
			std::int32_t a_optionId,
			std::string_view a_selector,
			std::string_view a_stateName,
			std::int32_t a_selectorIndex,
			const Value& a_value);

		void RecordHelperValue(
			std::string_view a_modName,
			std::string_view a_settingId,
			const Value& a_value);
		void SnapshotHelperState(std::string_view a_modName);
		void RecordHelperSettingChange(std::string_view a_modName, std::string_view a_settingId);

		void ScheduleApply(std::string_view a_reason);
		void ResetAll();
		void ForceApplyNow();

		[[nodiscard]] SessionStatus GetStatus() const;
		[[nodiscard]] bool IsApplying() const;
		[[nodiscard]] bool IsResetting() const;

	private:
		void StartRetryThread(std::string a_reason);
		void TryApplyTick();
		void ApplyDesiredEntries();
		bool ApplyEntry(const Entry& a_entry);
		bool ApplyGenericEntry(const Entry& a_entry);
		void SetStatus(const SessionStatus& a_status);

		std::atomic_bool _initialized{ false };
		std::atomic_bool _applyInProgress{ false };
		std::atomic_bool _resetInProgress{ false };
		std::atomic_bool _stopRetryThread{ false };
		mutable std::mutex _retryLock;
		mutable std::mutex _helperSnapshotLock;
		std::unordered_map<std::string, std::unordered_map<std::string, Value>> _pendingHelperSnapshots;
		std::jthread _retryThread{};
	};
}
