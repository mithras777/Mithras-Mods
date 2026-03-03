#include "perilous/PerilousForms.h"

#include "perilous/PerilousConfig.h"
#include "util/LogUtil.h"
#include <memory>

namespace
{
	void Play3DSound(RE::Actor* a_actor, RE::BGSSoundDescriptorForm* a_descriptor)
	{
		if (!a_actor || !a_descriptor) {
			return;
		}

		RE::BSSoundHandle handle;
		handle.soundID = static_cast<std::uint32_t>(-1);
		handle.assumeSuccess = false;
		*reinterpret_cast<std::uint32_t*>(&handle.state) = 0;

		auto* audioManager = RE::BSAudioManager::GetSingleton();
		if (!audioManager) {
			return;
		}

		using BuildHandle_t = void (*)(RE::BSAudioManager*, RE::BSSoundHandle*, RE::FormID, std::uint32_t);
		using SetPosition_t = bool (*)(RE::BSSoundHandle*, float, float, float);
		using SetObject_t = void (*)(RE::BSSoundHandle*, RE::NiAVObject*);
		using Play_t = bool (*)(RE::BSSoundHandle*);

		static REL::Relocation<BuildHandle_t> buildHandle{ RELOCATION_ID(66351, 67617) };
		static REL::Relocation<SetPosition_t> setPosition{ RELOCATION_ID(66441, 67713) };
		static REL::Relocation<SetObject_t> setObject{ RELOCATION_ID(66362, 67628) };
		static REL::Relocation<Play_t> play{ RELOCATION_ID(66370, 67636) };

		buildHandle(audioManager, std::addressof(handle), a_descriptor->GetFormID(), 16);
		if (setPosition(std::addressof(handle), a_actor->data.location.x, a_actor->data.location.y, a_actor->data.location.z)) {
			setObject(std::addressof(handle), a_actor->Get3D());
			play(std::addressof(handle));
		}
	}
}

namespace PERILOUS
{
	void Forms::InitForms()
	{
		_redVfx = nullptr;
		_sfx = nullptr;

		const auto cfg = ConfigStore::GetSingleton()->Get();
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			LOG_ERROR("Perilous forms: TESDataHandler unavailable.");
			return;
		}

		if (!dataHandler->LookupModByName(cfg.sPerilous_FormsPlugin)) {
			if (!_warnedMissingPlugin) {
				LOG_WARN("Perilous forms plugin '{}' not found. VFX/SFX will be skipped.", cfg.sPerilous_FormsPlugin);
				_warnedMissingPlugin = true;
			}
			return;
		}

		if (cfg.iPerilous_RedVfxFormID != 0) {
			_redVfx = dataHandler->LookupForm<RE::BGSArtObject>(cfg.iPerilous_RedVfxFormID, cfg.sPerilous_FormsPlugin);
			if (!_redVfx) {
				LOG_WARN("Perilous red VFX form {:#x} not found in {}", cfg.iPerilous_RedVfxFormID, cfg.sPerilous_FormsPlugin);
			}
		}

		if (cfg.iPerilous_SfxFormID != 0) {
			_sfx = dataHandler->LookupForm<RE::BGSSoundDescriptorForm>(cfg.iPerilous_SfxFormID, cfg.sPerilous_FormsPlugin);
			if (!_sfx) {
				LOG_WARN("Perilous SFX form {:#x} not found in {}", cfg.iPerilous_SfxFormID, cfg.sPerilous_FormsPlugin);
			}
		}

		LOG_INFO("Perilous forms loaded: redVfx={} sfx={}", _redVfx != nullptr, _sfx != nullptr);
	}

	void Forms::PlayRedVfx(RE::Actor* a_actor) const
	{
		if (!a_actor || !_redVfx) {
			return;
		}

		static bool warned = false;
		if (!warned) {
			LOG_WARN("Perilous VFX playback is currently disabled due to NG runtime linkage differences. SFX/gameplay remain active.");
			warned = true;
		}
	}

	void Forms::PlaySfx(RE::Actor* a_actor) const
	{
		Play3DSound(a_actor, _sfx);
	}

	bool Forms::HasRedVfx() const
	{
		return _redVfx != nullptr;
	}

	bool Forms::HasSfx() const
	{
		return _sfx != nullptr;
	}
}
