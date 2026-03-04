
import React, { useEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';

const triggerOrder = ['combatStart', 'combatEnd', 'healthBelow70', 'healthBelow50', 'healthBelow30', 'crouchStart', 'sprintStart', 'weaponDraw', 'powerAttackStart', 'shoutStart'];
const WINDOW_MIN_WIDTH = 960;
const WINDOW_MIN_HEIGHT = 620;
const WINDOW_MARGIN = 8;

function byPath(obj, path, fallback) {
  let cur = obj;
  for (const part of path) {
    if (cur == null || typeof cur !== 'object') return fallback;
    cur = cur[part];
  }
  return cur ?? fallback;
}

function callNative(name, payload = '') {
  const fn = window[name];
  if (typeof fn === 'function') fn(payload);
}

function setSetting(module, id, value, extra) {
  const payload = JSON.stringify(Object.assign({ module, id, value }, extra || {}));
  callNative('sbo_set_setting', payload);
}

function doAction(module, action, extra) {
  const payload = JSON.stringify(Object.assign({ module, action }, extra || {}));
  callNative('sbo_action', payload);
}

function hudAction(action, extra) {
  const payload = JSON.stringify(Object.assign({ action }, extra || {}));
  callNative('sbo_hud_action', payload);
}

function clampRect(rect) {
  const vw = window.innerWidth;
  const vh = window.innerHeight;
  const maxWidth = Math.max(WINDOW_MIN_WIDTH, vw - WINDOW_MARGIN * 2);
  const maxHeight = Math.max(WINDOW_MIN_HEIGHT, vh - WINDOW_MARGIN * 2);
  const width = Math.min(Math.max(rect.width || 1180, WINDOW_MIN_WIDTH), maxWidth);
  const height = Math.min(Math.max(rect.height || 760, WINDOW_MIN_HEIGHT), maxHeight);
  const minX = WINDOW_MARGIN;
  const minY = WINDOW_MARGIN;
  const maxX = Math.max(minX, vw - width - WINDOW_MARGIN);
  const maxY = Math.max(minY, vh - height - WINDOW_MARGIN);
  const x = Math.min(Math.max(rect.x ?? minX, minX), maxX);
  const y = Math.min(Math.max(rect.y ?? minY, minY), maxY);
  return { x, y, width, height, isFullscreen: !!rect.isFullscreen, hasSaved: true };
}

function defaultCenteredRect(width = 1180, height = 760) {
  const vw = window.innerWidth;
  const vh = window.innerHeight;
  const x = Math.max(WINDOW_MARGIN, Math.floor((vw - width) / 2));
  const y = Math.max(WINDOW_MARGIN, Math.floor((vh - height) / 2));
  return clampRect({ x, y, width, height, isFullscreen: false, hasSaved: false });
}

function fullscreenRect() {
  return {
    x: WINDOW_MARGIN,
    y: WINDOW_MARGIN,
    width: Math.max(WINDOW_MIN_WIDTH, window.innerWidth - WINDOW_MARGIN * 2),
    height: Math.max(WINDOW_MIN_HEIGHT, window.innerHeight - WINDOW_MARGIN * 2),
    isFullscreen: true,
    hasSaved: true
  };
}

function slotLabel(slot) {
  return slot === 1 ? 'Power' : slot === 2 ? 'Bash' : 'Light';
}

function spellTypeLabel(type) {
  return type === 1 || type === '1' ? 'Concentration' : 'Fire';
}

function castOnLabel(castOn) {
  if (castOn === 1 || castOn === '1') return 'Crosshair';
  if (castOn === 2 || castOn === '2') return 'Aimed';
  return 'Self';
}

function IconCog() {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <path d="M10.85 2.6c.36-1.2 2.04-1.2 2.4 0l.33 1.1a7.77 7.77 0 0 1 1.74.72l1-.56c1.08-.61 2.27.58 1.66 1.66l-.56 1a7.77 7.77 0 0 1 .72 1.74l1.1.33c1.2.36 1.2 2.04 0 2.4l-1.1.33a7.77 7.77 0 0 1-.72 1.74l.56 1c.61 1.08-.58 2.27-1.66 1.66l-1-.56a7.77 7.77 0 0 1-1.74.72l-.33 1.1c-.36 1.2-2.04 1.2-2.4 0l-.33-1.1a7.77 7.77 0 0 1-1.74-.72l-1 .56c-1.08.61-2.27-.58-1.66-1.66l.56-1a7.77 7.77 0 0 1-.72-1.74l-1.1-.33c-1.2-.36-1.2-2.04 0-2.4l1.1-.33c.16-.61.4-1.19.72-1.74l-.56-1c-.61-1.08.58-2.27 1.66-1.66l1 .56c.55-.32 1.13-.56 1.74-.72l.33-1.1zM12 9a3 3 0 1 0 0 6 3 3 0 0 0 0-6z" />
    </svg>
  );
}

function IconFullscreen() {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <path d="M4 4h6v2H6v4H4V4zm10 0h6v6h-2V6h-4V4zM4 14h2v4h4v2H4v-6zm14 0h2v6h-6v-2h4v-4z" />
    </svg>
  );
}

function IconClose() {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <path d="M6 6l12 12M18 6L6 18" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
    </svg>
  );
}

function App() {
  const [snapshot, setSnapshot] = useState({ spellBinding: {}, smartCast: {}, quickBuff: {}, mastery: {}, ui: {} });
  const [tab, setTab] = useState('spellBinding');
  const [masteryTab, setMasteryTab] = useState('spells');
  const [smartCastActiveChain, setSmartCastActiveChain] = useState(1);
  const [sbSearch, setSbSearch] = useState('');
  const [blacklistSearch, setBlacklistSearch] = useState('');
  const [toast, setToast] = useState('');
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [settingsTab, setSettingsTab] = useState('spellBinding');
  const [hudAdjustOpen, setHudAdjustOpen] = useState(false);
  const [windowState, setWindowState] = useState(defaultCenteredRect());

  const dragRef = useRef(null);
  const resizeRef = useRef(null);
  const lastWindowedRef = useRef(null);
  const toastTimerRef = useRef(null);
  const settingsOpenRef = useRef(false);
  const hudAdjustOpenRef = useRef(false);
  const hudConfigRef = useRef({ x: 48, y: 48 });

  const sb = snapshot.spellBinding || {};
  const sc = snapshot.smartCast || {};
  const qb = snapshot.quickBuff || {};
  const mastery = snapshot.mastery || {};

  const currentSlot = Number(byPath(sb, ['bindMode', 'slot'], 0));
  const currentWeapon = sb.currentWeapon || {};
  const currentSlots = currentWeapon.slots || {};
  const slotSpell = currentSlot === 1 ? currentSlots.power || {} : currentSlot === 2 ? currentSlots.bash || {} : currentSlots.light || {};

  const blacklistSet = useMemo(() => new Set(sb.blacklist || []), [sb.blacklist]);

  const bindRows = useMemo(() => {
    const query = sbSearch.toLowerCase().trim();
    return (sb.bindings || []).filter((it) => {
      const name = String(byPath(it, ['key', 'displayName'], '')).toLowerCase();
      return !query || name.includes(query);
    });
  }, [sb.bindings, sbSearch]);

  const knownSpellsFiltered = useMemo(() => {
    const query = blacklistSearch.toLowerCase().trim();
    return (sb.knownSpells || []).filter((spell) => {
      const name = String(spell.displayName || '').toLowerCase();
      return !query || name.includes(query);
    });
  }, [sb.knownSpells, blacklistSearch]);

  const showToast = (text) => {
    setToast(text || '');
    if (toastTimerRef.current) clearTimeout(toastTimerRef.current);
    toastTimerRef.current = setTimeout(() => setToast(''), 1400);
  };

  useEffect(() => {
    settingsOpenRef.current = settingsOpen;
  }, [settingsOpen]);

  useEffect(() => {
    hudAdjustOpenRef.current = hudAdjustOpen;
  }, [hudAdjustOpen]);

  useEffect(() => {
    hudConfigRef.current = {
      x: Number(byPath(sb, ['config', 'hudPosX'], 48)),
      y: Number(byPath(sb, ['config', 'hudPosY'], 48))
    };
  }, [sb]);

  const closeHudAdjust = () => {
    hudAction('saveHudPosition', {
      x: hudConfigRef.current.x,
      y: hudConfigRef.current.y,
      commit: true
    });
    setHudAdjustOpen(false);
  };

  useEffect(() => {
    window.sbo_renderSnapshot = (payload) => {
      try {
        const parsed = JSON.parse(payload || '{}');
        setSnapshot(parsed);
      } catch (_e) {
        showToast('Snapshot parse failed');
      }
    };
    window.sbo_showToast = showToast;
    window.sbo_setFocusState = () => {};
    window.sbo_native_escape = () => {
      if (hudAdjustOpenRef.current) {
        closeHudAdjust();
        return;
      }
      if (settingsOpenRef.current) {
        setSettingsOpen(false);
        return;
      }
      callNative('sbo_toggle_ui', '');
    };
    window.sbo_close_ui = () => callNative('sbo_toggle_ui', '');

    const keyHandler = (e) => {
      if (e.key === 'Escape') window.sbo_native_escape();
    };
    window.addEventListener('keydown', keyHandler);
    callNative('sbo_request_snapshot', '');

    return () => {
      window.removeEventListener('keydown', keyHandler);
      if (toastTimerRef.current) clearTimeout(toastTimerRef.current);
    };
  }, []);

  useEffect(() => {
    const incoming = byPath(snapshot, ['ui', 'spellBindingWindow'], null);
    if (!incoming) return;

    setWindowState(() => {
      const hasSaved = !!incoming.hasSaved;
      const baseRect = hasSaved
        ? {
            x: Number(incoming.x ?? 0),
            y: Number(incoming.y ?? 0),
            width: Number(incoming.width ?? 1180),
            height: Number(incoming.height ?? 760),
            isFullscreen: !!incoming.isFullscreen,
            hasSaved: true
          }
        : defaultCenteredRect(Number(incoming.width ?? 1180), Number(incoming.height ?? 760));
      if (baseRect.isFullscreen) return fullscreenRect();
      return clampRect(baseRect);
    });

    if (incoming && !incoming.isFullscreen) {
      lastWindowedRef.current = clampRect({
        x: Number(incoming.x ?? 0),
        y: Number(incoming.y ?? 0),
        width: Number(incoming.width ?? 1180),
        height: Number(incoming.height ?? 760),
        isFullscreen: false,
        hasSaved: true
      });
    }
  }, [snapshot.ui]);

  useEffect(() => {
    const resizeListener = () => {
      setWindowState((prev) => {
        if (prev.isFullscreen) return fullscreenRect();
        return clampRect(prev);
      });
    };
    window.addEventListener('resize', resizeListener);
    return () => window.removeEventListener('resize', resizeListener);
  }, []);

  const persistWindowState = (next) => {
    const payload = {
      x: next.x,
      y: next.y,
      width: next.width,
      height: next.height,
      isFullscreen: !!next.isFullscreen
    };
    setSetting('ui', 'spellBindingWindow', payload);
  };

  const startDrag = (event) => {
    if (windowState.isFullscreen) return;
    dragRef.current = {
      startX: event.clientX,
      startY: event.clientY,
      originX: windowState.x,
      originY: windowState.y
    };
    event.preventDefault();
  };

  const startResize = (event) => {
    if (windowState.isFullscreen) return;
    document.body.classList.add('is-resizing');
    resizeRef.current = {
      startX: event.clientX,
      startY: event.clientY,
      originW: windowState.width,
      originH: windowState.height
    };
    event.preventDefault();
  };

  useEffect(() => {
    const onMove = (event) => {
      if (dragRef.current) {
        const d = dragRef.current;
        const next = clampRect({
          ...windowState,
          x: d.originX + (event.clientX - d.startX),
          y: d.originY + (event.clientY - d.startY)
        });
        setWindowState((prev) => ({ ...prev, x: next.x, y: next.y }));
      } else if (resizeRef.current) {
        const r = resizeRef.current;
        const next = clampRect({
          ...windowState,
          width: r.originW + (event.clientX - r.startX),
          height: r.originH + (event.clientY - r.startY)
        });
        setWindowState((prev) => ({ ...prev, width: next.width, height: next.height }));
      }
    };

    const onUp = () => {
      if (dragRef.current || resizeRef.current) {
        const next = clampRect(windowState);
        setWindowState(next);
        persistWindowState(next);
      }
      document.body.classList.remove('is-resizing');
      dragRef.current = null;
      resizeRef.current = null;
    };

    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [windowState]);

  const toggleFullscreen = () => {
    if (windowState.isFullscreen) {
      const fallback = lastWindowedRef.current || defaultCenteredRect();
      const next = clampRect({ ...fallback, isFullscreen: false });
      setWindowState(next);
      persistWindowState(next);
    } else {
      lastWindowedRef.current = clampRect({ ...windowState, isFullscreen: false });
      const next = fullscreenRect();
      setWindowState(next);
      persistWindowState(next);
    }
  };

  const resetWindowLayout = () => {
    const next = defaultCenteredRect();
    setWindowState(next);
    lastWindowedRef.current = next;
    persistWindowState(next);
  };

  const toggleBlacklistSpell = (spellKey) => {
    const next = new Set(blacklistSet);
    if (next.has(spellKey)) next.delete(spellKey);
    else next.add(spellKey);
    const items = Array.from(next);
    setSetting('spellBinding', 'blacklist', items, { items });
  };

  const onCooldownChange = (value) => {
    if (!currentWeapon.key) return;
    setSetting('spellBinding', 'weapon', Number(value), {
      key: currentWeapon.key,
      id: 'triggerCooldownSec',
      value: Number(value)
    });
  };

  const onOnlyCombatChange = (checked) => {
    if (!currentWeapon.key) return;
    setSetting('spellBinding', 'weapon', checked, {
      key: currentWeapon.key,
      id: 'onlyInCombat',
      value: checked
    });
  };

  const chains = byPath(sc, ['config', 'chains'], []);
  const selectedChain = chains[Math.max(0, smartCastActiveChain - 1)] || { name: `Chain ${smartCastActiveChain}`, steps: [] };

  const panelStyle = windowState.isFullscreen
    ? {
        left: `${WINDOW_MARGIN}px`,
        top: `${WINDOW_MARGIN}px`,
        width: `calc(100vw - ${WINDOW_MARGIN * 2}px)`,
        height: `calc(100vh - ${WINDOW_MARGIN * 2}px)`
      }
    : {
        left: `${windowState.x}px`,
        top: `${windowState.y}px`,
        width: `${windowState.width}px`,
        height: `${windowState.height}px`
      };

  return (
    <>
      <div className="window" style={panelStyle}>
        <header className="window-header" onMouseDown={startDrag}>
          <div className="title-wrap">
            <h1>Spell Binding</h1>
            <p>A Spellblade Overhaul Control Panel</p>
          </div>
          <div className="window-actions" onMouseDown={(e) => e.stopPropagation()}>
            <button className="icon-btn" onClick={() => setSettingsOpen(true)} aria-label="Settings"><IconCog /></button>
            <button className="icon-btn" onClick={toggleFullscreen} aria-label="Fullscreen"><IconFullscreen /></button>
            <button className="icon-btn danger" onClick={() => callNative('sbo_toggle_ui', '')} aria-label="Close"><IconClose /></button>
          </div>
        </header>

        <nav className="tabs">
          <button className={tab === 'spellBinding' ? 'tab active' : 'tab'} onClick={() => setTab('spellBinding')}>Spell Binding</button>
          <button className={tab === 'smartCast' ? 'tab active' : 'tab'} onClick={() => setTab('smartCast')}>Smart Cast</button>
          <button className={tab === 'quickBuff' ? 'tab active' : 'tab'} onClick={() => setTab('quickBuff')}>Quick Buff</button>
          <button className={tab === 'mastery' ? 'tab active' : 'tab'} onClick={() => setTab('mastery')}>Mastery</button>
        </nav>

        <main className="content">
          {tab === 'spellBinding' && (
            <section className="grid sb-grid">
              <article className="card">
                <h3>Current</h3>
                <p><strong>Weapon:</strong> {byPath(sb, ['currentWeapon', 'displayName'], 'None')}</p>
                <p><strong>Slot:</strong> {slotLabel(currentSlot)}</p>
                <p><strong>Spell:</strong> {slotSpell.enabled ? slotSpell.displayName || 'Unknown' : 'None'}</p>
                <p><strong>Cost:</strong> {slotSpell.enabled ? slotSpell.metric || '-' : '-'}</p>
                <div className="row">
                  <button className="btn" onClick={() => doAction('spellBinding', 'cycleBindSlot')}>Cycle Slot</button>
                  <button className="btn" onClick={() => currentWeapon.key && doAction('spellBinding', 'unbindSlot', { key: currentWeapon.key, slot: currentSlot })}>Unbind Slot</button>
                  <button className="btn" onClick={() => { setHudAdjustOpen(true); hudAction('enterDragMode'); }}>Adjust HUD Donut</button>
                </div>
                <div className="row">
                  <label className="inline">
                    Cooldown
                    <input
                      type="range"
                      min="0.5"
                      max="5"
                      step="0.5"
                      value={Number(byPath(sb, ['currentWeapon', 'triggerCooldownSec'], 1.5))}
                      onChange={(e) => onCooldownChange(e.target.value)}
                    />
                  </label>
                  <span>{Number(byPath(sb, ['currentWeapon', 'triggerCooldownSec'], 1.5)).toFixed(1)}s</span>
                  <label className="inline checkbox-inline">
                    <input
                      type="checkbox"
                      checked={!!byPath(sb, ['currentWeapon', 'onlyInCombat'], false)}
                      onChange={(e) => onOnlyCombatChange(e.target.checked)}
                    />
                    Only combat
                  </label>
                </div>
              </article>

              <article className="card">
                <h3>Bound Weapons</h3>
                <input placeholder="Search weapon..." value={sbSearch} onChange={(e) => setSbSearch(e.target.value)} />
                <div className="scroll">
                  {bindRows.length === 0 && <p className="meta">No bound weapons.</p>}
                  {bindRows.map((row, idx) => (
                    <div className="item" key={`${row.key?.displayName || 'weapon'}-${idx}`}>
                      <div>
                        <div>{row.key?.displayName || 'Unknown'}</div>
                        <div className="meta">L: {row.summary?.light || '-'} | P: {row.summary?.power || '-'} | B: {row.summary?.bash || '-'}</div>
                      </div>
                      <button className="btn" onClick={() => doAction('spellBinding', 'unbindWeapon', row.key || {})}>Unbind</button>
                    </div>
                  ))}
                </div>
              </article>

              <article className="card">
                <h3>Blacklist</h3>
                <input placeholder="Search spell..." value={blacklistSearch} onChange={(e) => setBlacklistSearch(e.target.value)} />
                <div className="split">
                  <section>
                    <h4>Blocked</h4>
                    <div className="scroll">
                      {knownSpellsFiltered.filter((s) => blacklistSet.has(s.spellFormKey)).map((spell) => (
                        <button className="btn block" key={`blk-${spell.spellFormKey}`} onClick={() => toggleBlacklistSpell(spell.spellFormKey)}>
                          {spell.displayName}
                        </button>
                      ))}
                    </div>
                  </section>
                  <section>
                    <h4>Available</h4>
                    <div className="scroll">
                      {knownSpellsFiltered.filter((s) => !blacklistSet.has(s.spellFormKey)).map((spell) => (
                        <button className="btn block" key={`ok-${spell.spellFormKey}`} onClick={() => toggleBlacklistSpell(spell.spellFormKey)}>
                          {spell.displayName}
                        </button>
                      ))}
                    </div>
                  </section>
                </div>
              </article>
            </section>
          )}

          {tab === 'smartCast' && (
            <section className="grid sc-grid">
              <article className="card">
                <h3>Chains</h3>
                <div className="subtabs">
                  {chains.map((chain, i) => (
                    <button key={`chain-${i + 1}`} className={smartCastActiveChain === i + 1 ? 'subtab active' : 'subtab'} onClick={() => setSmartCastActiveChain(i + 1)}>
                      {chain.name || `Chain ${i + 1}`}
                    </button>
                  ))}
                </div>
                <div className="row">
                  <label className="grow">
                    Chain Name
                    <input id="sc-chain-name" defaultValue={selectedChain.name || `Chain ${smartCastActiveChain}`} onBlur={(e) => setSetting('smartCast', 'chain.name', e.target.value, { index: smartCastActiveChain })} />
                  </label>
                  <button className="btn" onClick={() => doAction('smartCast', 'clearChain', { index: smartCastActiveChain })}>Clear Chain</button>
                </div>
                <div className="scroll">
                  {(selectedChain.steps || []).length === 0 && <p className="meta">No recorded steps for this chain.</p>}
                  {(selectedChain.steps || []).map((step, idx) => (
                    <div className="item single" key={`step-${idx}`}>
                      {idx + 1}. {step.spellFormID || 'Unknown'} | {spellTypeLabel(step.type)} | {castOnLabel(step.castOn)} | Hold {Number(step.holdSec || 0).toFixed(2)}s
                    </div>
                  ))}
                </div>
              </article>

              <article className="card">
                <h3>Playback</h3>
                <div className="setting-grid">
                  <button className="btn" onClick={() => doAction('smartCast', 'startRecording', { index: smartCastActiveChain })}>Start Recording</button>
                  <button className="btn" onClick={() => doAction('smartCast', 'stopRecording')}>Stop Recording</button>
                  <button className="btn" onClick={() => doAction('smartCast', 'startPlayback', { index: smartCastActiveChain })}>Start Playback</button>
                  <button className="btn" onClick={() => doAction('smartCast', 'stopPlayback')}>Stop Playback</button>
                </div>
              </article>
            </section>
          )}

          {tab === 'quickBuff' && (
            <section className="grid qb-grid">
              <article className="card">
                <h3>Quick Buff</h3>
                <p className="meta">Triggers auto-cast by event and assigned spell.</p>
              </article>
              <article className="card">
                <h3>Triggers</h3>
                <div className="trigger-grid">
                  {triggerOrder.map((triggerId) => {
                    const t = byPath(qb, ['config', 'triggers', triggerId], {});
                    return (
                      <div className="trigger-card" key={triggerId}>
                        <div className="trigger-title">{triggerId}</div>
                        <label className="checkbox-inline">
                          <input
                            type="checkbox"
                            checked={!!t.enabled}
                            onChange={(e) => setSetting('quickBuff', 'trigger.enabled', e.target.checked, { trigger: triggerId })}
                          />
                          Enabled
                        </label>
                        <select
                          value={t.spellFormID || ''}
                          onChange={(e) => setSetting('quickBuff', 'trigger.spellFormID', e.target.value, { trigger: triggerId })}
                        >
                          <option value="">None</option>
                          {(qb.knownSpells || []).map((spell) => (
                            <option key={spell.formKey || spell.name} value={spell.formKey || ''}>{spell.name || 'Unknown'}</option>
                          ))}
                        </select>
                        <div className="row">
                          <input
                            type="number"
                            step="0.5"
                            min="0"
                            max="120"
                            value={Number(t.cooldownSec || 5).toFixed(1)}
                            onChange={(e) => setSetting('quickBuff', 'trigger.cooldownSec', Number(e.target.value || 0), { trigger: triggerId })}
                          />
                          <button className="btn" onClick={() => doAction('quickBuff', 'testTrigger', { trigger: triggerId })}>Test</button>
                        </div>
                      </div>
                    );
                  })}
                </div>
              </article>
            </section>
          )}

          {tab === 'mastery' && (
            <section className="grid mastery-grid">
              <article className="card">
                <div className="subtabs">
                  {['spells', 'shouts', 'weapons', 'overview'].map((id) => (
                    <button key={id} className={masteryTab === id ? 'subtab active' : 'subtab'} onClick={() => setMasteryTab(id)}>{id}</button>
                  ))}
                </div>
                {masteryTab === 'overview' && (
                  <div className="scroll">
                    <div className="item"><div>Spell Mastery Entries</div><div>{Number(byPath(mastery, ['spell', 'rows'], []).length)}</div></div>
                    <div className="item"><div>Shout Mastery Entries</div><div>{Number(byPath(mastery, ['shout', 'rows'], []).length)}</div></div>
                    <div className="item"><div>Weapon Mastery Entries</div><div>{Number(byPath(mastery, ['weapon', 'rows'], []).length)}</div></div>
                  </div>
                )}
                {masteryTab !== 'overview' && (
                  <div className="scroll">
                    {(masteryTab === 'spells' ? byPath(mastery, ['spell', 'rows'], []) : masteryTab === 'shouts' ? byPath(mastery, ['shout', 'rows'], []) : byPath(mastery, ['weapon', 'rows'], [])).map((row, idx) => (
                      <div className="item" key={`m-${idx}`}>
                        <div>
                          <div>{row.name || 'Unknown'}</div>
                          <div className="meta">Level {Number(row.level || 0)} | Uses {Number(row.uses || row.hits || 0)}</div>
                        </div>
                      </div>
                    ))}
                  </div>
                )}
              </article>
            </section>
          )}
        </main>

        {!windowState.isFullscreen && <div className="resize-handle" onMouseDown={startResize} />}
      </div>

      {settingsOpen && (
        <div className="modal-overlay" onMouseDown={() => setSettingsOpen(false)}>
          <div className="modal" onMouseDown={(e) => e.stopPropagation()}>
            <header className="modal-header">
              <h2>Settings</h2>
              <button className="icon-btn danger" onClick={() => setSettingsOpen(false)}><IconClose /></button>
            </header>
            <nav className="settings-tabs">
              {['spellBinding', 'smartCast', 'quickBuff', 'mastery', 'uiHud'].map((id) => (
                <button key={id} className={settingsTab === id ? 'subtab active' : 'subtab'} onClick={() => setSettingsTab(id)}>{id}</button>
              ))}
            </nav>
            <section className="settings-body">
              {settingsTab === 'spellBinding' && (
                <div className="setting-grid">
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'enabled'], true)} onChange={(e) => setSetting('spellBinding', 'enabled', e.target.checked)} /> Enabled</label>
                  <label>UI Toggle Key Code <input type="number" value={Number(byPath(sb, ['config', 'uiToggleKeyCode'], 0))} onChange={(e) => setSetting('spellBinding', 'uiToggleKey', Number(e.target.value || 0))} /></label>
                  <label>Bind Key Code <input type="number" value={Number(byPath(sb, ['config', 'bindKeyCode'], 0))} onChange={(e) => setSetting('spellBinding', 'bindKey', Number(e.target.value || 0))} /></label>
                  <label>Cycle Modifier Key Code <input type="number" value={Number(byPath(sb, ['config', 'cycleSlotModifierKey'], 0))} onChange={(e) => setSetting('spellBinding', 'cycleSlotModifierKey', Number(e.target.value || 0))} /></label>
                  <label>Fallback Dedupe (s) <input type="number" step="0.1" min="0.5" max="5" value={Number(byPath(sb, ['config', 'fallbackDedupeSec'], 1.5)).toFixed(1)} onChange={(e) => setSetting('spellBinding', 'fallbackDedupeSec', Number(e.target.value || 1.5))} /></label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'showHudNotifications'], true)} onChange={(e) => setSetting('spellBinding', 'showHudNotifications', e.target.checked)} /> Show HUD Notifications</label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'enableSoundCues'], true)} onChange={(e) => setSetting('spellBinding', 'enableSoundCues', e.target.checked)} /> Enable Sound Cues</label>
                  <label>Sound Cue Volume <input type="number" min="0" max="1" step="0.05" value={Number(byPath(sb, ['config', 'soundCueVolume'], 0.45)).toFixed(2)} onChange={(e) => setSetting('spellBinding', 'soundCueVolume', Number(e.target.value || 0))} /></label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'blacklistEnabled'], true)} onChange={(e) => setSetting('spellBinding', 'blacklistEnabled', e.target.checked)} /> Enable Blacklist</label>
                </div>
              )}

              {settingsTab === 'smartCast' && (
                <div className="setting-grid">
                  <label>Record Key <input value={String(byPath(sc, ['config', 'global', 'record', 'toggleKey'], 'R'))} onChange={(e) => setSetting('smartCast', 'record.toggleKey', e.target.value)} /></label>
                  <label>Play Key <input value={String(byPath(sc, ['config', 'global', 'playback', 'playKey'], 'G'))} onChange={(e) => setSetting('smartCast', 'playback.playKey', e.target.value)} /></label>
                  <label>Default Chain Index <input type="number" min="1" max="5" value={Number(byPath(sc, ['config', 'global', 'playback', 'defaultChainIndex'], 1))} onChange={(e) => setSetting('smartCast', 'playback.defaultChainIndex', Number(e.target.value || 1))} /></label>
                  <label>Step Delay (s) <input type="number" min="0" max="2" step="0.01" value={Number(byPath(sc, ['config', 'global', 'playback', 'stepDelaySec'], 0.1)).toFixed(2)} onChange={(e) => setSetting('smartCast', 'playback.stepDelaySec', Number(e.target.value || 0.1))} /></label>
                </div>
              )}

              {settingsTab === 'quickBuff' && (
                <div className="setting-grid">
                  <label>Min Time After Load (s) <input type="number" min="0" max="30" step="0.5" value={Number(byPath(qb, ['config', 'global', 'minTimeAfterLoadSeconds'], 2)).toFixed(1)} onChange={(e) => setSetting('quickBuff', 'global.minTimeAfterLoadSeconds', Number(e.target.value || 0))} /></label>
                </div>
              )}

              {settingsTab === 'mastery' && (
                <div className="setting-grid">
                  <div className="row">
                    <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['spell', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masterySpell.enabled', e.target.checked)} /> Spell Enabled</label>
                    <label>Spell Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['spell', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masterySpell.gainMultiplier', Number(e.target.value || 1))} /></label>
                  </div>
                  <div className="row">
                    <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['shout', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masteryShout.enabled', e.target.checked)} /> Shout Enabled</label>
                    <label>Shout Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['shout', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masteryShout.gainMultiplier', Number(e.target.value || 1))} /></label>
                  </div>
                  <div className="row">
                    <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['weapon', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masteryWeapon.enabled', e.target.checked)} /> Weapon Enabled</label>
                    <label>Weapon Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['weapon', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masteryWeapon.gainMultiplier', Number(e.target.value || 1))} /></label>
                  </div>
                  <div className="row">
                    <button className="btn" onClick={() => doAction('mastery', 'masterySpell.resetDefaults')}>Reset Spell Defaults</button>
                    <button className="btn" onClick={() => doAction('mastery', 'masteryShout.resetDefaults')}>Reset Shout Defaults</button>
                    <button className="btn" onClick={() => doAction('mastery', 'masteryWeapon.resetDefaults')}>Reset Weapon Defaults</button>
                  </div>
                </div>
              )}

              {settingsTab === 'uiHud' && (
                <div className="setting-grid">
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'hudDonutEnabled'], true)} onChange={(e) => setSetting('spellBinding', 'hudDonutEnabled', e.target.checked)} /> HUD Donut Enabled</label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'hudDonutOnlyUnsheathed'], true)} onChange={(e) => setSetting('spellBinding', 'hudDonutOnlyUnsheathed', e.target.checked)} /> HUD Only Unsheathed</label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'hudShowCooldownSeconds'], true)} onChange={(e) => setSetting('spellBinding', 'hudShowCooldownSeconds', e.target.checked)} /> Show Cooldown Seconds</label>
                  <label>HUD Donut Size <input type="number" min="48" max="220" step="2" value={Number(byPath(sb, ['config', 'hudDonutSize'], 88)).toFixed(0)} onChange={(e) => setSetting('spellBinding', 'hudDonutSize', Number(e.target.value || 88))} /></label>
                  <div className="row">
                    <button className="btn" onClick={resetWindowLayout}>Reset Window Layout</button>
                    {windowState.isFullscreen && <button className="btn" onClick={toggleFullscreen}>Exit Fullscreen</button>}
                    <button
                      className="btn"
                      onClick={() =>
                        hudAction('saveHudPosition', {
                          x: Number(byPath(sb, ['config', 'hudPosX'], 48)),
                          y: Number(byPath(sb, ['config', 'hudPosY'], 48)),
                          commit: true
                        })
                      }
                    >
                      Finish HUD Drag
                    </button>
                  </div>
                </div>
              )}
            </section>
          </div>
        </div>
      )}

      {hudAdjustOpen && (
        <div className="hud-adjust">
          <div className="hud-adjust-head">Drag the donut in game view, then click Done.</div>
          <div className="hud-adjust-controls">
            <label className="inline">
              Donut Size
              <input
                type="range"
                min="48"
                max="220"
                step="2"
                value={Number(byPath(sb, ['config', 'hudDonutSize'], 88))}
                onChange={(e) => setSetting('spellBinding', 'hudDonutSize', Number(e.target.value || 88))}
              />
            </label>
            <label className="checkbox-inline">
              <input
                type="checkbox"
                checked={!!byPath(sb, ['config', 'hudShowCooldownSeconds'], true)}
                onChange={(e) => setSetting('spellBinding', 'hudShowCooldownSeconds', e.target.checked)}
              />
              Show cooldown seconds
            </label>
            <button className="btn" onClick={closeHudAdjust}>Done</button>
          </div>
        </div>
      )}

      {toast && <div className="toast">{toast}</div>}
    </>
  );
}

createRoot(document.getElementById('app')).render(<App />);
