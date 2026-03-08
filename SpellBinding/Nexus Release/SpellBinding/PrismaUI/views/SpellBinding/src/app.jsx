
import React, { useEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { ArrowUpDown, Eraser, Filter, LoaderCircle, Plus, Settings, Maximize2, Minimize2, Trash2, X } from 'lucide-react';

const triggerOrder = ['combatStart', 'combatEnd', 'healthBelow70', 'healthBelow50', 'healthBelow30', 'crouchStart', 'sprintStart', 'weaponDraw', 'powerAttackStart', 'shoutStart'];
const triggerLabels = {
  combatStart: 'Combat Start',
  combatEnd: 'Combat End',
  healthBelow70: 'Health Below 70%',
  healthBelow50: 'Health Below 50%',
  healthBelow30: 'Health Below 30%',
  crouchStart: 'Crouch Start',
  sprintStart: 'Sprint Start',
  weaponDraw: 'Weapon Draw',
  powerAttackStart: 'Power Attack Start',
  shoutStart: 'Shout Start'
};
const settingsTabs = [
  { id: 'spellBinding', label: 'Spell Binding' },
  { id: 'smartCast', label: 'Smart Cast' },
  { id: 'quickBuff', label: 'Quick Buff' },
  { id: 'mastery', label: 'Mastery' },
  { id: 'uiHud', label: 'UI' }
];
const masteryTabs = [
  { id: 'spells', label: 'Spells' },
  { id: 'shouts', label: 'Shouts' },
  { id: 'weapons', label: 'Weapons' }
];
const HUD_TARGETS = ['donut', 'cycle', 'chain'];
const WINDOW_MIN_WIDTH = 960;
const WINDOW_MIN_HEIGHT = 620;
const WINDOW_MARGIN = 8;
const SPELLBIND_SCAN_BY_CODE = {
  Escape: 1,
  Digit1: 2, Digit2: 3, Digit3: 4, Digit4: 5, Digit5: 6, Digit6: 7, Digit7: 8, Digit8: 9, Digit9: 10, Digit0: 11,
  Minus: 12, Equal: 13, Backspace: 14,
  Tab: 15,
  KeyQ: 16, KeyW: 17, KeyE: 18, KeyR: 19, KeyT: 20, KeyY: 21, KeyU: 22, KeyI: 23, KeyO: 24, KeyP: 25,
  BracketLeft: 26, BracketRight: 27, Enter: 28,
  ControlLeft: 29,
  KeyA: 30, KeyS: 31, KeyD: 32, KeyF: 33, KeyG: 34, KeyH: 35, KeyJ: 36, KeyK: 37, KeyL: 38,
  Semicolon: 39, Quote: 40, Backquote: 41,
  ShiftLeft: 42, Backslash: 43,
  KeyZ: 44, KeyX: 45, KeyC: 46, KeyV: 47, KeyB: 48, KeyN: 49, KeyM: 50,
  Comma: 51, Period: 52, Slash: 53,
  ShiftRight: 54,
  NumpadMultiply: 55,
  AltLeft: 56,
  Space: 57,
  CapsLock: 58,
  F1: 59, F2: 60, F3: 61, F4: 62, F5: 63, F6: 64, F7: 65, F8: 66, F9: 67, F10: 68,
  NumLock: 69, ScrollLock: 70,
  Numpad7: 71, Numpad8: 72, Numpad9: 73, NumpadSubtract: 74,
  Numpad4: 75, Numpad5: 76, Numpad6: 77, NumpadAdd: 78,
  Numpad1: 79, Numpad2: 80, Numpad3: 81, Numpad0: 82, NumpadDecimal: 83,
  F11: 87, F12: 88
};
const SPELLBIND_CODE_BY_SCAN = Object.entries(SPELLBIND_SCAN_BY_CODE).reduce((out, [code, scan]) => {
  out[scan] = code;
  return out;
}, {});

function normalizeEventCode(event) {
  const rawCode = String(event?.code || '');
  if (rawCode && Object.prototype.hasOwnProperty.call(SPELLBIND_SCAN_BY_CODE, rawCode)) {
    return rawCode;
  }

  const rawKey = String(event?.key || '');
  if (!rawKey) return '';

  const upperKey = rawKey.toUpperCase();
  if (/^[A-Z]$/.test(upperKey)) return `Key${upperKey}`;
  if (/^[0-9]$/.test(rawKey)) {
    if (event?.location === 3) return `Numpad${rawKey}`;
    return `Digit${rawKey}`;
  }

  if (/^F([1-9]|1[0-2])$/i.test(rawKey)) return upperKey;

  switch (rawKey) {
    case 'Escape':
      return 'Escape';
    case 'Tab':
      return 'Tab';
    case 'Enter':
      return event?.location === 3 ? 'NumpadEnter' : 'Enter';
    case 'Backspace':
      return 'Backspace';
    case ' ':
    case 'Spacebar':
      return 'Space';
    case '-':
    case '_':
      return 'Minus';
    case '=':
      return event?.location === 3 ? 'NumpadAdd' : 'Equal';
    case '+':
      return event?.location === 3 ? 'NumpadAdd' : 'Equal';
    case 'Add':
      return 'NumpadAdd';
    case '[':
    case '{':
      return 'BracketLeft';
    case ']':
    case '}':
      return 'BracketRight';
    case ';':
    case ':':
      return 'Semicolon';
    case '\'':
    case '"':
      return 'Quote';
    case '`':
    case '~':
      return 'Backquote';
    case '\\':
    case '|':
      return 'Backslash';
    case ',':
    case '<':
      return 'Comma';
    case '.':
    case '>':
      return 'Period';
    case '/':
    case '?':
      return 'Slash';
    case 'Control':
      return 'ControlLeft';
    case 'Alt':
      return 'AltLeft';
    case 'Shift':
      return 'ShiftLeft';
    case '*':
      return event?.location === 3 ? 'NumpadMultiply' : '';
    case 'Subtract':
      return 'NumpadSubtract';
    case 'Decimal':
      return 'NumpadDecimal';
    case 'NumLock':
      return 'NumLock';
    case 'ScrollLock':
      return 'ScrollLock';
    case 'CapsLock':
      return 'CapsLock';
    default:
      return '';
  }
}

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

function spellTypeLabel(type) {
  return type === 1 || type === '1' ? 'Concentration' : 'Fire';
}

function castOnLabel(castOn) {
  if (castOn === 1 || castOn === '1') return 'Crosshair';
  if (castOn === 2 || castOn === '2') return 'Aimed';
  return 'Self';
}

function triggerLabel(id) {
  return triggerLabels[id] || id;
}

function normalizeMetric(metric) {
  const raw = String(metric || '').trim();
  if (!raw) return '-';
  return raw.replace(/^cost\s*:\s*/i, '').replace(/^cd\s*:\s*/i, '').replace(/^cooldown\s*:\s*/i, '');
}

function compactChainInputWidth(name) {
  const len = String(name || '').trim().length || 7;
  return `${Math.max(10, Math.min(24, len + 2))}ch`;
}

function formatCostLabel(step, baseCost) {
  const cost = Math.max(0, Number(baseCost || 0));
  const isConc = Number(step.type || 0) === 1;
  if (isConc) return `${cost.toFixed(0)}/s`;
  const casts = Math.max(1, Number(step.castCount || 1));
  return `${(cost * casts).toFixed(0)}`;
}

function isCooldownSpellType(spellType) {
  const t = String(spellType || '').toLowerCase();
  return t.includes('power') || t.includes('shout');
}

function formatSlotCostText(slot) {
  const castingType = String(slot?.castingType || '');
  if (castingType === 'Concentration') {
    return `${Math.max(0, Number(slot?.costPerSecond || 0)).toFixed(0)}/s`;
  }
  return `${Math.max(0, Number(slot?.baseCost || 0)).toFixed(0)}`;
}

function formatScanCode(scan) {
  const numeric = Number(scan);
  if (!Number.isFinite(numeric)) return 'G';
  const code = SPELLBIND_CODE_BY_SCAN[numeric];
  if (!code) return `Key ${scan}`;
  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  return code;
}

function toSmartCastToken(event, options = {}) {
  const code = normalizeEventCode(event);
  if (options.allowModifiers) {
    if (code === 'ShiftLeft' || code === 'ShiftRight') return 'Shift';
    if (code === 'ControlLeft' || code === 'ControlRight') return 'Ctrl';
  }
  if (code.startsWith('Key') && code.length === 4) return code.slice(3).toUpperCase();
  if (code.startsWith('Digit') && code.length === 6) return code.slice(5);
  if (/^F([1-9]|1[0-2])$/.test(code)) return code;
  return null;
}

function App() {
  const [snapshot, setSnapshot] = useState({ spellBinding: {}, smartCast: {}, quickBuff: {}, mastery: {}, ui: {} });
  const [tab, setTab] = useState('spellBinding');
  const [masteryTab, setMasteryTab] = useState('spells');
  const [masterySearch, setMasterySearch] = useState('');
  const [masteryTypeFilterByTab, setMasteryTypeFilterByTab] = useState({ spells: 'all', shouts: 'all', weapons: 'all' });
  const [masteryFilterOpen, setMasteryFilterOpen] = useState(false);
  const [masterySort, setMasterySort] = useState({ key: 'name', dir: 'asc' });
  const [smartCastActiveChain, setSmartCastActiveChain] = useState(1);
  const [sbSearch, setSbSearch] = useState('');
  const [blacklistSearch, setBlacklistSearch] = useState('');
  const [quickBuffSearch, setQuickBuffSearch] = useState('');
  const [toast, setToast] = useState('');
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [blacklistDialogOpen, setBlacklistDialogOpen] = useState(false);
  const [settingsTab, setSettingsTab] = useState('spellBinding');
  const [hudAdjustOpen, setHudAdjustOpen] = useState(false);
  const [hudDragPos, setHudDragPos] = useState({
    donut: { x: 48, y: 48 },
    cycle: { x: 48, y: 152 },
    chain: { x: 48, y: 216 }
  });
  const [hudDragTarget, setHudDragTarget] = useState('donut');
  const [captureField, setCaptureField] = useState(null);
  const [quickBuffSelectedTrigger, setQuickBuffSelectedTrigger] = useState(triggerOrder[0]);
  const [windowState, setWindowState] = useState(defaultCenteredRect());

  const dragRef = useRef(null);
  const resizeRef = useRef(null);
  const lastWindowedRef = useRef(null);
  const toastTimerRef = useRef(null);
  const settingsOpenRef = useRef(false);
  const blacklistDialogOpenRef = useRef(false);
  const hudAdjustOpenRef = useRef(false);
  const hudDragRef = useRef(null);
  const hudDragPosRef = useRef({
    donut: { x: 48, y: 48 },
    cycle: { x: 48, y: 152 },
    chain: { x: 48, y: 216 }
  });
  const captureFieldRef = useRef(null);
  const masteryFilterRef = useRef(null);
  const previousChainsLengthRef = useRef(0);

  const sb = snapshot.spellBinding || {};
  const sc = snapshot.smartCast || {};
  const qb = snapshot.quickBuff || {};
  const mastery = snapshot.mastery || {};

  const currentWeapon = sb.currentWeapon || {};
  const currentSlots = currentWeapon.slots || {};
  const attackSlots = [
    { id: 'light', label: 'Light', slot: 0, data: currentSlots.light || {} },
    { id: 'power', label: 'Power', slot: 1, data: currentSlots.power || {} },
    { id: 'bash', label: 'Bash', slot: 2, data: currentSlots.bash || {} }
  ];

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
  const quickBuffKnownSpellsFiltered = useMemo(() => {
    const query = quickBuffSearch.toLowerCase().trim();
    return (qb.knownSpells || []).filter((spell) => {
      const name = String(spell.name || '').toLowerCase();
      return !query || name.includes(query);
    });
  }, [qb.knownSpells, quickBuffSearch]);

  const showToast = (text) => {
    setToast(text || '');
    if (toastTimerRef.current) clearTimeout(toastTimerRef.current);
    toastTimerRef.current = setTimeout(() => setToast(''), 1400);
  };

  useEffect(() => {
    settingsOpenRef.current = settingsOpen;
  }, [settingsOpen]);

  useEffect(() => {
    blacklistDialogOpenRef.current = blacklistDialogOpen;
  }, [blacklistDialogOpen]);

  useEffect(() => {
    hudAdjustOpenRef.current = hudAdjustOpen;
  }, [hudAdjustOpen]);

  useEffect(() => {
    hudDragPosRef.current = hudDragPos;
  }, [hudDragPos]);

  useEffect(() => {
    captureFieldRef.current = captureField;
  }, [captureField]);

  useEffect(() => {
    if (!hudAdjustOpenRef.current) {
      setHudDragPos({
        donut: {
          x: Number(byPath(sb, ['config', 'hudPosX'], 48)),
          y: Number(byPath(sb, ['config', 'hudPosY'], 48))
        },
        cycle: {
          x: Number(byPath(sb, ['config', 'hudCyclePosX'], 48)),
          y: Number(byPath(sb, ['config', 'hudCyclePosY'], 152))
        },
        chain: {
          x: Number(byPath(sb, ['config', 'hudChainPosX'], 48)),
          y: Number(byPath(sb, ['config', 'hudChainPosY'], 216))
        }
      });
    }
  }, [sb]);

  const hudSizeForTarget = (target) => {
    if (target === 'cycle') return Number(byPath(sb, ['config', 'hudCycleSize'], 56));
    if (target === 'chain') return Number(byPath(sb, ['config', 'hudChainSize'], 56));
    return Number(byPath(sb, ['config', 'hudDonutSize'], 88));
  };

  const clampHudPos = (x, y, size) => {
    const width = window.innerWidth;
    const height = window.innerHeight;
    const nextX = Math.max(0, Math.min(width - size, x));
    const nextY = Math.max(0, Math.min(height - size, y));
    return { x: nextX, y: nextY };
  };

  const finalizeHudAdjust = (closeMenuAfter) => {
    HUD_TARGETS.forEach((target) => {
      const pos = hudDragPosRef.current[target] || { x: 48, y: 48 };
      const clamped = clampHudPos(pos.x, pos.y, hudSizeForTarget(target));
      hudAction('saveHudPosition', {
        target,
        x: clamped.x,
        y: clamped.y,
        commit: true
      });
    });
    setHudAdjustOpen(false);
    if (closeMenuAfter) {
      callNative('sbo_toggle_ui', '');
    }
  };

  const startHudAdjustMode = () => {
    setSettingsOpen(false);
    setHudDragPos({
      donut: {
        x: Number(byPath(sb, ['config', 'hudPosX'], 48)),
        y: Number(byPath(sb, ['config', 'hudPosY'], 48))
      },
      cycle: {
        x: Number(byPath(sb, ['config', 'hudCyclePosX'], 48)),
        y: Number(byPath(sb, ['config', 'hudCyclePosY'], 152))
      },
      chain: {
        x: Number(byPath(sb, ['config', 'hudChainPosX'], 48)),
        y: Number(byPath(sb, ['config', 'hudChainPosY'], 216))
      }
    });
    setHudDragTarget('donut');
    setHudAdjustOpen(true);
    hudAction('enterDragMode');
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
        finalizeHudAdjust(false);
        return;
      }
      if (captureFieldRef.current) {
        setCaptureField(null);
        return;
      }
      if (blacklistDialogOpenRef.current) {
        setBlacklistDialogOpen(false);
        return;
      }
      if (settingsOpenRef.current) {
        setSettingsOpen(false);
        return;
      }
      callNative('sbo_toggle_ui', '');
    };
    window.sbo_close_ui = () => {
      if (hudAdjustOpenRef.current) {
        finalizeHudAdjust(true);
        return;
      }
      callNative('sbo_toggle_ui', '');
    };

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

  useEffect(() => {
    const onMove = (event) => {
      if (!hudDragRef.current || !hudAdjustOpenRef.current) return;
      const drag = hudDragRef.current;
      const size = hudSizeForTarget(drag.target);
      const next = clampHudPos(drag.left + (event.clientX - drag.x), drag.top + (event.clientY - drag.y), size);
      setHudDragPos((prev) => ({ ...prev, [drag.target]: next }));
      hudAction('saveHudPosition', { target: drag.target, x: next.x, y: next.y, commit: false });
    };
    const onUp = () => {
      hudDragRef.current = null;
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [sb]);

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

  const onSlotSettingChange = (slot, id, value) => {
    if (!currentWeapon.key) return;
    setSetting('spellBinding', 'weapon', value, {
      key: currentWeapon.key,
      slot,
      id,
      value
    });
  };

  const chains = byPath(sc, ['config', 'chains'], []);
  const activeChainIndex = chains.length ? Math.max(0, Math.min(chains.length - 1, smartCastActiveChain - 1)) : 0;
  const selectedChain = chains[activeChainIndex] || { name: `Chain ${smartCastActiveChain}`, steps: [] };
  const selectedChainStepDelay = Number(byPath(sc, ['config', 'chains', activeChainIndex, 'stepDelaySec'], Number(selectedChain.stepDelaySec || 1.0)));
  const smartCastKnownSpellMap = useMemo(() => {
    const map = new Map();
    (sc.knownSpells || []).forEach((spell) => {
      map.set(String(spell.formKey || ''), spell);
    });
    return map;
  }, [sc.knownSpells]);
  const smartCastTotalCost = useMemo(() => {
    return (selectedChain.steps || []).reduce((sum, step) => {
      const spell = smartCastKnownSpellMap.get(String(step.spellFormID || '')) || {};
      const baseCost = Math.max(0, Number(spell.magickaCost || 0));
      const isConc = Number(step.type || 0) === 1;
      if (isConc) {
        const holdSec = Math.max(0, Number(step.holdSec || 0.15));
        return sum + baseCost * holdSec;
      }
      const casts = Math.max(1, Number(step.castCount || 1));
      return sum + baseCost * casts;
    }, 0);
  }, [selectedChain.steps, smartCastKnownSpellMap]);
  const weaponMasteryRows = byPath(mastery, ['weapon', 'rows'], []);
  const weaponMasteryLevel = useMemo(() => {
    const uniqueID = Number(byPath(sb, ['currentWeapon', 'key', 'uniqueID'], 0));
    const name = String(byPath(sb, ['currentWeapon', 'displayName'], ''));
    const byId = weaponMasteryRows.find((row) => Number(row.uniqueID || 0) === uniqueID && uniqueID !== 0);
    if (byId) return Number(byId.level || 0);
    const byName = weaponMasteryRows.find((row) => String(row.name || '') === name);
    if (byName) return Number(byName.level || 0);
    return 0;
  }, [weaponMasteryRows, sb]);

  const masteryRowsRaw = useMemo(() => {
    if (masteryTab === 'spells') return byPath(mastery, ['spell', 'knownRows'], byPath(mastery, ['spell', 'rows'], []));
    if (masteryTab === 'shouts') return byPath(mastery, ['shout', 'knownRows'], byPath(mastery, ['shout', 'rows'], []));
    return byPath(mastery, ['weapon', 'knownRows'], byPath(mastery, ['weapon', 'rows'], []));
  }, [mastery, masteryTab]);

  const masteryTypes = useMemo(() => {
    if (masteryTab === 'shouts') return [];
    const set = new Set();
    masteryRowsRaw.forEach((row) => set.add(String(row.type || 'Unknown')));
    return Array.from(set).sort((a, b) => a.localeCompare(b));
  }, [masteryRowsRaw, masteryTab]);

  const masteryRows = useMemo(() => {
    const query = masterySearch.toLowerCase().trim();
    const activeTypeFilter = masteryTypeFilterByTab[masteryTab] || 'all';
    const filtered = masteryRowsRaw.filter((row) => {
      const type = String(row.type || 'Unknown');
      const name = String(row.name || '');
      const passesType = masteryTab === 'shouts' || activeTypeFilter === 'all' || type === activeTypeFilter;
      const hay = `${name} ${type}`.toLowerCase();
      const passesSearch = !query || hay.includes(query);
      return passesType && passesSearch;
    });

    const sorted = [...filtered].sort((a, b) => {
      const dir = masterySort.dir === 'asc' ? 1 : -1;
      if (masterySort.key === 'uses') return (Number(a.uses || 0) - Number(b.uses || 0)) * dir;
      if (masterySort.key === 'level') return (Number(a.level || 0) - Number(b.level || 0)) * dir;
      if (masterySort.key === 'type') return String(a.type || '').localeCompare(String(b.type || '')) * dir;
      if (masterySort.key === 'progress') return (Number(a.progressPct || 0) - Number(b.progressPct || 0)) * dir;
      return String(a.name || '').localeCompare(String(b.name || '')) * dir;
    });
    return sorted;
  }, [masteryRowsRaw, masterySearch, masteryTypeFilterByTab, masterySort, masteryTab]);

  const toggleMasterySort = (key) => {
    setMasterySort((prev) => {
      if (prev.key === key) {
        return { key, dir: prev.dir === 'asc' ? 'desc' : 'asc' };
      }
      return { key, dir: 'asc' };
    });
  };

  useEffect(() => {
    if (chains.length === 0) {
      setSmartCastActiveChain(1);
    } else if (chains.length < previousChainsLengthRef.current && smartCastActiveChain > chains.length) {
      setSmartCastActiveChain(chains.length);
    }
    previousChainsLengthRef.current = chains.length;
  }, [chains.length, smartCastActiveChain]);

  useEffect(() => {
    if (!masteryFilterOpen) return;
    const onClickOutside = (event) => {
      if (!masteryFilterRef.current) return;
      if (!masteryFilterRef.current.contains(event.target)) {
        setMasteryFilterOpen(false);
      }
    };
    window.addEventListener('mousedown', onClickOutside);
    return () => window.removeEventListener('mousedown', onClickOutside);
  }, [masteryFilterOpen]);

  useEffect(() => {
    setMasteryFilterOpen(false);
  }, [masteryTab]);

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

  const captureLabel = (field) => {
    if (captureField === field) return 'Press key to define...';
    if (field === 'sb.uiToggleKey') return String(byPath(sb, ['config', 'uiToggleKey'], 'Unset'));
    if (field === 'sb.bindKey') {
      const bindCode = Number(byPath(sb, ['config', 'bindKeyCode'], byPath(sb, ['config', 'bindKey'], 34)));
      return formatScanCode(Number.isFinite(bindCode) ? bindCode : 34);
    }
    if (field === 'sb.cycleSlotModifierKey') return formatScanCode(Number(byPath(sb, ['config', 'cycleSlotModifierKey'], 0)));
    if (field === 'sc.record.toggleKey') return String(byPath(sc, ['config', 'global', 'record', 'toggleKey'], 'H'));
    if (field === 'sc.playback.playKey') return String(byPath(sc, ['config', 'global', 'playback', 'playKey'], 'X'));
    if (field === 'sc.playback.cycleModifierKey') return String(byPath(sc, ['config', 'global', 'playback', 'cycleModifierKey'], 'Shift'));
    if (field.startsWith('sc.chain.hotkey.')) {
      const idx = Math.max(0, Number(field.split('.').pop() || 1) - 1);
      return String(byPath(sc, ['config', 'chains', idx, 'hotkey'], 'None'));
    }
    return 'Unset';
  };

  useEffect(() => {
    if (!captureField) return;
    const onCapture = (event) => {
      event.preventDefault();
      event.stopPropagation();
      if (event.key === 'Escape') {
        setCaptureField(null);
        return;
      }

      if (captureField === 'sb.uiToggleKey' || captureField === 'sb.bindKey' || captureField === 'sb.cycleSlotModifierKey') {
        const scan = SPELLBIND_SCAN_BY_CODE[normalizeEventCode(event)];
        if (scan == null) {
          showToast('Unsupported key for this binding');
          return;
        }
        const id = captureField === 'sb.uiToggleKey' ? 'uiToggleKey' : captureField === 'sb.bindKey' ? 'bindKey' : 'cycleSlotModifierKey';
        setSetting('spellBinding', id, Number(scan));
        setCaptureField(null);
        return;
      }

      if (captureField === 'sc.record.toggleKey' || captureField === 'sc.playback.playKey' || captureField === 'sc.playback.cycleModifierKey' || captureField.startsWith('sc.chain.hotkey.')) {
        const token = toSmartCastToken(event, { allowModifiers: captureField === 'sc.playback.cycleModifierKey' });
        if (!token) {
          showToast(captureField === 'sc.playback.cycleModifierKey' ? 'Use Shift or Ctrl' : 'Use letters, digits, or F1-F12');
          return;
        }
        if (captureField === 'sc.record.toggleKey') {
          setSetting('smartCast', 'record.toggleKey', token);
        } else if (captureField === 'sc.playback.playKey') {
          setSetting('smartCast', 'playback.playKey', token);
        } else if (captureField === 'sc.playback.cycleModifierKey') {
          setSetting('smartCast', 'playback.cycleModifierKey', token);
        } else {
          const index = Number(captureField.split('.').pop() || smartCastActiveChain);
          setSetting('smartCast', 'chain.hotkey', token, { index });
        }
        setCaptureField(null);
      }
    };

    window.addEventListener('keydown', onCapture, true);
    return () => window.removeEventListener('keydown', onCapture, true);
  }, [captureField, sb, sc]);

  return (
    <>
      {!hudAdjustOpen && <div className="window" style={panelStyle}>
        <header className="window-header" onMouseDown={startDrag}>
          <div className="title-wrap">
            <h1>Spell Binding</h1>
            <p>A Spellblade Overhaul Control Panel</p>
          </div>
          <nav className="header-tabs" onMouseDown={(e) => e.stopPropagation()}>
            <button className={tab === 'spellBinding' ? 'tab active' : 'tab'} onClick={() => setTab('spellBinding')}>Spell Binding</button>
            <button className={tab === 'smartCast' ? 'tab active' : 'tab'} onClick={() => setTab('smartCast')}>Smart Cast</button>
            <button className={tab === 'quickBuff' ? 'tab active' : 'tab'} onClick={() => setTab('quickBuff')}>Quick Buff</button>
            <button className={tab === 'mastery' ? 'tab active' : 'tab'} onClick={() => setTab('mastery')}>Mastery</button>
          </nav>
          <div className="window-actions" onMouseDown={(e) => e.stopPropagation()}>
            <button className="icon-btn spin-icon-btn" onClick={startHudAdjustMode} aria-label="Drag UI elements"><LoaderCircle size={16} strokeWidth={2} /></button>
            <button className="icon-btn" onClick={() => setSettingsOpen(true)} aria-label="Settings"><Settings size={16} strokeWidth={2} /></button>
            <button className="icon-btn" onClick={toggleFullscreen} aria-label={windowState.isFullscreen ? 'Windowed' : 'Fullscreen'}>
              {windowState.isFullscreen ? <Minimize2 size={16} strokeWidth={2} /> : <Maximize2 size={16} strokeWidth={2} />}
            </button>
            <button className="icon-btn danger" onClick={() => callNative('sbo_toggle_ui', '')} aria-label="Close"><X size={16} strokeWidth={2} /></button>
          </div>
        </header>

        <main className="content">
          {tab === 'spellBinding' && (
            <section className="grid sb-grid">
              <article className="card bound-weapons-card">
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
                      <button className="ghost-x-btn" onClick={() => doAction('spellBinding', 'unbindWeapon', row.key || {})} aria-label={`Unbind ${row.key?.displayName || 'weapon'}`}>
                        <X size={14} strokeWidth={2.25} />
                      </button>
                    </div>
                  ))}
                </div>
              </article>

              <article className="card equipped-card">
                <h3>Equipped Weapon</h3>
                <div className="current-preview-grid equipped-center">
                  <div className="preview-window preview-weapon-full">
                    <div className="preview-title slot-title">Equipped Weapon</div>
                    <div className="preview-body preview-body-weapon">
                      <div className="preview-name">{byPath(sb, ['currentWeapon', 'displayName'], 'None')}</div>
                      <div className="preview-meta">Level {weaponMasteryLevel}</div>
                    </div>
                  </div>
                  <div className="attack-slot-grid compact-slot-grid">
                    {attackSlots.map(({ id, label, slot, data }) => {
                      const isConc = String(data.castingType || '') === 'Concentration';
                      const isPowerOrShout = isCooldownSpellType(data.spellType);
                      const supportsCastCount = !!data.supportsCastCount && !isConc;
                      return (
                        <div className="preview-window attack-slot-card compact-slot-card" key={`slot-${id}`}>
                          <div className="slot-header">
                            <div className="preview-title slot-title">{label}</div>
                            {!!((data.enabled || !!data.spellFormKey || !!data.displayName) && currentWeapon.key) && (
                              <button
                                className="ghost-x-btn"
                                onClick={() => doAction('spellBinding', 'unbindSlot', { key: currentWeapon.key, slot })}
                                aria-label={`Unbind ${label}`}
                              >
                                <X size={14} strokeWidth={2.25} />
                              </button>
                            )}
                          </div>
                          <div className="preview-body slot-body-tight">
                            <div className="preview-name">{data.enabled ? data.displayName || 'Unknown' : 'None'}</div>
                            {data.enabled && (
                              <>
                                <div className="preview-meta">Cost: {formatSlotCostText(data)}{!isConc ? ` | Net: ${Math.max(0, Number(data.netCost || 0)).toFixed(0)}` : ''}</div>
                                {isCooldownSpellType(data.spellType) && <div className="preview-meta">{normalizeMetric(data.metric)}</div>}
                                {!isPowerOrShout && (
                                  <label className="inline slot-slider-inline">
                                    <span>{isConc ? 'Hold' : 'Interval'} {Number(isConc ? data.castHoldSec : data.castIntervalSec || 0).toFixed(2)}s</span>
                                    <input
                                      type="range"
                                      min={isConc ? '0.3' : '0.0'}
                                      max={isConc ? '10' : '1.0'}
                                      step="0.05"
                                      value={Number(isConc ? data.castHoldSec : data.castIntervalSec || 0)}
                                      onChange={(e) => onSlotSettingChange(slot, isConc ? 'castHoldSec' : 'castIntervalSec', Number(e.target.value || 0))}
                                    />
                                  </label>
                                )}
                                {supportsCastCount && (
                                  <label className="inline slot-slider-inline">
                                    <span>Casts x{Math.max(1, Number(data.castCount || 1))}</span>
                                    <input
                                      type="range"
                                      min="1"
                                      max="10"
                                      step="1"
                                      value={Math.max(1, Number(data.castCount || 1))}
                                      onChange={(e) => onSlotSettingChange(slot, 'castCount', Math.max(1, Number(e.target.value || 1)))}
                                    />
                                  </label>
                                )}
                              </>
                            )}
                          </div>
                        </div>
                      );
                    })}
                  </div>
                </div>
                <div className="row">
                  <label className="inline">
                    Cooldown
                    <input
                      className="thin-range"
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
                  <button className="btn" onClick={() => setBlacklistDialogOpen(true)}>Edit Blacklist</button>
                </div>
              </article>
            </section>
          )}

          {tab === 'smartCast' && (
            <section className="grid sc-grid">
              <article className="card">
                <h3>Chains</h3>
                <div className="subtabs chain-subtabs">
                  {chains.map((chain, i) => (
                    <button
                      key={`chain-${i + 1}`}
                      className={smartCastActiveChain === i + 1 ? 'subtab active' : 'subtab'}
                      onClick={() => setSmartCastActiveChain(i + 1)}
                    >
                      {chain.name || `Chain ${i + 1}`}
                    </button>
                  ))}
                  <button
                    className="ghost-x-btn"
                    onClick={() => {
                      const nextCount = Math.max(1, chains.length + 1);
                      setSetting('smartCast', 'global.maxChains', nextCount);
                      setSmartCastActiveChain(nextCount);
                    }}
                    aria-label="Add chain"
                  >
                    <Plus size={14} strokeWidth={2.25} />
                  </button>
                </div>
                <div className="row chain-title-row">
                  <textarea
                    className="chain-name-input"
                    id="sc-chain-name"
                    key={`sc-chain-name-${smartCastActiveChain}-${selectedChain.name || ''}`}
                    defaultValue={selectedChain.name || `Chain ${smartCastActiveChain}`}
                    style={{ width: compactChainInputWidth(selectedChain.name || `Chain ${smartCastActiveChain}`) }}
                    rows={1}
                    onBlur={(e) => setSetting('smartCast', 'chain.name', e.target.value, { index: smartCastActiveChain })}
                  />
                  <div className="chain-actions-right">
                    <label className="chain-delay-control inline">
                      <span>{selectedChainStepDelay.toFixed(2)}s</span>
                      <input
                        type="range"
                        min="0.5"
                        max="3"
                        step="0.05"
                        value={selectedChainStepDelay}
                        onChange={(e) => setSetting('smartCast', 'chain.step.stepDelaySec', Number(e.target.value || 1.0), { index: smartCastActiveChain })}
                      />
                    </label>
                    <button
                      className="btn hotkey-btn compact-hotkey-btn"
                      onClick={() => setCaptureField(`sc.chain.hotkey.${smartCastActiveChain}`)}
                    >
                      {captureLabel(`sc.chain.hotkey.${smartCastActiveChain}`)}
                    </button>
                    <button className="ghost-x-btn" onClick={() => doAction('smartCast', 'clearChain', { index: smartCastActiveChain })} aria-label={`Clear ${selectedChain.name || `Chain ${smartCastActiveChain}`}`}>
                      <Eraser size={14} strokeWidth={2.25} />
                    </button>
                    <button
                      className="ghost-x-btn ghost-danger-btn"
                      onClick={() => doAction('smartCast', 'deleteChain', { index: smartCastActiveChain })}
                      disabled={chains.length <= 1}
                      aria-label={`Delete ${selectedChain.name || `Chain ${smartCastActiveChain}`}`}
                    >
                      <Trash2 size={14} strokeWidth={2.25} />
                    </button>
                  </div>
                </div>
                <div className="scroll">
                  {(selectedChain.steps || []).length === 0 && <p className="meta">No recorded steps for this chain.</p>}
                  {(selectedChain.steps || []).length > 0 && (
                    <>
                      <table className="chain-step-table">
                        <thead>
                          <tr>
                            <th>Spell</th>
                            <th>Cost</th>
                            <th>Adjust</th>
                          </tr>
                        </thead>
                        <tbody>
                          {(selectedChain.steps || []).map((step, idx) => {
                            const spell = smartCastKnownSpellMap.get(String(step.spellFormID || '')) || {};
                            const isConc = Number(step.type || 0) === 1;
                            const spellName = spell.name || step.spellFormID || 'Unknown';
                            const baseCost = Number(spell.magickaCost || 0);
                            return (
                              <tr key={`step-${idx}`}>
                                <td>{spellName}</td>
                                <td>{formatCostLabel(step, baseCost)}</td>
                                <td>
                                  {isConc ? (
                                    <label className="inline chain-step-slider">
                                      <span>{Number(step.holdSec || 0.15).toFixed(2)}s</span>
                                      <input
                                        type="range"
                                        min="0.10"
                                        max="10.0"
                                        step="0.05"
                                        value={Number(step.holdSec || 0.15)}
                                        onChange={(e) => setSetting('smartCast', 'chain.step.holdSec', Number(e.target.value || 0.15), { index: smartCastActiveChain, step: idx })}
                                      />
                                    </label>
                                  ) : (
                                    <label className="inline chain-step-slider">
                                      <span>x{Math.max(1, Number(step.castCount || 1))}</span>
                                      <input
                                        type="range"
                                        min="1"
                                        max="10"
                                        step="1"
                                        value={Math.max(1, Number(step.castCount || 1))}
                                        onChange={(e) => setSetting('smartCast', 'chain.step.castCount', Math.max(1, Number(e.target.value || 1)), { index: smartCastActiveChain, step: idx })}
                                      />
                                    </label>
                                  )}
                                </td>
                              </tr>
                            );
                          })}
                        </tbody>
                      </table>
                      <div className="chain-total-cost">Total Magicka Cost: {smartCastTotalCost.toFixed(1)}</div>
                    </>
                  )}
                </div>
              </article>
            </section>
          )}

          {tab === 'quickBuff' && (
            <section className="grid qb-grid">
              <article className="card">
                <h3>Quick Buff</h3>
                <p className="meta">Select a trigger and then assign a spell to it.</p>
                <input placeholder="Search spell..." value={quickBuffSearch} onChange={(e) => setQuickBuffSearch(e.target.value)} />
                <div className="scroll">
                  <button
                    className="btn block"
                    onClick={() => {
                      setSetting('quickBuff', 'trigger.spellFormID', '', { trigger: quickBuffSelectedTrigger });
                      setSetting('quickBuff', 'trigger.enabled', false, { trigger: quickBuffSelectedTrigger });
                    }}
                  >
                    None
                  </button>
                  {quickBuffKnownSpellsFiltered.map((spell) => (
                    <button
                      className="btn block"
                      key={`qb-spell-${spell.formKey || spell.name}`}
                      onClick={() => {
                        const value = spell.formKey || '';
                        setSetting('quickBuff', 'trigger.spellFormID', value, { trigger: quickBuffSelectedTrigger });
                        setSetting('quickBuff', 'trigger.enabled', !!value, { trigger: quickBuffSelectedTrigger });
                      }}
                    >
                      {spell.name || 'Unknown'}
                    </button>
                  ))}
                </div>
              </article>
              <article className="card">
                <h3>Triggers</h3>
                <div className="trigger-grid">
                  {triggerOrder.map((triggerId) => {
                    const t = byPath(qb, ['config', 'triggers', triggerId], {});
                    const selected = quickBuffSelectedTrigger === triggerId;
                    const assigned = t.spellFormID
                      ? ((qb.knownSpells || []).find((spell) => (spell.formKey || '') === t.spellFormID)?.name || t.spellFormID)
                      : 'None';
                    return (
                      <button
                        className={selected ? 'trigger-card active' : 'trigger-card'}
                        key={triggerId}
                        onClick={() => setQuickBuffSelectedTrigger(triggerId)}
                      >
                        <div className="trigger-title">{triggerLabel(triggerId)}</div>
                        <div className="meta">Assigned: {assigned}</div>
                      </button>
                    );
                  })}
                </div>
              </article>
            </section>
          )}

          {tab === 'mastery' && (
            <section className="grid mastery-grid">
              <article className="card">
                <div className="subtabs mastery-subtabs">
                  {masteryTabs.map(({ id, label }) => (
                    <button key={id} className={masteryTab === id ? 'subtab active' : 'subtab'} onClick={() => setMasteryTab(id)}>{label}</button>
                  ))}
                </div>
                <div className="mastery-controls">
                  <input
                    placeholder="Search name or type..."
                    value={masterySearch}
                    onChange={(e) => setMasterySearch(e.target.value)}
                  />
                  {masteryTab !== 'shouts' && (
                    <div className="mastery-filter-wrap" ref={masteryFilterRef}>
                      <button className="mastery-filter-trigger" onClick={() => setMasteryFilterOpen((v) => !v)}>
                        <Filter size={14} strokeWidth={2} />
                        <span>{masteryTypeFilterByTab[masteryTab] === 'all' ? 'All Types' : (masteryTypeFilterByTab[masteryTab] || 'All Types')}</span>
                      </button>
                      {masteryFilterOpen && (
                        <div className="mastery-filter-menu">
                          <button
                            className={(masteryTypeFilterByTab[masteryTab] || 'all') === 'all' ? 'mastery-filter-item active' : 'mastery-filter-item'}
                            onClick={() => {
                              setMasteryTypeFilterByTab((prev) => ({ ...prev, [masteryTab]: 'all' }));
                              setMasteryFilterOpen(false);
                            }}
                          >
                            All Types
                          </button>
                          {masteryTypes.map((type) => (
                            <button
                              key={`mastery-type-${type}`}
                              className={(masteryTypeFilterByTab[masteryTab] || 'all') === type ? 'mastery-filter-item active' : 'mastery-filter-item'}
                              onClick={() => {
                                setMasteryTypeFilterByTab((prev) => ({ ...prev, [masteryTab]: type }));
                                setMasteryFilterOpen(false);
                              }}
                            >
                              {type}
                            </button>
                          ))}
                        </div>
                      )}
                    </div>
                  )}
                </div>
                <div className={masteryTab === 'weapons' ? 'mastery-table-wrap mastery-table-wrap-weapons' : 'scroll mastery-table-wrap'}>
                  <table className="mastery-table">
                    <thead>
                      <tr>
                        <th>
                          <button className="table-sort-btn" onClick={() => toggleMasterySort('name')}>
                            Name <ArrowUpDown size={13} strokeWidth={2} />
                          </button>
                        </th>
                        <th>
                          <button className="table-sort-btn" onClick={() => toggleMasterySort('type')}>
                            Type <ArrowUpDown size={13} strokeWidth={2} />
                          </button>
                        </th>
                        <th>
                          <button className="table-sort-btn" onClick={() => toggleMasterySort('uses')}>
                            Uses <ArrowUpDown size={13} strokeWidth={2} />
                          </button>
                        </th>
                        <th>
                          <button className="table-sort-btn" onClick={() => toggleMasterySort('progress')}>
                            Progression <ArrowUpDown size={13} strokeWidth={2} />
                          </button>
                        </th>
                        <th>
                          <button className="table-sort-btn" onClick={() => toggleMasterySort('level')}>
                            Level <ArrowUpDown size={13} strokeWidth={2} />
                          </button>
                        </th>
                      </tr>
                    </thead>
                    <tbody>
                      {masteryRows.map((row, idx) => {
                        const uses = Number(row.uses || 0);
                        const hits = Number(row.hits || 0);
                        const progressPoints = Number(row.progressPoints || 0);
                        const nextThreshold = Math.max(1, Number(row.nextThreshold || 1));
                        const progressRatio = Math.max(0, Math.min(1, Number(row.progressPct || 0)));
                        const progressPct = progressRatio * 100;
                        const usesCell = masteryTab === 'weapons' ? `${uses} / ${hits}` : `${uses}`;
                        return (
                          <tr key={`m-${masteryTab}-${idx}-${row.name || 'unknown'}`}>
                            <td>{row.name || 'Unknown'}</td>
                            <td>{row.type || 'Unknown'}</td>
                            <td>{usesCell}</td>
                            <td>
                              <div className="mastery-progress">
                                <div className="mastery-progress-meta">{progressPoints} / {nextThreshold} ({progressPct.toFixed(0)}%)</div>
                                <div className="mastery-progress-bar">
                                  <span style={{ width: `${progressPct}%` }} />
                                </div>
                              </div>
                            </td>
                            <td>{Number(row.level ?? 0)}</td>
                          </tr>
                        );
                      })}
                      {masteryRows.length === 0 && (
                        <tr>
                          <td colSpan={5} className="meta">No entries for current filters.</td>
                        </tr>
                      )}
                    </tbody>
                  </table>
                  </div>
              </article>
            </section>
          )}
        </main>

        {!windowState.isFullscreen && <div className="resize-handle" onMouseDown={startResize} />}
      </div>}

      {settingsOpen && (
        <div className="modal-overlay" onMouseDown={() => setSettingsOpen(false)}>
          <div className="modal" onMouseDown={(e) => e.stopPropagation()}>
            <header className="modal-header">
              <h2>Settings</h2>
              <button className="icon-btn danger" onClick={() => setSettingsOpen(false)}><X size={16} strokeWidth={2} /></button>
            </header>
            <nav className="settings-tabs">
              {settingsTabs.map(({ id, label }) => (
                <button key={id} className={settingsTab === id ? 'subtab active' : 'subtab'} onClick={() => setSettingsTab(id)}>{label}</button>
              ))}
            </nav>
            <section className="settings-body">
              {settingsTab === 'spellBinding' && (
                <div className="setting-grid">
                  <label>UI Toggle Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.uiToggleKey')}>{captureLabel('sb.uiToggleKey')}</button></label>
                  <label>Bind Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.bindKey')}>{captureLabel('sb.bindKey')}</button></label>
                  <label>Cycle Modifier Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.cycleSlotModifierKey')}>{captureLabel('sb.cycleSlotModifierKey')}</button></label>
                </div>
              )}

              {settingsTab === 'smartCast' && (
                <div className="setting-grid">
                  <label>Record Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sc.record.toggleKey')}>{captureLabel('sc.record.toggleKey')}</button></label>
                  <label>Play Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sc.playback.playKey')}>{captureLabel('sc.playback.playKey')}</button></label>
                  <label>Cycle Modifier <button className="btn hotkey-btn" onClick={() => setCaptureField('sc.playback.cycleModifierKey')}>{captureLabel('sc.playback.cycleModifierKey')}</button></label>
                  <label className="checkbox-inline">
                    <input
                      type="checkbox"
                      checked={!!byPath(sb, ['config', 'hudChainAlwaysShowInCombat'], false)}
                      onChange={(e) => setSetting('smartCast', 'hud.alwaysShowInCombat', e.target.checked)}
                    />
                    Always show in Combat
                  </label>
                </div>
              )}

              {settingsTab === 'quickBuff' && (
                <div className="setting-grid">
                  <p className="meta">More settings coming soon.</p>
                </div>
              )}

              {settingsTab === 'mastery' && (
                <div className="setting-grid">
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['spell', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masterySpell.enabled', e.target.checked)} /> Spell Enabled</label>
                  <label>Spell Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['spell', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masterySpell.gainMultiplier', Number(e.target.value || 1))} /></label>

                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['shout', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masteryShout.enabled', e.target.checked)} /> Shout Enabled</label>
                  <label>Shout Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['shout', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masteryShout.gainMultiplier', Number(e.target.value || 1))} /></label>

                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(mastery, ['weapon', 'config', 'enabled'], true)} onChange={(e) => setSetting('mastery', 'masteryWeapon.enabled', e.target.checked)} /> Weapon Enabled</label>
                  <label>Weapon Gain <input type="number" min="0.1" step="0.1" value={Number(byPath(mastery, ['weapon', 'config', 'gainMultiplier'], 1)).toFixed(1)} onChange={(e) => setSetting('mastery', 'masteryWeapon.gainMultiplier', Number(e.target.value || 1))} /></label>

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
                  <div className="row">
                    <button className="btn" onClick={resetWindowLayout}>Reset Window Layout</button>
                    {windowState.isFullscreen && <button className="btn" onClick={toggleFullscreen}>Exit Fullscreen</button>}
                  </div>
                </div>
              )}
            </section>
          </div>
        </div>
      )}

      {blacklistDialogOpen && (
        <div className="modal-overlay" onMouseDown={() => setBlacklistDialogOpen(false)}>
          <div className="modal" onMouseDown={(e) => e.stopPropagation()}>
            <header className="modal-header">
              <h2>Spell Blacklist</h2>
              <button className="icon-btn danger" onClick={() => setBlacklistDialogOpen(false)}><X size={16} strokeWidth={2} /></button>
            </header>
            <section className="settings-body">
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
            </section>
          </div>
        </div>
      )}

      {hudAdjustOpen && (
        <div className="hud-adjust">
          <div className="hud-adjust-head">Drag the UI elements, adjust sizes, then click Done.</div>
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
            <label className="inline">
              Cycle Size
              <input
                type="range"
                min="36"
                max="200"
                step="2"
                value={Number(byPath(sb, ['config', 'hudCycleSize'], 56))}
                onChange={(e) => setSetting('spellBinding', 'hudCycleSize', Number(e.target.value || 56))}
              />
            </label>
            <label className="inline">
              Chain Size
              <input
                type="range"
                min="36"
                max="200"
                step="2"
                value={Number(byPath(sb, ['config', 'hudChainSize'], 56))}
                onChange={(e) => setSetting('spellBinding', 'hudChainSize', Number(e.target.value || 56))}
              />
            </label>
            <button className="btn" onClick={() => finalizeHudAdjust(false)}>Done</button>
          </div>
        </div>
      )}
      {hudAdjustOpen && (
        <>
          <div
            className="hud-adjust-donut"
            style={{
              left: `${hudDragPos.donut?.x ?? 48}px`,
              top: `${hudDragPos.donut?.y ?? 48}px`,
              width: `${Number(byPath(sb, ['config', 'hudDonutSize'], 88))}px`,
              height: `${Number(byPath(sb, ['config', 'hudDonutSize'], 88))}px`
            }}
            onMouseDown={(e) => {
              setHudDragTarget('donut');
              hudDragRef.current = { target: 'donut', x: e.clientX, y: e.clientY, left: hudDragPos.donut?.x ?? 48, top: hudDragPos.donut?.y ?? 48 };
              e.preventDefault();
            }}
          >
            <svg viewBox="0 0 100 100">
              <circle className="track" cx="50" cy="50" r="42" />
              <circle className="progress" cx="50" cy="50" r="42" />
            </svg>
            <div className="label">1.5s</div>
          </div>
          <div
            className={`hud-adjust-chip ${hudDragTarget === 'cycle' ? 'active' : ''}`}
            style={{
              left: `${hudDragPos.cycle?.x ?? 48}px`,
              top: `${hudDragPos.cycle?.y ?? 152}px`,
              width: `${Number(byPath(sb, ['config', 'hudCycleSize'], 56))}px`,
              height: `${Number(byPath(sb, ['config', 'hudCycleSize'], 56))}px`
            }}
            onMouseDown={(e) => {
              setHudDragTarget('cycle');
              hudDragRef.current = { target: 'cycle', x: e.clientX, y: e.clientY, left: hudDragPos.cycle?.x ?? 48, top: hudDragPos.cycle?.y ?? 152 };
              e.preventDefault();
            }}
          >
            Light
          </div>
          <div
            className={`hud-adjust-chip ${hudDragTarget === 'chain' ? 'active' : ''}`}
            style={{
              left: `${hudDragPos.chain?.x ?? 48}px`,
              top: `${hudDragPos.chain?.y ?? 216}px`,
              width: `${Number(byPath(sb, ['config', 'hudChainSize'], 56))}px`,
              height: `${Number(byPath(sb, ['config', 'hudChainSize'], 56))}px`
            }}
            onMouseDown={(e) => {
              setHudDragTarget('chain');
              hudDragRef.current = { target: 'chain', x: e.clientX, y: e.clientY, left: hudDragPos.chain?.x ?? 48, top: hudDragPos.chain?.y ?? 216 };
              e.preventDefault();
            }}
          >
            Chain 1
          </div>
        </>
      )}

      {toast && <div className="toast">{toast}</div>}
    </>
  );
}

createRoot(document.getElementById('app')).render(<App />);
