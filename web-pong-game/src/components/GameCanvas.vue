<template>
  <div class="canvas-container">
    <canvas ref="canvasRef" id="canvas" tabindex="0"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { loadWasm } from '../composables/useWasm.js'

const canvasRef = ref(null)
let pongModule = null

function handleVisibilityChange() { // pause game when tab is not active
  if (document.hidden && pongModule) {
    pongModule._pause_game()
  }
}

onMounted(async () => {
  if (!canvasRef.value) {
    console.error('Canvas ref is not available')
    return
  }
  canvasRef.value.focus()
  pongModule = await loadWasm(canvasRef.value)

  document.addEventListener('visibilitychange', handleVisibilityChange)
})

onBeforeUnmount(() => {
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})

</script>

<style scoped>
.canvas-container {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: black;
}
canvas {
  width: 100%;
  height: 100vh;
  image-rendering: crisp-edges;
}
</style>