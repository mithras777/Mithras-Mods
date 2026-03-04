# Workflow

## New mod setup

1. Unzip the template and run `CMake_Build.bat`.
2. Choose `2` (alandtse / NG).
3. Enter project metadata and template type.
4. The script auto-copies shared dependencies from:
   - `SpellBinding/SpellBinding/extern/` to `<NewMod>/<NewMod>/extern/`
5. The script then automatically runs:
   - `cmake --preset vs2022-ng` (or selected preset)
   - `cmake --build --preset vs2022-ng-rel` (matching `-rel` preset)

After that, the project is configured and release-built once.

## Day-to-day

1. Edit code under `ModName/ModName/include/` and `ModName/ModName/src/`.
2. Rebuild release:
   - `cmake --build --preset vs2022-ng-rel`

Build outputs are in `.bin/x64-release/`.

# Where To Find Functions/Hooks

- RE/game API headers:
  - `ModName/extern/CommonLibSSE/include/RE/`
- SKSE headers:
  - `ModName/extern/CommonLibSSE/include/SKSE/`
- NG proxy headers:
  - `ModName/extern/CommonLibSSE/include/PROXY/`

Use your project `extern/CommonLibSSE/include` tree as the source of truth for available types and function signatures.

# Save Data Behavior

- SKSE serialization (cosave):
  - Per save game (`.ess`)
  - Used for playthrough/runtime progression data
- JSON config:
  - Global file-based settings
  - Shared across saves unless changed

Loading an older save restores the serialized state from that save, not the latest runtime state from another save.
