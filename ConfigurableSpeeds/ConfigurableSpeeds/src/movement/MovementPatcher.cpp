#include "movement/MovementPatcher.h"

#include "plugin.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <vector>

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

		if (a_settings.general.dumpOnStartup) {
			DumpMovementTypesNow();
		}
	}

	void MovementPatcher::OnSettingsChanged(const SettingsData& a_oldSettings, const SettingsData& a_newSettings)
	{
		if (a_newSettings.general.enabled) {
			// Always restore first to avoid stacking multipliers on repeated Apply.
			RestoreCurrentCache();
			RebuildResolvedTargets(a_newSettings);
			CaptureOriginalsOnce();
			ApplyResolved(a_newSettings);
			return;
		}

		// Disabled path: restore only if explicitly requested.
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

	std::size_t MovementPatcher::DumpMovementTypesNow() const
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			LOG_ERROR("[MovementDump] TESDataHandler unavailable");
			return 0;
		}

		struct DumpEntry
		{
			std::string plugin{};
			std::string editorID{};
			std::string displayName{};
			std::uint32_t formID{ 0 };
			float speeds[5][2]{};
		};

		std::vector<DumpEntry> entries{};
		std::size_t count = 0;
		auto& forms = dataHandler->GetFormArray(RE::FormType::MovementType);
		for (auto* form : forms) {
			auto* movementType = form ? form->As<RE::BGSMovementType>() : nullptr;
			if (!movementType) {
				continue;
			}

			DumpEntry entry{};
			const auto* file = movementType->GetFile(0);
			entry.plugin = file ? std::string(file->GetFilename()) : std::string("<unknown>");
			entry.formID = movementType->GetFormID();
			const char* editor = movementType->GetFormEditorID();
			entry.editorID = (editor && editor[0] != '\0') ? editor : "";
			const char* fullName = movementType->GetName();
			const bool hasFullName = fullName && fullName[0] != '\0';
			if (!entry.editorID.empty()) {
				entry.displayName = entry.editorID;
			} else if (hasFullName) {
				entry.displayName = fullName;
			} else {
				entry.displayName = FormatPluginAndForm(movementType);
			}
			for (std::size_t i = 0; i < 5; ++i) {
				for (std::size_t j = 0; j < 2; ++j) {
					entry.speeds[i][j] = movementType->movementTypeData.defaultData.speeds[i][j];
				}
			}
			entries.push_back(std::move(entry));
			++count;
		}

		std::sort(
			entries.begin(),
			entries.end(),
			[](const DumpEntry& a_lhs, const DumpEntry& a_rhs) {
				if (a_lhs.plugin != a_rhs.plugin) {
					return a_lhs.plugin < a_rhs.plugin;
				}
				return a_lhs.formID < a_rhs.formID;
			});

		for (const auto& entry : entries) {
			const std::string formString = std::format("{}|0x{:08X}", entry.plugin, entry.formID);
			std::string pluginLower = entry.plugin;
			std::transform(
				pluginLower.begin(),
				pluginLower.end(),
				pluginLower.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			LOG_INFO("[MovementDump] ------------------------------------------------------------");
			LOG_INFO("Name      : {}", entry.displayName);
			LOG_INFO("EditorID  : {}", entry.editorID.empty() ? "<none>" : entry.editorID);
			LOG_INFO("Form      : {}", formString);
			LOG_INFO("Resolved  : BGSMovementType*");
			if (pluginLower == "skyrim.esm") {
				LOG_INFO("Tag       : Vanilla");
			}
			LOG_INFO("Speeds    : (indexTwo: 0=Forward, 1=Strafe)");
			LOG_INFO("  [0] Walk      : F={:7.3f}  S={:7.3f}", entry.speeds[0][0], entry.speeds[0][1]);
			LOG_INFO("  [1] Run       : F={:7.3f}  S={:7.3f}", entry.speeds[1][0], entry.speeds[1][1]);
			LOG_INFO("  [2] Sprint    : F={:7.3f}  S={:7.3f}", entry.speeds[2][0], entry.speeds[2][1]);
			LOG_INFO("  [3] SneakWalk : F={:7.3f}  S={:7.3f}", entry.speeds[3][0], entry.speeds[3][1]);
			LOG_INFO("  [4] SneakRun  : F={:7.3f}  S={:7.3f}", entry.speeds[4][0], entry.speeds[4][1]);
			LOG_INFO("CopyTarget: \"{}\"", formString);
		}

		LOG_INFO("[MovementDump] Complete: wrote {} movement types. Target strings are in CopyTarget lines.", count);
		LOG_INFO("[MovementDump] Tip: Log file is in {}", GetLogPathHint());
		return count;
	}

	TargetPreview MovementPatcher::PreviewTarget(const TargetEntry& a_target) const
	{
		return BuildPreviewInternal(a_target);
	}

	std::string MovementPatcher::GetLogPathHint() const
	{
		const auto logDir = SKSE::log::log_directory();
		const auto pluginName = DLLMAIN::Plugin::GetSingleton()->Info().name;
		if (logDir) {
			return std::format("{}\\{}.log", logDir->string(), pluginName);
		}
		return std::format("Documents\\My Games\\Skyrim Special Edition\\SKSE\\{}.log", pluginName);
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
		m_resolved.reserve(a_settings.targets.size());

		for (const auto& target : a_settings.targets) {
			auto resolved = ResolveTarget(target);
			if (!resolved.valid) {
				continue;
			}

			const auto key = resolved.formKey;
			if (const auto it = previous.find(key); it != previous.end() && it->second.originalsCaptured) {
				resolved.originalsCaptured = true;
				for (std::size_t i = 0; i < 5; ++i) {
					for (std::size_t j = 0; j < 2; ++j) {
						resolved.originals[i][j] = it->second.originals[i][j];
					}
				}
			}

			m_resolved[key] = std::move(resolved);
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
		for (const auto& target : a_settings.targets) {
			if (!target.enabled) {
				continue;
			}

			const auto key = CanonicalTargetKey(target.form);
			auto it = m_resolved.find(key);
			if (it == m_resolved.end()) {
				continue;
			}

			auto& resolved = it->second;
			if (!resolved.form || !resolved.originalsCaptured) {
				continue;
			}

			for (const auto category : kAllCategories) {
				const auto i = IndexFor(category);
				const float mult = MultiplierFor(a_settings, category);
				const auto& over = OverrideFor(a_settings, category);

				for (std::size_t j = 0; j < 2; ++j) {
					const float base = resolved.originals[i][j];
					float value = base;
					if (a_settings.general.policy.useOverrides && over.enabled) {
						value = over.value;
					} else if (a_settings.general.policy.useMultipliers) {
						value = base * mult;
					}
					resolved.form->movementTypeData.defaultData.speeds[i][j] = std::max(0.0f, value);
				}
			}
		}
	}

	float MovementPatcher::MultiplierFor(const SettingsData& a_settings, SpeedCategory a_category)
	{
		switch (a_category) {
			case SpeedCategory::kWalk:
				return a_settings.multipliers.walk;
			case SpeedCategory::kRun:
				return a_settings.multipliers.run;
			case SpeedCategory::kSprint:
				return a_settings.multipliers.sprint;
			case SpeedCategory::kSneakWalk:
				return a_settings.multipliers.sneakWalk;
			case SpeedCategory::kSneakRun:
				return a_settings.multipliers.sneakRun;
			default:
				return 1.0f;
		}
	}

	const SpeedOverrides::Entry& MovementPatcher::OverrideFor(const SettingsData& a_settings, SpeedCategory a_category)
	{
		switch (a_category) {
			case SpeedCategory::kWalk:
				return a_settings.overrides.walk;
			case SpeedCategory::kRun:
				return a_settings.overrides.run;
			case SpeedCategory::kSprint:
				return a_settings.overrides.sprint;
			case SpeedCategory::kSneakWalk:
				return a_settings.overrides.sneakWalk;
			case SpeedCategory::kSneakRun:
				return a_settings.overrides.sneakRun;
			default:
				return a_settings.overrides.walk;
		}
	}

	std::size_t MovementPatcher::IndexFor(SpeedCategory a_category)
	{
		return static_cast<std::size_t>(a_category);
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

	std::string MovementPatcher::FormatPluginAndForm(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return "<invalid>";
		}

		const auto* file = a_form->GetFile(0);
		const std::string plugin = file ? std::string(file->GetFilename()) : std::string("<unknown>");
		return std::format("{}|0x{:08X}", plugin, a_form->GetFormID());
	}

	TargetPreview MovementPatcher::BuildPreviewInternal(const TargetEntry& a_target)
	{
		TargetPreview preview{};
		const auto resolved = ResolveTarget(a_target);
		if (!resolved.valid || !resolved.form) {
			return preview;
		}

		preview.resolved = true;
		preview.pluginAndForm = resolved.pluginAndForm;
		preview.displayName = resolved.name;
		return preview;
	}

	MovementPatcher::ResolvedTarget MovementPatcher::ResolveTarget(const TargetEntry& a_target)
	{
		ResolvedTarget out{};
		out.formKey = CanonicalTargetKey(a_target.form);
		if (out.formKey.empty()) {
			return out;
		}

		const auto separator = a_target.form.find('|');
		if (separator == std::string::npos) {
			return out;
		}

		const std::string plugin = Trim(a_target.form.substr(0, separator));
		const std::string idText = Trim(a_target.form.substr(separator + 1));
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
		out.pluginAndForm = FormatPluginAndForm(movementType);
		const char* editorID = movementType->GetFormEditorID();
		if (!a_target.name.empty()) {
			out.name = a_target.name;
		} else if (editorID && editorID[0] != '\0') {
			out.name = editorID;
		} else {
			out.name = out.pluginAndForm;
		}
		return out;
	}
}
