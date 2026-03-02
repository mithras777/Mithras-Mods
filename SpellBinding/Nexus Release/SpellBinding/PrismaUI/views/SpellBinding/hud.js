(function () {
  const root = document.getElementById('root')
  const donut = document.getElementById('donut')
  const progress = document.getElementById('progress')
  const label = document.getElementById('label')
  const circumference = 2 * Math.PI * 42
  let drag = null
  let dragMode = false

  function call(name, payload) {
    const fn = window[name]
    if (typeof fn === 'function') fn(payload || '')
  }

  function apply(state) {
    const visible = !!state.visible
    root.classList.toggle('hidden', !visible)
    dragMode = !!state.dragMode

    const p = Math.max(0, Math.min(1, Number(state.progress || 0)))
    progress.style.strokeDashoffset = String(circumference * p)
    const showSeconds = state.showSeconds !== false
    label.style.display = showSeconds ? 'grid' : 'none'
    label.textContent = Number(state.remainingSec || 0).toFixed(1) + 's'

    donut.style.left = Number(state.x || 48) + 'px'
    donut.style.top = Number(state.y || 48) + 'px'
    const size = Math.max(48, Math.min(220, Number(state.size || 88)))
    donut.style.width = size + 'px'
    donut.style.height = size + 'px'
    donut.style.pointerEvents = dragMode ? 'auto' : 'none'
  }

  window.sb_renderHud = function (payload) {
    try {
      const state = JSON.parse(payload || '{}')
      apply(state)
    } catch (_) {}
  }

  donut.addEventListener('mousedown', function (e) {
    if (!dragMode) return
    drag = { x: e.clientX, y: e.clientY, left: donut.offsetLeft, top: donut.offsetTop }
  })

  window.addEventListener('mousemove', function (e) {
    if (!drag) return
    const x = drag.left + (e.clientX - drag.x)
    const y = drag.top + (e.clientY - drag.y)
    donut.style.left = x + 'px'
    donut.style.top = y + 'px'
    call('sb_hud_drag_update', JSON.stringify({ x: x, y: y }))
  })

  window.addEventListener('mouseup', function () {
    if (!drag) return
    const x = donut.offsetLeft
    const y = donut.offsetTop
    drag = null
    call('sb_hud_drag_commit', JSON.stringify({ x: x, y: y, commit: true }))
  })
})()
