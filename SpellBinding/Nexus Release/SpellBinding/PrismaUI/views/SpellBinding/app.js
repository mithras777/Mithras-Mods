(function () {
  const state = { snapshot: null, focused: false };
  const byId = (id) => document.getElementById(id);

  const els = {
    search: byId('search'),
    list: byId('binding-list'),
    currentWeapon: byId('current-weapon'),
    currentSlot: byId('current-slot'),
    currentSpell: byId('current-spell'),
    currentCost: byId('current-cost'),
    uiHotkey: byId('ui-hotkey'),
    bindHotkey: byId('bind-hotkey'),
    lastTrigger: byId('last-trigger'),
    lastCost: byId('last-cost'),
    lastMode: byId('last-mode'),
    lastError: byId('last-error'),
    toast: byId('toast'),
    enabled: byId('enabled'),
    hudNotify: byId('hud-notify'),
    powerDmg: byId('power-dmg'),
    powerMag: byId('power-mag'),
    bindBtn: byId('bind-btn'),
    toggleBtn: byId('toggle-btn'),
    refreshBtn: byId('refresh-btn'),
  };

  function handName(slot) {
    if (slot === 1) return 'Left Hand';
    if (slot === 2) return 'Unarmed';
    return 'Right Hand';
  }

  function renderList() {
    if (!state.snapshot) return;
    const query = (els.search.value || '').toLowerCase();
    const items = (state.snapshot.bindings || []).filter((entry) => {
      const weapon = entry.key?.displayName || '';
      const spell = entry.spell?.displayName || '';
      return weapon.toLowerCase().includes(query) || spell.toLowerCase().includes(query);
    });

    els.list.innerHTML = '';
    if (!items.length) {
      els.list.innerHTML = '<div class="muted">No bound weapons.</div>';
      return;
    }

    items.forEach((entry) => {
      const node = document.createElement('div');
      node.className = 'binding-item';
      node.innerHTML =
        `<div class="weapon">${entry.key.displayName} (${handName(entry.key.handSlot)})</div>` +
        `<div class="spell">${entry.spell.displayName}</div>`;

      const unbind = document.createElement('button');
      unbind.className = 'btn subtle';
      unbind.style.marginTop = '6px';
      unbind.textContent = 'Unbind';
      unbind.addEventListener('click', () => {
        if (window.sb_unbind_weapon) {
          window.sb_unbind_weapon(JSON.stringify(entry.key));
        }
      });
      node.appendChild(unbind);
      els.list.appendChild(node);
    });
  }

  function renderSnapshot(snapshot) {
    state.snapshot = snapshot;

    const current = snapshot.currentWeapon || {};
    els.currentWeapon.textContent = current.displayName || 'None';
    els.currentSlot.textContent = handName(current.handSlot || 0);
    els.currentSpell.textContent = current.hasBinding ? (current.spellName || 'Unknown') : 'None';
    els.currentCost.textContent = `Cost: ${current.hasBinding ? (current.lastKnownCost || 0).toFixed(1) : '-'}`;

    const config = snapshot.config || {};
    els.uiHotkey.textContent = config.uiToggleKey || 'F2';
    els.bindHotkey.textContent = config.bindKey || 'G';
    els.enabled.checked = !!config.enabled;
    els.hudNotify.checked = !!config.showHudNotifications;
    els.powerDmg.value = config.powerDamageScale || 2.0;
    els.powerMag.value = config.powerMagickaScale || 2.0;

    const status = snapshot.status || {};
    els.lastTrigger.textContent = status.lastTriggerSpell ? `${status.lastTriggerSpell} via ${status.lastTriggerWeapon}` : '-';
    els.lastCost.textContent = status.lastMagickaCost ? Number(status.lastMagickaCost).toFixed(1) : '-';
    els.lastMode.textContent = status.lastWasPowerAttack ? 'Power Attack' : 'Normal';
    els.lastError.textContent = status.lastError || '-';

    renderList();
  }

  window.sb_renderSnapshot = function (payload) {
    try {
      renderSnapshot(JSON.parse(payload || '{}'));
    } catch {
      window.sb_showToast('Failed to parse snapshot payload');
    }
  };

  window.sb_showToast = function (text) {
    els.toast.textContent = text || '';
    els.toast.classList.add('show');
    setTimeout(() => els.toast.classList.remove('show'), 1600);
  };

  window.sb_setFocusState = function (payload) {
    try {
      const parsed = JSON.parse(payload || '{}');
      state.focused = !!parsed.focused;
    } catch {
      state.focused = false;
    }
  };

  els.search.addEventListener('input', renderList);
  els.bindBtn.addEventListener('click', () => window.sb_bind_selected_magic_menu_spell && window.sb_bind_selected_magic_menu_spell(''));
  els.toggleBtn.addEventListener('click', () => window.sb_toggle_ui && window.sb_toggle_ui(''));
  els.refreshBtn.addEventListener('click', () => window.sb_request_snapshot && window.sb_request_snapshot(''));

  els.enabled.addEventListener('change', () => {
    window.sb_set_setting && window.sb_set_setting(JSON.stringify({ id: 'enabled', value: !!els.enabled.checked }));
  });
  els.hudNotify.addEventListener('change', () => {
    window.sb_set_setting && window.sb_set_setting(JSON.stringify({ id: 'showHudNotifications', value: !!els.hudNotify.checked }));
  });
  els.powerDmg.addEventListener('change', () => {
    window.sb_set_setting && window.sb_set_setting(JSON.stringify({ id: 'powerDamageScale', value: Number(els.powerDmg.value) }));
  });
  els.powerMag.addEventListener('change', () => {
    window.sb_set_setting && window.sb_set_setting(JSON.stringify({ id: 'powerMagickaScale', value: Number(els.powerMag.value) }));
  });

  if (window.sb_request_snapshot) {
    window.sb_request_snapshot('');
  }
})();
