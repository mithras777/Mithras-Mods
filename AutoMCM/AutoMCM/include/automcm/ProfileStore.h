#pragma once

#include "automcm/Types.h"

namespace AUTOMCM
{
	class ProfileStore
	{
	public:
		static ProfileStore& GetSingleton();

		void Load();
		void Save();
		void ClearDesired();
		void ClearAll();

		void UpsertDesired(const Entry& a_entry);
		void CaptureBaselineIfMissing(const Entry& a_entry);
		bool TryGetDesired(std::string_view a_key, Entry& a_out) const;
		bool TryGetBaseline(std::string_view a_key, Entry& a_out) const;
		std::vector<Entry> GetDesiredEntries() const;
		std::vector<Entry> GetBaselineEntries() const;

		const SessionStatus& GetStatus() const;
		void SetStatus(const SessionStatus& a_status);

		[[nodiscard]] std::filesystem::path GetProfilePath() const;

	private:
		void SaveLocked() const;

		mutable std::shared_mutex _lock;
		std::unordered_map<std::string, Entry> _desired;
		std::unordered_map<std::string, Entry> _baseline;
		SessionStatus _status{};
	};
}
