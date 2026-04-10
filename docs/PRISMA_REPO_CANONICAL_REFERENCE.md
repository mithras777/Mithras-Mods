# Prisma + Repo Canonical Reference

- Last verified date: 2026-04-06
- Canonical sources: Repo code/config under `C:\Modding\Mithras Mods`, official Prisma UI docs at `https://www.prismaui.dev/`
- Excluded sources: `extern/`, `build/`, `.bin/`, `.lib/`, `node_modules/`, packaged release copies under `Nexus Release/`, vendored `CommonLibSSE/` content, generated bundles for behavior when source files exist
- Coverage scope: Repo-wide SKSE conventions, current UI patterns, Prisma integration guidance, and first-party file atlas

## Table Of Contents

- [Purpose And Source-Of-Truth Rules](#purpose-and-source-of-truth-rules)
- [Repo Map](#repo-map)
- [Prisma UI Canonical Reference](#prisma-ui-canonical-reference)
- [Repo Architecture Conventions](#repo-architecture-conventions)
- [How To Build New Mods In This Repo](#how-to-build-new-mods-in-this-repo)
- [Known Implementation Examples](#known-implementation-examples)
- [File-By-File Atlas](#file-by-file-atlas)
- [Anti-Hallucination Rules](#anti-hallucination-rules)
- [Maintenance Workflow](#maintenance-workflow)

## Purpose And Source-Of-Truth Rules

This file is the canonical human-readable reference for how this workspace is structured and how Prisma UI may be used inside it. It is intended to stop future documentation drift and reduce architectural hallucination.

### Documentation Contract

- `Source: Repo` means the statement is directly grounded in local code or config.
- `Source: Official Prisma docs` means the statement is grounded in `prismaui.dev`.
- `Status: Established` means the statement is directly supported by source.
- `Status: Inferred` means the statement is a narrow synthesis from established code/docs and is called out as inference.
- `Status: Unknown` means the behavior is not established by repo code or official docs and must not be claimed as fact.

### Canonical vs Non-Canonical Material

- Canonical: local first-party sources, project build/config files, and official Prisma pages cited in this document.
- Non-canonical: generated bundles, packaged release copies, vendored dependencies, unofficial community claims, and assumptions not backed by code/docs.
- Rule: if source `src/*.jsx` exists for a bundled web asset, the source file is canonical for behavior and the bundle is only canonical for packaging/output shape.

## Repo Map

Source: Repo
Status: Established

- Workspace root contains a shared SKSE mod collection rather than a single project.
- Current top-level projects: `.vscode`, `AssassinationSKSE`, `ConfigurableSpeeds`, `DiagonalSprint`, `JumpAttackFix`, `MagicOrganizer`, `MordhauCombat`, `SpellBinding`.
- Each gameplay mod mostly follows the same pattern: project root build files, inner project folder, `include/`, `src/`, `pch/`, `resource/`, `cmake/`.
- Prisma UI currently appears only in `SpellBinding` under `SpellBinding\SpellBinding\web\SpellBinding` and the native bridge under `SpellBinding\SpellBinding\include\ui\PrismaBridge.h` / `src\ui\PrismaBridge.cpp`.
- Non-Prisma UI examples exist: `MagicOrganizer` uses native UI helpers; `MordhauCombat` exposes overlay/menu bridge code via `ui/SmfMenuBridge` and `ui/VectorOverlay`.

## Prisma UI Canonical Reference

### Official Pages Used

- https://www.prismaui.dev/getting-started/introduction/
- https://www.prismaui.dev/getting-started/installation/
- https://www.prismaui.dev/getting-started/basic-usage/
- https://www.prismaui.dev/getting-started/limitations/
- https://www.prismaui.dev/getting-started/planned-features/
- https://www.prismaui.dev/api/create-view/
- https://www.prismaui.dev/api/focus/
- https://www.prismaui.dev/api/invoke/
- https://www.prismaui.dev/api/register-js-listener/
- https://www.prismaui.dev/api/interop-call/
- https://www.prismaui.dev/api/has-any-active-focus/
- https://www.prismaui.dev/inspector-api/create-inspector-view/
- https://www.prismaui.dev/configuration/custom-cursor/
- https://www.prismaui.dev/configuration/focus-managements/
- https://www.prismaui.dev/guides/modern-frameworks/

### Capability Summary

- Source: Official Prisma docs
- Status: Established
- Prisma UI is an embedded WebKit-based Skyrim UI framework for HTML/CSS/JavaScript frameworks including React and Vue.
- `CreateView` is the primary entrypoint and may load a local HTML path under `Data/PrismaUI/views/` or an `http://` / `https://` URL.
- Official guidance recommends one `PrismaView` per plugin, with the web app managing its own internal screens and states.
- Bidirectional communication is provided through `Invoke`, `InteropCall`, and `RegisterJSListener`.
- Focus, cursor behavior, z-order, scrolling config, inspector support, and focus-menu integration are first-class parts of the model.

### Hard Limitations

- Source: Official Prisma docs
- Status: Established
- WebKit version: `615.1.18.100.1`.
- JavaScript support: ES2022 and below.
- `Video` is not supported.
- `Audio` is not supported; sounds must be played from SKSE/native code.
- `WebGL` is not supported.
- Rendering is capped at 60 FPS for now.
- Rendering is CPU-only for now.
- Tailwind guidance is pinned to v3, not newer major versions.
- Heavy CSS effects such as large-scale shadows, filters, gradients, and blur should be treated as performance risks.
- `contextmenu` requires a custom workaround using `mousedown` and synthetic dispatch.
- Blocking certain key input may require `keydown` + `beforeinput` capture listeners.

### Core API Usage Rules

- Source: Official Prisma docs + Repo
- Status: Established
- Use `CreateView` to create the view and wait for DOM readiness before sending data that depends on JS handlers existing.
- Use `RegisterJSListener` for JS -> native calls when JavaScript must trigger named native callbacks.
- Use `InteropCall` when native code wants to call a known JS function name with a payload string, which is how `SpellBinding` pushes snapshots and HUD updates.
- Use `Invoke` for executing arbitrary JS snippets or function expressions when that is the intended API pattern. Prefer `InteropCall` for stable named bridge calls because it is easier to reason about and document.
- Use `Show` / `Hide` for visibility control and `SetOrder` for overlay layering.
- Use `Focus` / `Unfocus` intentionally; focus also shows cursor and may optionally pause the game or suppress the focus overlay menu.
- Use `HasAnyActiveFocus` when coordinating multiple views or avoiding focus conflicts across plugins.

### Focus Lifecycle

- Source: Official Prisma docs + Repo
- Status: Established
- When a `PrismaView` gains focus, Prisma creates or uses the native `PrismaUI_FocusMenu` within Skyrim UI.
- The focus menu can be accessed through `RE::UI::GetSingleton()->GetMenu("PrismaUI_FocusMenu")` and its flags can be adjusted if needed.
- In this repo, `SpellBinding` uses `Show`, then `Focus`, then sends an updated UI snapshot; it also tracks focus state back into JS through `sbo_setFocusState`.
- Inference: any future Prisma mod here should make focus changes explicit and pair every focus acquisition with either `Unfocus` or full view closure logic.
- Status: Inferred

### React / Framework Guidance

- Source: Official Prisma docs + Repo
- Status: Established
- React/Vue apps can miss early native messages if handlers are registered inside component lifecycle hooks after native initialization has already fired.
- Handlers for native -> JS calls should exist on `window` early and be stable.
- In `SpellBinding`, `window.sbo_renderSnapshot`, `window.sbo_showToast`, `window.sbo_native_escape`, and related functions are installed once in the top-level effect of the main React app.
- Rule for this repo: if native code uses `InteropCall(view, "name", payload)`, document and create `window.name` in the canonical app entrypoint, not in a late-mounted child component.

### Repo-Specific Prisma Implementation Pattern

- Source: Repo
- Status: Established
- Native bridge files: `SpellBinding\SpellBinding\include\ui\PrismaBridge.h` and `src\ui\PrismaBridge.cpp`.
- Canonical web source files: `SpellBinding\SpellBinding\web\SpellBinding\src\app.jsx` and `src\hud.jsx`.
- HTML entrypoints: `index.html` for the main menu and `hud.html` for the overlay HUD.
- Build tool: `build.mjs` bundles the two React entrypoints into `app.js` and `hud.js` using esbuild.
- Packaged/runtime layout expectation: HTML and bundled assets are what Prisma loads; React source is canonical for authoring and behavior.

## Repo Architecture Conventions

- Source: Repo
- Status: Established
- Standard SKSE project shape: root build/config files, inner project folder named after the mod, `include/`, `src/`, `pch/`, `resource/`, and `cmake/`.
- `plugin.cpp` owns plugin bootstrap concerns: SKSE initialization, runtime checks, logging setup, and first hook installation.
- `api/skse_api.cpp` owns SKSE load/query/message registration and event-time subsystem startup.
- `hook/` contains low-level patches and input/runtime interception.
- `event/` contains event managers or sinks tied to game/menu/combat state changes.
- `input/` contains higher-level input sinks and hotkey routing.
- `util/` contains generic helpers reused across the mod.
- Manager classes are commonly singletons using `REX::Singleton<T>`.
- Serialization is centralized behind a small registration layer, then delegated into a gameplay manager.
- CMake structure is heavily templated and reused across projects, including bare/basic/menu scaffolds.
- UI bridges are separate from gameplay managers even when tightly integrated; `SpellBinding` is the strongest example of this separation.

### Boot Path Convention

- Source: Repo
- Status: Established
1. `plugin.cpp` initializes SKSE, logging, runtime checks, and base hooks.
2. `api/skse_api.cpp` registers for SKSE messages.
3. On `kDataLoaded` and related events, managers, input sinks, event sinks, serialization, and UI bridges are initialized.
4. UI bridges request initial snapshots only after relevant managers and state have been initialized.

### Serialization Convention

- Source: Repo
- Status: Established
- `SpellBinding\SpellBinding\src\serialization\Serialization.cpp` is the clearest example: register SKSE callbacks in one place, then delegate save/load/revert into a central manager.
- Rule: do not scatter raw `SetSaveCallback` / `SetLoadCallback` calls across subsystems; keep one registration point and one orchestrator.

## How To Build New Mods In This Repo

### Create A New Conventional SKSE Mod

- Source: Repo
- Status: Established
1. Copy or derive from the shared project shape already present in `AssassinationSKSE`, `ConfigurableSpeeds`, `DiagonalSprint`, `JumpAttackFix`, `MagicOrganizer`, `MordhauCombat`, or `SpellBinding`.
2. Keep root-level `CMakeLists.txt`, `CMakePresets.json`, and `CMake_Build.bat` aligned with the existing pattern.
3. Create the inner project folder with `include/`, `src/`, `pch/`, and `resource/`.
4. Implement `plugin.cpp` for bootstrap and `src/api/skse_api.cpp` for SKSE lifecycle registration.
5. Add manager, hook, event, and input files under their conventional folders rather than mixing responsibilities.
6. If the mod needs persistence, add `include/serialization/Serialization.h` and `src/serialization/Serialization.cpp` with a single registration point.
7. Keep UI separate from gameplay logic; if the UI is native, isolate it under `ui/`.

### Create A New Prisma-Powered Mod

- Source: Repo + Official Prisma docs
- Status: Established
1. Start from the same SKSE project shape as a conventional mod.
2. Add a native bridge under `include/ui/` and `src/ui/` that wraps Prisma API access, view creation, focus, hide/show, and JS/native bridge registration.
3. Add a frontend app under `YourMod\web\YourMod\` with `index.html`, optional `hud.html`, CSS, `src/*.jsx`, `package.json`, and `build.mjs`.
4. Treat the `src/*.jsx` files as behavioral source of truth; bundles are output artifacts only.
5. Use `CreateView` from `kDataLoaded` or a later safe init point after `RequestPluginAPI` succeeds.
6. Create one `PrismaView` per plugin unless there is a concrete reason not to; manage multiple screens in the web app itself.
7. Register named JS listeners for JS -> native events and named `window.*` bridge functions for native -> JS calls.
8. Push initial snapshots only after DOM readiness and bridge registration are in place.
9. Keep audio/video/WebGL assumptions out of the design; Prisma does not support them per official docs.

### Add A Native-To-Web Bridge

- Source: Repo + Official Prisma docs
- Status: Established
1. Define the bridge wrapper in `include/ui/<Bridge>.h`.
2. Request Prisma API once and cache it on the bridge singleton.
3. Create views with `CreateView` and set order/visibility explicitly.
4. Register JS listeners for UI-originated actions such as setting changes, button actions, and HUD commands.
5. Define stable `window.<name>` functions in the frontend entrypoint for native `InteropCall` targets.
6. Sanitize payloads before sending them into JS where appropriate.

### Add A HUD Overlay

- Source: Repo
- Status: Established
1. Create a separate HUD HTML entrypoint and source file if the HUD must render independently from the main panel.
2. Create a second Prisma view or a clearly separated overlay path only when the HUD has different focus/visibility requirements.
3. Set the HUD view order higher than the settings/control panel when needed.
4. Keep HUD bridge functions narrow and data-driven, as in `sbo_renderHud` plus small drag-related actions.

### Add JSON Snapshot Push Flow

- Source: Repo
- Status: Established
1. Centralize the authoritative snapshot builder in the relevant gameplay manager.
2. Have the UI bridge call `InteropCall(view, "namedHandler", payload)` with serialized JSON.
3. Parse payloads in the frontend entrypoint and update top-level state there.
4. Avoid distributing JSON parsing logic across many leaf components.

### Add Settings Persistence

- Source: Repo
- Status: Established
1. Store config structs on the gameplay manager or controller that owns the subsystem.
2. Add explicit `LoadConfig`, `SaveConfig`, and `ClampConfig` style helpers.
3. Route UI-originated settings changes through one native handler instead of direct mutation from many callbacks.
4. Include UI window geometry in explicit config structs when the app supports draggable/resizable panels.

### Add Save/Load Serialization

- Source: Repo
- Status: Established
1. Register unique serialization callbacks once during plugin load.
2. Delegate save/load/revert into the owning high-level manager.
3. Keep subsystem-specific serialization behind that manager boundary.

### Add Hotkeys And Input Handling

- Source: Repo
- Status: Established
1. Register an input sink from `kInputLoaded` and also from `kDataLoaded` if the mod currently follows that redundancy pattern.
2. Separate low-level hooks from higher-level input event sinks.
3. For Prisma apps, decide which keys are handled natively and which are forwarded into the web app.
4. For text-entry edge cases, use Prisma-documented input workarounds rather than assuming browser parity.

### Add Debug / Inspector Support

- Source: Official Prisma docs + Repo
- Status: Established / Inferred
- Established: Prisma exposes inspector API and focus/menu customization APIs.
- Established: the repo already has console command patterns and logging setup in most mods.
- Inferred: a future Prisma mod here should wire inspector visibility and console callback support behind debug flags in the bridge layer instead of mixing them into gameplay logic.

## Known Implementation Examples

- Source: Repo
- Status: Established
- Prisma example: `SpellBinding`
  - Native bridge: `SpellBinding\SpellBinding\include\ui\PrismaBridge.h`, `src\ui\PrismaBridge.cpp`
  - Frontend source: `SpellBinding\SpellBinding\web\SpellBinding\src\app.jsx`, `src\hud.jsx`
- Non-Prisma native UI example: `MagicOrganizer`
  - UI files: `MagicOrganizer\MagicOrganizer\include\ui\UI.h`, `src\ui\UI.cpp`
- Alternative overlay/menu bridge example: `MordhauCombat`
  - Bridge/overlay files: `MordhauCombat\MordhauCombat\include\ui\SmfMenuBridge.h`, `include\ui\VectorOverlay.h`, matching `src\ui\*.cpp`

## File-By-File Atlas

Source: Repo
Status: Established


### .vscode

- `.vscode\settings.json`: Workspace editor configuration. Source: Repo. Status: Established.

### AssassinationSKSE

- `AssassinationSKSE\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\game\Assassination.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\game\AssassinationSettings.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\ui\UI.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\game\Assassination.cpp`: Gameplay subsystem implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\game\AssassinationSettings.cpp`: Gameplay subsystem implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\AssassinationSKSE\src\ui\UI.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.
- `AssassinationSKSE\CHANGELOG.md`: Project release/change log. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `AssassinationSKSE\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `AssassinationSKSE\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `AssassinationSKSE\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `AssassinationSKSE\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `AssassinationSKSE\LICENSE`: License text for the project. Source: Repo. Status: Established.

### ConfigurableSpeeds

- `ConfigurableSpeeds\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `ConfigurableSpeeds\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `ConfigurableSpeeds\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `ConfigurableSpeeds\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `ConfigurableSpeeds\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\movement\MovementPatcher.h`: Movement/config subsystem header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\movement\Settings.h`: Movement/config subsystem header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\ui\MCM_Menu.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\movement\MovementPatcher.cpp`: Movement/config subsystem implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\movement\Settings.cpp`: Movement/config subsystem implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\ConfigurableSpeeds\src\ui\MCM_Menu.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.
- `ConfigurableSpeeds\LICENSE`: License text for the project. Source: Repo. Status: Established.

### DiagonalSprint

- `DiagonalSprint\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `DiagonalSprint\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `DiagonalSprint\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `DiagonalSprint\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `DiagonalSprint\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\diagonal\FakeDiagonalSprint.h`: First-party project file with a role implied by its subsystem path and filename. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\ui\UI.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\diagonal\FakeDiagonalSprint.cpp`: First-party project file with a role implied by its subsystem path and filename. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `DiagonalSprint\DiagonalSprint\src\ui\UI.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.
- `DiagonalSprint\LICENSE`: License text for the project. Source: Repo. Status: Established.

### JumpAttackFix

- `JumpAttackFix\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `JumpAttackFix\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `JumpAttackFix\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `JumpAttackFix\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `JumpAttackFix\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `JumpAttackFix\JumpAttackFix\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `JumpAttackFix\LICENSE`: License text for the project. Source: Repo. Status: Established.

### MagicOrganizer

- `MagicOrganizer\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `MagicOrganizer\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MagicOrganizer\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `MagicOrganizer\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `MagicOrganizer\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `MagicOrganizer\LICENSE`: License text for the project. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\input\InputEventSink.h`: Input sink and hotkey routing header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\magic\MagicOrganizerManager.h`: MagicOrganizer subsystem header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\ui\UI.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\input\InputEventSink.cpp`: Input sink or hotkey routing implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\magic\MagicOrganizerManager.cpp`: MagicOrganizer subsystem implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `MagicOrganizer\MagicOrganizer\src\ui\UI.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.

### MordhauCombat

- `MordhauCombat\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `MordhauCombat\AnimationMappings.md`: First-party project file with a role implied by its subsystem path and filename. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `MordhauCombat\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `MordhauCombat\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `MordhauCombat\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `MordhauCombat\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `MordhauCombat\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `MordhauCombat\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `MordhauCombat\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `MordhauCombat\LICENSE`: License text for the project. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\combat\DirectionalConfig.h`: Combat subsystem header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\combat\DirectionalController.h`: Combat subsystem header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\combat\DirectionalTypes.h`: Combat subsystem header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\event\AttackAnimationEventSink.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\input\InputEventSink.h`: Input sink and hotkey routing header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\oar\OarConditionBridge.h`: Open Animation Replacer bridge/condition header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\oar\OarConditions.h`: Open Animation Replacer bridge/condition header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\oar\OpenAnimationReplacerAPI-Conditions.h`: Open Animation Replacer bridge/condition header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\oar\OpenAnimationReplacer-ConditionTypes.h`: Open Animation Replacer bridge/condition header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\oar\OpenAnimationReplacer-SharedTypes.h`: Open Animation Replacer bridge/condition header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\ui\SmfMenuBridge.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\ui\VectorOverlay.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\combat\DirectionalConfig.cpp`: Combat subsystem implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\combat\DirectionalController.cpp`: Combat subsystem implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\event\AttackAnimationEventSink.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\input\InputEventSink.cpp`: Input sink or hotkey routing implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\oar\OarConditionBridge.cpp`: Open Animation Replacer bridge/condition implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\oar\OarConditions.cpp`: Open Animation Replacer bridge/condition implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\oar\OpenAnimationReplacerAPI-Conditions.cpp`: Open Animation Replacer bridge/condition implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\oar\OpenAnimationReplacer-ConditionTypes.cpp`: Open Animation Replacer bridge/condition implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\ui\SmfMenuBridge.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.
- `MordhauCombat\MordhauCombat\src\ui\VectorOverlay.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.

### README.md

- `README.md`: Workspace-level overview. Source: Repo. Status: Established.

### SpellBinding

- `SpellBinding\.gitignore`: Ignore rules for generated or local-only files. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\build.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\compatibility.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\configuration.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\hooking.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\intro.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\started.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\structure.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\content\styles.css`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\documentation\readme.html`: Static scaffold documentation asset. Source: Repo. Status: Established.
- `SpellBinding\cmake\project_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule\imgui_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule\nlohmann-json_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule\simpleini_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule\toml11_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule\xbyak_config.cmake`: Dependency configuration for the project CMake build. Source: Repo. Status: Established.
- `SpellBinding\cmake\submodule_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\bare\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\event\GameEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\game\GameHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\hook\FunctionHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\event\GameEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\hook\FunctionHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\basic\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\api\skse_api.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\api\versionlibdb.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\console\ConsoleCommands.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\event\InputEventManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\hook\GraphicsHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\hook\InputHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\hook\MainHook.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\plugin.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\ImguiHelper.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\ConsoleMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\fonts\fa-solid-900.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\fonts\IconsFontAwesome6Menu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\IMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\MainMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\Menus.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\menu\PlayerMenu.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\ui\UIManager.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\util\HookUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\util\KeyboardUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\util\LogUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\include\util\StringUtil.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\pch\PCH.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\pch\PCH.h`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\resource\plugin.rc`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\api\skse_api.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\console\ConsoleCommands.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\dllmain.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\event\InputEventManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\hook\GraphicsHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\hook\InputHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\hook\MainHook.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\plugin.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\ImguiHelper.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\menu\ConsoleMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\menu\IMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\menu\MainMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\menu\PlayerMenu.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\menu\src\ui\UIManager.cpp`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\templates\version.h.in`: Reusable scaffold/template file for generating SKSE projects. Source: Repo. Status: Established.
- `SpellBinding\cmake\utility\fetch_helpers.cmake`: Shared CMake utility helper. Source: Repo. Status: Established.
- `SpellBinding\cmake\utility_config.cmake`: Project-level CMake configuration module. Source: Repo. Status: Established.
- `SpellBinding\CMake_Build.bat`: Batch wrapper for local builds. Source: Repo. Status: Established.
- `SpellBinding\CMakeLists.txt`: Top-level CMake entry for this project. Source: Repo. Status: Established.
- `SpellBinding\CMakePresets.json`: Shared CMake configure/build presets. Source: Repo. Status: Established.
- `SpellBinding\LICENSE`: License text for the project. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\api\skse_api.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\api\versionlibdb.h`: SKSE/API-facing header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\buff\QuickBuffManager.h`: QuickBuff subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\console\ConsoleCommands.h`: Console command interface header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\event\AttackAnimationEventSink.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\event\EquipEventSink.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\event\GameEventManager.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\event\mastery\MasteryEventSinks.h`: Event sink or event manager header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\game\GameHelper.h`: Gameplay helper or subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\hook\FunctionHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\hook\InputHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\hook\MainHook.h`: Hook declaration header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\input\InputEventSink.h`: Input sink and hotkey routing header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_shout\BonusApplier.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_shout\MasteryData.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_shout\MasteryManager.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_spell\BonusApplier.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_spell\MasteryData.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_spell\MasteryManager.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_weapon\BonusApplier.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_weapon\MasteryData.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\mastery_weapon\MasteryManager.h`: Mastery subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\overhaul\SpellbladeOverhaulManager.h`: High-level orchestrator header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\plugin.h`: Plugin-wide metadata and shared declarations. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\serialization\Serialization.h`: Serialization interface header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\smartcast\SmartCastController.h`: SmartCast subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\spellbinding\SpellBindingManager.h`: SpellBinding subsystem header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\ui\PrismaBridge.h`: UI or UI-bridge header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\util\HookUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\util\LogUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\util\NotificationCompat.h`: Shared utility header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\util\StringUtil.h`: Shared utility header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\include\version.h`: Version metadata header. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\pch\PCH.cpp`: Precompiled-header implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\pch\PCH.h`: Precompiled-header declaration. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\resource\plugin.rc`: Windows resource script for the plugin DLL. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\api\skse_api.cpp`: SKSE load/query/message implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\buff\QuickBuffManager.cpp`: QuickBuff runtime/persistence implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\console\ConsoleCommands.cpp`: Console command implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\dllmain.cpp`: DLL entrypoint implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\event\AttackAnimationEventSink.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\event\EquipEventSink.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\event\GameEventManager.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\event\mastery\MasteryEventSinks.cpp`: Event registration or sink implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\hook\FunctionHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\hook\InputHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\hook\MainHook.cpp`: Hook implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\input\InputEventSink.cpp`: Input sink or hotkey routing implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_shout\BonusApplier.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_shout\MasteryManager.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_spell\BonusApplier.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_spell\MasteryManager.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_weapon\BonusApplier.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\mastery_weapon\MasteryManager.cpp`: Mastery subsystem implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\overhaul\SpellbladeOverhaulManager.cpp`: High-level Spellblade Overhaul orchestration implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\plugin.cpp`: Plugin bootstrap implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\serialization\Serialization.cpp`: SKSE serialization callback registration implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\smartcast\SmartCastController.cpp`: SmartCast runtime/persistence implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\spellbinding\SpellBindingManager.cpp`: SpellBinding runtime/persistence implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\src\ui\PrismaBridge.cpp`: UI or UI-bridge implementation. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\app.js`: Generated Prisma bundle output; packaging artifact, not canonical behavior source. Source: Repo (generated output; non-canonical for behavior). Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\build.mjs`: esbuild bundling script for the Prisma app. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\hud.css`: Prisma stylesheet asset. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\hud.html`: Prisma HTML entrypoint loaded by the native bridge. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\hud.js`: Generated Prisma bundle output; packaging artifact, not canonical behavior source. Source: Repo (generated output; non-canonical for behavior). Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\index.html`: Prisma HTML entrypoint loaded by the native bridge. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\package.json`: Canonical frontend package manifest. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\package-lock.json`: Frontend dependency lockfile. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\src\app.jsx`: Canonical Prisma frontend source file. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\src\hud.jsx`: Canonical Prisma frontend source file. Source: Repo. Status: Established.
- `SpellBinding\SpellBinding\web\SpellBinding\styles.css`: Prisma stylesheet asset. Source: Repo. Status: Established.

### Template.7z

- `Template.7z`: Archived template payload kept in the workspace root. Source: Repo. Status: Established.

## Anti-Hallucination Rules

- Never claim Prisma supports video, audio, WebGL, GPU rendering, or a full browser-equivalent feature set unless official docs explicitly establish it.
- Never treat generated bundles, packaged release copies, or vendored dependencies as the source of truth for behavior when source files exist.
- Never cite community memory as canonical. If a claim is not in this repo or official Prisma docs, mark it `Status: Unknown`.
- When documenting a bridge call, point to both sides: the native caller and the JS `window.*` handler.
- When documenting a new subsystem, describe the owner manager, persistence path, input path, and UI path separately.
- If future code diverges from this document, update the document rather than layering oral tradition on top of stale text.

## Maintenance Workflow

- When code changes, update the relevant cookbook section and the affected atlas entries in the same change.
- When Prisma docs are re-checked, update the `Last verified date` and revise only claims supported by current official pages.
- If a new mod adopts Prisma, add it under `Known Implementation Examples` and document its bridge/frontend files in the atlas.
- If a directory becomes generated, packaged, or vendored, move it into the non-canonical exclusion list before documenting it as architecture.
- If there is any uncertainty, add an explicit `Status: Unknown` note instead of filling the gap with assumptions.
