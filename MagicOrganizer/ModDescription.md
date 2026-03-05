# Magic Organizer

## Summary
Magic Organizer is an SKSE quality-of-life plugin for cleaning up the Magic Menu by hiding unwanted spells, powers, shouts, and active effects directly in-game. It provides hotkey-driven curation plus an MCM management interface for restoring hidden entries.

The mod is designed to reduce menu clutter while keeping restoration straightforward and safe.

## Current Feature Set

### 1. In-menu hide workflow
- Primary flow:
- Open Magic Menu
- Highlight entry
- Press configured hotkey
- Plugin resolves current selected form and hides the target type.
- Supports multiple target categories:
- Spells
- Powers/Shouts
- Magic effects (including selected active effects)

### 2. Multiple selection-resolution strategies
- Reads selected form from GFx movie variables when available.
- Falls back to entry index probing when direct selected form lookup is unavailable.
- For active effects, resolves by:
- Direct selected effect form
- Linked magic item effects
- Name matching fallback against active effect list

### 3. Persistent hidden-state tracking
- Tracks hidden form IDs internally by category.
- Persists state to:
- JSON config
- SKSE co-save serialization records
- Reapplies tracked hide flags on load/init.
- Properly reverts and reloads hidden states through serialization callbacks.

### 4. Hide/unhide APIs by content type
- Dedicated operations for:
- `HideSpell` / `UnhideSpell`
- `HidePowerOrShout` / `UnhidePowerOrShout`
- `HideEffect` / `UnhideEffect`
- Effect hiding uses engine hide flags (`kHideInUI`) for menu-level suppression.

### 5. Hotkey configuration and capture
- Configurable hotkey value in settings.
- Supports capture mode for hotkey reassignment.
- Keyboard key name resolution through input manager APIs.

### 6. In-game management UI (SKSE Menu Framework)
- Tabs:
- General
- Spells
- Powers
- Magic Effects
- General tab provides:
- Enable toggle
- Hotkey selector
- Usage tip text
- Per-category tabs provide table views of hidden entries with one-click unhide actions.

### 7. Organizer self-cleanup behavior
- Includes logic to avoid exposing organizer-internal helper entries where appropriate.
- Applies hide state refreshes when config changes.

## Design Notes
- Menu-level UX and persistence are primary goals.
- Fallback-heavy selection resolution improves reliability across UI states.
- Separation of hidden categories keeps restoration precise and user-auditable.

## Future Feature Ideas

### Short-term
- Batch hide/unhide operations (by school, spell type, source plugin).
- Search and filter in hidden-entry tables.
- Optional "temporary hide until next load" mode.

### Mid-term
- Profile system (Mage build, Warrior build, Minimalist build) with quick switching.
- Rule-based auto-hide (for example: hide novice spells after reaching skill thresholds).
- Export/import hidden lists for cross-profile or cross-save reuse.

### Long-term
- Smart clutter scoring and recommended hide suggestions.
- Integration with load-order metadata to group entries by plugin/module packs.
- Optional category auto-organization for fully curated magic menus.

## Who This Mod Is For
- Players with large spell lists who want cleaner magic menus.
- Users who frequently respec playstyles and want reversible organization tools.
- Modders/testers who need fast menu curation without deleting spell access permanently.
