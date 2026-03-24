import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';

function call(name, payload = '') {
  const fn = window[name];
  if (typeof fn === 'function') fn(payload);
}

function hudDimensionsForChip(type, size) {
  const h = Math.max(36, Math.min(200, Number(size || 56)));
  const w = type === 'cycle' ? Math.round(h * 3.1) : Math.round(h * 3.2);
  return { width: w, height: h };
}

function Hud() {
  const [state, setState] = useState({
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

  const normalizeChip = (raw, fallbackText, fallbackX, fallbackY) => ({
    visible: !!raw.visible,
    text: String(raw.text || fallbackText),
    subtext: String(raw.subtext || ''),
    isError: !!raw.isError,
    currentSpell: String(raw.currentSpell || ''),
    stepProgress: Math.max(0, Math.min(1, Number(raw.stepProgress || 0))),
    x: Number(raw.x || fallbackX),
    y: Number(raw.y || fallbackY),
    size: Math.max(36, Math.min(200, Number(raw.size || 56))),
    dragMode: !!raw.dragMode,
    isRecording: !!raw.isRecording,
    recordingChainText: String(raw.recordingChainText || ''),
    isPlaying: !!raw.isPlaying,
    playingChainText: String(raw.playingChainText || fallbackText)
  });

  useEffect(() => {
    window.sbo_renderHud = (payload) => {
      try {
        const parsed = JSON.parse(payload || '{}');
        setState({
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
      call('sbo_hud_drag_update', JSON.stringify({ target: drag.target === 'cycleHud' ? 'cycle' : 'chain', x, y }));
    };

    const onUp = () => {
      if (!drag) return;
      const targetState = state[drag.target];
      call('sbo_hud_drag_commit', JSON.stringify({
        target: drag.target === 'cycleHud' ? 'cycle' : 'chain',
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

  if (!state.cycleHud.visible && !state.chainHud.visible) return null;

  return (
    <div className="root">
      {state.cycleHud.visible && (
        <div
          className={['mini-hud', 'cycle-hud', state.cycleHud.isError ? 'error' : ''].join(' ').trim()}
          style={{
            left: `${state.cycleHud.x}px`,
            top: `${state.cycleHud.y}px`,
            width: `${hudDimensionsForChip('cycle', state.cycleHud.size).width}px`,
            height: `${hudDimensionsForChip('cycle', state.cycleHud.size).height}px`,
            pointerEvents: state.cycleHud.dragMode ? 'auto' : 'none'
          }}
          onMouseDown={(e) => {
            if (!state.cycleHud.dragMode) return;
            setDrag({ target: 'cycleHud', x: e.clientX, y: e.clientY, left: state.cycleHud.x, top: state.cycleHud.y });
          }}
        >
          <span className="cycle-title">{state.cycleHud.text || 'Light'}</span>
          <span className="cycle-subtext">{state.cycleHud.subtext || 'None'}</span>
        </div>
      )}
      {state.chainHud.visible && (
        <div
          className={[
            'mini-hud',
            'chain-hud',
            state.chainHud.isRecording ? 'recording' : '',
            state.chainHud.isPlaying ? 'playing' : ''
          ].join(' ').trim()}
          style={{
            left: `${state.chainHud.x}px`,
            top: `${state.chainHud.y}px`,
            width: `${hudDimensionsForChip('chain', state.chainHud.size).width}px`,
            height: `${hudDimensionsForChip('chain', state.chainHud.size).height}px`,
            pointerEvents: state.chainHud.dragMode ? 'auto' : 'none'
          }}
          onMouseDown={(e) => {
            if (!state.chainHud.dragMode) return;
            setDrag({ target: 'chainHud', x: e.clientX, y: e.clientY, left: state.chainHud.x, top: state.chainHud.y });
          }}
        >
          <span>
            {state.chainHud.isRecording
              ? state.chainHud.recordingChainText
              : state.chainHud.isPlaying
                ? state.chainHud.playingChainText
                : state.chainHud.text}
          </span>
          {state.chainHud.isPlaying && (
            <>
              <span className="chain-spell">{state.chainHud.currentSpell || '...'}</span>
              <div className="chain-progress-track">
                <span className="chain-progress-fill" style={{ width: `${Math.round(state.chainHud.stepProgress * 100)}%` }} />
              </div>
            </>
          )}
        </div>
      )}
    </div>
  );
}

createRoot(document.getElementById('root')).render(<Hud />);
