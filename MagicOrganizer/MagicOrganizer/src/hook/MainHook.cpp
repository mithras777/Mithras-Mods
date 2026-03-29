#include "hook/MainHook.h"

#include "hook/FunctionHook.h"
#include "hook/InputHook.h"
#include "magic/MagicOrganizerManager.h"

#include "util/LogUtil.h"

namespace HOOK::MAIN {

	namespace
	{
		struct MagicMenuAddSpellVisitor_Visit
		{
			static RE::BSContainer::ForEachResult thunk(RE::MagicMenuAddSpellVisitor* a_this, RE::SpellItem* a_spell)
			{
				if (!a_spell) {
					return func(a_this, a_spell);
				}

				const auto formID = a_spell->GetFormID();
				auto* manager = MITHRAS::MAGIC_ORGANIZER::Manager::GetSingleton();
				const auto cfg = manager->GetConfig();
				if (!cfg.enabled) {
					return func(a_this, a_spell);
				}

				const auto type = a_spell->GetSpellType();
				const bool isPowerLike = type == RE::MagicSystem::SpellType::kPower ||
				                         type == RE::MagicSystem::SpellType::kLesserPower ||
				                         type == RE::MagicSystem::SpellType::kVoicePower;
				const bool shouldHide = isPowerLike ? manager->IsPowerShoutHidden(formID) : manager->IsSpellHidden(formID);
				if (shouldHide) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				return func(a_this, a_spell);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct MagicMenuAddActiveEffectVisitor_Visit
		{
			static RE::BSContainer::ForEachResult thunk(RE::MagicMenuAddActiveEffectVisitor* a_this, RE::ActiveEffect* a_effect)
			{
				if (!a_effect) {
					return func(a_this, a_effect);
				}

				auto* effect = a_effect->GetBaseObject();
				if (!effect) {
					return func(a_this, a_effect);
				}

				auto* manager = MITHRAS::MAGIC_ORGANIZER::Manager::GetSingleton();
				const auto cfg = manager->GetConfig();
				if (cfg.enabled && manager->IsEffectHidden(effect->GetFormID())) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				return func(a_this, a_effect);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Install()
	{
		LOG_INFO("Hooks initializing...");
		// Setup Variables
		HOOK::VARIABLE::fDeltaWorldTime = reinterpret_cast<const float*>(RELOCATION_ID(523660, 410199).address());

		HOOK::INPUT::Install();
		stl::Hook::virtual_function<RE::MagicMenuAddSpellVisitor, MagicMenuAddSpellVisitor_Visit>(0, 1);
		LOG_INFO("Hooked MagicMenuAddSpellVisitor::Visit");
		stl::Hook::virtual_function<RE::MagicMenuAddActiveEffectVisitor, MagicMenuAddActiveEffectVisitor_Visit>(0, 1);
		LOG_INFO("Hooked MagicMenuAddActiveEffectVisitor::Visit");
	}
}
