# Mithras Mods â€“ SKSE Plugin Development Guide

This folder contains Skyrim SE/AE SKSE plugins built from the **SkyrimSE-Plugin-Template**. The guide below covers the workflow, where to find game APIs, and how to add the SKSE Menu Framework.

---

## 1. Template and Workflow

### Source

- Start from **SkyrimSE-Plugin-Template-main.zip**: unzip it and use the folder that contains `CMake_Build.bat`.

### First-time setup (new mod, dependency-reuse flow)

1. Keep at least one working mod folder that already has dependencies prepared, including:
   - `SpellMastery/SpellMastery/extern/CommonLibSSE`
   - `SpellMastery/SpellMastery/extern/CommonLibSSE/extern/openvr`
   - `SpellMastery/SpellMastery/extern/CommonLibSSE/extern/spdlog`
   - `SpellMastery/SpellMastery/extern/CommonLibSSE/extern/DirectXTK`
   - `SpellMastery/SpellMastery/extern/SKSEMenuFramework` (if the mod needs menu UI)
2. Unzip template and run **`CMake_Build.bat`**.
3. Choose **2 (alandtse / NG)**.
4. Enter project metadata (name, author, template).
5. When asked **`Configure project now? (will download dependencies if missing)`**, choose **`N`**.
6. Copy dependency folders into the new project `extern/` from `SpellMastery/SpellMastery/extern/`.
7. Run configure manually from the new mod root:
   - `cmake --preset vs2022-ng`
8. Build manually:
   - `cmake --build --preset vs2022-ng-rel`
9. Keep `SpellMastery` in your workspace as your dependency source project for future mod creation.

Resulting layout:

```
ModName/                    â† renamed from SkyrimSE-Plugin-Template-main
â”œâ”€â”€ CMake_Build.bat
â”œâ”€â”€ CMakeLists.txt
â”œâ”€â”€ cmake/
â””â”€â”€ ModName/                â† project folder (scripts, sources)
    â”œâ”€â”€ include/
    â”œâ”€â”€ src/
    â”œâ”€â”€ pch/
    â”œâ”€â”€ resource/
    â””â”€â”€ extern/             â† CommonLibSSE etc. go here (created by CMake)
```

### Daily workflow

- **Edit or add** code under `ModName/ModName/` (e.g. `include/`, `src/`).
- **Run `CMake_Build.bat`** again to configure (if needed) and build. Output DLL/PDB goes to `.bin/x64-release` or `.bin/x64-debug` at the mod root (same level as `CMake_Build.bat`).
- Copy the built DLL (and PDB if debugging) into `Skyrim SE/Data/SKSE/Plugins/` for testing.

---

## 2. Where to Find Functions and Hooks (RE / CommonLibSSE)

### Location of headers

After the first successful configure/build, CommonLibSSE is fetched into your projectâ€™s **extern** folder. All game/RE and SKSE headers live there:

| What you need | Path (relative to mod root) |
|---------------|------------------------------|
| RE (game) API | `ModName/extern/CommonLibSSE/include/RE/` |
| RE subfolders | `ModName/extern/CommonLibSSE/include/RE/A/`, `RE/B/`, `RE/P/`, etc. |
| Single include | `ModName/extern/CommonLibSSE/include/RE/Skyrim.h` |
| SKSE API | `ModName/extern/CommonLibSSE/include/SKSE/` |
| PROXY (NG compatibility) | `ModName/extern/CommonLibSSE/include/PROXY/` |

Example: in **WeaponMastery**, `WeaponMastery/extern/CommonLibSSE/include/RE/` holds the full RE tree (Actor, PlayerCharacter, TESForm, etc.). Use that tree as the reference for types and function signatures.

### Template patch (NG builds)

For the **NG (alandtse)** variant, the template can copy extra patches into CommonLibSSE. Those patches live under the **template** repo, not under your mod:

- **Path in template:** `cmake/submodule/CommonLibSSE/patch/`
- Contents include PROXY headers and any patched RE headers. They are copied into the fetched CommonLibSSE during configure. You donâ€™t need to edit these for normal plugin code.

### How to use them (match WeaponMastery style)

- **PCH:** In your precompiled header (e.g. `ModName/pch/PCH.h`), include once:
  - `#include <RE/Skyrim.h>`
  - `#include <SKSE/SKSE.h>`
  - If using NG proxies: `#include <PROXY/Proxies.h>`
- **Includes in .cpp:** Prefer the same style as in **WeaponMastery**: include via the PCH or use specific RE headers (e.g. `#include "RE/P/PlayerCharacter.h"`) and the same REL/RELOCATION_ID and hook patterns as in WeaponMastery.
- **Hooks / addresses:** Use the same patterns as in `WeaponMastery/WeaponMastery` (e.g. `RELOCATION_ID`, `REL::`, and the hook utilities in `include/util/` and `include/hook/`). Keeping the same formatting and include style avoids subtle ABI/header mismatches.

If a type or function is missing in your modâ€™s `extern`, compare with **WeaponMastery**â€™s `extern/CommonLibSSE/include/RE/` (and its docs, e.g. `extern/CommonLibSSE/docs/VERIFICATION_MIGRATION.md` for patch verification).

---

## 3. SKSE Menu Framework

To use **SKSE Menu Framework** in your plugin:

### Get the headers

**Option A â€“ Reusable copy at repo root (recommended)**  
Keep a single copy at the **Mithras Mods** root so you can add it to any project:

- **Location:** `Mithras Mods/External Libraries to Copy into extern/` (contents of the example repoâ€™s `include` folder, e.g. `SKSEMenuFramework.h`).
- **Use in a project:** Copy this folder into the modâ€™s extern, e.g. `ModName/extern/SKSEMenuFramework/`, or add the root path to the projectâ€™s include path in CMake so all mods share the same copy.

**Option B â€“ Perâ€‘project setup**  
1. Clone or download the example repo:  
   **https://github.com/Thiago099/SKSE-Menu-Framework-2-Example**
2. Open the **`include`** folder and copy its contents into your modâ€™s **extern** with a clear name (no repo suffix), e.g. `ModName/extern/SKSEMenuFramework/`.

So you end up with either:

- `Mithras Mods/SKSEMenuFramework/SKSEMenuFramework.h` (shared), **or**
- `ModName/extern/SKSEMenuFramework/SKSEMenuFramework.h` (perâ€‘project)

### Use it in your code

- Add the framework include path to your target if itâ€™s not already under `extern` (in this setup, `target_include_directories` already has `PROJECT_EXTERN_ROOT_DIR`, so `extern/SKSEMenuFramework` is enough).
- In your plugin code (e.g. in a source or a header that builds with your plugin), include the framework:
  - `#include "SKSEMenuFramework.h"`  
  if the file is at `extern/SKSEMenuFramework/SKSEMenuFramework.h`, or
  - `#include "SKSEMenuFramework/SKSEMenuFramework.h"`  
  if you prefer a subfolder name in the path.

Follow the rest of the frameworkâ€™s docs (registration, menu callbacks, etc.) as in the example repo.

---

## 4. Configuration File Location

Mod config files should be saved next to the DLL for portability and better distribution.

### Setting Up Portable Config Location

In your config manager (e.g., `MasteryManager.cpp`), implement the `GetConfigPath()` method:

```cpp
std::filesystem::path Manager::GetConfigPath() const
{
	// Get the DLL directory and place config next to the DLL
	wchar_t dllPath[MAX_PATH];
	GetModuleFileNameW(GetModuleHandleW(L"YourModName.dll"), dllPath, MAX_PATH);
	std::filesystem::path dllDir = std::filesystem::path(dllPath).parent_path();
	return dllDir / "YourModName.json";
}
```

**Benefits:**
- âœ… Config files stay with your mod files
- âœ… Easier distribution and backups
- âœ… No conflicts between different mod installations
- âœ… Users can keep multiple config versions

---

## 4.5. Save Data Behavior

**Important:** Mod save data is stored **per save game file**, not globally across all playthroughs.

### Save Data vs Config Data

**Save Data (SKSE Serialization):**
- Stored in each Skyrim `.ess` save file
- Contains: mastery levels, kill counts, equipped time
- **Per-playthrough**: Loading an older save restores progress as it was when saved
- If you load a save from before gaining mastery, you won't have those levels

**Config Data (JSON file):**
- Stored next to DLL (portable)
- Contains: settings, multipliers, thresholds, enabled weapons
- **Global**: Same config applies to all saves/playthroughs

### Examples:
- Save from Day 1: No mastery data â†’ fresh start
- Save from Day 30: Full mastery progress â†’ loads with all progress
- New game: No mastery data â†’ fresh start (even if other saves have progress)

This is standard SKSE behavior. Players can backup/modify individual save files as needed.

---

## 5. Unified Settings Pattern (SKSE Menu + JSON + Internal Tabs)

Use this pattern for all Mithras mods (Kick, WeaponMastery, SpellMastery, ShoutMastery) to keep UX and config behavior consistent.

### SKSE Menu layout

Register a single page named `Settings`, then render internal tabs in one panel.

```cpp
void Register()
{
	SKSEMenuFramework::SetSection("Your Mod Name");
	SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
}

void __stdcall MainPanel::Render()
{
	if (ImGui::BeginTabBar("YourModTabs")) {
		if (ImGui::BeginTabItem("General")) { /* ... */ ImGui::EndTabItem(); }
		if (ImGui::BeginTabItem("Progression")) { /* ... */ ImGui::EndTabItem(); }
		if (ImGui::BeginTabItem("Bonuses")) { /* ... */ ImGui::EndTabItem(); }
		if (ImGui::BeginTabItem("Debug")) { /* ... */ ImGui::EndTabItem(); }
		ImGui::EndTabBar();
	}
}
```

### Recommended tab conventions

- Keep global toggles in `General`.
- Put tunables in collapsed `Settings` blocks inside feature tabs when appropriate.
- Add `Defaults` buttons per feature tab (not required for global/general tab).
- Keep `Debug` tools isolated in `Debug`.

### JSON schema policy (important)

For this repo, use a strict schema. Do not add legacy fallback key reads unless explicitly requested.

- Good: `a_json.value("npcForce", a_config.npcForce)`
- Avoid: nested fallback chains such as reading old keys first
  - Example to avoid: `a_json.value("newKey", a_json.value("oldKey", ...))`

### JSON save/load workflow

1. Define `ToJson(const Config&)` and `FromJson(const json&, Config&)`.
2. Clamp values in one place (`ClampConfig` or equivalent).
3. On missing config file: write defaults.
4. On parse error: reset to defaults and rewrite config.
5. Save only the current schema keys.

### Runtime update pattern

- Read config once at panel start.
- Edit a local copy in UI.
- Compare `before` vs `after`.
- Call `SetConfig(updated, true)` only if changed.

This keeps JSON writes clean and avoids unnecessary file churn.

---

## 6. Building Your Mods

### Prerequisites
- Visual Studio 2022 with C++ workload
- Git
- CMake 3.24+

### Dependency Reuse (Required Workflow)

For this repo workflow, do not rely on a fresh dependency download for every new project.

1. Use `SpellMastery/SpellMastery/extern/` as the source of truth.
2. Create new mod with `CMake_Build.bat`, answer `N` on initial configure.
3. Copy needed extern folders from `SpellMastery`.
4. Run:
   - `cmake --preset vs2022-ng`
   - `cmake --build --preset vs2022-ng-rel`

If you skip this and let the batch auto-configure from empty extern, CMake will re-fetch `openvr` and other dependencies.

### Build Commands
```bash
# Configure and build Release
cmake --preset vs2022-ng
cmake --build --preset vs2022-ng-rel

# Or use the interactive batch script
./CMake_Build.bat
```

### Available Build Presets
- `vs2022-ng-rel` - NG variant, RelWithDebInfo (recommended for release)
- `vs2022-ng-debug` - NG variant, Debug (for development)
- `vs2022-po3-rel` - PowerOf3 variant, RelWithDebInfo
- `vs2022-po3-debug` - PowerOf3 variant, Debug

### Output Location
Built DLLs are placed in `.bin/x64-release/` or `.bin/x64-debug/` at the project root.

---

## 7. Quick Reference

| Task | Action |
|------|--------|
| New mod from scratch | Run `CMake_Build.bat` -> choose NG (2) -> answer `N` for initial configure -> copy `extern/` from `SpellMastery/SpellMastery/extern` -> run `cmake --preset vs2022-ng` and `cmake --build --preset vs2022-ng-rel`. |
| Edit plugin code | Edit under `ModName/ModName/include/` and `ModName/ModName/src/`. |
| Rebuild | Run `CMake_Build.bat`; choose build type (e.g. RelWithDebInfo). |
| Output | `.bin/x64-release/ModName.dll` (or `x64-debug` for debug). |
| RE / game API | `ModName/extern/CommonLibSSE/include/RE/` (use same style as WeaponMastery). |
| SKSE Menu Framework | Keep `SKSEMenuFramework/` at **Mithras Mods** root for reuse; copy into `ModName/extern/` (or add root to includes). In code: `#include "SKSEMenuFramework/SKSEMenuFramework.h"`. |

Always use **NG (alandtse)** when prompted so the same build works on all Skyrim versions.
