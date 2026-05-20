<template>
  <div class="screen-container">
    <!-- Player 1 score -->
     <div class="score score-left">{{ leftScore }}</div>
    <!-- The canvas frame -->
    <div class="frame" ref="frameRef">
      <!-- The canvas element -->
      <canvas
        ref="canvasRef"
        id="canvas"
        tabindex="0"
        @click="canvasRef?.focus()"
      ></canvas>
    </div>
    <!-- Player 2 score -->
     <div class="score score-right">{{ rightScore }}</div>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { loadWasm } from '../composables/useWasm.js'

const frameRef = ref(null)
const canvasRef = ref(null)
const leftScore = ref(0)
const rightScore = ref(0)

let pongModule = null

function handleVisibilityChange() { // pause game when tab is not active
  if (document.hidden && pongModule) {
    pongModule._pause_game()
  }
}

function updateFrameStyles() { // Set frame styles based on window size
  if (!frameRef.value) return
  const min_size = Math.min(window.innerHeight, window.innerWidth)

  const borderSize = Math.max(2, Math.min(6, min_size * 0.015))
  const cornerRadius = Math.max(8, Math.min(20, min_size * 0.03))
  
  frameRef.value.style.setProperty('--border-size', borderSize + 'px')
  frameRef.value.style.setProperty('--corner-radius', cornerRadius + 'px')
}

onMounted(async () => {
  if (!canvasRef.value) {
    console.error('Canvas ref is not available')
    return
  }
  
  window.updateScore = (left, right) => {
    leftScore.value = left
    rightScore.value = right
  }

  window.addEventListener('resize', updateFrameStyles)
  document.addEventListener('visibilitychange', handleVisibilityChange)

  pongModule = await loadWasm(canvasRef.value)
})

onBeforeUnmount(() => {
  delete window.updateScore
  window.removeEventListener('resize', updateFrameStyles)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})

</script>

<style scoped>
.screen-container {
  width: 100%;
  height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  overflow: hidden;
  gap: 20px;
  padding:0 10px;
  box-sizing: border-box;
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

  display: inline-block; /* Adapts to the size of canvas */
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
</style>