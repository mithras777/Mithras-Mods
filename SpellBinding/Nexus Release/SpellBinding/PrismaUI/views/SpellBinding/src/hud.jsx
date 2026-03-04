import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';

const CIRCUMFERENCE = 2 * Math.PI * 42;

function call(name, payload = '') {
  const fn = window[name];
  if (typeof fn === 'function') fn(payload);
}

function Hud() {
  const [state, setState] = useState({
    visible: false,
    progress: 0,
    remainingSec: 0,
    x: 48,
    y: 48,
    size: 88,
    showSeconds: true,
    dragMode: false
  });

  const [drag, setDrag] = useState(null);

  useEffect(() => {
    window.sbo_renderHud = (payload) => {
      try {
        const parsed = JSON.parse(payload || '{}');
        setState((prev) => ({
          ...prev,
          ...parsed,
          progress: Math.max(0, Math.min(1, Number(parsed.progress || 0))),
          size: Math.max(48, Math.min(220, Number(parsed.size || 88)))
        }));
      } catch (_e) {}
    };
  }, []);

  useEffect(() => {
    const onMove = (e) => {
      if (!drag) return;
      const x = drag.left + (e.clientX - drag.x);
      const y = drag.top + (e.clientY - drag.y);
      setState((prev) => ({ ...prev, x, y }));
      call('sbo_hud_drag_update', JSON.stringify({ x, y }));
    };

    const onUp = () => {
      if (!drag) return;
      call('sbo_hud_drag_commit', JSON.stringify({ x: state.x, y: state.y, commit: true }));
      setDrag(null);
    };

    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [drag, state.x, state.y]);

  if (!state.visible) return null;

  return (
    <div className="root">
      <div
        className="donut"
        style={{ left: `${state.x}px`, top: `${state.y}px`, width: `${state.size}px`, height: `${state.size}px`, pointerEvents: state.dragMode ? 'auto' : 'none' }}
        onMouseDown={(e) => {
          if (!state.dragMode) return;
          setDrag({ x: e.clientX, y: e.clientY, left: state.x, top: state.y });
        }}
      >
        <svg viewBox="0 0 100 100">
          <circle className="track" cx="50" cy="50" r="42" />
          <circle className="progress" cx="50" cy="50" r="42" style={{ strokeDashoffset: String(CIRCUMFERENCE * state.progress) }} />
        </svg>
        {state.showSeconds !== false && <div className="label">{Number(state.remainingSec || 0).toFixed(1)}s</div>}
      </div>
    </div>
  );
}

createRoot(document.getElementById('root')).render(<Hud />);
