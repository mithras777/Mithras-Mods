
import React, { useEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { LoaderCircle, Settings, Maximize2, Minimize2, X } from 'lucide-react';

const triggerOrder = ['combatStart', 'combatEnd', 'healthBelow70', 'healthBelow50', 'healthBelow30', 'crouchStart', 'sprintStart', 'weaponDraw', 'powerAttackStart', 'shoutStart'];
const settingsTabs = [
  { id: 'spellBinding', label: 'Spell Binding' },
  { id: 'smartCast', label: 'Smart Cast' },
  { id: 'quickBuff', label: 'Quick Buff' },
  { id: 'mastery', label: 'Mastery' },
  { id: 'uiHud', label: 'UI' }
];
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

function formatScanCode(scan) {
  const code = SPELLBIND_CODE_BY_SCAN[Number(scan)];
  if (!code) return `Key ${scan}`;
  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  return code;
}

function toSmartCastToken(event) {
  const code = normalizeEventCode(event);
  if (code.startsWith('Key') && code.length === 4) return code.slice(3).toUpperCase();
  if (code.startsWith('Digit') && code.length === 6) return code.slice(5);
  if (/^F([1-9]|1[0-2])$/.test(code)) return code;
  return null;
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
  const [hudDragPos, setHudDragPos] = useState({ x: 48, y: 48 });
  const [captureField, setCaptureField] = useState(null);
  const [windowState, setWindowState] = useState(defaultCenteredRect());

  const dragRef = useRef(null);
  const resizeRef = useRef(null);
  const lastWindowedRef = useRef(null);
  const toastTimerRef = useRef(null);
  const settingsOpenRef = useRef(false);
  const hudAdjustOpenRef = useRef(false);
  const hudConfigRef = useRef({ x: 48, y: 48 });
  const hudDragRef = useRef(null);
  const hudDragPosRef = useRef({ x: 48, y: 48 });
  const hudSizeRef = useRef(88);
  const captureFieldRef = useRef(null);

  const sb = snapshot.spellBinding || {};
  const sc = snapshot.smartCast || {};
  const qb = snapshot.quickBuff || {};
  const mastery = snapshot.mastery || {};

  const currentWeapon = sb.currentWeapon || {};
  const currentSlots = currentWeapon.slots || {};
  const attackSlots = [
    { id: 'light', label: 'Light Attack', slot: 0, data: currentSlots.light || {} },
    { id: 'power', label: 'Power Attack', slot: 1, data: currentSlots.power || {} },
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
    hudDragPosRef.current = hudDragPos;
  }, [hudDragPos]);

  useEffect(() => {
    hudSizeRef.current = Number(byPath(sb, ['config', 'hudDonutSize'], 88));
  }, [sb]);

  useEffect(() => {
    captureFieldRef.current = captureField;
  }, [captureField]);

  useEffect(() => {
    hudConfigRef.current = {
      x: Number(byPath(sb, ['config', 'hudPosX'], 48)),
      y: Number(byPath(sb, ['config', 'hudPosY'], 48))
    };
    if (!hudAdjustOpenRef.current) {
      setHudDragPos({ x: Number(byPath(sb, ['config', 'hudPosX'], 48)), y: Number(byPath(sb, ['config', 'hudPosY'], 48)) });
    }
  }, [sb]);

  const clampHudPos = (x, y, size) => {
    const width = window.innerWidth;
    const height = window.innerHeight;
    const nextX = Math.max(0, Math.min(width - size, x));
    const nextY = Math.max(0, Math.min(height - size, y));
    return { x: nextX, y: nextY };
  };

  const finalizeHudAdjust = (closeMenuAfter) => {
    const clamped = clampHudPos(hudDragPosRef.current.x, hudDragPosRef.current.y, hudSizeRef.current);
    hudAction('saveHudPosition', {
      x: clamped.x,
      y: clamped.y,
      commit: true
    });
    setHudAdjustOpen(false);
    if (closeMenuAfter) {
      callNative('sbo_toggle_ui', '');
    }
  };

  const startHudAdjustMode = () => {
    setSettingsOpen(false);
    setHudDragPos({ x: Number(byPath(sb, ['config', 'hudPosX'], 48)), y: Number(byPath(sb, ['config', 'hudPosY'], 48)) });
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
      const size = Number(byPath(sb, ['config', 'hudDonutSize'], 88));
      const next = clampHudPos(drag.left + (event.clientX - drag.x), drag.top + (event.clientY - drag.y), size);
      setHudDragPos(next);
      hudAction('saveHudPosition', { x: next.x, y: next.y, commit: false });
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

  const captureLabel = (field) => {
    if (captureField === field) return 'Press key to define...';
    if (field === 'sb.uiToggleKey') return String(byPath(sb, ['config', 'uiToggleKey'], 'Unset'));
    if (field === 'sb.bindKey') return String(byPath(sb, ['config', 'bindKey'], 'Unset'));
    if (field === 'sb.cycleSlotModifierKey') return formatScanCode(Number(byPath(sb, ['config', 'cycleSlotModifierKey'], 0)));
    if (field === 'sc.record.toggleKey') return String(byPath(sc, ['config', 'global', 'record', 'toggleKey'], 'Unset'));
    if (field === 'sc.playback.playKey') return String(byPath(sc, ['config', 'global', 'playback', 'playKey'], 'Unset'));
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

      if (captureField === 'sc.record.toggleKey' || captureField === 'sc.playback.playKey') {
        const token = toSmartCastToken(event);
        if (!token) {
          showToast('Use letters, digits, or F1-F12');
          return;
        }
        const id = captureField === 'sc.record.toggleKey' ? 'record.toggleKey' : 'playback.playKey';
        setSetting('smartCast', id, token);
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
          <div className="window-actions" onMouseDown={(e) => e.stopPropagation()}>
            <button className="icon-btn spin-icon-btn" onClick={startHudAdjustMode} aria-label="Adjust HUD Donut"><LoaderCircle size={16} strokeWidth={2} /></button>
            <button className="icon-btn" onClick={() => setSettingsOpen(true)} aria-label="Settings"><Settings size={16} strokeWidth={2} /></button>
            <button className="icon-btn" onClick={toggleFullscreen} aria-label={windowState.isFullscreen ? 'Windowed' : 'Fullscreen'}>
              {windowState.isFullscreen ? <Minimize2 size={16} strokeWidth={2} /> : <Maximize2 size={16} strokeWidth={2} />}
            </button>
            <button className="icon-btn danger" onClick={() => callNative('sbo_toggle_ui', '')} aria-label="Close"><X size={16} strokeWidth={2} /></button>
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
                <div className="current-preview-grid">
                  <div className="preview-window preview-weapon-full">
                    <div className="preview-title slot-title">Weapon</div>
                    <div className="preview-body preview-body-weapon">
                      <div className="preview-name">{byPath(sb, ['currentWeapon', 'displayName'], 'None')}</div>
                      <div className="preview-meta">{byPath(sb, ['currentWeapon', 'key', 'pluginName'], '') || 'No equipped weapon key'}</div>
                    </div>
                  </div>
                  <div className="attack-slot-grid">
                    {attackSlots.map(({ id, label, slot, data }) => (
                      <div className="preview-window attack-slot-card" key={`slot-${id}`}>
                        <div className="slot-header">
                          <div className="preview-title slot-title">{label}</div>
                          {!!(data.enabled && currentWeapon.key) && (
                            <button
                              className="icon-btn slot-unbind-btn"
                              onClick={() => doAction('spellBinding', 'unbindSlot', { key: currentWeapon.key, slot })}
                              aria-label={`Unbind ${label}`}
                            >
                              <X size={14} strokeWidth={2.25} />
                            </button>
                          )}
                        </div>
                        <div className="preview-body">
                          <div className="preview-name">{data.enabled ? data.displayName || 'Unknown' : 'None'}</div>
                          <div className="preview-meta">{data.enabled ? data.spellFormKey || '' : ''}</div>
                        </div>
                      </div>
                    ))}
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
                      <button className="icon-btn list-unbind-btn" onClick={() => doAction('spellBinding', 'unbindWeapon', row.key || {})} aria-label={`Unbind ${row.key?.displayName || 'weapon'}`}>
                        <X size={14} strokeWidth={2.25} />
                      </button>
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
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'enabled'], true)} onChange={(e) => setSetting('spellBinding', 'enabled', e.target.checked)} /> Enabled</label>
                  <label>UI Toggle Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.uiToggleKey')}>{captureLabel('sb.uiToggleKey')}</button></label>
                  <label>Bind Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.bindKey')}>{captureLabel('sb.bindKey')}</button></label>
                  <label>Cycle Modifier Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sb.cycleSlotModifierKey')}>{captureLabel('sb.cycleSlotModifierKey')}</button></label>
                  <label>Fallback Dedupe (s) <input type="number" step="0.1" min="0.5" max="5" value={Number(byPath(sb, ['config', 'fallbackDedupeSec'], 1.5)).toFixed(1)} onChange={(e) => setSetting('spellBinding', 'fallbackDedupeSec', Number(e.target.value || 1.5))} /></label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'showHudNotifications'], true)} onChange={(e) => setSetting('spellBinding', 'showHudNotifications', e.target.checked)} /> Show HUD Notifications</label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'enableSoundCues'], true)} onChange={(e) => setSetting('spellBinding', 'enableSoundCues', e.target.checked)} /> Enable Sound Cues</label>
                  <label>Sound Cue Volume <input type="number" min="0" max="1" step="0.05" value={Number(byPath(sb, ['config', 'soundCueVolume'], 0.45)).toFixed(2)} onChange={(e) => setSetting('spellBinding', 'soundCueVolume', Number(e.target.value || 0))} /></label>
                  <label className="checkbox-inline"><input type="checkbox" checked={!!byPath(sb, ['config', 'blacklistEnabled'], true)} onChange={(e) => setSetting('spellBinding', 'blacklistEnabled', e.target.checked)} /> Enable Blacklist</label>
                </div>
              )}

              {settingsTab === 'smartCast' && (
                <div className="setting-grid">
                  <label>Record Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sc.record.toggleKey')}>{captureLabel('sc.record.toggleKey')}</button></label>
                  <label>Play Key <button className="btn hotkey-btn" onClick={() => setCaptureField('sc.playback.playKey')}>{captureLabel('sc.playback.playKey')}</button></label>
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
          <div className="hud-adjust-head">Drag the donut below, adjust size/seconds, then click Done.</div>
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
            <button className="btn" onClick={() => finalizeHudAdjust(false)}>Done</button>
          </div>
        </div>
      )}
      {hudAdjustOpen && (
        <div
          className="hud-adjust-donut"
          style={{
            left: `${hudDragPos.x}px`,
            top: `${hudDragPos.y}px`,
            width: `${Number(byPath(sb, ['config', 'hudDonutSize'], 88))}px`,
            height: `${Number(byPath(sb, ['config', 'hudDonutSize'], 88))}px`
          }}
          onMouseDown={(e) => {
            hudDragRef.current = { x: e.clientX, y: e.clientY, left: hudDragPos.x, top: hudDragPos.y };
            e.preventDefault();
          }}
        >
          <svg viewBox="0 0 100 100">
            <circle className="track" cx="50" cy="50" r="42" />
            <circle className="progress" cx="50" cy="50" r="42" />
          </svg>
          {!!byPath(sb, ['config', 'hudShowCooldownSeconds'], true) && <div className="label">1.5s</div>}
        </div>
      )}

      {toast && <div className="toast">{toast}</div>}
    </>
  );
}

createRoot(document.getElementById('app')).render(<App />);
