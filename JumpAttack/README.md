# JumpAttacksNoBehaviors

SKSE plugin for Skyrim SE/AE (CommonLibSSE NG) that enables mid-air melee jump hits in first and third person without FNIS/Nemesis behavior edits.

## Build (VS2022)

1. Configure:
   - `cmake --preset vs2022-ng`
2. Build release:
   - `cmake --build --preset vs2022-ng-rel`

Output DLL:
- `JumpAttack/.bin/x64-release/JumpAttacksNoBehaviors.dll`

## Install

1. Copy DLL to:
   - `Data/SKSE/Plugins/JumpAttacksNoBehaviors.dll`
2. Copy INI to:
   - `Data/SKSE/Plugins/JumpAttacks.ini`
3. (Optional, auto-created) JSON settings:
   - `Data/SKSE/Plugins/JumpAttacksNoBehaviors.json`

Sample INI is included at:
- `JumpAttack/JumpAttacks.ini`

## SKSE Menu + JSON

- If `SKSEMenuFramework` is installed, the mod registers a `Jump Attacks` section.
- In-game changes are saved to:
  - `Data/SKSE/Plugins/JumpAttacksNoBehaviors.json`
- Load order for settings:
  1. Built-in defaults
  2. INI defaults (`JumpAttacks.ini`)
  3. JSON overrides (`JumpAttacksNoBehaviors.json`)

## INI keys

- `enable`
- `range`
- `raysCount`
- `coneAngleDegrees`
- `swingDelayMs`
- `cooldownMs`
- `allowThroughActorsOnly`
- `allowIfBowDrawn`
- `allowIfSpellEquipped`
- `debugLog`

Extra optional keys:
- `applyStagger`
- `staggerMagnitude`
- `landingGraceMs`
