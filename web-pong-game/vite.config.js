import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import { VitePWA } from 'vite-plugin-pwa'

// https://vite.dev/config/
export default defineConfig({
  base: '/web-pong-game/',
  plugins: [
    vue(),
    vueDevTools(),
    VitePWA({
      registerType: 'autoUpdate',
      scope: '/web-pong-game/',
      workbox: {
        maximumFileSizeToCacheInBytes: 4096 * 1024, // 4 MB
        globPatterns: ['**/*.{js,css,html,ico,png,svg,wasm}'],
        runtimeCaching: [
          {
            urlPattern: /^https:\/\/.*\.(wasm)$/, // Cache wasm files
            handler: 'StaleWhileRevalidate',
            options: {
              cacheName: 'wasm-cache',
              expiration: {
                maxAgeSeconds: 60 * 60 * 24 * 30, // Update every 30 days
              },
            },
          },
          {
            urlPattern: /^https:\/\/.*\.(js)$/,
            handler: 'StaleWhileRevalidate',
            options: {
              cacheName: 'js-cache',
            },
          },
        ],
      },
    }),
  ],
  assetsInclude: [
    '**/*.wasm',
    '**/*.data'
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
})
