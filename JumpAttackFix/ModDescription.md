# JumpAttackFix

## Summary
JumpAttackFix is a focused SKSE runtime fix that stabilizes first-person jump/landing combat flow by ensuring a delayed `JumpLandEnd` animation graph event is sent after valid airborne-to-ground transitions.

This mod is intentionally narrow in scope: it targets a specific edge case and avoids broad movement-system rewrites.

## Current Feature Set

### 1. Airborne state tracking on player update
- Hooks player update and inspects:
- Character controller movement state
- Midair flags
- Delta-time progression
- Maintains runtime state:
- `wasAirborne`
- Airborne duration
- Cooldown timers
- Pending delayed-send flags

### 2. Controlled landing event injection
- Detects landing edges (`airborne -> grounded`) under first-person conditions.
- Requires minimum airborne time threshold before considering the transition valid.
- Queues delayed send window before firing event to align better with animation state timing.
- Sends `JumpLandEnd` via animation graph notification when all guards pass.

### 3. Anti-spam safety guards
- Per-event cooldown prevents repeated immediate retrigger.
- Delayed pending timer ensures event is not fired too early.
- Timer resets and floor checks reduce risk of noisy graph event spam.

### 4. First-person constrained behavior
- Fix logic is explicitly tied to first-person camera state to avoid unintended third-person side effects.

### 5. Console hook plumbing support
- Includes console command prefix interception path for plugin commands/tests.
- Keeps command path isolated from main movement fix logic.

## Design Notes
- The implementation favors deterministic timing windows over aggressive repeated graph pushes.
- Minimal invasive design:
- No broad actor value rewrites
- No movement-data table edits
- No external config dependency in current implementation
- Goal is reliability and low incompatibility risk.

## Future Feature Ideas

### Short-term
- Add optional JSON config for tuning:
- Min airborne threshold
- Post-landing delay
- Cooldown duration
- Add lightweight logging toggle for troubleshooting incompatible animation stacks.
- Add optional third-person mode flag (off by default).

### Mid-term
- Detect additional edge states (staggered landings, interrupted attacks, forced animation overrides).
- Add compatibility presets for popular combat overhauls and animation packs.
- Add debug metrics (landings detected, events sent, events suppressed by guardrails).

### Long-term
- Expand into a compact "jump combat transition" suite:
- Landing recovery timing normalization
- Jump attack chain continuity rules
- Custom transition profiles for light/power attack styles

## Who This Mod Is For
- Players using first-person combat who notice inconsistent landing transition behavior.
- Users wanting a low-footprint fix instead of full movement/combat overhauls.
- Modders building larger combat stacks who need a targeted stability patch.
