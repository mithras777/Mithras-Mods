(function () {
  const state = {
    snapshot: { bindings: [], config: {}, currentWeapon: {}, status: {}, knownSpells: [], blacklist: [], bindMode: {} },
    blacklistSet: new Set(),
    captureTarget: null,
    drag: null,
    dragPos: { x: 48, y: 48 },
    bindKeyCode: 0x22
  }

  const byId = (id) => document.getElementById(id)
  const els = {
    mainApp: byId('main-app'),
    dragOverlay: byId('drag-overlay'),
    dragWidget: byId('drag-widget'),
    dragSaveBtn: byId('drag-save-btn'),
    dragSizeSlider: byId('drag-size-slider'),
    dragSizeLabel: byId('drag-size-label'),

    search: byId('search'),
    bindingList: byId('binding-list'),
    cooldownSlider: byId('cooldown-slider'),
    cooldownLabel: byId('cooldown-label'),
    onlyCombat: byId('only-combat'),
    currentWeapon: byId('current-weapon'),
    currentSlot: byId('current-slot'),
    currentSpell: byId('current-spell'),
    currentMetric: byId('current-metric'),
    currentSlotUnbind: byId('current-slot-unbind'),
    bindMode: byId('bind-mode'),
    lastTrigger: byId('last-trigger'),
    lastMode: byId('last-mode'),
    lastError: byId('last-error'),

    settingsBtn: byId('settings-btn'),
    settingsModal: byId('settings-modal'),
    settingsClose: byId('settings-close'),
    uiHotkeyBtn: byId('ui-hotkey-btn'),
    bindHotkeyBtn: byId('bind-hotkey-btn'),
    modifierSelect: byId('modifier-select'),

    blacklistBtn: byId('blacklist-btn'),
    blacklistModal: byId('blacklist-modal'),
    blacklistClose: byId('blacklist-close'),
    blacklistSearch: byId('blacklist-search'),
    blacklistBlocked: byId('blacklist-list-blocked'),
    blacklistAvailable: byId('blacklist-list-available'),

    closeBtn: byId('close-btn'),
    cooldownEditBtn: byId('cooldown-edit-btn'),

    toast: byId('toast'),
    enabled: byId('enabled'),
    hudNotify: byId('hud-notify'),
    soundCues: byId('sound-cues'),
    hudDonut: byId('hud-donut'),
    hudUnsheathed: byId('hud-unsheathed'),
    blacklistEnabled: byId('blacklist-enabled'),
    dedupeSlider: byId('dedupe-slider'),
    dedupeLabel: byId('dedupe-label')
  }

  const DIK_BY_CODE = {
    Escape: 0x01,
    Digit1: 0x02, Digit2: 0x03, Digit3: 0x04, Digit4: 0x05, Digit5: 0x06, Digit6: 0x07, Digit7: 0x08, Digit8: 0x09, Digit9: 0x0A, Digit0: 0x0B,
    Minus: 0x0C, Equal: 0x0D, Backspace: 0x0E, Tab: 0x0F,
    KeyQ: 0x10, KeyW: 0x11, KeyE: 0x12, KeyR: 0x13, KeyT: 0x14, KeyY: 0x15, KeyU: 0x16, KeyI: 0x17, KeyO: 0x18, KeyP: 0x19,
    BracketLeft: 0x1A, BracketRight: 0x1B, Enter: 0x1C,
    ControlLeft: 0x1D,
    KeyA: 0x1E, KeyS: 0x1F, KeyD: 0x20, KeyF: 0x21, KeyG: 0x22, KeyH: 0x23, KeyJ: 0x24, KeyK: 0x25, KeyL: 0x26,
    Semicolon: 0x27, Quote: 0x28, Backquote: 0x29,
    ShiftLeft: 0x2A, Backslash: 0x2B,
    KeyZ: 0x2C, KeyX: 0x2D, KeyC: 0x2E, KeyV: 0x2F, KeyB: 0x30, KeyN: 0x31, KeyM: 0x32,
    Comma: 0x33, Period: 0x34, Slash: 0x35, ShiftRight: 0x36,
    NumpadMultiply: 0x37, AltLeft: 0x38, Space: 0x39,
    CapsLock: 0x3A,
    F1: 0x3B, F2: 0x3C, F3: 0x3D, F4: 0x3E, F5: 0x3F, F6: 0x40, F7: 0x41, F8: 0x42, F9: 0x43, F10: 0x44,
    NumLock: 0x45, ScrollLock: 0x46,
    Numpad7: 0x47, Numpad8: 0x48, Numpad9: 0x49, NumpadSubtract: 0x4A,
    Numpad4: 0x4B, Numpad5: 0x4C, Numpad6: 0x4D, NumpadAdd: 0x4E,
    Numpad1: 0x4F, Numpad2: 0x50, Numpad3: 0x51, Numpad0: 0x52, NumpadDecimal: 0x53,
    F11: 0x57, F12: 0x58,
    Insert: 0xD2, Delete: 0xD3, Home: 0xC7, End: 0xCF, PageUp: 0xC9, PageDown: 0xD1,
    ArrowUp: 0xC8, ArrowDown: 0xD0, ArrowLeft: 0xCB, ArrowRight: 0xCD
  }

  const DIK_BY_KEY = {
    Escape: 0x01,
    '-': 0x0C, '=': 0x0D, Backspace: 0x0E, Tab: 0x0F, Enter: 0x1C,
    ';': 0x27, "'": 0x28, '`': 0x29, '\\': 0x2B, ',': 0x33, '.': 0x34, '/': 0x35,
    Shift: 0x2A, Control: 0x1D, Alt: 0x38, ' ': 0x39,
    CapsLock: 0x3A,
    ArrowUp: 0xC8, ArrowDown: 0xD0, ArrowLeft: 0xCB, ArrowRight: 0xCD,
    Insert: 0xD2, Delete: 0xD3, Home: 0xC7, End: 0xCF, PageUp: 0xC9, PageDown: 0xD1
  }
  const DIK_BY_LETTER = {
    A: 0x1E, B: 0x30, C: 0x2E, D: 0x20, E: 0x12, F: 0x21, G: 0x22, H: 0x23, I: 0x17, J: 0x24, K: 0x25, L: 0x26,
    M: 0x32, N: 0x31, O: 0x18, P: 0x19, Q: 0x10, R: 0x13, S: 0x1F, T: 0x14, U: 0x16, V: 0x2F, W: 0x11, X: 0x2D,
    Y: 0x15, Z: 0x2C
  }

  function slotLabel(slot) { return slot === 1 ? 'Power' : (slot === 2 ? 'Bash' : 'Light') }
  function handName(slot) { return slot === 1 ? 'Left Hand' : (slot === 2 ? 'Unarmed' : 'Right Hand') }
  function currentSlot() { return Number(state.snapshot.bindMode?.slot || 0) }

  function skse(name, payload) {
    const fn = window[name]
    if (typeof fn === 'function') fn(payload || '')
  }

  function applyDragWidgetSize(sizePx) {
    if (!els.dragWidget) return
    const s = Math.max(48, Math.min(220, Number(sizePx || 88)))
    els.dragWidget.style.width = s + 'px'
    els.dragWidget.style.height = s + 'px'
    if (els.dragSizeSlider) els.dragSizeSlider.value = String(s)
    if (els.dragSizeLabel) els.dragSizeLabel.textContent = s.toFixed(0) + 'px'
  }

  function showToast(text) {
    if (!els.toast) return
    els.toast.textContent = text || ''
    els.toast.classList.add('show')
    clearTimeout(window.__sb_toast)
    window.__sb_toast = setTimeout(() => els.toast.classList.remove('show'), 1500)
  }

  function currentKey() {
    return state.snapshot.currentWeapon && state.snapshot.currentWeapon.key ? state.snapshot.currentWeapon.key : null
  }

  function saveBlacklistNow() {
    skse('sb_set_blacklist', JSON.stringify({ items: Array.from(state.blacklistSet) }))
  }

  function toggleBlacklist(spellKey) {
    if (!spellKey) return
    if (state.blacklistSet.has(spellKey)) {
      state.blacklistSet.delete(spellKey)
    } else {
      state.blacklistSet.add(spellKey)
    }
    saveBlacklistNow()
    renderBlacklist()
  }

  function renderBindings() {
    if (!els.bindingList) return
    const query = (els.search?.value || '').toLowerCase().trim()
    const rows = (state.snapshot.bindings || []).filter((it) => {
      const name = it?.key?.displayName || ''
      return !query || name.toLowerCase().includes(query)
    })
    els.bindingList.innerHTML = ''
    if (!rows.length) {
      els.bindingList.innerHTML = '<p class="muted">No bound weapons.</p>'
      return
    }

    rows.forEach((row) => {
      const node = document.createElement('div')
      node.className = 'binding-item'
      node.innerHTML = '<div class="binding-main"><div class="weapon">' + (row.key?.displayName || 'Unknown') + '</div><div class="meta">L: ' + (row.summary?.light || '-') + ' | P: ' + (row.summary?.power || '-') + ' | B: ' + (row.summary?.bash || '-') + '</div></div>'
      const x = document.createElement('button')
      x.className = 'ghost-x'
      x.textContent = 'X'
      x.addEventListener('click', () => skse('sb_unbind_weapon', JSON.stringify(row.key || {})))
      node.appendChild(x)
      els.bindingList.appendChild(node)
    })
  }

  function makeSpellButton(spell, blocked) {
    const btn = document.createElement('button')
    btn.className = 'blacklist-item' + (blocked ? ' blocked' : '')
    btn.textContent = (spell.displayName || 'Unknown') + ' (' + (spell.spellType || 'Magic') + ')'
    btn.title = blocked ? 'Click to remove from blacklist' : 'Click to blacklist'
    btn.addEventListener('click', () => toggleBlacklist(spell.spellFormKey))
    return btn
  }

  function renderBlacklist() {
    if (!els.blacklistBlocked || !els.blacklistAvailable) return
    const query = (els.blacklistSearch?.value || '').toLowerCase().trim()
    const all = state.snapshot.knownSpells || []
    els.blacklistBlocked.innerHTML = ''
    els.blacklistAvailable.innerHTML = ''

    let blockedCount = 0
    let availableCount = 0
    all.forEach((spell) => {
      if (query && !(spell.displayName || '').toLowerCase().includes(query)) return
      const isBlocked = state.blacklistSet.has(spell.spellFormKey)
      const btn = makeSpellButton(spell, isBlocked)
      if (isBlocked) {
        blockedCount += 1
        els.blacklistBlocked.appendChild(btn)
      } else {
        availableCount += 1
        els.blacklistAvailable.appendChild(btn)
      }
    })

    if (blockedCount === 0) {
      els.blacklistBlocked.innerHTML = '<p class="muted">No blacklisted spells.</p>'
    }
    if (availableCount === 0) {
      els.blacklistAvailable.innerHTML = '<p class="muted">No available spells.</p>'
    }
  }

  function renderCurrent() {
    const current = state.snapshot.currentWeapon || {}
    const selected = currentSlot()
    const slots = current.slots || {}
    const slotKey = selected === 1 ? 'power' : (selected === 2 ? 'bash' : 'light')
    const slot = slots[slotKey] || {}

    if (els.currentWeapon) els.currentWeapon.textContent = current.displayName || 'None'
    if (els.currentSlot) els.currentSlot.textContent = handName(current.handSlot || 0) + ' - ' + slotLabel(selected)
    if (els.currentSpell) els.currentSpell.textContent = slot.enabled ? (slot.displayName || 'Unknown') : 'None'
    if (els.currentMetric) els.currentMetric.textContent = slot.enabled ? (slot.metric || 'N/A') : 'Cost: -'
    if (els.bindMode) els.bindMode.textContent = 'Bind Mode: ' + ((state.snapshot.bindMode && state.snapshot.bindMode.label) || slotLabel(selected))

    const cool = Number(current.triggerCooldownSec || 1.5)
    if (els.cooldownSlider) els.cooldownSlider.value = cool
    if (els.cooldownLabel) els.cooldownLabel.textContent = cool.toFixed(1) + 's'
    if (els.onlyCombat) els.onlyCombat.checked = !!current.onlyInCombat
  }

  function renderSettings() {
    const cfg = state.snapshot.config || {}
    if (els.uiHotkeyBtn) els.uiHotkeyBtn.textContent = cfg.uiToggleKey || 'F2'
    if (els.bindHotkeyBtn) els.bindHotkeyBtn.textContent = cfg.bindKey || 'G'
    if (els.modifierSelect) els.modifierSelect.value = String(Number(cfg.cycleSlotModifierKey || 42))

    state.bindKeyCode = Number(cfg.bindKeyCode || 0x22)

    if (els.enabled) els.enabled.checked = !!cfg.enabled
    if (els.hudNotify) els.hudNotify.checked = !!cfg.showHudNotifications
    if (els.soundCues) els.soundCues.checked = !!cfg.enableSoundCues
    if (els.hudDonut) els.hudDonut.checked = !!cfg.hudDonutEnabled
    if (els.hudUnsheathed) els.hudUnsheathed.checked = !!cfg.hudDonutOnlyUnsheathed
    if (els.blacklistEnabled) els.blacklistEnabled.checked = !!cfg.blacklistEnabled

    const dedupe = Number(cfg.fallbackDedupeSec || 1.5)
    if (els.dedupeSlider) els.dedupeSlider.value = dedupe
    if (els.dedupeLabel) els.dedupeLabel.textContent = dedupe.toFixed(1) + 's'

    state.dragPos.x = Number(cfg.hudPosX || 48)
    state.dragPos.y = Number(cfg.hudPosY || 48)
    applyDragWidgetSize(Number(cfg.hudDonutSize || 88))
  }

  function renderStatus() {
    const status = state.snapshot.status || {}
    if (els.lastTrigger) els.lastTrigger.textContent = status.lastTriggerSpell ? (status.lastTriggerSpell + ' via ' + status.lastTriggerWeapon) : '-'
    if (els.lastMode) els.lastMode.textContent = status.lastWasPowerAttack ? 'Power' : 'Normal'
    if (els.lastError) els.lastError.textContent = status.lastError || '-'
  }

  function renderAll() {
    renderBindings()
    renderBlacklist()
    renderCurrent()
    renderSettings()
    renderStatus()
  }

  function setSetting(id, value) { skse('sb_set_setting', JSON.stringify({ id, value })) }

  function parseFromKeyString(key) {
    if (!key) return null
    const keyNorm = String(key)
    const keyUpper = keyNorm.toUpperCase()
    if (key.length === 1) {
      const ch = keyUpper
      if (Object.prototype.hasOwnProperty.call(DIK_BY_LETTER, ch)) {
        return DIK_BY_LETTER[ch]
      }
      if (ch >= '1' && ch <= '9') {
        return 0x01 + Number(ch)
      }
      if (ch === '0') return 0x0B
    }
    if (/^F\d{1,2}$/.test(keyUpper)) {
      const n = Number(keyUpper.slice(1))
      if (n >= 1 && n <= 10) return 0x3A + n
      if (n === 11) return 0x57
      if (n === 12) return 0x58
    }
    if (Object.prototype.hasOwnProperty.call(DIK_BY_KEY, keyNorm)) {
      return DIK_BY_KEY[keyNorm]
    }
    return null
  }

  function parseFromLegacyCode(event) {
    const code = Number(event.keyCode || event.which || 0)
    if (!Number.isFinite(code) || code <= 0) return null

    if (code >= 65 && code <= 90) {
      const ch = String.fromCharCode(code)
      if (Object.prototype.hasOwnProperty.call(DIK_BY_LETTER, ch)) {
        return DIK_BY_LETTER[ch]
      }
    }
    if (code >= 48 && code <= 57) {
      if (code === 48) return 0x0B
      return 0x01 + (code - 48)
    }
    if (code >= 112 && code <= 121) {
      return 0x3B + (code - 112)
    }

    const legacyMap = {
      27: 0x01,
      8: 0x0E,
      9: 0x0F,
      13: 0x1C,
      16: 0x2A,
      17: 0x1D,
      18: 0x38,
      32: 0x39,
      37: 0xCB,
      38: 0xC8,
      39: 0xCD,
      40: 0xD0,
      45: 0xD2,
      46: 0xD3,
      36: 0xC7,
      35: 0xCF,
      33: 0xC9,
      34: 0xD1,
      186: 0x27,
      187: 0x0D,
      188: 0x33,
      189: 0x0C,
      190: 0x34,
      191: 0x35,
      192: 0x29,
      219: 0x1A,
      220: 0x2B,
      221: 0x1B,
      222: 0x28
    }
    if (Object.prototype.hasOwnProperty.call(legacyMap, code)) {
      return legacyMap[code]
    }

    return null
  }

  function toDIK(event) {
    if (event && event.code && Object.prototype.hasOwnProperty.call(DIK_BY_CODE, event.code)) {
      return DIK_BY_CODE[event.code]
    }

    const keyBased = parseFromKeyString(event ? String(event.key || '') : '')
    if (keyBased !== null) return keyBased

    return parseFromLegacyCode(event)
  }

  function openDragOverlay() {
    if (!els.dragOverlay || !els.mainApp || !els.dragWidget) return
    els.mainApp.classList.add('hidden')
    els.dragOverlay.classList.remove('hidden')
    els.dragWidget.style.left = state.dragPos.x + 'px'
    els.dragWidget.style.top = state.dragPos.y + 'px'
    applyDragWidgetSize(Number(state.snapshot?.config?.hudDonutSize || 88))
    skse('sb_enter_hud_drag_mode', '')
  }

  function closeDragOverlay(save) {
    if (!els.dragOverlay || !els.mainApp || !els.dragWidget) return
    if (save) {
      const x = els.dragWidget.offsetLeft
      const y = els.dragWidget.offsetTop
      state.dragPos.x = x
      state.dragPos.y = y
      skse('sb_save_hud_position', JSON.stringify({ x, y, commit: true }))
    } else {
      skse('sb_save_hud_position', JSON.stringify({ x: state.dragPos.x, y: state.dragPos.y, commit: true }))
    }
    els.dragOverlay.classList.add('hidden')
    els.mainApp.classList.remove('hidden')
  }

  function closeTopmostLayer() {
    if (!els.dragOverlay?.classList.contains('hidden')) { closeDragOverlay(false); return true }
    if (!els.settingsModal?.classList.contains('hidden')) { els.settingsModal.classList.add('hidden'); return true }
    if (!els.blacklistModal?.classList.contains('hidden')) { els.blacklistModal.classList.add('hidden'); return true }
    return false
  }

  window.sb_renderSnapshot = function (payload) {
    try {
      state.snapshot = JSON.parse(payload || '{}')
      state.blacklistSet = new Set(state.snapshot.blacklist || [])
      renderAll()
    } catch (_) {
      showToast('Failed snapshot parse')
    }
  }
  window.sb_showToast = showToast
  window.sb_setFocusState = function () {}

  els.search?.addEventListener('input', renderBindings)
  els.blacklistSearch?.addEventListener('input', renderBlacklist)

  els.cooldownSlider?.addEventListener('input', () => {
    els.cooldownLabel.textContent = Number(els.cooldownSlider.value).toFixed(1) + 's'
  })
  els.cooldownSlider?.addEventListener('change', () => {
    const key = currentKey(); if (!key) return
    skse('sb_set_weapon_setting', JSON.stringify({ key, id: 'triggerCooldownSec', value: Number(els.cooldownSlider.value) }))
  })

  els.onlyCombat?.addEventListener('change', () => {
    const key = currentKey(); if (!key) return
    skse('sb_set_weapon_setting', JSON.stringify({ key, id: 'onlyInCombat', value: !!els.onlyCombat.checked }))
  })

  els.currentSlotUnbind?.addEventListener('click', () => {
    const key = currentKey(); if (!key) return
    skse('sb_unbind_slot', JSON.stringify({ key, slot: currentSlot() }))
  })

  els.settingsBtn?.addEventListener('click', () => els.settingsModal?.classList.remove('hidden'))
  els.settingsClose?.addEventListener('click', () => els.settingsModal?.classList.add('hidden'))
  els.blacklistBtn?.addEventListener('click', () => {
    els.blacklistModal?.classList.remove('hidden')
    renderBlacklist()
  })
  els.blacklistClose?.addEventListener('click', () => els.blacklistModal?.classList.add('hidden'))

  els.settingsModal?.addEventListener('click', (e) => {
    if (e.target === els.settingsModal) {
      els.settingsModal.classList.add('hidden')
    }
  })
  els.blacklistModal?.addEventListener('click', (e) => {
    if (e.target === els.blacklistModal) {
      els.blacklistModal.classList.add('hidden')
    }
  })

  els.closeBtn?.addEventListener('click', () => skse('sb_toggle_ui', ''))
  els.cooldownEditBtn?.addEventListener('click', openDragOverlay)
  els.dragSaveBtn?.addEventListener('click', () => closeDragOverlay(true))
  els.dragSizeSlider?.addEventListener('input', () => {
    applyDragWidgetSize(Number(els.dragSizeSlider.value))
  })
  els.dragSizeSlider?.addEventListener('change', () => {
    setSetting('hudDonutSize', Number(els.dragSizeSlider.value))
  })

  els.enabled?.addEventListener('change', () => setSetting('enabled', !!els.enabled.checked))
  els.hudNotify?.addEventListener('change', () => setSetting('showHudNotifications', !!els.hudNotify.checked))
  els.soundCues?.addEventListener('change', () => setSetting('enableSoundCues', !!els.soundCues.checked))
  els.hudDonut?.addEventListener('change', () => setSetting('hudDonutEnabled', !!els.hudDonut.checked))
  els.hudUnsheathed?.addEventListener('change', () => setSetting('hudDonutOnlyUnsheathed', !!els.hudUnsheathed.checked))
  els.blacklistEnabled?.addEventListener('change', () => setSetting('blacklistEnabled', !!els.blacklistEnabled.checked))
  els.dedupeSlider?.addEventListener('input', () => {
    els.dedupeLabel.textContent = Number(els.dedupeSlider.value).toFixed(1) + 's'
  })
  els.dedupeSlider?.addEventListener('change', () => setSetting('fallbackDedupeSec', Number(els.dedupeSlider.value)))

  els.modifierSelect?.addEventListener('change', () => {
    setSetting('cycleSlotModifierKey', Number(els.modifierSelect.value))
  })

  els.uiHotkeyBtn?.addEventListener('click', () => {
    state.captureTarget = 'uiToggleKey'
    els.uiHotkeyBtn.textContent = 'Press key...'
  })
  els.bindHotkeyBtn?.addEventListener('click', () => {
    state.captureTarget = 'bindKey'
    els.bindHotkeyBtn.textContent = 'Press key...'
  })

  if (els.dragWidget && els.dragOverlay) {
    els.dragWidget.addEventListener('mousedown', function (e) {
      if (els.dragOverlay.classList.contains('hidden')) return
      state.drag = {
        x: e.clientX,
        y: e.clientY,
        left: els.dragWidget.offsetLeft,
        top: els.dragWidget.offsetTop
      }
      e.preventDefault()
    })

    window.addEventListener('mousemove', function (e) {
      if (!state.drag) return
      const nextX = Math.max(0, Math.min(window.innerWidth - els.dragWidget.offsetWidth, state.drag.left + (e.clientX - state.drag.x)))
      const nextY = Math.max(0, Math.min(window.innerHeight - els.dragWidget.offsetHeight, state.drag.top + (e.clientY - state.drag.y)))
      els.dragWidget.style.left = nextX + 'px'
      els.dragWidget.style.top = nextY + 'px'
    })

    window.addEventListener('mouseup', function () {
      if (!state.drag) return
      state.drag = null
    })
  }

  window.addEventListener('keydown', function (e) {
    if (state.captureTarget) {
      e.preventDefault()
      const dik = toDIK(e)
      if (dik === null) {
        showToast('Unsupported key')
      } else {
        setSetting(state.captureTarget, dik)
        if (state.captureTarget === 'bindKey') {
          state.bindKeyCode = dik
        }
      }
      state.captureTarget = null
      renderSettings()
      return
    }

    const dik = toDIK(e)
    if (dik !== null && dik === state.bindKeyCode && e.shiftKey) {
      skse('sb_cycle_bind_slot', '')
      e.preventDefault()
      return
    }

    if (e.key !== 'Escape') return
    if (closeTopmostLayer()) return
    skse('sb_toggle_ui', '')
  })

  skse('sb_request_snapshot', '')
})()
