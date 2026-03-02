(function () {
  const state = {
    snapshot: { bindings: [], config: {}, currentWeapon: {}, status: {}, knownSpells: [], blacklist: [], bindMode: {} },
    selectedSlot: 0,
    selectedSpellKey: '',
    blacklistSet: new Set()
  }

  const byId = (id) => document.getElementById(id)
  const els = {
    search: byId('search'),
    bindingList: byId('binding-list'),
    spellSearch: byId('spell-search'),
    spellList: byId('spell-list'),
    bindSelected: byId('bind-selected'),
    unbindSelected: byId('unbind-selected'),
    bindMagicMenu: byId('bind-magicmenu'),
    cycleSlotBtn: byId('cycle-slot-btn'),
    slotTabs: Array.from(document.querySelectorAll('.slot-tab')),
    cooldownSlider: byId('cooldown-slider'),
    cooldownLabel: byId('cooldown-label'),
    onlyCombat: byId('only-combat'),
    currentWeapon: byId('current-weapon'),
    currentSlot: byId('current-slot'),
    currentSpell: byId('current-spell'),
    currentMetric: byId('current-metric'),
    bindMode: byId('bind-mode'),
    lastTrigger: byId('last-trigger'),
    lastMode: byId('last-mode'),
    lastError: byId('last-error'),
    settingsBtn: byId('settings-btn'),
    settingsModal: byId('settings-modal'),
    settingsClose: byId('settings-close'),
    blacklistBtn: byId('blacklist-btn'),
    blacklistModal: byId('blacklist-modal'),
    blacklistClose: byId('blacklist-close'),
    blacklistSearch: byId('blacklist-search'),
    blacklistList: byId('blacklist-list'),
    blacklistSave: byId('blacklist-save'),
    closeBtn: byId('close-btn'),
    cooldownEditBtn: byId('cooldown-edit-btn'),
    dragSaveBtn: byId('drag-save-btn'),
    dragBanner: byId('drag-banner'),
    toast: byId('toast'),
    uiHotkey: byId('ui-hotkey'),
    bindHotkey: byId('bind-hotkey'),
    enabled: byId('enabled'),
    hudNotify: byId('hud-notify'),
    soundCues: byId('sound-cues'),
    hudDonut: byId('hud-donut'),
    hudUnsheathed: byId('hud-unsheathed'),
    blacklistEnabled: byId('blacklist-enabled'),
    dedupeSlider: byId('dedupe-slider'),
    dedupeLabel: byId('dedupe-label')
  }

  function slotLabel(slot) { return slot === 1 ? 'Power' : (slot === 2 ? 'Bash' : 'Light') }
  function handName(slot) { return slot === 1 ? 'Left Hand' : (slot === 2 ? 'Unarmed' : 'Right Hand') }

  function skse(name, payload) {
    const fn = window[name]
    if (typeof fn === 'function') fn(payload || '')
  }

  function showToast(text) {
    els.toast.textContent = text || ''
    els.toast.classList.add('show')
    clearTimeout(window.__sb_toast)
    window.__sb_toast = setTimeout(() => els.toast.classList.remove('show'), 1500)
  }

  function currentKey() {
    return state.snapshot.currentWeapon && state.snapshot.currentWeapon.key ? state.snapshot.currentWeapon.key : null
  }

  function renderBindings() {
    const query = (els.search.value || '').toLowerCase().trim()
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

  function renderSpells() {
    const query = (els.spellSearch.value || '').toLowerCase().trim()
    const all = state.snapshot.knownSpells || []
    const list = all.filter((s) => {
      if (query && !(s.displayName || '').toLowerCase().includes(query)) return false
      return true
    })
    els.spellList.innerHTML = ''
    list.forEach((spell) => {
      const row = document.createElement('button')
      row.className = 'spell-row' + (state.selectedSpellKey === spell.spellFormKey ? ' active' : '')
      row.textContent = (spell.blacklisted ? '[BL] ' : '') + (spell.displayName || 'Unknown') + ' (' + (spell.spellType || 'Magic') + ')'
      row.addEventListener('click', () => {
        state.selectedSpellKey = spell.spellFormKey || ''
        renderSpells()
      })
      els.spellList.appendChild(row)
    })
  }

  function renderBlacklist() {
    const query = (els.blacklistSearch.value || '').toLowerCase().trim()
    const all = state.snapshot.knownSpells || []
    els.blacklistList.innerHTML = ''
    all.forEach((spell) => {
      if (query && !(spell.displayName || '').toLowerCase().includes(query)) return
      const row = document.createElement('label')
      row.className = 'spell-check'
      const checked = state.blacklistSet.has(spell.spellFormKey)
      row.innerHTML = '<input type="checkbox" ' + (checked ? 'checked' : '') + ' /> <span>' + (spell.displayName || 'Unknown') + ' (' + (spell.spellType || 'Magic') + ')</span>'
      row.querySelector('input').addEventListener('change', (e) => {
        if (e.target.checked) state.blacklistSet.add(spell.spellFormKey)
        else state.blacklistSet.delete(spell.spellFormKey)
      })
      els.blacklistList.appendChild(row)
    })
  }

  function renderCurrent() {
    const current = state.snapshot.currentWeapon || {}
    const slots = current.slots || {}
    const slotKey = state.selectedSlot === 1 ? 'power' : (state.selectedSlot === 2 ? 'bash' : 'light')
    const slot = slots[slotKey] || {}

    els.currentWeapon.textContent = current.displayName || 'None'
    els.currentSlot.textContent = handName(current.handSlot || 0) + ' - ' + slotLabel(state.selectedSlot)
    els.currentSpell.textContent = slot.enabled ? (slot.displayName || 'Unknown') : 'None'
    els.currentMetric.textContent = slot.enabled ? (slot.metric || 'N/A') : 'Cost: -'
    els.bindMode.textContent = 'Bind Mode: ' + ((state.snapshot.bindMode && state.snapshot.bindMode.label) || slotLabel(state.selectedSlot))

    const cool = Number(current.triggerCooldownSec || 1.5)
    els.cooldownSlider.value = cool
    els.cooldownLabel.textContent = cool.toFixed(1) + 's'
    els.onlyCombat.checked = !!current.onlyInCombat

    els.slotTabs.forEach((tab) => {
      tab.classList.toggle('active', Number(tab.dataset.slot) === state.selectedSlot)
    })
  }

  function renderSettings() {
    const cfg = state.snapshot.config || {}
    els.uiHotkey.textContent = cfg.uiToggleKey || 'F2'
    els.bindHotkey.textContent = cfg.bindKey || 'G'
    els.enabled.checked = !!cfg.enabled
    els.hudNotify.checked = !!cfg.showHudNotifications
    els.soundCues.checked = !!cfg.enableSoundCues
    els.hudDonut.checked = !!cfg.hudDonutEnabled
    els.hudUnsheathed.checked = !!cfg.hudDonutOnlyUnsheathed
    els.blacklistEnabled.checked = !!cfg.blacklistEnabled
    els.dedupeSlider.value = Number(cfg.fallbackDedupeSec || 1.5)
    els.dedupeLabel.textContent = Number(cfg.fallbackDedupeSec || 1.5).toFixed(1) + 's'
  }

  function renderStatus() {
    const status = state.snapshot.status || {}
    els.lastTrigger.textContent = status.lastTriggerSpell ? (status.lastTriggerSpell + ' via ' + status.lastTriggerWeapon) : '-'
    els.lastMode.textContent = status.lastWasPowerAttack ? 'Power' : 'Normal'
    els.lastError.textContent = status.lastError || '-'
  }

  function renderAll() {
    renderBindings(); renderSpells(); renderBlacklist(); renderCurrent(); renderSettings(); renderStatus()
  }

  function setSetting(id, value) { skse('sb_set_setting', JSON.stringify({ id, value })) }

  window.sb_renderSnapshot = function (payload) {
    try {
      state.snapshot = JSON.parse(payload || '{}')
      state.blacklistSet = new Set(state.snapshot.blacklist || [])
      renderAll()
    } catch (e) { showToast('Failed snapshot parse') }
  }
  window.sb_showToast = showToast
  window.sb_setFocusState = function () {}

  els.search.addEventListener('input', renderBindings)
  els.spellSearch.addEventListener('input', renderSpells)
  els.blacklistSearch.addEventListener('input', renderBlacklist)

  els.slotTabs.forEach((tab) => tab.addEventListener('click', () => { state.selectedSlot = Number(tab.dataset.slot || '0'); renderCurrent() }))
  els.cycleSlotBtn.addEventListener('click', () => skse('sb_cycle_bind_slot', ''))

  els.bindSelected.addEventListener('click', () => {
    const key = currentKey(); if (!key || !state.selectedSpellKey) return
    skse('sb_bind_spell_for_slot', JSON.stringify({ key, slot: state.selectedSlot, spellFormKey: state.selectedSpellKey }))
  })
  els.unbindSelected.addEventListener('click', () => {
    const key = currentKey(); if (!key) return
    skse('sb_unbind_slot', JSON.stringify({ key, slot: state.selectedSlot }))
  })
  els.bindMagicMenu.addEventListener('click', () => skse('sb_bind_selected_magic_menu_spell', ''))

  els.cooldownSlider.addEventListener('input', () => { els.cooldownLabel.textContent = Number(els.cooldownSlider.value).toFixed(1) + 's' })
  els.cooldownSlider.addEventListener('change', () => {
    const key = currentKey(); if (!key) return
    skse('sb_set_weapon_setting', JSON.stringify({ key, id: 'triggerCooldownSec', value: Number(els.cooldownSlider.value) }))
  })
  els.onlyCombat.addEventListener('change', () => {
    const key = currentKey(); if (!key) return
    skse('sb_set_weapon_setting', JSON.stringify({ key, id: 'onlyInCombat', value: !!els.onlyCombat.checked }))
  })

  els.settingsBtn.addEventListener('click', () => els.settingsModal.classList.remove('hidden'))
  els.settingsClose.addEventListener('click', () => els.settingsModal.classList.add('hidden'))
  els.blacklistBtn.addEventListener('click', () => els.blacklistModal.classList.remove('hidden'))
  els.blacklistClose.addEventListener('click', () => els.blacklistModal.classList.add('hidden'))
  els.blacklistSave.addEventListener('click', () => skse('sb_set_blacklist', JSON.stringify({ items: Array.from(state.blacklistSet) })))

  els.closeBtn.addEventListener('click', () => skse('sb_toggle_ui', ''))
  els.cooldownEditBtn.addEventListener('click', () => {
    els.dragBanner.classList.remove('hidden')
    skse('sb_enter_hud_drag_mode', '')
  })
  els.dragSaveBtn.addEventListener('click', () => {
    const cfg = state.snapshot.config || {}
    skse('sb_save_hud_position', JSON.stringify({ x: Number(cfg.hudPosX || 48), y: Number(cfg.hudPosY || 48), commit: true }))
    els.dragBanner.classList.add('hidden')
  })

  els.enabled.addEventListener('change', () => setSetting('enabled', !!els.enabled.checked))
  els.hudNotify.addEventListener('change', () => setSetting('showHudNotifications', !!els.hudNotify.checked))
  els.soundCues.addEventListener('change', () => setSetting('enableSoundCues', !!els.soundCues.checked))
  els.hudDonut.addEventListener('change', () => setSetting('hudDonutEnabled', !!els.hudDonut.checked))
  els.hudUnsheathed.addEventListener('change', () => setSetting('hudDonutOnlyUnsheathed', !!els.hudUnsheathed.checked))
  els.blacklistEnabled.addEventListener('change', () => setSetting('blacklistEnabled', !!els.blacklistEnabled.checked))
  els.dedupeSlider.addEventListener('input', () => { els.dedupeLabel.textContent = Number(els.dedupeSlider.value).toFixed(1) + 's' })
  els.dedupeSlider.addEventListener('change', () => setSetting('fallbackDedupeSec', Number(els.dedupeSlider.value)))

  window.addEventListener('keydown', function (e) {
    if (e.key !== 'Escape') return
    if (!els.settingsModal.classList.contains('hidden')) { els.settingsModal.classList.add('hidden'); return }
    if (!els.blacklistModal.classList.contains('hidden')) { els.blacklistModal.classList.add('hidden'); return }
    skse('sb_toggle_ui', '')
  })

  skse('sb_request_snapshot', '')
})()
