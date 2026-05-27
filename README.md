# Web Pong Game

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Built with Emscripten](https://img.shields.io/badge/Built%20with-Emscripten-9cf)](https://emscripten.org)

A classic Pong game that runs both natively (SDL3) and in the browser (WebAssembly + Vue 3).  
Featuring two‑player gameplay, visual effects, local score history, and authentic arcade physics.

## ✨ Features

- Two‑player local play (W/S vs I/K)
- Classic mode (first to 11) & Infinite mode
- Adjustable snowflake particles
- Random color themes (press `C`)
- Hit‑angle based ball bounce (original Pong behaviour)
- Score history stored in browser (implemented by `localStorage`, **Classic mode only**)
- Responsive layout with on‑screen controls (**Vue app only**)

## 🔧 Dependencies

- **Make** – build automation
- **CLANG** – for native build
- **Emscripten** (emsdk) – for WebAssembly build
- **Node.js** (≥18) and **npm** – for Vue frontend (web build)
- **SDL3** & **SDL3_ttf** – the base game engine

## 🏗️ Build Instructions

### Web Build (WebAssembly + Vue)

#### 1. Build the WebAssembly module

```bash
make
```

This compiles the C source to WebAssembly and copies the generated files into the Vue project's `public` directory.

#### 2. Run the Vue development server

```bash
make run_server
```

Then open your browser at http://localhost:5173 (The port may vary depending on your configuration).

### Local Build (Native SDL3 Application)

#### 1. Compile the native executable

```bash
make local
```

The executable will be placed in the `game/` directory.

#### 2. Run the game

```bash
make run_local
```

Then enjoy 🎉!

## 🎮 How to Play

| Actions | Key(s) |
| :-------: | :------: |
| Left paddle up/down | W / S |
| Right paddle up/down | I / K |
| Start / Pause / Resume | Space |
| Switch game mode | M |
| Change theme | C |
| Add / Remove snow | B / V |
| Return to menu | Esc |

The game automatically pauses when the canvas loses focus. Click the canvas and press Space to resume **(Vue app only)**.