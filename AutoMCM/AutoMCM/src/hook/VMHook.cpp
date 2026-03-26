#include "hook/VMHook.h"

#include "automcm/Runtime.h"
#include "script/ScriptObject.h"
#include "util/HookUtil.h"
#include "util/LogUtil.h"

#undef GetObject

namespace
{
	using Script::Object;
	using Script::ScriptArrayPtr;
	using Script::ScriptObjectPtr;

	std::string GetPageName(const ScriptObjectPtr& a_object)
	{
		auto page = Object::GetString(a_object, "_currentPage");
		return page.empty() ? "SKYUI_DEFAULT_PAGE" : page;
	}

	std::string GetStateName(const ScriptObjectPtr& a_object, std::int32_t a_index)
	{
		if (auto array = Object::GetArray(a_object, "_stateOptionMap"); array && a_index >= 0 && static_cast<std::uint32_t>(a_index) < array->size()) {
			return std::string(array->data()[a_index].GetString());
		}
		return {};
	}

	std::string GetTextValue(const ScriptObjectPtr& a_object, std::string_view a_varName, std::int32_t a_index)
	{
		if (auto array = Object::GetArray(a_object, a_varName); array && a_index >= 0 && static_cast<std::uint32_t>(a_index) < array->size()) {
			return std::string(array->data()[a_index].GetString());
		}
		return {};
	}

	float GetNumValue(const ScriptObjectPtr& a_object, std::int32_t a_index)
	{
		if (auto array = Object::GetArray(a_object, "_numValueBuf"); array && a_index >= 0 && static_cast<std::uint32_t>(a_index) < array->size()) {
			return array->data()[a_index].GetFloat();
		}
		return 0.0f;
	}

	std::int32_t GetOptionType(const ScriptObjectPtr& a_object, std::int32_t a_index)
	{
		if (auto array = Object::GetArray(a_object, "_optionFlagsBuf"); array && a_index >= 0 && static_cast<std::uint32_t>(a_index) < array->size()) {
			return array->data()[a_index].GetSInt() % 256;
		}
		return -1;
	}

	std::string GetOptionTypeName(std::int32_t a_type)
	{
		switch (a_type) {
		case 2:
			return "text";
		case 3:
			return "toggle";
		case 4:
			return "slider";
		case 5:
			return "menu";
		case 6:
			return "color";
		case 7:
			return "keymap";
		case 8:
			return "input";
		default:
			return "unknown";
		}
	}

	std::vector<RE::BSScript::Variable> UnpackArgs(RE::BSScript::IFunctionArguments* a_args)
	{
		std::vector<RE::BSScript::Variable> values;
		if (!a_args) {
			return values;
		}

		RE::BSScrapArray<RE::BSScript::Variable> vars;
		if ((*a_args)(vars)) {
			values.reserve(vars.size());
			for (auto& var : vars) {
				values.push_back(var);
			}
		}
		return values;
	}

	std::vector<RE::BSScript::Variable> GetStackArgs(
		const RE::BSTSmartPointer<RE::BSScript::Stack>& a_stack,
		const RE::BSScript::IFunction* a_function)
	{
		std::vector<RE::BSScript::Variable> values;
		if (!a_stack || !a_stack->top || !a_function) {
			return values;
		}

		const auto* frame = a_stack->top;
		const auto pageHint = frame->GetPageForFrame();
		const auto paramCount = a_function->GetParamCount();
		values.reserve(paramCount);
		for (std::uint32_t i = 0; i < paramCount; ++i) {
			values.push_back(frame->GetStackFrameVariable(i, pageHint));
		}
		return values;
	}

	AUTOMCM::Value MakeValueFromObject(
		const ScriptObjectPtr& a_object,
		const std::string& a_optionType,
		std::int32_t a_index,
		const std::vector<RE::BSScript::Variable>& a_args)
	{
		AUTOMCM::Value value{};
		if (a_optionType == "toggle") {
			value.type = AUTOMCM::ValueType::kBool;
			value.boolValue = GetNumValue(a_object, a_index) >= 0.5f;
			value.intValue = value.boolValue ? 1 : 0;
			value.floatValue = value.boolValue ? 1.0f : 0.0f;
		} else if (a_optionType == "slider") {
			value.type = AUTOMCM::ValueType::kFloat;
			value.floatValue = GetNumValue(a_object, a_index);
			value.intValue = static_cast<std::int32_t>(value.floatValue);
		} else if (a_optionType == "menu") {
			value.type = AUTOMCM::ValueType::kInt;
			value.intValue = !a_args.empty() ? a_args[0].GetSInt() : 0;
			value.floatValue = static_cast<float>(value.intValue);
			value.stringValue = GetTextValue(a_object, "_strValueBuf", a_index);
		} else if (a_optionType == "color" || a_optionType == "keymap") {
			value.type = AUTOMCM::ValueType::kInt;
			value.intValue = !a_args.empty() ? a_args[0].GetSInt() : static_cast<std::int32_t>(GetNumValue(a_object, a_index));
			value.floatValue = static_cast<float>(value.intValue);
		} else if (a_optionType == "input") {
			value.type = AUTOMCM::ValueType::kString;
			value.stringValue = !a_args.empty() ? a_args[0].GetString() : GetTextValue(a_object, "_strValueBuf", a_index);
		} else {
			value.type = AUTOMCM::ValueType::kString;
			value.stringValue = GetTextValue(a_object, "_strValueBuf", a_index);
		}
		return value;
	}

	void CaptureSelection(
		const ScriptObjectPtr& a_object,
		std::int32_t a_index,
		const std::vector<RE::BSScript::Variable>& a_args)
	{
		auto* modNameProperty = a_object ? a_object->GetProperty("ModName") : nullptr;
		const auto modName = modNameProperty ? std::string(modNameProperty->GetString()) : std::string{};
		if (modName.empty() || modName == "AutoMCM") {
			return;
		}

		const auto optionType = GetOptionTypeName(GetOptionType(a_object, a_index));
		const auto selector = GetTextValue(a_object, "_textBuf", a_index);
		const auto stateName = GetStateName(a_object, a_index);
		const auto pageName = GetPageName(a_object);
		const auto pageNum = Object::GetInt(a_object, "_currentPageNum");
		const auto optionId = a_index + (pageNum * 256);
		const auto value = MakeValueFromObject(a_object, optionType, a_index, a_args);

		AUTOMCM::Runtime::GetSingleton().RecordGenericValue(
			modName,
			pageName,
			optionType,
			optionId,
			selector,
			stateName,
			-1,
			value);

		LOG_INFO(
			"AutoMCM captured generic change: mod='{}' page='{}' optionType='{}' optionId={} state='{}' selector='{}'",
			modName,
			pageName,
			optionType,
			optionId,
			stateName,
			selector);
	}

	void HandleHelperDispatch(
		const ScriptObjectPtr& a_object,
		std::string_view a_fnName,
		const std::vector<RE::BSScript::Variable>& a_args)
	{
		auto* modNameProperty = a_object ? a_object->GetProperty("ModName") : nullptr;
		const auto modName = modNameProperty ? std::string(modNameProperty->GetString()) : std::string{};
		if (modName.empty() || modName == "AutoMCM") {
			return;
		}

		if (a_fnName == "OnOptionSelect" ||
			a_fnName == "OnOptionSliderAccept" ||
			a_fnName == "OnOptionMenuAccept" ||
			a_fnName == "OnOptionColorAccept" ||
			a_fnName == "OnOptionKeyMapChange" ||
			a_fnName == "OnOptionInputAccept") {
			AUTOMCM::Runtime::GetSingleton().SnapshotHelperState(modName);
			return;
		}

		if (a_fnName == "OnSettingChange" && !a_args.empty()) {
			AUTOMCM::Runtime::GetSingleton().RecordHelperSettingChange(modName, a_args[0].GetString());
		}
	}

	bool ShouldSnapshotHelperBeforeCall(std::string_view a_fnName)
	{
		return a_fnName == "OnOptionSelect" ||
			a_fnName == "OnOptionSliderAccept" ||
			a_fnName == "OnOptionMenuAccept" ||
			a_fnName == "OnOptionColorAccept" ||
			a_fnName == "OnOptionKeyMapChange" ||
			a_fnName == "OnOptionInputAccept";
	}

	std::optional<std::int32_t> GetTrackedGenericIndex(
		const ScriptObjectPtr& a_object,
		std::string_view a_fnName,
		const std::vector<RE::BSScript::Variable>& a_args)
	{
		if (a_fnName == "SelectOption" || a_fnName == "RemapKey" ||
			a_fnName == "OnOptionSelect" || a_fnName == "OnOptionSliderAccept" ||
			a_fnName == "OnOptionMenuAccept" || a_fnName == "OnOptionColorAccept" ||
			a_fnName == "OnOptionKeyMapChange" || a_fnName == "OnOptionInputAccept") {
			if (!a_args.empty()) {
				return a_args[0].GetSInt();
			}
		}

		if (a_fnName == "SetInputText" || a_fnName == "SetColorValue" || a_fnName == "SetMenuIndex" || a_fnName == "SetSliderValue") {
			const auto activeOption = Object::GetInt(a_object, "_activeOption");
			if (activeOption >= 0) {
				return activeOption % 256;
			}
		}

		return std::nullopt;
	}

	void HandleScriptCallAfter(
		const ScriptObjectPtr& a_object,
		std::string_view a_fnName,
		const std::vector<RE::BSScript::Variable>& a_args)
	{
		const auto isHelperConfig = Object::IsType(a_object, "MCM_ConfigBase");
		const auto isSkyUIConfig = Object::IsType(a_object, "SKI_ConfigBase");

		if (!isHelperConfig && !isSkyUIConfig) {
			return;
		}

		if (isHelperConfig) {
			LOG_INFO("AutoMCM saw helper script call: fn='{}'", a_fnName);
			HandleHelperDispatch(a_object, a_fnName, a_args);
		}

		if (isSkyUIConfig && !isHelperConfig) {
			const auto trackedIndex = GetTrackedGenericIndex(a_object, a_fnName, a_args);
			if (trackedIndex.has_value() && *trackedIndex >= 0) {
				CaptureSelection(a_object, *trackedIndex, a_args);
			}
		}
	}
}

namespace HOOK::VM
{
	struct DispatchMethodCallHook
	{
		using fn_t = bool (*)(
			RE::BSScript::Internal::VirtualMachine*,
			ScriptObjectPtr&,
			const RE::BSFixedString&,
			RE::BSScript::IFunctionArguments*,
			Script::ScriptCallbackPtr&);

		static bool thunk(
			RE::BSScript::Internal::VirtualMachine* a_self,
			ScriptObjectPtr& a_obj,
			const RE::BSFixedString& a_fnName,
			RE::BSScript::IFunctionArguments* a_args,
			Script::ScriptCallbackPtr& a_result)
		{
			const auto isHelperConfig = Object::IsType(a_obj, "MCM_ConfigBase");
			const auto isSkyUIConfig = Object::IsType(a_obj, "SKI_ConfigBase");
			const auto fnName = std::string_view(a_fnName);
			const auto args = UnpackArgs(a_args);

			std::optional<std::int32_t> trackedIndex;
			if (isSkyUIConfig && !isHelperConfig) {
				trackedIndex = GetTrackedGenericIndex(a_obj, fnName, args);
			}

			if (isHelperConfig && ShouldSnapshotHelperBeforeCall(fnName)) {
				auto* modNameProperty = a_obj ? a_obj->GetProperty("ModName") : nullptr;
				const auto modName = modNameProperty ? std::string(modNameProperty->GetString()) : std::string{};
				if (!modName.empty() && modName != "AutoMCM") {
					AUTOMCM::Runtime::GetSingleton().SnapshotHelperState(modName);
				}
			}

			const bool result = reinterpret_cast<fn_t>(func)(a_self, a_obj, a_fnName, a_args, a_result);

			if (isHelperConfig) {
				LOG_INFO("AutoMCM saw helper callback: fn='{}'", fnName);
				HandleHelperDispatch(a_obj, fnName, args);
			}

			if (isSkyUIConfig && trackedIndex.has_value() && *trackedIndex >= 0) {
				CaptureSelection(a_obj, *trackedIndex, args);
			}

			return result;
		}

		static inline std::uintptr_t func;
	};

	struct ScriptFunctionCallHook
	{
		using fn_t = RE::BSScript::IFunction::CallResult (*)(
			RE::BSScript::Internal::ScriptFunction*,
			const RE::BSTSmartPointer<RE::BSScript::Stack>&,
			RE::BSScript::ErrorLogger*,
			RE::BSScript::Internal::VirtualMachine*,
			bool);

		static RE::BSScript::IFunction::CallResult thunk(
			RE::BSScript::Internal::ScriptFunction* a_self,
			const RE::BSTSmartPointer<RE::BSScript::Stack>& a_stack,
			RE::BSScript::ErrorLogger* a_logger,
			RE::BSScript::Internal::VirtualMachine* a_vm,
			bool a_arg4)
		{
			ScriptObjectPtr object;
			std::vector<RE::BSScript::Variable> args;
			const auto fnName = std::string_view(a_self ? a_self->GetName().c_str() : "");

			if (a_stack && a_stack->top) {
				object = a_stack->top->self.GetObject();
				args = GetStackArgs(a_stack, a_self);
			}

			if (Object::IsType(object, "MCM_ConfigBase") && ShouldSnapshotHelperBeforeCall(fnName)) {
				auto* modNameProperty = object ? object->GetProperty("ModName") : nullptr;
				const auto modName = modNameProperty ? std::string(modNameProperty->GetString()) : std::string{};
				if (!modName.empty() && modName != "AutoMCM") {
					AUTOMCM::Runtime::GetSingleton().SnapshotHelperState(modName);
				}
			}

			const auto result = reinterpret_cast<fn_t>(func)(a_self, a_stack, a_logger, a_vm, a_arg4);
			HandleScriptCallAfter(object, fnName, args);
			return result;
		}

		static inline std::uintptr_t func;
	};

	void Install()
	{
		stl::Hook::virtual_function<RE::BSScript::Internal::VirtualMachine, DispatchMethodCallHook>(0, 0x27);
		stl::Hook::virtual_function<RE::BSScript::Internal::ScriptFunction, ScriptFunctionCallHook>(0, 0x0F);
		LOG_INFO("Installed BSScript dispatch hook for AutoMCM");
	}
}
