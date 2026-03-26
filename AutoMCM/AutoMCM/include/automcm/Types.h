#pragma once

namespace AUTOMCM
{
	enum class Backend
	{
		kGeneric = 0,
		kMCMHelper = 1
	};

	enum class ValueType
	{
		kNone = 0,
		kBool = 1,
		kInt = 2,
		kFloat = 3,
		kString = 4
	};

	struct Value
	{
		ValueType type{ ValueType::kNone };
		bool boolValue{ false };
		std::int32_t intValue{ 0 };
		float floatValue{ 0.0f };
		std::string stringValue{};
	};

	struct Entry
	{
		std::string key{};
		Backend backend{ Backend::kGeneric };
		std::string modName{};
		std::string pageName{};
		std::string optionType{};
		std::int32_t optionId{ -1 };
		std::string settingId{};
		std::string stateName{};
		std::string selector{};
		std::int32_t selectorIndex{ -1 };
		std::string side{ "left" };
		Value value{};
	};

	struct SessionStatus
	{
		bool applyScheduled{ false };
		bool applyInProgress{ false };
		bool resetInProgress{ false };
		bool applyComplete{ false };
		std::string lastReason{};
		std::string lastError{};
		std::int32_t lastApplySuccessCount{ 0 };
		std::int32_t lastApplyFailureCount{ 0 };
	};

	[[nodiscard]] inline std::string MakeGenericKey(
		std::string_view a_modName,
		std::string_view a_pageName,
		std::int32_t a_optionId,
		std::string_view a_optionType,
		std::string_view a_stateName)
	{
		return std::format(
			"generic|{}|{}|{}|{}|{}",
			a_modName,
			a_pageName,
			a_optionId,
			a_optionType,
			a_stateName);
	}

	[[nodiscard]] inline std::string MakeHelperKey(
		std::string_view a_modName,
		std::string_view a_settingId)
	{
		return std::format("helper|{}|{}", a_modName, a_settingId);
	}
}
