# SpellBinding

## Summary
SpellBinding is a large modular SKSE spellblade framework that combines weapon-bound spell triggers, mastery progression systems, auto-buff automation, chain casting, and a Prisma-based UI/HUD layer.

This project is not a single mechanic mod. It is a gameplay platform composed of coordinated subsystems.

## Core Modules

### 1. Weapon-to-spell binding system
- Binds spells per weapon context and per trigger slot:
- Light attack
- Power attack
- Bash
- Supports right hand, left hand, and unarmed binding contexts.
- Per-weapon profile settings include:
- Trigger cooldown
- Combat-only trigger option
- Maintains runtime cooldown maps and trigger deduplication logic.
- Includes blacklist support to prevent specific spells from being auto-triggered.

### 2. Runtime event-driven casting
- Integrates with attack animation and equip/menu state events.
- Resolves attack slot from incoming animation tags/payload.
- Applies context guards (combat state, mounted state, dialogue, killmove, etc.).
- Handles pending-light timing windows and power-input timing.

### 3. Config + serialization architecture
- Shared JSON config path currently points to `Data/SKSE/Plugins/SpellbladeOverhaul.json`.
- Versioned config and co-save serialization for bound profiles.
- Supports runtime updates from UI JSON payloads:
- Global settings
- Per-weapon settings
- Blacklist updates
- UI window placement data

### 4. UI/HUD integration (Prisma bridge)
- UI toggle hotkey and bind hotkey support.
- Live snapshot pipeline for UI and HUD state.
- HUD features include:
- Donut/cycle/chain elements
- Configurable sizes and positions
- Combat visibility options
- Cooldown second display options
- Drag/save HUD position support.

## Extended Systems

### 5. SmartCast chain system
- Records and replays spell chains.
- Chain steps support:
- Fire-and-forget and concentration spell types
- Cast target mode (self/crosshair/aimed)
- Cast count and hold duration
- Per-chain hotkeys, enable toggles, and step delay.
- Playback/recording controls with cancel/interrupt behavior.
- Supports max chains and max steps limits with config clamping.

### 6. QuickBuff automation
- Trigger-driven buff casting based on gameplay events:
- Combat start/end
- Health thresholds (70/50/30)
- Crouch start
- Sprint start
- Weapon draw
- Power attack start
- Shout start
- Per-trigger controls:
- Enable
- Cooldown
- Conditions (first person, in combat, out of combat, etc.)
- Rules (magicka threshold, skip if already active)
- Handles concentration buff maintenance with timed updates.

### 7. Mastery systems
- Separate progression tracks for:
- Spells (`mastery_spell`)
- Weapons/shields (`mastery_weapon`)
- Shouts (`mastery_shout`)
- Tracks usage/combat/equip metrics and computes progression levels from thresholds.
- Applies configurable bonus tuning through dedicated bonus appliers.
- Supports config reset, database clear, and serialization persistence.

### 8. Spellblade Overhaul aggregator
- Central coordinator for module settings/actions and unified snapshot generation.
- Exposes consolidated JSON data to UI:
- QuickBuff state
- SmartCast chains
- Mastery progress/rows
- SpellBinding data
- Provides conversion helpers for cross-module JSON payload handling.

## Current Notable Features (Player-Facing)
- Bind different spells to light/power/bash for each weapon context.
- Cycle binding slot mode during gameplay.
- Toggle and configure UI/HUD presentation extensively.
- Auto-cast utility spells using event triggers.
- Record and replay multi-step spell chains.
- Progress persistent mastery tracks for spells, shouts, and weapons.
- Save/restore mod state across sessions through SKSE serialization + JSON.

## Design Notes
- Highly modular architecture with clear namespace boundaries by subsystem.
- UI-driven runtime configuration allows live tuning without rebuild/restart cycles.
- Shared config file for multiple systems centralizes player setup but increases schema coordination complexity.

## Future Feature Ideas

### Short-term
- Split monolithic config into per-module sections/files with migration tooling.
- Add in-UI validation for invalid spell keys and broken plugin references.
- Improve per-weapon identity (optional formID+plugin strict mode) for edge-case naming collisions.

### Mid-term
- Chain simulation sandbox in UI (dry-run with expected cast order and timing).
- Rule editor for advanced trigger logic (AND/OR conditions for QuickBuff and SpellBinding triggers).
- Profile presets for archetypes (Spellsword, Battlemage, Assassin Mage, Support Mage).

### Long-term
- Cross-mod integration bus for combat frameworks and animation frameworks.
- Machine-assisted balancing suggestions for cooldowns/costs based on observed player behavior.
- Multiplayer-style encounter templates (PvE role loadouts) exported/imported as module bundles.

## Who This Mod Is For
- Players building advanced spellblade combat loops.
- Users who want automation + manual control in the same system.
- Modders who need a customizable framework rather than a fixed single-feature plugin.
