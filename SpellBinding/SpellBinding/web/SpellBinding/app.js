(function () {
  const state = {
    snapshot: { spellBinding: {}, smartCast: {}, quickBuff: {}, mastery: {} },
    blacklistSet: new Set(),
    tab: "spellBinding",
    masteryTab: "spells",
    smartCastActiveChain: 1,
    drag: null,
    dragPos: { x: 48, y: 48 }
  }

  const byId = (id) => document.getElementById(id)
  const callNative = (name, payload) => {
    const fn = window[name]
    if (typeof fn === "function") {
      fn(payload || "")
    }
  }

  const triggerOrder = ["combatStart", "combatEnd", "healthBelow70", "healthBelow50", "healthBelow30", "crouchStart", "sprintStart", "weaponDraw", "powerAttackStart", "shoutStart"]

  function toast(text) {
    const el = byId("toast")
    if (!el) return
    el.textContent = text || ""
    el.classList.add("show")
    clearTimeout(window.__sboToast)
    window.__sboToast = setTimeout(() => el.classList.remove("show"), 1400)
  }

  function setSetting(module, id, value, extra) {
    const payload = Object.assign({ module, id, value }, extra || {})
    callNative("sbo_set_setting", JSON.stringify(payload))
  }

  function doAction(module, action, extra) {
    const payload = Object.assign({ module, action }, extra || {})
    callNative("sbo_action", JSON.stringify(payload))
  }

  function hudAction(action, extra) {
    const payload = Object.assign({ action }, extra || {})
    callNative("sbo_hud_action", JSON.stringify(payload))
  }

  function slotLabel(slot) {
    return slot === 1 ? "Power" : (slot === 2 ? "Bash" : "Light")
  }

  function spellTypeLabel(type) {
    if (type === 1 || type === "1") return "Concentration"
    return "Fire"
  }

  function castOnLabel(castOn) {
    if (castOn === 1 || castOn === "1") return "Crosshair"
    if (castOn === 2 || castOn === "2") return "Aimed"
    return "Self"
  }

  function currentSlot() {
    const sb = state.snapshot.spellBinding || {}
    return Number(sb.bindMode?.slot || 0)
  }

  function currentWeaponKey() {
    const sb = state.snapshot.spellBinding || {}
    return sb.currentWeapon && sb.currentWeapon.key ? sb.currentWeapon.key : null
  }

  function getCurrentSlotSpell() {
    const sb = state.snapshot.spellBinding || {}
    const current = sb.currentWeapon || {}
    const slots = current.slots || {}
    const slot = currentSlot()
    if (slot === 1) return slots.power || {}
    if (slot === 2) return slots.bash || {}
    return slots.light || {}
  }

  function renderTabs() {
    document.querySelectorAll(".tab").forEach((tab) => {
      tab.classList.toggle("active", tab.dataset.tab === state.tab)
    })
    document.querySelectorAll(".tab-panel").forEach((panel) => {
      panel.classList.toggle("active", panel.dataset.panel === state.tab)
    })
  }

  function renderSpellBinding() {
    const sb = state.snapshot.spellBinding || {}
    state.blacklistSet = new Set(sb.blacklist || [])

    byId("sb-current-weapon").textContent = sb.currentWeapon?.displayName || "None"
    byId("sb-current-slot").textContent = slotLabel(currentSlot())
    const slotSpell = getCurrentSlotSpell()
    byId("sb-current-spell").textContent = slotSpell.enabled ? (slotSpell.displayName || "Unknown") : "None"
    byId("sb-current-metric").textContent = slotSpell.enabled ? (slotSpell.metric || "-") : "-"

    const cooldown = Number(sb.currentWeapon?.triggerCooldownSec || 1.5)
    const cooldownSlider = byId("sb-cooldown")
    cooldownSlider.value = String(cooldown)
    byId("sb-cooldown-label").textContent = cooldown.toFixed(1) + "s"
    byId("sb-only-combat").checked = !!sb.currentWeapon?.onlyInCombat

    const query = (byId("sb-search").value || "").toLowerCase().trim()
    const list = byId("sb-binding-list")
    list.innerHTML = ""
    const rows = (sb.bindings || []).filter((it) => {
      const name = String(it?.key?.displayName || "").toLowerCase()
      return !query || name.includes(query)
    })
    if (!rows.length) {
      list.innerHTML = "<p class='meta'>No bound weapons.</p>"
    } else {
      rows.forEach((row) => {
        const node = document.createElement("div")
        node.className = "item"
        node.innerHTML = "<div><div>" + (row.key?.displayName || "Unknown") + "</div><div class='meta'>L: " + (row.summary?.light || "-") + " | P: " + (row.summary?.power || "-") + " | B: " + (row.summary?.bash || "-") + "</div></div>"
        const btn = document.createElement("button")
        btn.className = "btn"
        btn.textContent = "Unbind"
        btn.addEventListener("click", () => doAction("spellBinding", "unbindWeapon", row.key || {}))
        node.appendChild(btn)
        list.appendChild(node)
      })
    }

    const search = (byId("sb-blacklist-search").value || "").toLowerCase().trim()
    const blocked = byId("sb-blacklist-blocked")
    const available = byId("sb-blacklist-available")
    blocked.innerHTML = ""
    available.innerHTML = ""

    ;(sb.knownSpells || []).forEach((spell) => {
      const name = String(spell.displayName || "")
      if (search && !name.toLowerCase().includes(search)) return
      const isBlocked = state.blacklistSet.has(spell.spellFormKey)
      const btn = document.createElement("button")
      btn.className = "btn"
      btn.textContent = name
      btn.addEventListener("click", () => {
        if (isBlocked) state.blacklistSet.delete(spell.spellFormKey)
        else state.blacklistSet.add(spell.spellFormKey)
        setSetting("spellBinding", "blacklist", Array.from(state.blacklistSet), { items: Array.from(state.blacklistSet) })
      })
      ;(isBlocked ? blocked : available).appendChild(btn)
    })
  }

  function renderSmartCast() {
    const sc = state.snapshot.smartCast || {}
    const cfg = sc.config || {}
    const globalCfg = cfg.global || {}
    const record = globalCfg.record || {}
    const playback = globalCfg.playback || {}
    const chains = cfg.chains || []

    if (chains.length > 0 && state.smartCastActiveChain > chains.length) {
      state.smartCastActiveChain = 1
    }

    byId("sc-record-key").value = record.toggleKey || "R"
    byId("sc-play-key").value = playback.playKey || "G"
    byId("sc-step-delay").value = String(Number(playback.stepDelaySec || 0.1).toFixed(2))

    const tabs = byId("sc-chain-tabs")
    tabs.innerHTML = ""
    chains.forEach((chain, i) => {
      const idx = i + 1
      const tab = document.createElement("button")
      tab.className = "subtab" + (idx === state.smartCastActiveChain ? " active" : "")
      tab.textContent = chain.name || ("Chain " + idx)
      tab.addEventListener("click", () => {
        state.smartCastActiveChain = idx
        renderSmartCast()
      })
      tabs.appendChild(tab)
    })

    const selected = chains[Math.max(0, state.smartCastActiveChain - 1)] || { name: "Chain 1", steps: [] }
    const panel = byId("sc-chain-panel")
    panel.innerHTML = ""

    const nameRow = document.createElement("div")
    nameRow.className = "row"
    nameRow.innerHTML = "<label>Chain Name <input id='sc-chain-name' /></label>"
    const saveName = document.createElement("button")
    saveName.className = "btn"
    saveName.textContent = "Save Name"
    saveName.addEventListener("click", () => {
      setSetting("smartCast", "chain.name", byId("sc-chain-name").value, { index: state.smartCastActiveChain })
    })
    const clearChain = document.createElement("button")
    clearChain.className = "btn"
    clearChain.textContent = "Clear Chain"
    clearChain.addEventListener("click", () => doAction("smartCast", "clearChain", { index: state.smartCastActiveChain }))
    nameRow.appendChild(saveName)
    nameRow.appendChild(clearChain)

    const stepsHeader = document.createElement("div")
    stepsHeader.className = "muted"
    stepsHeader.textContent = "Steps: " + Number(selected.steps?.length || 0)

    const stepList = document.createElement("div")
    stepList.className = "step-list"
    if (!selected.steps || selected.steps.length === 0) {
      stepList.innerHTML = "<p class='muted'>No recorded steps for this chain.</p>"
    } else {
      selected.steps.forEach((step, idx) => {
        const row = document.createElement("div")
        row.className = "step-row"
        row.textContent = (idx + 1) + ". " + (step.spellFormID || "Unknown") + " | " + spellTypeLabel(step.type) + " | " + castOnLabel(step.castOn) + " | Hold " + Number(step.holdSec || 0).toFixed(2) + "s"
        stepList.appendChild(row)
      })
    }

    panel.appendChild(nameRow)
    panel.appendChild(stepsHeader)
    panel.appendChild(stepList)

    const nameInput = byId("sc-chain-name")
    if (nameInput) nameInput.value = selected.name || ("Chain " + state.smartCastActiveChain)
  }

  function renderQuickBuff() {
    const qb = state.snapshot.quickBuff || {}
    const cfg = qb.config || {}
    const triggers = cfg.triggers || {}
    const knownSpells = qb.knownSpells || []

    const grid = byId("qb-trigger-grid")
    grid.innerHTML = ""

    triggerOrder.forEach((triggerId) => {
      const t = triggers[triggerId] || {}
      const card = document.createElement("div")
      card.className = "trigger-card"

      const title = document.createElement("div")
      title.className = "trigger-title"
      title.textContent = triggerId

      const row1 = document.createElement("div")
      row1.className = "trigger-row"
      const enabledLabel = document.createElement("label")
      enabledLabel.innerHTML = "<input type='checkbox' " + (t.enabled ? "checked" : "") + " /> Enabled"
      enabledLabel.querySelector("input").addEventListener("change", (e) => {
        setSetting("quickBuff", "trigger.enabled", !!e.target.checked, { trigger: triggerId })
      })
      row1.appendChild(enabledLabel)

      const row2 = document.createElement("div")
      row2.className = "trigger-row"
      const spellSelect = document.createElement("select")
      const none = document.createElement("option")
      none.value = ""
      none.textContent = "None"
      spellSelect.appendChild(none)
      knownSpells.forEach((spell) => {
        const opt = document.createElement("option")
        opt.value = spell.formKey || ""
        opt.textContent = spell.name || "Unknown"
        spellSelect.appendChild(opt)
      })
      spellSelect.value = t.spellFormID || ""
      spellSelect.addEventListener("change", () => {
        setSetting("quickBuff", "trigger.spellFormID", spellSelect.value, { trigger: triggerId })
      })
      row2.appendChild(spellSelect)

      const row3 = document.createElement("div")
      row3.className = "trigger-row"
      const cooldown = document.createElement("input")
      cooldown.type = "number"
      cooldown.step = "0.5"
      cooldown.min = "0"
      cooldown.max = "120"
      cooldown.value = String(Number(t.cooldownSec || 5).toFixed(1))
      cooldown.addEventListener("change", () => {
        setSetting("quickBuff", "trigger.cooldownSec", Number(cooldown.value || 0), { trigger: triggerId })
      })
      const testBtn = document.createElement("button")
      testBtn.className = "btn"
      testBtn.textContent = "Test"
      testBtn.addEventListener("click", () => doAction("quickBuff", "testTrigger", { trigger: triggerId }))
      row3.appendChild(cooldown)
      row3.appendChild(testBtn)

      card.appendChild(title)
      card.appendChild(row1)
      card.appendChild(row2)
      card.appendChild(row3)
      grid.appendChild(card)
    })
  }

  function renderMastery() {
    const m = state.snapshot.mastery || {}
    const panel = byId("mastery-panel")
    if (!panel) return

    document.querySelectorAll("[data-mastery-tab]").forEach((btn) => {
      btn.classList.toggle("active", btn.dataset.masteryTab === state.masteryTab)
    })

    const section = state.masteryTab === "spells" ? (m.spell || {}) :
      state.masteryTab === "shouts" ? (m.shout || {}) :
      state.masteryTab === "weapons" ? (m.weapon || {}) : null

    if (state.masteryTab === "overview") {
      panel.innerHTML = ""
      const summary = document.createElement("div")
      summary.className = "mastery-table"
      summary.innerHTML =
        "<div class='item'><div><div>Spell Mastery Entries</div><div class='meta'>" + Number(m.spell?.rows?.length || 0) + "</div></div></div>" +
        "<div class='item'><div><div>Shout Mastery Entries</div><div class='meta'>" + Number(m.shout?.rows?.length || 0) + "</div></div></div>" +
        "<div class='item'><div><div>Weapon Mastery Entries</div><div class='meta'>" + Number(m.weapon?.rows?.length || 0) + "</div></div></div>"
      panel.appendChild(summary)
      return
    }

    const prefix = state.masteryTab === "spells" ? "masterySpell" :
      state.masteryTab === "shouts" ? "masteryShout" : "masteryWeapon"
    const rows = section?.rows || []
    const cfg = section?.config || {}

    panel.innerHTML = ""
    const controls = document.createElement("div")
    controls.className = "row"
    controls.innerHTML = "<label><input id='m-enabled' type='checkbox' " + (cfg.enabled ? "checked" : "") + " /> Enabled</label>" +
      "<label>Gain x <input id='m-gain' type='number' step='0.1' min='0.1' value='" + Number(cfg.gainMultiplier || 1).toFixed(1) + "' /></label>"
    const saveBtn = document.createElement("button")
    saveBtn.className = "btn"
    saveBtn.textContent = "Save"
    saveBtn.addEventListener("click", () => {
      setSetting("mastery", prefix + ".enabled", !!byId("m-enabled").checked)
      setSetting("mastery", prefix + ".gainMultiplier", Number(byId("m-gain").value || 1))
    })
    const resetBtn = document.createElement("button")
    resetBtn.className = "btn"
    resetBtn.textContent = "Reset Defaults"
    resetBtn.addEventListener("click", () => doAction("mastery", prefix + ".resetDefaults"))
    const clearBtn = document.createElement("button")
    clearBtn.className = "btn"
    clearBtn.textContent = "Clear DB"
    clearBtn.addEventListener("click", () => doAction("mastery", prefix + ".clearDatabase"))
    const saveAllBtn = document.createElement("button")
    saveAllBtn.className = "btn"
    saveAllBtn.textContent = "Save All Settings"
    saveAllBtn.addEventListener("click", () => {
      try {
        const parsed = JSON.parse(byId("m-config-json").value || "{}")
        setSetting("mastery", prefix + ".config", parsed)
      } catch (_) {
        toast("Invalid mastery JSON")
      }
    })
    controls.appendChild(saveBtn)
    controls.appendChild(saveAllBtn)
    controls.appendChild(resetBtn)
    controls.appendChild(clearBtn)

    const info = document.createElement("div")
    info.className = "muted"
    info.textContent = "Entries: " + Number(rows.length)

    const jsonWrap = document.createElement("div")
    jsonWrap.className = "row"
    jsonWrap.innerHTML = "<label style='display:grid;gap:6px;width:100%'>All Settings JSON<textarea id='m-config-json' style='width:100%;min-height:210px;resize:vertical;font-family:Consolas,monospace'></textarea></label>"

    const table = document.createElement("div")
    table.className = "mastery-table"
    if (!rows.length) {
      table.innerHTML = "<p class='muted'>No mastery data yet.</p>"
    } else {
      rows.forEach((row) => {
        const item = document.createElement("div")
        item.className = "item"
        item.innerHTML = "<div><div>" + (row.name || "Unknown") + "</div><div class='meta'>Level " + Number(row.level || 0) + " | Uses " + Number(row.uses || row.hits || 0) + "</div></div>"
        table.appendChild(item)
      })
    }

    panel.appendChild(controls)
    panel.appendChild(info)
    panel.appendChild(jsonWrap)
    panel.appendChild(table)
    const editor = byId("m-config-json")
    if (editor) {
      editor.value = JSON.stringify(cfg || {}, null, 2)
    }
  }

  function renderAll() {
    renderTabs()
    renderSpellBinding()
    renderSmartCast()
    renderQuickBuff()
    renderMastery()

    const sbCfg = state.snapshot.spellBinding?.config || {}
    state.dragPos.x = Number(sbCfg.hudPosX || 48)
    state.dragPos.y = Number(sbCfg.hudPosY || 48)
    const dragSize = byId("drag-size")
    dragSize.value = String(Number(sbCfg.hudDonutSize || 88))
    byId("drag-seconds").checked = sbCfg.hudShowCooldownSeconds !== false
    const widget = byId("drag-widget")
    widget.style.left = state.dragPos.x + "px"
    widget.style.top = state.dragPos.y + "px"
    widget.style.width = dragSize.value + "px"
    widget.style.height = dragSize.value + "px"
  }

  window.sbo_renderSnapshot = function (payload) {
    try {
      state.snapshot = JSON.parse(payload || "{}")
      renderAll()
    } catch (_) {
      toast("Snapshot parse failed")
    }
  }
  window.sbo_showToast = toast
  window.sbo_setFocusState = function () {}
  window.sbo_native_escape = function () {
    if (!byId("drag-overlay").classList.contains("hidden")) {
      closeDragOverlay()
      return
    }
    callNative("sbo_toggle_ui", "")
  }
  window.sbo_close_ui = function () {
    callNative("sbo_toggle_ui", "")
  }

  document.querySelectorAll(".tab").forEach((tab) => {
    tab.addEventListener("click", () => {
      state.tab = tab.dataset.tab
      renderTabs()
    })
  })
  document.querySelectorAll("[data-mastery-tab]").forEach((tab) => {
    tab.addEventListener("click", () => {
      state.masteryTab = tab.dataset.masteryTab
      renderMastery()
    })
  })

  byId("close-ui")?.addEventListener("click", () => callNative("sbo_toggle_ui", ""))
  byId("sb-search")?.addEventListener("input", renderSpellBinding)
  byId("sb-blacklist-search")?.addEventListener("input", renderSpellBinding)

  byId("sb-cycle-slot")?.addEventListener("click", () => doAction("spellBinding", "cycleBindSlot"))
  byId("sb-unbind-slot")?.addEventListener("click", () => {
    const key = currentWeaponKey()
    if (!key) return
    doAction("spellBinding", "unbindSlot", { key, slot: currentSlot() })
  })

  byId("sb-cooldown")?.addEventListener("input", () => {
    byId("sb-cooldown-label").textContent = Number(byId("sb-cooldown").value).toFixed(1) + "s"
  })
  byId("sb-cooldown")?.addEventListener("change", () => {
    const key = currentWeaponKey()
    if (!key) return
    setSetting("spellBinding", "weapon", Number(byId("sb-cooldown").value), { key, id: "triggerCooldownSec", value: Number(byId("sb-cooldown").value) })
  })
  byId("sb-only-combat")?.addEventListener("change", () => {
    const key = currentWeaponKey()
    if (!key) return
    setSetting("spellBinding", "weapon", !!byId("sb-only-combat").checked, { key, id: "onlyInCombat", value: !!byId("sb-only-combat").checked })
  })

  byId("sc-save-settings")?.addEventListener("click", () => {
    setSetting("smartCast", "record.toggleKey", byId("sc-record-key").value)
    setSetting("smartCast", "playback.playKey", byId("sc-play-key").value)
    setSetting("smartCast", "playback.stepDelaySec", Number(byId("sc-step-delay").value || 0.1))
  })

  function openDragOverlay() {
    byId("drag-overlay").classList.remove("hidden")
    const widget = byId("drag-widget")
    widget.style.left = state.dragPos.x + "px"
    widget.style.top = state.dragPos.y + "px"
    hudAction("enterDragMode")
  }

  function closeDragOverlay() {
    const widget = byId("drag-widget")
    state.dragPos.x = widget.offsetLeft
    state.dragPos.y = widget.offsetTop
    byId("drag-overlay").classList.add("hidden")
    hudAction("saveHudPosition", { x: state.dragPos.x, y: state.dragPos.y, commit: true })
  }

  byId("sb-open-drag")?.addEventListener("click", openDragOverlay)
  byId("drag-close")?.addEventListener("click", closeDragOverlay)
  byId("drag-size")?.addEventListener("input", () => {
    const size = Number(byId("drag-size").value || 88)
    const widget = byId("drag-widget")
    widget.style.width = size + "px"
    widget.style.height = size + "px"
  })
  byId("drag-size")?.addEventListener("change", () => {
    setSetting("spellBinding", "hudDonutSize", Number(byId("drag-size").value || 88))
  })
  byId("drag-seconds")?.addEventListener("change", () => {
    setSetting("spellBinding", "hudShowCooldownSeconds", !!byId("drag-seconds").checked)
  })

  const dragWidget = byId("drag-widget")
  dragWidget?.addEventListener("mousedown", (e) => {
    if (byId("drag-overlay").classList.contains("hidden")) return
    state.drag = { x: e.clientX, y: e.clientY, left: dragWidget.offsetLeft, top: dragWidget.offsetTop }
    e.preventDefault()
  })
  window.addEventListener("mousemove", (e) => {
    if (!state.drag || !dragWidget) return
    const nextX = Math.max(0, Math.min(window.innerWidth - dragWidget.offsetWidth, state.drag.left + (e.clientX - state.drag.x)))
    const nextY = Math.max(0, Math.min(window.innerHeight - dragWidget.offsetHeight, state.drag.top + (e.clientY - state.drag.y)))
    dragWidget.style.left = nextX + "px"
    dragWidget.style.top = nextY + "px"
    callNative("sbo_hud_drag_update", JSON.stringify({ x: nextX, y: nextY }))
  })
  window.addEventListener("mouseup", () => {
    if (!state.drag || !dragWidget) return
    state.drag = null
    callNative("sbo_hud_drag_commit", JSON.stringify({ x: dragWidget.offsetLeft, y: dragWidget.offsetTop, commit: true }))
  })

  window.addEventListener("keydown", (e) => {
    if (e.key === "Escape") {
      window.sbo_native_escape()
    }
  })

  callNative("sbo_request_snapshot", "")
})()
