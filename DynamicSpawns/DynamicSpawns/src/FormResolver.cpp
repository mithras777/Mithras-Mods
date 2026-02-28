#include "FormResolver.h"

#include "util/LogUtil.h"
#include <optional>

namespace DYNAMIC_SPAWNS
{
	namespace
	{
		std::string Trim(const std::string& a_value)
		{
			const auto start = a_value.find_first_not_of(" \t\r\n");
			if (start == std::string::npos) {
				return {};
			}
			const auto end = a_value.find_last_not_of(" \t\r\n");
			return a_value.substr(start, end - start + 1);
		}

		std::optional<std::uint32_t> ParseHexFormID(std::string value)
		{
			value = Trim(value);
			if (value.starts_with("0x") || value.starts_with("0X")) {
				value = value.substr(2);
			}
			try {
				return static_cast<std::uint32_t>(std::stoul(value, nullptr, 16));
			} catch (...) {
				return std::nullopt;
			}
		}
	}

	RE::TESForm* FormResolver::ResolveFormString(const std::string& a_formString) const
	{
		const auto [pluginName, formID] = ParsePluginForm(a_formString);
		if (formID == 0) {
			return nullptr;
		}

		if (!pluginName.empty()) {
			if (auto* handler = RE::TESDataHandler::GetSingleton()) {
				return handler->LookupForm(formID, pluginName);
			}
			return nullptr;
		}

		return RE::TESForm::LookupByID(formID);
	}

	ResolvedSpawnForm FormResolver::ResolveSpawnForm(const std::string& a_formString) const
	{
		ResolvedSpawnForm resolved{};
		auto* form = ResolveFormString(a_formString);
		if (!form) {
			return resolved;
		}

		if (auto* npc = form->As<RE::TESNPC>()) {
			resolved.kind = ResolvedSpawnForm::Kind::kActorBase;
			resolved.boundObject = npc;
			return resolved;
		}

		if (auto* levChar = form->As<RE::TESLevCharacter>()) {
			resolved.kind = ResolvedSpawnForm::Kind::kLeveledList;
			resolved.boundObject = levChar;
			return resolved;
		}

		if (auto* formList = form->As<RE::BGSListForm>()) {
			resolved.kind = ResolvedSpawnForm::Kind::kFormList;
			resolved.formList = formList;
			return resolved;
		}

		return resolved;
	}

	RE::BGSKeyword* FormResolver::ResolveKeywordByEditorID(const std::string& a_editorID)
	{
		if (a_editorID.empty()) {
			return nullptr;
		}

		std::scoped_lock lk(m_lock);
		if (const auto it = m_keywordCache.find(a_editorID); it != m_keywordCache.end()) {
			return it->second;
		}

		auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(a_editorID);
		m_keywordCache.emplace(a_editorID, keyword);
		return keyword;
	}

	std::pair<std::string, std::uint32_t> FormResolver::ParsePluginForm(const std::string& a_formString)
	{
		const auto text = Trim(a_formString);
		if (text.empty()) {
			return {};
		}

		const auto separator = text.find('|');
		if (separator == std::string::npos) {
			if (const auto parsed = ParseHexFormID(text)) {
				return { "", *parsed };
			}
			return {};
		}

		const auto pluginName = Trim(text.substr(0, separator));
		const auto formText = Trim(text.substr(separator + 1));
		if (const auto parsed = ParseHexFormID(formText)) {
			return { pluginName, *parsed };
		}

		LOG_WARN("Failed to parse form '{}'", a_formString);
		return {};
	}
}
