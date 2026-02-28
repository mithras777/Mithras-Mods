#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace DYNAMIC_SPAWNS
{
	struct ResolvedSpawnForm
	{
		enum class Kind
		{
			kNone,
			kActorBase,
			kLeveledList,
			kFormList
		};

		Kind              kind{ Kind::kNone };
		RE::TESBoundObject* boundObject{ nullptr };
		RE::BGSListForm*    formList{ nullptr };
	};

	class FormResolver final : public REX::Singleton<FormResolver>
	{
	public:
		[[nodiscard]] RE::TESForm* ResolveFormString(const std::string& a_formString) const;
		[[nodiscard]] ResolvedSpawnForm ResolveSpawnForm(const std::string& a_formString) const;
		[[nodiscard]] RE::BGSKeyword* ResolveKeywordByEditorID(const std::string& a_editorID);

	private:
		[[nodiscard]] static std::pair<std::string, std::uint32_t> ParsePluginForm(const std::string& a_formString);

		std::mutex                                        m_lock;
		std::unordered_map<std::string, RE::BGSKeyword*> m_keywordCache;
	};
}
