#pragma once

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5Stick LED Controller</title>
<style>
  :root {
    --bg: #000;
    --fg: #fff;
    --border: #222;
    --accent: #fff;
    --muted: #999;
  }

  * {
    box-sizing: border-box;
    font-family: "Inter", sans-serif;
  }

  body {
    background: var(--bg);
    color: var(--fg);
    margin: 0;
    padding: 0;
  }

  main {
    max-width: 600px;
    margin: 2rem auto;
  }

  nav.tabs {
    display: flex;
    border-bottom: 1px solid var(--border);
  }

  .tabs button {
    flex: 1;
    padding: 0.75rem;
    background: none;
    color: var(--muted);
    border: none;
    border-bottom: 2px solid transparent;
    cursor: pointer;
    font-size: 1rem;
    border-radius: 0;
  }

  .tabs button.active {
    color: var(--fg);
    border-color: var(--accent);
  }

  section {
    margin-top: 2rem;
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 1rem;
  }

  input, select, button {
    width: 100%;
    background: #111;
    color: var(--fg);
    border: 1px solid var(--border);
    padding: 0.3rem 1rem;
    font-size: 1rem;
    margin-top: 0.5rem;
    border-radius: 6px;
  }

  label {
    font-size: 0.9rem;
    color: var(--muted);
    display: block;
    margin-top: 1rem;
  }

  button {
    cursor: pointer;
    background: var(--fg);
    color: var(--bg);
    border: none;
    font-weight: 500;
  }

  button.secondary {
    background: #111;
    color: var(--fg);
    border: 1px solid var(--border);
  }

  button.danger {
    background: #fff;
    color: #000;
    border: 1px solid var(--fg);
  }

  .grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 1rem;
  }

  .checkbox-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.5rem;
    margin-top: 1rem;
  }

  .checkbox-grid label {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    font-size: 0.9rem;
  }

  input[type="checkbox"] {
    accent-color: var(--fg);
    width: 16px;
    height: 16px;
  }

  #pixelEditor {
    display: grid;
    grid-template-columns: repeat(32, 1fr);
    border: 1px solid var(--border);
    height: 120px;
    user-select: none;
  }

  .pixel {
    border: 1px solid var(--border);
    cursor: pointer;
  }

  .pixel.on {
    background: var(--fg);
  }

  .editor-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: .5rem;
  }

  .editor-header h2 {
    font-size: 1rem;
    margin: 0;
  }

  .editor-header button {
    width: auto;
    padding: 0.4rem 1rem;
  }
  
  /* --- NEW --- Snake UI Styles */
  #snakeControls p {
    color: var(--muted);
    font-size: 0.9rem;
    text-align: center;
    margin: 0.5rem 0;
  }
  #snakeControls h2 {
    text-align: center;
    color: #28a745; /* Green */
    margin-top: 0;
  }
</style>
</head>
<body>
  <nav class="tabs">
    <button id="tab-text" onclick="setMode('text')">Text</button>
    <button id="tab-graphics" onclick="setMode('graphics')">Graphics</button>
    <button id="tab-snake" onclick="setMode('snake')">Snake</button> </nav>

  <main>
    <section id="textControls">
      <input id="msg" type="text" placeholder="Enter message" value="WELCOME TO SRC -" oninput="debouncedSendSettings()">
      <div class="grid">
        <div>
          <label>Speed</label>
          <input id="speed" type="number" min="25" max="250" value="70" oninput="sendAllSettings()">
        </div>
        <div>
          <label>Pause (ms)</label>
          <input id="pause" type="number" min="0" max="5000" step="100" value="0" oninput="sendAllSettings()">
        </div>
        <div>
          <label>Brightness</label>
          <input id="brightness" type="number" min="0" max="15" value="10" oninput="sendAllSettings()">
        </div>
        <div>
          <label>Char Spacing</label>
          <input id="spacing" type="number" min="1" max="5" value="1" oninput="sendAllSettings()">
        </div>
        <div>
          <label>Scroll Spacing</label>
          <input id="scrollSpacing" type="number" min="1" max="5" value="3" oninput="sendAllSettings()">
        </div>
        <div>
          <label>Effect</label>
          <select id="effect" onchange="sendAllSettings()">
            <option value="0" selected>Scroll Left</option>
            <option value="1">No Effect</option>
            <option value="2">Print (Static)</option>
            <option value="3">Scroll Up</option>
            <option value="4">Scroll Down</option>
            <option value="5">Scroll Right</option>
            <option value="6">Sprite</option>
            <option value="7">Slice</option>
            <option value="8">Mesh</option>
            <option value="9">Fade</option>
            <option value="10">Dissolve</option>
            <option value="11">Blinds</option>
            <option value="12">Random</option>
            <option value="13">Wipe</option>
            <option value="14">Wipe Cursor</option>
            <option value="15">Scan Horiz</option>
            <option value="16">Scan Horiz X</option>
            <option value="17">Scan Vert</option>
            <option value="18">Scan Vert X</option>
            <option value="19">Opening</option>
            <option value="20">Opening Cursor</option>
            <option value="21">Closing</option>
            <option value="22">Closing Cursor</option>
            <option value="23">Scroll Up Left</option>
            <option value="24">Scroll Up Right</option>
            <option value="25">Scroll Down Left</option>
            <option value="26">Scroll Down Right</option>
            <option value="27">Grow Up</option>
            <option value="28">Grow Down</option>
          </select>
        </div>
        <div>
          <label>Align</label>
          <select id="align" onchange="sendAllSettings()">
            <option value="0" selected>Left</option>
            <option value="1">Center</option>
            <option value="2">Right</option>
          </select>
        </div>
      </div>

      <div class="checkbox-grid">
        <label><input id="invert" type="checkbox" onchange="sendAllSettings()">Invert</label>
        <label><input id="displayOn" type="checkbox" checked onchange="sendAllSettings()">Display On</label>
      </div>

      <div class="grid" style="margin-top:1.5rem;">
        <button class="secondary" onclick="sendAllSettings()">Resend</button>
        <button class="secondary" onclick="clearDisplay()">Clear</button>
      </div>
    </section>

    <section id="graphicsControls" style="display:none;">
      <div class="editor-header">
        <h2>Pixel Editor</h2>
        <button class="secondary" onclick="clearEditor()">Clear</button>
      </div>
      <div id="pixelEditor"></div>
    </section>

    <section id="snakeControls" style="display:none;">
      <h2>SNAKE GAME</h2>
      <p>Connect your BLE controller to play.</p>
      <p>Controls: (Y/Up), (A/Down), (X/Left), (B/Right)</p>
      <p>Press any button to restart if game is over.</p>
    </section>

    <button class="danger" onclick="rebootDevice()">Reboot Device</button>
  </main>

<script>
let ws;
const PIXEL_WIDTH = 32, PIXEL_HEIGHT = 8;
let pixelData = Array(PIXEL_HEIGHT).fill(0).map(() => Array(PIXEL_WIDTH).fill(false));
let debounceTimer, isDrawing = false, drawMode = true;

window.onload = () => { initWebSocket(); initPixelEditor(); setMode('text'); };

function initWebSocket() {
  ws = new WebSocket('ws://' + location.hostname + '/ws');
  ws.onclose = () => setTimeout(initWebSocket, 2000);
}

function debouncedSendSettings() {
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(sendAllSettings, 400);
}

function sendAllSettings() {
  ws.send(JSON.stringify({
    type: 'text_update',
    msg: msg.value,
    speed: speed.value,
    brightness: brightness.value,
    effect: effect.value,
    align: align.value,
    invert: invert.checked,
    pause: pause.value,
    spacing: spacing.value,
    scrollSpacing: scrollSpacing.value,
    displayOn: displayOn.checked
  }));
}

function clearDisplay() { msg.value = ""; sendAllSettings(); }

// --- CHANGED --- Updated for 3 modes
function setMode(mode) {
  textControls.style.display = mode === 'text' ? 'block' : 'none';
  graphicsControls.style.display = mode === 'graphics' ? 'block' : 'none';
  snakeControls.style.display = mode === 'snake' ? 'block' : 'none'; // --- NEW ---
  
  document.getElementById('tab-text').classList.toggle('active', mode === 'text');
  document.getElementById('tab-graphics').classList.toggle('active', mode === 'graphics');
  document.getElementById('tab-snake').classList.toggle('active', mode === 'snake'); // --- NEW ---
  
  ws.send(JSON.stringify({ type: 'set_mode', mode }));
}

function initPixelEditor() {
  const editor = document.getElementById('pixelEditor');
  for (let y = 0; y < PIXEL_HEIGHT; y++) {
    for (let x = 0; x < PIXEL_WIDTH; x++) {
      const pixel = document.createElement('div');
      pixel.className = 'pixel';
      pixel.dataset.x = x; pixel.dataset.y = y;
      editor.appendChild(pixel);
    }
  }
  editor.addEventListener('mousedown', e => {
    isDrawing = true;
    if (!e.target.classList.contains('pixel')) return;
    const x = +e.target.dataset.x, y = +e.target.dataset.y;
    drawMode = !pixelData[y][x]; draw(e);
  });
  editor.addEventListener('mousemove', draw);
  ['mouseup', 'mouseleave'].forEach(ev => editor.addEventListener(ev, () => isDrawing = false));
}

function draw(e) {
  if (!isDrawing || !e.target.classList.contains('pixel')) return;
  const x = +e.target.dataset.x, y = +e.target.dataset.y;
  if (pixelData[y][x] !== drawMode) {
    pixelData[y][x] = drawMode;
    e.target.classList.toggle('on', drawMode);
    ws.send(JSON.stringify({ type: 'pixel_update', x, y, state: drawMode }));
  }
}

function clearEditor() {
  pixelData = Array(PIXEL_HEIGHT).fill(0).map(() => Array(PIXEL_WIDTH).fill(false));
  document.querySelectorAll('.pixel').forEach(p => p.classList.remove('on'));
  ws.send(JSON.stringify({ type: 'clear_graphics' }));
}

function rebootDevice() {
  if (confirm("Reboot device?")) ws.send(JSON.stringify({ type: 'reboot' }));
}
</script>
</body>
</html>
)rawliteral";