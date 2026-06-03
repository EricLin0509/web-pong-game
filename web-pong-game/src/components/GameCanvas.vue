<template>
  <div class="game-wrapper">
  <div class="game-info" :class="{ 'animate-slide-down': isWasmLoaded }">
    <div class="info-item">
      <span class="info-icon">🎮</span>
      <span :class="['info-text', gameMode === 'Classic' ? 'mode-classic' : 'mode-infinite']">{{ gameMode }}</span>
    </div>
    <div class="info-item">
      <span class="info-icon">👥</span>
      <span :class="['info-text', Players === 'Single' ? 'mode-single' : 'mode-double']">{{ Players }}</span>
    </div>
    <div class="info-item">
      <span class="info-icon">🎨</span>
      <span class="info-text">Theme <span class="theme-badge">{{ themeNumber }}</span></span>
    </div>
  </div>
    <div class="screen-container">
      <!-- Player 1 score -->
      <div :class="['score score-left', { show: gameState === 1 || gameState === 2, flash: flashLeft }]">{{ leftScore }}</div>
      <!-- The canvas frame -->
        <div :class="['frame', { 'animate-slide-down': isWasmLoaded }]" ref="frameRef">
          <canvas ref="canvasRef" id="canvas" tabindex="0" @click="canvasRef?.focus()"></canvas>
          <div v-if="!isCanvasFocused && (gameState === 1 || gameState === 2)" class="focus-overlay">
            ⚠️ Canvas is not focused! ⚠️
          </div>

          <div class="button-bar" v-if="isWasmLoaded">
            <button v-if="gameState === 0" @click="startGame" class="game-btn">▶ Start Game</button>
            <button v-if="gameState === 0" @click="toggleGameMode" class="game-btn">🔄 Switch Mode</button>
            <button v-if="gameState === 0" @click="togglePlayer" class="game-btn">👥 Switch Player</button>
            <button v-if="gameState === 2" @click="resumeGame" class="game-btn">➡️ Resume Game</button>
            <button v-if="gameState === 2 || gameState === 3" @click="gotoMenu" class="game-btn">↩️ Back to Menu</button>
            <button v-if="gameState === 3" @click="restartGame" class="game-btn">🔄 Restart Game</button>
          </div>
        </div>
      <!-- Player 2 score -->
      <div :class="['score score-right', { show: gameState === 1 || gameState === 2, flash: flashRight }]">{{ rightScore }}</div>
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
  <div ref="introRef" class="game-intro" :class="{ visible: introVisible }">
    <div class="intro-content">
      <h2>🎮 About Pong</h2>
      <p>
        Pong is a 1972 sports video game developed and published by Atari, Inc. for arcades. It was created by Allan Alcorn as a training exercise assigned to him by Atari co-founder Nolan Bushnell. Bushnell and Atari co-founder Ted Dabney were so surprised by the quality of Alcorn's work that they decided to manufacture the game. Bushnell based the game's concept on an electronic ping-pong game included on the Magnavox Odyssey, the first home video game console; in response, Magnavox later sued Atari for patent infringement.
      </p>
      <h6>Source: <a href="https://en.wikipedia.org/wiki/Pong" target="_blank">Wikipedia</a></h6>
    </div>
  </div>

  <div :class="['keyboard-hint', { 'animate-slide-right': isWasmLoaded}]" v-if="isWasmLoaded">
    <h3>⌨️ Keyboard Controls</h3>
    <ul>
      <li><kbd>W</kbd> / <kbd>S</kbd> : Left paddle</li>
      <li><kbd>I</kbd> / <kbd>K</kbd> : Right paddle</li>
      <li><kbd>Space</kbd> : Start / Pause / Resume</li>
      <li><kbd>M</kbd> : Switch mode</li>
      <li><kbd>C</kbd> : Change theme</li>
      <li><kbd>B</kbd> / <kbd>V</kbd> : Snowflakes +/-</li>
      <li><kbd>ESC</kbd> : Back to menu</li>
    </ul>
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
const isCanvasFocused = ref(false)

const leftScore = ref(0)
const flashLeft = ref(false)
const rightScore = ref(0)
const flashRight = ref(false)

const isWasmLoaded = ref(false)
const gameState = ref(-1)  // 0:INIT, 1:RUNNING, 2:PAUSED, 3:OVER

const introRef = ref(null)
const introVisible = ref(false)
let observer = null

// Game mode and theme number
const gameMode = ref('Classic')
const Players = ref('Single')
const themeNumber = ref(1)

// Snowflake count
const snowCount = ref(0)

let pongModule = null

// When snowflake count changes, update the C side
function onSnowCountChange() {
  if (pongModule && typeof pongModule._set_snowflake_count === 'function') {
    pongModule._set_snowflake_count(snowCount.value)
  }
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

window.onGameStateChange = (state) => {
  gameState.value = state
}

window.onModeThemeChange = (game_mode, player, themeIdx) => {
  gameMode.value = game_mode === 0 ? 'Classic' : 'Infinite'
  Players.value = player === 0 ? 'Single' : 'Double'
  themeNumber.value = themeIdx
}

function startGame() {
  if (pongModule && pongModule._start_game) {
    pongModule._start_game()
    canvasRef.value?.focus()
  }
}

function resumeGame() {
  if (pongModule && pongModule._resume_game) {
    pongModule._resume_game()
    canvasRef.value?.focus()
  }
}

function restartGame() {
  if (pongModule && pongModule._restart_game) {
    pongModule._restart_game()
    canvasRef.value?.focus()
  }
}

function gotoMenu() {
  if (pongModule && pongModule._goto_menu) {
    pongModule._goto_menu()
  }
}

function toggleGameMode() {
  if (pongModule && pongModule._toggle_game_mode) {
    pongModule._toggle_game_mode()
  }
}

function togglePlayer() {
  if (pongModule && pongModule._toggle_player) {
    pongModule._toggle_player()
  }
}

const handleCanvasFocus = () => {
  isCanvasFocused.value = true
}

const handleCanvasBlur = () => {
  isCanvasFocused.value = false
  if (pongModule) {
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

  window.storeScoreHistory = (left, right) => {
    saveScoreRecord(left, right)
  }

  window.addEventListener('resize', updateFrameStyles)
  document.addEventListener('visibilitychange', handleVisibilityChange)

  canvasRef.value.addEventListener('focus', handleCanvasFocus)
  canvasRef.value.addEventListener('blur', handleCanvasBlur)

  pongModule = await loadWasm(canvasRef.value)
  isWasmLoaded.value = true

  // Initialize snowflake count
  if (pongModule && typeof pongModule._set_snowflake_count === 'function') {
    pongModule._set_snowflake_count(snowCount.value)
  }

  if (introRef.value) {
    observer = new IntersectionObserver(
      (entries) => {
        entries.forEach(entry => {
          if (entry.isIntersecting) {
            introVisible.value = true
            if (observer && introRef.value) observer.unobserve(introRef.value)
          }
        })
      },
      { threshold: 0.3 }
    )
    observer.observe(introRef.value)
  }

  // Load history records
  loadHistory()
})

onBeforeUnmount(() => {
  delete window.updateScore
  delete window.onGameOver
  delete window.updateSnowflakeCount
  delete window.onModeThemeChange

  window.removeEventListener('resize', updateFrameStyles)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  canvasRef.value?.removeEventListener('focus', handleCanvasFocus)
  canvasRef.value?.removeEventListener('blur', handleCanvasBlur)
  if (rafId) cancelAnimationFrame(rafId)
  if (observer) observer.disconnect()
})
</script>

<style>
:root {
  --panel-bg: rgba(16, 36, 60, 0.9);
  --panel-bg-hover: rgba(16, 36, 60, 0.95);
  --btn-bg: rgba(20, 44, 70, 0.92);
}
</style>

<style scoped>

.app-container {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.canvas-container {
  position: relative;
  display: inline-block;
  width: 100%;
  height: 100%;
}

.focus-overlay {
  position: absolute;
  top: 16px;
  left: 50%;
  transform: translateX(-50%);
  backdrop-filter: blur(8px);
  padding: 14px 24px;
  border-radius: 40px;
  font-family: monospace;
  font-size: 16px;
  font-weight: bold;
  color: white;
  pointer-events: none;
  white-space: nowrap;
  z-index: 10;
  box-shadow: 0 4px 12px rgba(0,0,0,0.4);
  border: 1px solid rgba(255,255,255,0.2);
  letter-spacing: 0.5px;
}

.game-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100vh;
  flex-shrink: 0;
}

.control-bar {
  margin-bottom: 15px;
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

.game-info {
  font-family: 'Courier New', 'Fira Code', monospace;
  font-size: 1.2rem;
  font-weight: bold;
  background: rgba(10, 20, 30, 0.7);
  backdrop-filter: blur(8px);
  color: #ffdd99;
  padding: 8px 20px;
  border-radius: 50px;
  border: 2px solid rgba(255, 220, 150, 0.5);
  box-shadow: 0 0 12px rgba(0,0,0,0.3);
  display: inline-flex;
  align-items: center;
  gap: 24px;
  letter-spacing: 1px;
  transition: all 0.2s;
  position: relative;
  z-index: 10;
}

.info-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.info-icon {
  font-size: 1.3rem;
  filter: drop-shadow(0 0 2px rgba(0,0,0,0.5));
}

.info-text {
  color: #f8f8f2;
  text-shadow: 0 0 2px #000;
}

.mode-classic,
.mode-single {
  color: #88c0ff;
}
.mode-infinite,
.mode-double {
  color: #ffb86c;
}

.theme-badge {
  background: rgba(0,0,0,0.5);
  border-radius: 20px;
  padding: 2px 10px;
  margin-left: 4px;
  font-size: 0.9rem;
  color: #ffdd99;
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
  position: relative;
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

.button-bar {
  position: absolute;
  bottom: 20px;
  left: 0;
  right: 0;
  display: flex;
  justify-content: center;
  gap: 15px;
  z-index: 20;
  pointer-events: auto;
}

.game-btn {
  background: var(--btn-bg);
  backdrop-filter: blur(5px);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: white;
  font-family: monospace;
  font-size: clamp(10px, 2vw, 14px);
  padding: clamp(6px, 2vw, 12px) clamp(12px, 4vw, 28px);
  border-radius: 40px;
  cursor: pointer;
  transition: 0.2s;
  box-shadow: 0 2px 6px rgba(0,0,0,0.3);
}

.game-btn:hover {
  background: #ffaa66;
  color: #0a0f1a;
  border-color: #ffaa66;
}

.game-intro {
  background: linear-gradient(135deg, #1a2a3a, #0f1a24);
  color: #eef4ff;
  padding: 40px 20px;
  font-family: system-ui, 'Segoe UI', monospace;
  opacity: 0;
  transform: translateY(30px);
  transition: opacity 0.8s ease, transform 0.8s ease;
  border-top: 2px solid #ffaa66;
  width: 100%;
  box-sizing: border-box;
}

.game-intro.visible {
  opacity: 1;
  transform: translateY(0);
}

.intro-content {
  max-width: 900px;
  margin: 0 auto;
  line-height: 1.6;
}

.intro-content h2 {
  color: #ffaa66;
  margin-top: 0;
  font-size: 2rem;
}

.intro-content p {
  margin: 0;
  font-size: 1.2rem;
}

.intro-content h6 {
  margin-top: 10px;
  font-size: 0.9rem;
  color: #ffaa66;
}

.intro-content h6 a {
  color: #a1cbf3;
  text-decoration: none;
}

.keyboard-hint {
  position: fixed;
  top: 20px;
  left: 20px;
  backdrop-filter: blur(10px);
  border-radius: 4px;
  font-family: monospace;
  color: #ccddee;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
  transition: all 0.3s ease;
  width: 48px;
  height: 48px;
  overflow: hidden;
  cursor: pointer;
  z-index: 100;
  border-left: 4px solid #5a9eff;
  display: flex;
  align-items: center;
  justify-content: center;
}

.keyboard-hint:hover {
  width: 320px;
  height: auto;
  border-radius: 12px;
  border-left: 12px solid #5a9eff;
  padding: 12px 20px;
  display: block;
}

.keyboard-hint h3 {
  color: #5a9eff;
}

.keyboard-hint h3,
.keyboard-hint ul {
  display: none;
}

.keyboard-hint:hover h3,
.keyboard-hint:hover ul {
  display: block;
}

.keyboard-hint::before {
  content: "⌨️";
  font-size: 28px;
  line-height: 1;
}

.keyboard-hint:hover::before {
  display: none;
}

.keyboard-hint ul {
  padding-left: 20px;
  margin: 8px 0;
}

.history-panel ul {
  padding-left: 20px;
  margin: 8px 0;
}

.history-panel {
  position: fixed;
  top: 100px;
  left: 20px;
  backdrop-filter: blur(10px);
  border-radius: 4px;
  font-family: monospace;
  color: #ccddee;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
  transition: all 0.3s ease;
  width: 48px;
  height: 48px;
  overflow: hidden;
  cursor: pointer;
  z-index: 100;
  border-left: 4px solid #ffaa66;
  display: flex;
  align-items: center;
  justify-content: center;
}

.history-panel:hover {
  width: 260px;
  height: auto;
  border-radius: 12px;
  border-left: 12px solid #ffaa66;
  padding: 12px 20px;
  display: block;
}

.history-panel h3 {
  color: #ffaa66;
}

.history-panel h3,
.history-panel ul,
.history-panel .clear-btn {
  display: none;
}

.history-panel:hover h3,
.history-panel:hover ul,
.history-panel:hover .clear-btn {
  display: block;
}

.history-panel:hover ul {
  display: block;
  margin: 8px 0;
}

.history-panel::before {
  content: "🏆";
  font-size: 28px;
  line-height: 1;
}

.history-panel:hover::before {
  display: none;
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
  padding: 6px 14px;
  cursor: pointer;
  font-size: 12px;
}

.clear-btn:hover {
  background: #ffaa66;
  color: #0a0f1a;
}

.history-panel:hover,
.keyboard-hint:hover {
  z-index: 10000;
}

.keyboard-hint h3,
.history-panel h3,
.keyboard-hint li,
.history-panel li {
  font-size: 14px;
  flex-shrink: 0;
  white-space: nowrap;
}

kbd {
  background: #2a3a4a;
  border-radius: 6px;
  padding: 2px 8px;
  font-family: monospace;
  font-weight: bold;
  color: #ffaa66;
  margin: 0 2px;
  font-size: 0.9em;
  box-shadow: inset 0 -1px 0 rgba(0,0,0,0.3), 0 1px 0 rgba(255,255,255,0.1);
  display: inline-block;
}

.control-bar,
.history-panel,
.keyboard-hint,
.focus-overlay {
  background: var(--panel-bg);
}

.control-bar:hover,
.history-panel:hover,
.keyboard-hint:hover {
  background: var(--panel-bg-hover);
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

.game-info,
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