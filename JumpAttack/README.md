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

Sample INI is included at:
- `JumpAttack/JumpAttacks.ini`

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
