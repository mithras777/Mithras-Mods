#include "ui/UI.h"

#include "automcm/ProfileStore.h"
#include "automcm/Runtime.h"
#include "util/LogUtil.h"

#include "SKSEMenuFramework/SKSEMenuFramework.h"

namespace
{
	std::string GetBackendLabel(AUTOMCM::Backend a_backend)
	{
		return a_backend == AUTOMCM::Backend::kMCMHelper ? "MCM Helper" : "SkyUI";
	}

	std::string GetControlLabel(const AUTOMCM::Entry& a_entry)
	{
		if (a_entry.backend == AUTOMCM::Backend::kMCMHelper) {
			return "Setting";
		}

		if (a_entry.optionType == "toggle") {
			return "Toggle";
		}
		if (a_entry.optionType == "slider") {
			return "Slider";
		}
		if (a_entry.optionType == "menu") {
			return "Menu";
		}
		if (a_entry.optionType == "color") {
			return "Color";
		}
		if (a_entry.optionType == "keymap") {
			return "Keymap";
		}
		if (a_entry.optionType == "input") {
			return "Input";
		}
		if (a_entry.optionType == "text") {
			return "Text";
		}
		if (a_entry.optionType == "clickable") {
			return "Action";
		}
		return a_entry.optionType.empty() ? "Unknown" : a_entry.optionType;
	}

	std::string GetTargetLabel(const AUTOMCM::Entry& a_entry)
	{
		if (!a_entry.selector.empty()) {
			return a_entry.selector;
		}
		if (!a_entry.stateName.empty()) {
			return a_entry.stateName;
		}
		if (!a_entry.settingId.empty()) {
			return a_entry.settingId;
		}
		if (a_entry.optionId >= 0) {
			return std::format("Option {}", a_entry.optionId);
		}
		return "Unknown";
	}

	std::string GetLocationLabel(const AUTOMCM::Entry& a_entry)
	{
		if (a_entry.backend == AUTOMCM::Backend::kMCMHelper) {
			return "-";
		}
		return a_entry.pageName.empty() ? "Default" : a_entry.pageName;
	}

	std::string FormatValue(const AUTOMCM::Entry& a_entry)
	{
		const auto& value = a_entry.value;
		switch (value.type) {
		case AUTOMCM::ValueType::kBool:
			return value.boolValue ? "ON" : "OFF";
		case AUTOMCM::ValueType::kInt:
			if (a_entry.optionType == "toggle") {
				return value.intValue != 0 ? "ON" : "OFF";
			}
			if (a_entry.optionType == "color") {
				return std::format("#{0:06X}", static_cast<std::uint32_t>(value.intValue) & 0xFFFFFF);
			}
			return std::to_string(value.intValue);
		case AUTOMCM::ValueType::kFloat:
			return std::format("{:.1f}", value.floatValue);
		case AUTOMCM::ValueType::kString:
			return value.stringValue.empty() ? "(empty)" : value.stringValue;
		default:
			return "-";
		}
	}
}

namespace UI
{
	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("AutoMCM: SKSE Menu Framework not installed - UI unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("AutoMCM");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("AutoMCM: registered SKSE Menu Framework panel");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			const auto status = AUTOMCM::Runtime::GetSingleton().GetStatus();
			auto desiredEntries = AUTOMCM::ProfileStore::GetSingleton().GetDesiredEntries();
			std::ranges::sort(desiredEntries, [](const AUTOMCM::Entry& a_lhs, const AUTOMCM::Entry& a_rhs) {
				if (a_lhs.modName != a_rhs.modName) {
					return a_lhs.modName < a_rhs.modName;
				}
				if (a_lhs.pageName != a_rhs.pageName) {
					return a_lhs.pageName < a_rhs.pageName;
				}
				if (a_lhs.optionType != a_rhs.optionType) {
					return a_lhs.optionType < a_rhs.optionType;
				}
				return GetTargetLabel(a_lhs) < GetTargetLabel(a_rhs);
			});

			if (ImGui::BeginTabBar("AutoMCMTabs")) {
				if (ImGui::BeginTabItem("General")) {
					ImGui::TextWrapped("AutoMCM learns MCM changes you make and reapplies them on future loads.");
					ImGui::Separator();
					ImGui::Text("Last trigger: %s", status.lastReason.empty() ? "n/a" : status.lastReason.c_str());
					ImGui::Text("Last apply: %d success / %d failure", status.lastApplySuccessCount, status.lastApplyFailureCount);
					if (!status.lastError.empty()) {
						ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", status.lastError.c_str());
					}
					ImGui::BeginDisabled(AUTOMCM::Runtime::GetSingleton().IsApplying() || AUTOMCM::Runtime::GetSingleton().IsResetting());
					if (ImGui::Button("Apply saved MCM settings now")) {
						AUTOMCM::Runtime::GetSingleton().ForceApplyNow();
					}
					if (ImGui::Button("Reset all MCM settings")) {
						AUTOMCM::Runtime::GetSingleton().ResetAll();
					}
					ImGui::EndDisabled();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Recorded")) {
					ImGui::Text("Recorded changes: %d", static_cast<int>(desiredEntries.size()));
					ImGui::Separator();

					if (desiredEntries.empty()) {
						ImGui::TextDisabled("No saved MCM changes recorded yet.");
					} else if (ImGui::BeginTable("AutoMCMRecordedTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, 360.0f))) {
						ImGui::TableSetupColumn("Mod");
						ImGui::TableSetupColumn("Backend");
						ImGui::TableSetupColumn("Control");
						ImGui::TableSetupColumn("Target");
						ImGui::TableSetupColumn("Page");
						ImGui::TableSetupColumn("Value");
						ImGui::TableHeadersRow();

						for (const auto& entry : desiredEntries) {
							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex(0);
							ImGui::TextUnformatted(entry.modName.empty() ? "Unknown" : entry.modName.c_str());

							ImGui::TableSetColumnIndex(1);
							const auto backend = GetBackendLabel(entry.backend);
							ImGui::TextUnformatted(backend.c_str());

							ImGui::TableSetColumnIndex(2);
							const auto control = GetControlLabel(entry);
							ImGui::TextUnformatted(control.c_str());

							ImGui::TableSetColumnIndex(3);
							const auto target = GetTargetLabel(entry);
							ImGui::TextUnformatted(target.c_str());

							ImGui::TableSetColumnIndex(4);
							const auto page = GetLocationLabel(entry);
							ImGui::TextUnformatted(page.c_str());

							ImGui::TableSetColumnIndex(5);
							const auto value = FormatValue(entry);
							ImGui::TextUnformatted(value.c_str());
						}

						ImGui::EndTable();
					}

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
	}
}
