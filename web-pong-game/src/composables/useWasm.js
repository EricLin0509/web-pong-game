export function loadWasm(canvas) {
  return new Promise((resolve, reject) => {
    window.Module = {
      canvas: canvas,
      onRuntimeInitialized: () => {
        const module = window.Module;
        module._start_game = module.cwrap('start_game', null, []);
        module._pause_game = module.cwrap('pause_game', null, []);
        module._resume_game = module.cwrap('resume_game', null, []);
        module._restart_game = module.cwrap('restart_game', null, []);
        module._toggle_mode = module.cwrap('toggle_mode', null, []);
        module._goto_menu = module.cwrap('goto_menu', null, []);

        console.log('☑️ Pong Module is set!');
        resolve(window.Module);
      },
      setCanvasSize: (w, h) => {
        canvas.width = w;
        canvas.height = h;
      },
      locateFile: (path) => {
        if (path.endsWith('.wasm')) return '/wasm/pong.wasm';
        if (path.endsWith('.data')) return '/wasm/pong.data';
        return '/wasm/' + path; // Load all other files from the same directory
      },
      print: (text) => console.log('[C]', text),
      printErr: (text) => console.error('[C]', text),
    };

    const script = document.createElement('script');
    script.src = '/wasm/pong.js'; // Load the script
    script.onerror = () => reject(new Error('❌ Failed to load pong.js'));
    document.body.appendChild(script);
  });
}