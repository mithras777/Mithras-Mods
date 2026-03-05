# Configurable Speeds

## Summary
Configurable Speeds is an SKSE plugin that lets you tune movement-speed data directly from the in-game movement types used by Skyrim. It focuses on practical runtime control: you can adjust values in MCM, apply them immediately, and optionally restore vanilla values when disabling the mod.

The mod targets the movement form records themselves, which means the behavior applies consistently to the actor movement systems that consume those records.

## Current Feature Set

### 1. JSON-backed movement profile system
- Stores settings in `Data/SKSE/Plugins/ConfigMovementSpeed.json`.
- Versioned config handling:
- Automatically regenerates defaults if the stored version is incompatible.
- Rewrites defaults if JSON is missing, malformed, or structurally mismatched.
- Keeps an internal default dataset so users can reset individual entries or the full config safely.

### 2. Multi-group movement tuning
- Entries are grouped into:
- `Default`
- `Combat`
- `Horses`
- Includes rule-based grouping for NPC movement forms (combat-specific entries vs. general movement).

### 3. Directional speed controls
- Supports directional run-speed tuning for:
- Left
- Right
- Forward
- Back
- Intentionally avoids patching rotation speed channel to preserve camera/turn feel.
- Sprint-specific and horse-specific handling:
- Sprint entries focus on sprint-forward run lane.
- Horse entries are forward-run focused.

### 4. Runtime patching with safe restore flow
- Captures original movement data once per resolved target.
- Applies patched values only when mod enable state requires it.
- Restores captured originals:
- On disable (if configured).
- During shutdown (if configured).
- During re-apply cycles before writing new values.
- Sends movement refresh pulses (graph variables + sprint start/stop events) across active actors to force state refresh after changes.

### 5. NPC compatibility logic
- Resolves base NPC default movement and additionally syncs one-handed and two-handed NPC movement forms.
- When default NPC run values are changed, those values are propagated to 1HM/2HM NPC movement entries.

### 6. MCM integration (SKSE Menu Framework)
- Dedicated sections/tabs for:
- General
- Default
- Combat
- Horses
- Entry-level enable toggle.
- Entry-level reset button (restore per-entry defaults).
- Slider-based tuning with broad range (0.0 to 2000.0 in UI).
- Lateral-to-forward guardrail:
- If lateral movement is raised, forward is enforced not to drop below lateral values.

### 7. Operational behavior
- Changes are committed only when config differs from the baseline snapshot.
- Includes an enable toggle and restore-on-disable behavior in general settings.

## Design Notes
- The plugin emphasizes reversible runtime edits rather than permanent form edits in an ESP.
- Internal canonicalization of form strings helps avoid case/format mismatch issues.
- Settings and patch application are separated (`Settings` vs `MovementPatcher`) which keeps persistence and runtime mutation responsibilities clean.

## Future Feature Ideas

### Short-term
- Per-profile presets (Vanilla+, Combat+, Mounted+, etc.) with one-click apply.
- Import/export profile packs to share tuned movement sets.
- Granular actor-class filters (player-only, followers-only, hostile NPC-only).
- Optional interpolation/smoothing when applying major speed deltas.

### Mid-term
- Context-aware profiles:
- Indoors vs outdoors
- Combat vs exploration
- Armor-weight or stamina-state based overlays
- Per-animation-state preview overlay in MCM showing live effective values.
- Conflict diagnostics panel that highlights external systems modifying the same movement data.

### Long-term
- Rule engine for dynamic speed policies (example: reduce strafe while low stamina).
- Lightweight telemetry mode to log movement-state usage and suggest balancing adjustments.
- Optional integration layer for weapon stance/combat behavior mods so speed profiles can switch by combat style.

## Who This Mod Is For
- Players who want precise control over movement feel.
- Combat-focused load orders that need separate traversal vs combat tuning.
- Modders who want runtime-adjustable movement behavior without rebuilding plugin records repeatedly.
