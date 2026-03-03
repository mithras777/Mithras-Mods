#pragma once

#include "PCH.h"

namespace PERILOUS
{
	class Forms final : public REX::Singleton<Forms>
	{
	public:
		void InitForms();

		void PlayRedVfx(RE::Actor* a_actor) const;
		void PlaySfx(RE::Actor* a_actor) const;

		bool HasRedVfx() const;
		bool HasSfx() const;

	private:
		RE::BGSArtObject* _redVfx{ nullptr };
		RE::BGSSoundDescriptorForm* _sfx{ nullptr };
		bool _warnedMissingPlugin{ false };
	};
}