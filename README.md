# <img src="./web-pong-game/public/logo.ico" height="64"/> Web Pong Game

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Built with Emscripten](https://img.shields.io/badge/Built%20with-Emscripten-9cf)](https://emscripten.org)

A classic Pong game that runs both natively (SDL3) and in the browser (WebAssembly + Vue 3).  
Featuring two‑player gameplay, visual effects, local score history, and authentic arcade physics.

### 🌐 Website: https://ericlin0509.github.io/web-pong-game/

## ✨ Features

- Single‑player mode (AI opponent)
- Two‑player local play (`W`/`S` vs `I`/`K`)
- Classic mode (first to 11) & Infinite mode
- Adjustable snowflake particles
- Random color themes (press `C`)
- Hit‑angle based ball bounce (original Pong behaviour)
- Score history stored in browser (implemented using `localStorage`, **Classic mode only**)
- **Touch support (Partial)** – slide on left/right half to move paddles, double‑tap anywhere to (re)start the game
    - **Note:** double‑tap only supports native and android build
- **Idle energy saving** – reduces GPU usage when game is paused or in menus (Web & native)
- Responsive layout with on‑screen controls (**Vue app only**)

## 🔧 Dependencies

- **SDL3** & **SDL3_ttf** – the base game engine
- **Make** – build automation
- **CLANG** – for native build
- **Emscripten** (emsdk) – for WebAssembly build
- **Node.js** (≥26) & **npm** – for Vue frontend (web build)
- **Android SDK** & **NDK** – for Android build

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

Then open your browser at `http://localhost:5173/web-pong-game/` (The port may vary depending on your configuration).

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

#### Install it to system directories

If you want to install the game to your system directories, you can use the following commands:

```bash
sudo make install
```

#### Uninstall from system directories

```bash
sudo make uninstall
```

### Build for Android

This game also has **limited** support for Android. To build for Android, you need to install the Android SDK and NDK from [Android Studio](https://developer.android.com/studio)

#### 1. Initialize android support submodule

Due to the Android support being out-of-tree, you need to initialize the submodule first:

```bash
git submodule update --init --recursive
```

#### 2. Build the Android APK

For Debug Build:

```bash
cd android
./gradlew assembleDebug
```

For Release Build:

```bash
cd android
./gradlew assembleRelease
```

The APK will be placed in the `android/app/build/outputs/apk/debug/` or `android/app/build/outputs/apk/release/` directory respectively.

## 🎮 How to Play

There are two input methods available: **Keyboard** and **Touch**.

| Actions | Key(s) |
| :-------: | :------: |
| Left paddle up/down | W / S |
| Right paddle up/down | I / K |
| Start / Pause / Resume | Space |
| Switch game mode | M |
| Change theme | C |
| Add / Remove snow | B / V |
| Return to menu | Esc |

| Touch Controls | Description |
| :-------: | :------: |
| Drag on left half | Move left paddle up/down |
| Drag on right half | Move right paddle up/down |
| Double‑tap anywhere | (Re)start the game |

The game automatically pauses when the canvas loses focus. Click the canvas and press Space to resume **(Vue app only)**.