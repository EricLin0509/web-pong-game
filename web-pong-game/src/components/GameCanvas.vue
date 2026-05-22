<template>
  <div class="game-wrapper">
    <div :class="['fps-display', { 'animate-slide-down': isWasmLoaded },
      { 'fps-low': fps < 60 && fps >= 30, 'fps-very-low': fps < 30 }]">FPS: {{ fps }}</div>
    <div class="screen-container">
      <!-- Player 1 score -->
      <div :class="['score score-left', { show: isGameRunning, flash: flashLeft }]">{{ leftScore }}</div>
      <!-- The canvas frame -->
      <div :class="['frame', { 'animate-slide-down': isWasmLoaded }]" ref="frameRef">
        <canvas ref="canvasRef" id="canvas" tabindex="0" @click="canvasRef?.focus()"></canvas>
      </div>
      <!-- Player 2 score -->
      <div :class="['score score-right', { show: isGameRunning, flash: flashRight }]">{{ rightScore }}</div>
    </div>

    <div :class="['control-bar', { 'animate-slide-up': isWasmLoaded }]">
      <label>❄️ Snowflakes: <span class="count-value">{{ snowCount }}</span></label>
      <input
        type="range"
        class="snow-slider"
        min="0"
        max="8000"
        step="500"
        v-model="snowCount"
        @input="onSnowCountChange"
        :disabled="!isWasmLoaded"
      />
    </div>
  </div>
  <div :class="['history-panel', { 'animate-slide-right': isWasmLoaded }]" v-if="historyRecords.length">
    <h3>🏆 Recent scores (Classic)</h3>
    <ul>
      <li v-for="(record, idx) in historyRecords" :key="idx">
        {{ record.left }} : {{ record.right }} &nbsp; 
        <span class="time">{{ formatTime(record.timestamp) }}</span>
      </li>
    </ul>
    <button @click="clearHistory" class="clear-btn">Clear History</button>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { loadWasm } from '../composables/useWasm.js'

// History records
const historyRecords = ref([])

const frameRef = ref(null)
const canvasRef = ref(null)

const leftScore = ref(0)
const flashLeft = ref(false)
const rightScore = ref(0)
const flashRight = ref(false)

const isWasmLoaded = ref(false)
const isGameRunning = ref(false)

// FPS display
const fps = ref(0)
let frameCount = 0
let lastFpsTime = performance.now()
let rafId = null

// Snowflake count
const snowCount = ref(0)

let pongModule = null

// When snowflake count changes, update the C side
function onSnowCountChange() {
  if (pongModule && typeof pongModule._set_snowflake_count === 'function') {
    pongModule._set_snowflake_count(snowCount.value)
  }
}

// Update FPS every second
function updateFPS() {
  const now = performance.now()
  frameCount++
  if (now - lastFpsTime >= 1000) {
    fps.value = frameCount
    frameCount = 0
    lastFpsTime = now
  }
  rafId = requestAnimationFrame(updateFPS)
}

// Update snowflake count from C side
window.updateSnowflakeCount = (count) => {
  snowCount.value = count
}

function handleVisibilityChange() {
  if (document.hidden && pongModule) {
    pongModule._pause_game()
  }
}

function updateFrameStyles() {
  if (!frameRef.value) return
  const min_size = Math.min(window.innerHeight, window.innerWidth)

  const borderSize = Math.max(2, Math.min(6, min_size * 0.015))
  const cornerRadius = Math.max(8, Math.min(20, min_size * 0.03))

  frameRef.value.style.setProperty('--border-size', borderSize + 'px')
  frameRef.value.style.setProperty('--corner-radius', cornerRadius + 'px')
}

function loadHistory() {
  const stored = localStorage.getItem('pong_score_history')
  if (stored) {
    try {
      historyRecords.value = JSON.parse(stored)
    } catch(e) { console.error(e) }
  }
}

function saveScoreRecord(left, right) {
  const newRecord = {
    left: left,
    right: right,
    timestamp: Date.now()
  }
  let records = [...historyRecords.value]
  records.unshift(newRecord)
  if (records.length > 5) records = records.slice(0, 5) // Only keep the last 5 records
  historyRecords.value = records
  localStorage.setItem('pong_score_history', JSON.stringify(records))
}

function clearHistory() {
  historyRecords.value = []
  localStorage.removeItem('pong_score_history')
}

function formatTime(timestamp) {
  return new Date(timestamp).toLocaleString()
}

onMounted(async () => {
  if (!canvasRef.value) {
    console.error('Canvas ref is not available')
    return
  }

  window.updateScore = (left, right) => {
    if (left > leftScore.value) {
      flashLeft.value = true
      setTimeout(() => { flashLeft.value = false }, 200)
    }

    if (right > rightScore.value) {
      flashRight.value = true
      setTimeout(() => { flashRight.value = false }, 200)
    }

    leftScore.value = left
    rightScore.value = right
  }

  window.isGameRunning = (isRunning) => {
    isGameRunning.value = isRunning
  }

  window.onGameOver = (left, right) => {
    saveScoreRecord(left, right)
  }

  window.addEventListener('resize', updateFrameStyles)
  document.addEventListener('visibilitychange', handleVisibilityChange)

  pongModule = await loadWasm(canvasRef.value)
  isWasmLoaded.value = true

  // Initialize snowflake count
  if (pongModule && typeof pongModule._set_snowflake_count === 'function') {
    pongModule._set_snowflake_count(snowCount.value)
  }

  // Initialize FPS
  updateFPS()

  // Load history records
  loadHistory()
})

onBeforeUnmount(() => {
  delete window.updateScore
  delete window.onGameOver
  delete window.updateSnowflakeCount
  window.removeEventListener('resize', updateFrameStyles)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  if (rafId) cancelAnimationFrame(rafId)
})
</script>

<style scoped>
.game-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100vh;
}

.control-bar {
  margin-bottom: 15px;
  background: rgba(0,0,0,0.6);
  padding: 8px 20px;
  border-radius: 30px;
  backdrop-filter: blur(5px);
  font-family: monospace;
  font-size: 18px;
  color: #ccddee;
  display: flex;
  gap: 20px;
  align-items: center;
}

.fps-display {
  font-family: monospace;
  font-size: 18px;
  background: #1e2a3a;
  padding: 4px 12px;
  border-radius: 20px;
  color: #88ffaa;
  font-weight: bold;
  letter-spacing: 1px;
  box-shadow: 0 0 4px rgba(0,0,0,0.3);
}

.fps-display.fps-low {
  color: #ffaa66;
  text-shadow: 0 0 4px rgba(255,170,0,0.5);
}

.fps-display.fps-very-low {
  color: #ff4444;
  text-shadow: 0 0 4px rgba(255,0,0,0.5);
}
  

.count-value {
  font-weight: bold;
  color: #ffaa66;
  min-width: 70px;
  display: inline-block;
  text-align: center;
}

.snow-slider {
  width: 300px;
  cursor: pointer;
  background: #2a3a5a;
  height: 4px;
  border-radius: 2px;
  -webkit-appearance: none;
  appearance: none;
}
.snow-slider:focus {
  outline: none;
}
.snow-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: #ffaa66;
  cursor: pointer;
}

.screen-container {
  width: 100%;
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 20px;
  padding: 0 10px;
  box-sizing: border-box;
  flex: 1;
}

.score {
  flex-shrink: 1;
  font-family: 'Courier New', monospace;
  font-size: 5vw;
  font-weight: bold;
  color: white;
  text-shadow: 0 0 10px rgba(255,255,255,0.5);
  min-width: 80px;
  text-align: center;
  user-select: none;
}

.frame {
  --border-size: 6px;
  --corner-radius: 20px;
  display: inline-block;
  border: var(--border-size) solid #55868b;
  border-radius: var(--corner-radius);
  overflow: hidden;
  box-shadow: 0 0 20px rgba(255,255,255,0.5);
  line-height: 0;
  background-color: #092427;
}

canvas {
  display: block;
  width: 100%;
  height: 100%;
  max-width: 75vw;
  max-height: 75vh;
  object-fit: contain;
  image-rendering: crisp-edges;
}

.history-panel {
  position: fixed;
  bottom: 20px;
  left: 20px;
  background: rgba(0,0,0,0.7);
  backdrop-filter: blur(8px);
  border-radius: 16px;
  padding: 12px 20px;
  font-family: monospace;
  color: #ccddee;
  min-width: 200px;
  border-left: 4px solid #ffaa66;
  box-shadow: 0 4px 12px rgba(0,0,0,0.3);
}
.history-panel h3 {
  margin: 0 0 8px 0;
  font-size: 16px;
  color: #ffaa66;
}
.history-panel ul {
  margin: 0;
  padding-left: 20px;
}
.history-panel li {
  font-size: 14px;
  line-height: 1.5;
}
.time {
  font-size: 11px;
  opacity: 0.7;
}
.clear-btn {
  margin-top: 8px;
  background: #3a4a6a;
  border: none;
  color: white;
  border-radius: 20px;
  padding: 4px 12px;
  cursor: pointer;
  font-size: 12px;
}
.clear-btn:hover {
  background: #ffaa66;
  color: #0a0f1a;
}

/* Animation */
@keyframes slideDownEnter {
  0% {
    opacity: 0;
    transform: translateY(-60px);
  }
  100% {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes slideUpEnter {
  0% {
    opacity: 0;
    transform: translateY(60px);
  }
  100% {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes slideRightEnter {
  from {
    opacity: 0;
    transform: translateX(-60px);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@keyframes scoreFlash {
  0% { text-shadow: 0 0 0px rgba(255,255,255,0); opacity: 1; }
  50% { text-shadow: 0 0 20px rgba(255,255,255,0.9); opacity: 0.7; transform: scale(1.1); }
  100% { text-shadow: 0 0 10px rgba(255,255,255,0.5); opacity: 1; transform: scale(1); }
}

.score.flash {
  animation: scoreFlash 0.3s ease-out;
}

.animate-slide-down {
  animation: slideDownEnter 0.5s ease-out forwards;
}

.animate-slide-up {
  animation: slideUpEnter 0.8s ease-out forwards;
}

.animate-slide-right {
  animation: slideRightEnter 0.7s ease-out forwards;
}

.fps-display,
.control-bar,
.score,
.frame {
  opacity: 0;
  transform: translateY(30px);
  transition: opacity 0.4s ease-out, transform 0.4s ease-out;
}

.score.show {
  opacity: 1;
  transform: translateY(0);
}
</style>