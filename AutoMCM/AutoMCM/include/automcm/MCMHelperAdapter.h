#pragma once

#include "automcm/Types.h"

namespace AUTOMCM
{
	struct HelperSettingDescriptor
	{
		ValueType valueType{ ValueType::kNone };
	};

	class MCMHelperAdapter
	{
	public:
		static MCMHelperAdapter& GetSingleton();

		bool ApplyEntry(const Entry& a_entry) const;
		bool SupportsMod(std::string_view a_modName) const;
		bool TryGetDescriptor(
			std::string_view a_modName,
			std::string_view a_settingId,
			HelperSettingDescriptor& a_out) const;
		bool TryReadCurrentValue(
			std::string_view a_modName,
			std::string_view a_settingId,
			Value& a_out) const;
		std::unordered_map<std::string, Value> SnapshotValues(std::string_view a_modName) const;

	private:
		const std::unordered_map<std::string, HelperSettingDescriptor>* GetMetadata(std::string_view a_modName) const;
		std::unordered_map<std::string, HelperSettingDescriptor> ParseMetadata(std::string_view a_modName) const;
		static std::optional<std::pair<std::string, std::string>> SplitSettingKey(std::string_view a_settingId);
		static bool ParseBool(std::string_view a_value);

		bool DispatchSet(std::string_view a_functionName, const Entry& a_entry) const;

		mutable std::mutex _metadataLock;
		mutable std::unordered_map<std::string, std::unordered_map<std::string, HelperSettingDescriptor>> _metadataCache;
	};
}
