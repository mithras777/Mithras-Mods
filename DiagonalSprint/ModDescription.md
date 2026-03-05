# Diagonal Sprint

## Summary
Diagonal Sprint is an SKSE gameplay plugin focused on improving first-person diagonal sprint movement and airborne control feel. It combines input-state tracking, movement-assist logic, and safety timers to reduce awkward sprint/jump transitions.

The mod is built as a runtime behavior layer rather than a static data patch.

## Current Feature Set

### 1. Configurable runtime behavior
- Uses JSON config at `Data/SKSE/Plugins/DiagonalSprint.json`.
- Current tunables:
- `enabled`
- `lateralSpeed` (clamped to safe range)
- `freeAirControl`
- Supports save/reload/reset workflows and writes defaults automatically when missing.

### 2. First-person scoped logic
- Behavior only engages in first person.
- If player leaves first person, runtime assist state resets to avoid carryover artifacts.
- Includes FOV resync guard logic to keep first-person FOV channels aligned when drift is detected.

### 3. Sprint assist state machine
- Tracks user intent from directional + sprint input combinations.
- Activates a short sprint-assist window when diagonal sprint intent is detected.
- Uses timing windows and side/input checks to keep assistance responsive without permanently overriding player control.

### 4. Jump handling and synthetic input support
- Detects jump press events and applies suppression/intention windows.
- Supports synthetic jump preparation/held timing to improve consistency of jump transitions under diagonal sprint scenarios.
- Includes cooldown and grace windows to avoid repeated unstable injections.

### 5. Air-control and terrain bias handling
- Airborne logic can apply lateral control adjustments when enabled.
- Contains climb-bias logic and uphill threshold handling to reduce awkward movement behavior on sloped/stair surfaces.
- Includes grounded-reacquire and post-liftoff timing guards to stabilize transition states.

### 6. In-game menu integration
- MCM section via SKSE Menu Framework:
- General tab
- Controls tab
- Exposes:
- Enable toggle
- Lateral speed slider
- Free air control toggle
- Defaults buttons per tab for quick reset.

## Design Notes
- The plugin is timer-driven and stateful, with many micro-cooldowns to prevent event spam and animation instability.
- Input processing is event-based but coupled with guarded runtime state, helping avoid uncontrolled behavior loops.
- Emphasis is on preserving player intent while smoothing edge-case transitions (diagonal sprint + jump + camera mode).

## Future Feature Ideas

### Short-term
- Separate lateral-speed profiles for walking, sprinting, and airborne phases.
- Optional per-input-device profiles (keyboard vs controller).
- Configurable first-person-only toggle so advanced users can opt into third-person behavior.

### Mid-term
- Adaptive assist strength based on stamina, encumbrance, or active effects.
- Optional "soft correction mode" with reduced intervention for users who want minimal input shaping.
- Runtime debug overlay (state flags, timers, and last assist decisions) for balancing/testing.

### Long-term
- Deeper compatibility mode for movement/combat frameworks that alter sprint or jump semantics.
- Predictive terrain-aware smoothing (stairs, uneven terrain, steep slopes) with user presets.
- Optional per-weapon-stance movement response curves for immersive combat movement styles.

## Who This Mod Is For
- First-person players who dislike vanilla diagonal sprint feel.
- Users who want cleaner sprint-to-jump transitions.
- Load orders aiming for more responsive movement without heavy animation replacer dependence.
