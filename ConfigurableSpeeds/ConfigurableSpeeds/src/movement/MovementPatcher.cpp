#include "movement/MovementPatcher.h"

#include <algorithm>
#include <cctype>

namespace MOVEMENT
{
	void MovementPatcher::StartupApply(const SettingsData& a_settings)
	{
		RestoreCurrentCache();
		RebuildResolvedTargets(a_settings);
		CaptureOriginalsOnce();
		if (a_settings.general.enabled) {
			ApplyResolved(a_settings);
		}
	}

	void MovementPatcher::OnSettingsChanged(const SettingsData& a_oldSettings, const SettingsData& a_newSettings)
	{
		if (a_newSettings.general.enabled) {
			RestoreCurrentCache();
			RebuildResolvedTargets(a_newSettings);
			CaptureOriginalsOnce();
			ApplyResolved(a_newSettings);
			return;
		}

		if (a_oldSettings.general.enabled && a_newSettings.general.restoreOnDisable) {
			RestoreCurrentCache();
		}

		RebuildResolvedTargets(a_newSettings);
		CaptureOriginalsOnce();
	}

	void MovementPatcher::ApplyNow(const SettingsData& a_settings)
	{
		RestoreCurrentCache();
		RebuildResolvedTargets(a_settings);
		CaptureOriginalsOnce();
		if (a_settings.general.enabled) {
			ApplyResolved(a_settings);
		}
	}

	void MovementPatcher::Restore()
	{
		RestoreCurrentCache();
	}

	void MovementPatcher::ShutdownRestoreIfNeeded(const SettingsData& a_settings)
	{
		if (a_settings.general.restoreOnDisable) {
			RestoreCurrentCache();
		}
	}

	void MovementPatcher::RestoreCurrentCache()
	{
		for (auto& [_, entry] : m_resolved) {
			if (!entry.form || !entry.originalsCaptured) {
				continue;
			}
			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					entry.form->movementTypeData.defaultData.speeds[i][j] = entry.originals[i][j];
				}
			}
		}
	}

	void MovementPatcher::RebuildResolvedTargets(const SettingsData& a_settings)
	{
		auto previous = m_resolved;
		m_resolved.clear();
		m_resolved.reserve(a_settings.entries.size());

		for (const auto& entry : a_settings.entries) {
			auto resolved = ResolveTarget(entry);
			if (!resolved.valid) {
				continue;
			}

			if (const auto it = previous.find(resolved.formKey); it != previous.end() && it->second.originalsCaptured) {
				resolved.originalsCaptured = true;
				for (std::size_t i = 0; i < 5; ++i) {
					for (std::size_t j = 0; j < 2; ++j) {
						resolved.originals[i][j] = it->second.originals[i][j];
					}
				}
			}

			m_resolved[resolved.formKey] = std::move(resolved);
		}
	}

	void MovementPatcher::CaptureOriginalsOnce()
	{
		for (auto& [_, entry] : m_resolved) {
			if (!entry.form || entry.originalsCaptured) {
				continue;
			}
			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					entry.originals[i][j] = entry.form->movementTypeData.defaultData.speeds[i][j];
				}
			}
			entry.originalsCaptured = true;
		}
	}

	void MovementPatcher::ApplyResolved(const SettingsData& a_settings)
	{
		for (const auto& setting : a_settings.entries) {
			if (!setting.enabled) {
				continue;
			}

			const auto key = CanonicalTargetKey(setting.form);
			const auto it = m_resolved.find(key);
			if (it == m_resolved.end() || !it->second.form) {
				continue;
			}

			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					it->second.form->movementTypeData.defaultData.speeds[i][j] = setting.speeds[i][j];
				}
			}
		}
	}

	std::string MovementPatcher::Trim(std::string a_text)
	{
		while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front())) != 0) {
			a_text.erase(a_text.begin());
		}
		while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back())) != 0) {
			a_text.pop_back();
		}
		return a_text;
	}

	std::string MovementPatcher::CanonicalTargetKey(const std::string& a_formString)
	{
		std::string key = Trim(a_formString);
		std::transform(
			key.begin(),
			key.end(),
			key.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return key;
	}

	MovementPatcher::ResolvedTarget MovementPatcher::ResolveTarget(const MovementEntry& a_entry)
	{
		ResolvedTarget out{};
		out.formKey = CanonicalTargetKey(a_entry.form);
		if (out.formKey.empty()) {
			return out;
		}

		const auto separator = a_entry.form.find('|');
		if (separator == std::string::npos) {
			return out;
		}

		const std::string plugin = Trim(a_entry.form.substr(0, separator));
		const std::string idText = Trim(a_entry.form.substr(separator + 1));
		if (plugin.empty() || idText.empty()) {
			return out;
		}

		std::uint32_t parsedID = 0;
		try {
			parsedID = static_cast<std::uint32_t>(std::stoul(idText, nullptr, 0));
		} catch (...) {
			return out;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return out;
		}

		auto* movementType = dataHandler->LookupForm<RE::BGSMovementType>(parsedID, plugin);
		if (!movementType) {
			movementType = dataHandler->LookupFormRaw<RE::BGSMovementType>(parsedID, plugin);
		}
		if (!movementType) {
			return out;
		}

		out.valid = true;
		out.form = movementType;
		return out;
	}
}
