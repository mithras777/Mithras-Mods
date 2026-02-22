# Kick Mod Feature Plan

## Scope

Add the following features to `Kick`:

1. Configurable stamina cost for kicks (separate for object kicks and NPC kicks).
2. Kick blocked when player stamina is lower than the required cost.
3. Configurable stamina drain applied to NPC targets on kick.
4. `Guard Break Kick` toggle (default `false`) that ragdolls enemies only when their stamina is `0`.

---

## Proposed Config Changes

Extend `KickConfig` with:

- `float objectStaminaCost` (default: `0.0f`)
- `float npcStaminaCost` (default: `0.0f`)
- `float npcStaminaDrain` (default: `0.0f`)
- `bool guardBreakKick` (default: `false`)

Notes:

- Keep object/NPC separation consistent with current split settings model.
- Clamp costs/drain to non-negative ranges.
- Keep `guardBreakKick` default `false` for backward-safe behavior.

---

## JSON Changes

Update `ToJson` / `FromJson` in `KickManager.cpp`:

- Write/read only current schema keys (no legacy fallback logic).
- Add new keys:
  - `objectStaminaCost`
  - `npcStaminaCost`
  - `npcStaminaDrain`
  - `guardBreakKick`

Load-time behavior:

- Missing key => use in-code default.
- Invalid values => clamped in `SetConfig` / load clamp pass.

---

## Runtime Behavior Changes

### 1) Stamina Cost Gate

In `TryKick()`:

- Determine selected target type (object vs NPC) as currently done.
- Resolve required stamina cost by target type:
  - object target => `objectStaminaCost`
  - actor target => `npcStaminaCost`
- Read player stamina (`ActorValue::kStamina` current value).
- If `currentStamina < requiredCost`, abort kick before impulse application.
- If kick proceeds, subtract stamina cost from player.

### 2) NPC Stamina Drain

On successful NPC kick:

- Apply configurable drain to target NPC stamina (`npcStaminaDrain`).
- Clamp to avoid negative-side anomalies (engine already handles floor, but keep logic safe).

### 3) Guard Break Kick

When `guardBreakKick == true` and target is actor:

- If target stamina is greater than `0`, do not force ragdoll.
- If target stamina is `0`, allow ragdoll behavior.

Implementation detail:

- Reuse current actor kick path (`KnockExplosion` + ragdoll current).
- Add explicit stamina gate around ragdoll trigger logic.
- Keep default path unchanged when toggle is `false`.

---

## SKSE Menu Changes

Update `Kick` Settings tabs:

- **Objects tab** (`Settings` section):
  - Add slider/input for `Object Stamina Cost`.
- **NPCs tab** (`Settings` section):
  - Add slider/input for `NPC Stamina Cost`.
  - Add slider/input for `NPC Stamina Drain`.
  - Add checkbox for `Guard Break Kick`.

Defaults buttons:

- Ensure Objects/NPCs Defaults also reset the new values.

---

## Suggested Defaults

- `objectStaminaCost = 0.0`
- `npcStaminaCost = 0.0`
- `npcStaminaDrain = 0.0`
- `guardBreakKick = false`

Rationale: no behavior change unless explicitly configured.

---

## Validation Checklist

1. JSON file writes/reads new keys correctly.
2. Object kick fails when stamina below `objectStaminaCost`.
3. NPC kick fails when stamina below `npcStaminaCost`.
4. Successful NPC kick drains target stamina by `npcStaminaDrain`.
5. With `guardBreakKick = true`:
   - NPC with stamina > 0: no ragdoll force.
   - NPC with stamina == 0: ragdoll force allowed.
6. With `guardBreakKick = false`: behavior matches current baseline.
7. Release build succeeds:
   - `cmake --build --preset vs2022-ng-rel`

---

## Implementation Order

1. Add config fields + clamps.
2. Add JSON read/write fields.
3. Add stamina cost gate in `TryKick()`.
4. Add NPC stamina drain on successful actor kick.
5. Add guard break logic around ragdoll path.
6. Add SKSE UI controls + defaults integration.
7. Build/test and tune defaults/ranges.

---

## RE Functions / Hooks Used

All behavior uses existing **CommonLibSSE (RE)** and **PROXY** APIs; no new hooks or custom RE code were added.

### RE (CommonLibSSE) – Actor / ActorValues

- **`RE::ActorValue::kStamina`** – Actor value ID for stamina (read cost gate, guard-break check, drain).
- **`RE::Actor::AsActorValueOwner()`** – Returns the actor’s `ActorValueOwner` interface.
- **`RE::ActorValueOwner::GetActorValue(RE::ActorValue a_av)`** – Used to read player current stamina before allowing a kick, and target stamina before applying drain for the guard-break check (via `player->AsActorValueOwner()->GetActorValue(...)` and same for target actor).
- **`RE::ActorValueOwner::DamageActorValue(RE::ActorValue a_av, float a_damage)`** – Used to: (1) deduct kick cost from the player’s stamina after a successful kick attempt, (2) apply configurable stamina drain to the kicked NPC. No custom damage or hooks; engine handles clamping.

### RE (CommonLibSSE) – Ragdoll / Knock (unchanged from pre‑plan)

- **`PROXY::Actor::Data(RE::Actor*)`** – Existing proxy used to get `currentProcess` for the knock explosion (NG vs non‑NG).
- **`RE::ActorProcessManager::KnockExplosion(RE::Actor* a_target, RE::NiPoint3 a_sourcePos, float a_force)`** – Existing call used to trigger knock/ragdoll when `allowRagdoll` is true.
- **`RE::Actor::IsInRagdollState()`** – Used to decide whether to apply extra impulse via `ApplyCurrent`.
- **`RE::Actor::ApplyCurrent(float a_delta, const RE::hkVector4& a_impulse)`** – Existing; applies impulse when target is already ragdolled.
- **`RE::Actor::PotentiallyFixRagdollState()`** – Existing; called after applying impulse.

### Summary

- **No new hooks.** All logic uses the above RE/PROXY types and methods.
- **No RE APIs were added or invented.** If a future RE version renames or removes any of these, the implementation would need to follow the updated CommonLibSSE API.
