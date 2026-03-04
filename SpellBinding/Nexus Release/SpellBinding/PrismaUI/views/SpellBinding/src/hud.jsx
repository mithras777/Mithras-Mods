import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';

const CIRCUMFERENCE = 2 * Math.PI * 42;

function call(name, payload = '') {
  const fn = window[name];
  if (typeof fn === 'function') fn(payload);
}

function Hud() {
  const [state, setState] = useState({
    donut: {
      visible: false,
      progress: 0,
      remainingSec: 0,
      x: 48,
      y: 48,
      size: 88,
      showSeconds: true,
      dragMode: false
    },
    cycleHud: {
      visible: false,
      text: 'Light',
      x: 48,
      y: 152,
      size: 56,
      dragMode: false
    },
    chainHud: {
      visible: false,
      text: 'Chain 1',
      x: 48,
      y: 216,
      size: 56,
      dragMode: false
    }
  });

  const [drag, setDrag] = useState(null);

  const normalizeDonut = (raw) => ({
    visible: !!raw.visible,
    progress: Math.max(0, Math.min(1, Number(raw.progress || 0))),
    remainingSec: Number(raw.remainingSec || 0),
    x: Number(raw.x || 48),
    y: Number(raw.y || 48),
    size: Math.max(48, Math.min(220, Number(raw.size || 88))),
    showSeconds: raw.showSeconds !== false,
    dragMode: !!raw.dragMode
  });

  const normalizeChip = (raw, fallbackText, fallbackX, fallbackY) => ({
    visible: !!raw.visible,
    text: String(raw.text || fallbackText),
    x: Number(raw.x || fallbackX),
    y: Number(raw.y || fallbackY),
    size: Math.max(36, Math.min(200, Number(raw.size || 56))),
    dragMode: !!raw.dragMode
  });

  useEffect(() => {
    window.sbo_renderHud = (payload) => {
      try {
        const parsed = JSON.parse(payload || '{}');
        const donutPayload = parsed.donut || parsed;
        setState({
          donut: normalizeDonut(donutPayload),
          cycleHud: normalizeChip(parsed.cycleHud || {}, 'Light', 48, 152),
          chainHud: normalizeChip(parsed.chainHud || {}, 'Chain 1', 48, 216)
        });
      } catch (_e) {}
    };
  }, []);

  useEffect(() => {
    const onMove = (e) => {
      if (!drag) return;
      const x = drag.left + (e.clientX - drag.x);
      const y = drag.top + (e.clientY - drag.y);
      setState((prev) => ({
        ...prev,
        [drag.target]: {
          ...prev[drag.target],
          x,
          y
        }
      }));
      call('sbo_hud_drag_update', JSON.stringify({ target: drag.target === 'donut' ? 'donut' : drag.target === 'cycleHud' ? 'cycle' : 'chain', x, y }));
    };

    const onUp = () => {
      if (!drag) return;
      const targetState = state[drag.target];
      call('sbo_hud_drag_commit', JSON.stringify({
        target: drag.target === 'donut' ? 'donut' : drag.target === 'cycleHud' ? 'cycle' : 'chain',
        x: targetState.x,
        y: targetState.y,
        commit: true
      }));
      setDrag(null);
    };

    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [drag, state]);

  if (!state.donut.visible && !state.cycleHud.visible && !state.chainHud.visible) return null;

  return (
    <div className="root">
      {state.donut.visible && (
        <div
          className="donut"
          style={{ left: `${state.donut.x}px`, top: `${state.donut.y}px`, width: `${state.donut.size}px`, height: `${state.donut.size}px`, pointerEvents: state.donut.dragMode ? 'auto' : 'none' }}
          onMouseDown={(e) => {
            if (!state.donut.dragMode) return;
            setDrag({ target: 'donut', x: e.clientX, y: e.clientY, left: state.donut.x, top: state.donut.y });
          }}
        >
          <svg viewBox="0 0 100 100">
            <circle className="track" cx="50" cy="50" r="42" />
            <circle className="progress" cx="50" cy="50" r="42" style={{ strokeDashoffset: String(CIRCUMFERENCE * state.donut.progress) }} />
          </svg>
          {state.donut.showSeconds !== false && <div className="label">{Number(state.donut.remainingSec || 0).toFixed(1)}s</div>}
        </div>
      )}
      {state.cycleHud.visible && (
        <div
          className="mini-hud"
          style={{ left: `${state.cycleHud.x}px`, top: `${state.cycleHud.y}px`, width: `${state.cycleHud.size}px`, height: `${state.cycleHud.size}px`, pointerEvents: state.cycleHud.dragMode ? 'auto' : 'none' }}
          onMouseDown={(e) => {
            if (!state.cycleHud.dragMode) return;
            setDrag({ target: 'cycleHud', x: e.clientX, y: e.clientY, left: state.cycleHud.x, top: state.cycleHud.y });
          }}
        >
          <span>{state.cycleHud.text || 'Light'}</span>
        </div>
      )}
      {state.chainHud.visible && (
        <div
          className="mini-hud"
          style={{ left: `${state.chainHud.x}px`, top: `${state.chainHud.y}px`, width: `${state.chainHud.size}px`, height: `${state.chainHud.size}px`, pointerEvents: state.chainHud.dragMode ? 'auto' : 'none' }}
          onMouseDown={(e) => {
            if (!state.chainHud.dragMode) return;
            setDrag({ target: 'chainHud', x: e.clientX, y: e.clientY, left: state.chainHud.x, top: state.chainHud.y });
          }}
        >
          <span>{state.chainHud.text || 'Chain 1'}</span>
        </div>
      )}
    </div>
  );
}

createRoot(document.getElementById('root')).render(<Hud />);
