#pragma once

namespace Script
{
	using ScriptObjectPtr = RE::BSTSmartPointer<RE::BSScript::Object>;
	using ScriptArrayPtr = RE::BSTSmartPointer<RE::BSScript::Array>;
	using ScriptCallbackPtr = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>;

	class Object
	{
	public:
		static ScriptObjectPtr FromForm(const RE::TESForm* a_form, std::string_view a_scriptName);
		static RE::BSScript::Variable* GetVariable(
			ScriptObjectPtr a_object,
			std::string_view a_variableName);
		static bool IsType(ScriptObjectPtr a_object, std::string_view a_scriptName);
		static std::string GetString(ScriptObjectPtr a_object, std::string_view a_variableName);
		static std::int32_t GetInt(ScriptObjectPtr a_object, std::string_view a_variableName);
		static void SetString(
			ScriptObjectPtr a_object,
			std::string_view a_variableName,
			std::string_view a_value);
		static void SetInt(
			ScriptObjectPtr a_object,
			std::string_view a_variableName,
			std::int32_t a_value);
		static ScriptArrayPtr GetArray(ScriptObjectPtr a_object, std::string_view a_variableName);

		template <class... Args>
		static void DispatchMethodCall(
			ScriptObjectPtr a_object,
			std::string_view a_functionName,
			Args&&... a_args)
		{
			const auto skyrimVM = RE::SkyrimVM::GetSingleton();
			const auto vm = skyrimVM ? skyrimVM->impl : nullptr;
			if (!vm || !a_object) {
				return;
			}

			auto args = RE::MakeFunctionArguments(static_cast<std::decay_t<Args>>(a_args)...);
			ScriptCallbackPtr callback;
			vm->DispatchMethodCall(a_object, RE::BSFixedString(a_functionName.data()), args, callback);
			delete args;
		}
	};
}
