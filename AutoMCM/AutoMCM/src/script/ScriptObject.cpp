#include "script/ScriptObject.h"

namespace Script
{
	ScriptObjectPtr Object::FromForm(const RE::TESForm* a_form, std::string_view a_scriptName)
	{
		ScriptObjectPtr object;
		if (!a_form) {
			return object;
		}

		const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			return object;
		}

		const auto typeID = static_cast<RE::VMTypeID>(a_form->GetFormType());
		const auto policy = vm->GetObjectHandlePolicy();
		const auto handle = policy ? policy->GetHandleForObject(typeID, a_form) : 0;

		vm->FindBoundObject(handle, a_scriptName.data(), object);
		return object;
	}

	RE::BSScript::Variable* Object::GetVariable(
		ScriptObjectPtr a_object,
		std::string_view a_variableName)
	{
		if (!a_object) {
			return nullptr;
		}

		constexpr auto kInvalid = static_cast<std::uint32_t>(-1);
		auto index = kInvalid;
		decltype(index) offset = 0;

		for (auto cls = a_object->type.get(); cls; cls = cls->GetParent()) {
			const auto vars = cls->GetVariableIter();
			if (index == kInvalid) {
				if (vars) {
					for (std::uint32_t i = 0; i < cls->GetNumVariables(); ++i) {
						if (vars[i].name == a_variableName) {
							index = i;
							break;
						}
					}
				}
			} else {
				offset += cls->GetNumVariables();
			}
		}

		if (index == kInvalid) {
			return nullptr;
		}

		return std::addressof(a_object->variables[offset + index]);
	}

	bool Object::IsType(ScriptObjectPtr a_object, std::string_view a_scriptName)
	{
		for (auto cls = a_object ? a_object->type.get() : nullptr; cls; cls = cls->GetParent()) {
			if (_stricmp(cls->GetName(), a_scriptName.data()) == 0) {
				return true;
			}
		}

		return false;
	}

	std::string Object::GetString(ScriptObjectPtr a_object, std::string_view a_variableName)
	{
		auto* variable = GetVariable(a_object, a_variableName);
		return variable ? std::string(variable->GetString()) : std::string{};
	}

	std::int32_t Object::GetInt(ScriptObjectPtr a_object, std::string_view a_variableName)
	{
		auto* variable = GetVariable(a_object, a_variableName);
		return variable ? variable->GetSInt() : 0;
	}

	void Object::SetString(
		ScriptObjectPtr a_object,
		std::string_view a_variableName,
		std::string_view a_value)
	{
		auto* variable = GetVariable(a_object, a_variableName);
		if (variable) {
			variable->SetString(a_value);
		}
	}

	void Object::SetInt(
		ScriptObjectPtr a_object,
		std::string_view a_variableName,
		std::int32_t a_value)
	{
		auto* variable = GetVariable(a_object, a_variableName);
		if (variable) {
			variable->SetSInt(a_value);
		}
	}

	ScriptArrayPtr Object::GetArray(ScriptObjectPtr a_object, std::string_view a_variableName)
	{
		auto* variable = GetVariable(a_object, a_variableName);
		return variable ? variable->GetArray() : nullptr;
	}
}
