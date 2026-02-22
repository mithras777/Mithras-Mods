# JumpAttack

SKSE plugin for Skyrim SE/AE (CommonLibSSE NG) that enables mid-air melee jump hits in first and third person without behavior edits.

## Build (VS2022)

1. Configure:
   - `cmake --preset vs2022-ng`
2. Build release:
   - `cmake --build --preset vs2022-ng-rel`

Output DLL:
- `JumpAttack/.bin/x64-release/JumpAttack.dll`

## Install

1. Copy DLL to:
   - `Data/SKSE/Plugins/JumpAttack.dll`
2. JSON settings file (auto-created and used at runtime):
   - `Data/SKSE/Plugins/JumpAttack.json`

## SKSE Menu Framework + JSON

- If `SKSEMenuFramework` is installed, the mod registers a `Jump Attacks` section.
- In-game changes save immediately to `JumpAttack.json`.
- Runtime config is JSON-only (no INI/backward compatibility).

## JSON keys

- `enable`
- `range`
- `raysCount`
- `coneAngleDegrees`
- `swingDelayMs`
- `cooldownMs`
- `allowThroughActorsOnly`
- `allowIfBowDrawn`
- `allowIfSpellEquipped`
- `applyStagger`
- `staggerMagnitude`
- `landingGraceMs`
- `debugLog`
