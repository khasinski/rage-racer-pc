import {bindingName} from './key-binding.mjs';
import {mountLogo} from './logo.js';

const $ = s => document.querySelector(s);
const api = window.launcher;
const schema = await (await fetch('./settings.json')).json();
const esc = value => String(value ?? '').replace(/[&<>"']/g, c => ({
  '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
}[c]));

// Menu swatches from g_PaintColorTable in src/port/host_state_menu.c
const paintColors = [
  '#989878', '#002040', '#a08010', '#909090', '#985020', '#303840',
  '#107028', '#803020', '#405080', '#882020', '#243058', '#a03838',
  '#181818', '#884878', '#283020', '#988848', '#608090', '#586848'
];

let state = null;
let page = new URLSearchParams(window.location.search).get('page') === 'saves' ? 'saves' : 'settings'; // 'settings' | 'saves'
let draft = {};

let save = null;
let saveList = [];
let selectedSaveId = '';
let edits = {};
let saveDirty = false;
let modalDirty = false;
let garageMode = 'Grand Prix'; // 'Grand Prix' | 'Extra Grand Prix' | 'Time Attack'
let newSaveRegion = 'PAL';
let toastTimer;

async function request(name, ...args) {
  const response = await api[name](...args);
  if (!response.ok) throw Error(response.error);
  return response.value;
}

function toast(text) {
  const el = $('#toast');
  if (!el) return;
  el.textContent = text;
  el.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.hidden = true; }, 3200);
}

function showError(e) {
  const dialog = $('#dialog');
  if (!dialog) return;
  dialog.innerHTML = `
    <h2>Operation failed</h2>
    <p>${esc(e.message || e)}</p>
    <footer><button type="button" data-action="close">Close</button></footer>`;
  if (!dialog.open) dialog.showModal();
}

function getFieldObj(key) {
  return save?.fields?.find(f => f.key === key) || null;
}

function getSaveVal(key) {
  if (Object.hasOwn(edits, key)) return edits[key];
  const f = getFieldObj(key);
  return f ? f.value : '';
}

function getCarNamesList() {
  if (!save) return [];
  const reg = (save.region === 'Unknown' ? state?.disc?.region : save.region) === 'NTSC-J'
    ? 'japanese'
    : 'international';
  return save.carNames?.[reg] || [];
}

function renderTeamLogoSvg() {
  if (!save?.logo) return '';
  const pixels = String(edits['logo.pixels'] ?? save.logo.pixels ?? '');
  const colors = save.logo.colors.map((c, i) => {
    const v = edits['logo.color.' + i];
    return v ? {rgb: '#' + v.slice(0, 6), transparent: v[6] === '1'} : {...c};
  });
  let rects = '';
  for (let y = 0; y < 16; y++) {
    for (let x = 0; x < 16; x++) {
      const idx = parseInt(pixels[y * 16 + x] || '0', 16);
      const col = colors[idx];
      if (!col || col.transparent) continue;
      rects += `<rect x="${x}" y="${y}" width="1" height="1" fill="${esc(col.rgb)}"/>`;
    }
  }
  return `<svg viewBox="0 0 16 16" width="44" height="44" style="background:#07090d;border:1px solid var(--studio-border);border-radius:6px;image-rendering:pixelated;flex-shrink:0">${rects}</svg>`;
}

function renderRowSetting(key, customLabel) {
  const f = schema.find(x => x.key === key);
  if (!f) return '';
  const id = 'setting-' + f.key;
  const value = draft[f.key];
  let control = '';

  if (f.type === 'bool') {
    control = `<input id="${id}" data-setting="${esc(f.key)}" type="checkbox" ${value ? 'checked' : ''} style="width:17px;height:17px;accent-color:var(--accent);cursor:pointer">`;
  } else if (f.type === 'select') {
    control = `
      <select id="${id}" data-setting="${esc(f.key)}" style="width:175px;padding:6px 8px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-size:12px">
        ${f.options.map(([v, n]) => `<option value="${esc(v)}" ${String(value) === String(v) ? 'selected' : ''}>${esc(n)}</option>`).join('')}
      </select>`;
  } else {
    control = `
      <input id="${id}" data-setting="${esc(f.key)}" type="number" min="${f.min}" max="${f.max}" step="${f.step || 'any'}" value="${esc(value)}"
        style="width:90px;padding:6px 8px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono);font-size:12px;text-align:right">`;
  }

  return `
    <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;padding:8px 0;border-bottom:1px solid rgba(255,255,255,0.04)">
      <label for="${id}" style="font-size:13px;color:var(--text);cursor:pointer">${esc(customLabel || f.label)}</label>
      ${control}
    </div>`;
}

function renderSettingsView() {
  const keyGroups = [
    ['Steering / D-Pad', [
      ['input.up', 'Up'],
      ['input.down', 'Down'],
      ['input.left', 'Left'],
      ['input.right', 'Right']
    ]],
    ['Race Actions', [
      ['input.square', 'Accelerate (□)'],
      ['input.cross', 'Brake (✕)'],
      ['input.circle', ' Circle (○)'],
      ['input.triangle', 'View / Triangle (△)']
    ]],
    ['Gears & System', [
      ['input.l1', 'Shift Down (L1)'],
      ['input.r1', 'Shift Up (R1)'],
      ['input.l2', 'L2'],
      ['input.r2', 'R2'],
      ['input.start', 'Start / Pause'],
      ['input.select', 'Select']
    ]]
  ];

  return `
    <div style="max-width:1040px;margin:0 auto;padding:24px;display:flex;flex-direction:column;gap:18px">
      <!-- Disc Source Bar -->
      <div style="display:flex;align-items:center;justify-content:space-between;gap:16px;padding:12px 16px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
        <div style="display:flex;align-items:center;gap:12px;min-width:0">
          <span class="studio-dot ${state?.ready ? '' : 'warn'}"></span>
          <div style="min-width:0">
            <div style="font-weight:700;font-size:13px">
              ${state?.disc ? `Disc Image (${esc(state.disc.region)})` : 'No Rage Racer Disc Loaded'}
            </div>
            <div class="muted" style="font-size:11px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">
              ${state?.disc ? esc(state.disc.path) : 'Choose your legally obtained CUE, CHD, or Track 01 BIN image.'}
            </div>
          </div>
        </div>
        <div style="display:flex;gap:8px;flex-shrink:0">
          <button type="button" class="studio-btn" data-action="choose-game" ${state?.busy || state?.running ? 'disabled' : ''}>
            📀 ${state?.disc ? 'Change Disc…' : 'Choose Disc…'}
          </button>
          <button type="button" class="studio-btn" data-action="defaults">↺ Reset Defaults</button>
        </div>
      </div>

      <!-- Two-Column Clean Settings Grid -->
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:18px;align-items:start">
        <!-- Left Column: Graphics, HUD & Camera -->
        <div style="display:flex;flex-direction:column;gap:18px">
          <section style="padding:18px 20px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
            <h3 style="margin:0 0 10px;font-size:14px;text-transform:uppercase;letter-spacing:0.05em;color:var(--muted)">Display &amp; Graphics</h3>
            ${renderRowSetting('video.aspect', 'Aspect Ratio')}
            ${renderRowSetting('video.internal_scale', 'Resolution Scale (0.5× – 16×)')}
            ${renderRowSetting('video.fps', 'Frame Rate')}
            ${renderRowSetting('video.post', 'Anti-Aliasing')}
            ${renderRowSetting('video.texture_filter', 'Texture Filtering')}
            ${renderRowSetting('video.grading', 'Color Grading')}
            ${renderRowSetting('video.ssao', 'Ambient Occlusion')}
            ${renderRowSetting('video.car_lights', 'Car Lights')}
            ${renderRowSetting('video.shadows', 'Shadows')}
            ${renderRowSetting('modern.authored_cars', '3D Car Models')}
          </section>

          <section style="padding:18px 20px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
            <h3 style="margin:0 0 10px;font-size:14px;text-transform:uppercase;letter-spacing:0.05em;color:var(--muted)">HUD &amp; Camera</h3>
            ${renderRowSetting('hud.anchor', 'Widescreen HUD Position')}
            ${renderRowSetting('hud.show_lap_times', 'Show Lap Times')}
            ${renderRowSetting('hud.show_time_limit', 'Show Time Limit')}
            ${renderRowSetting('camera.chase_height', 'Chase Camera Height')}
            ${renderRowSetting('camera.chase_distance', 'Chase Camera Distance')}
            ${renderRowSetting('camera.chase_pitch', 'Chase Camera Pitch (°)')}
            ${renderRowSetting('game.tick_rate', 'Race Physics Rate (experimental)')}
          </section>
        </div>

        <!-- Right Column: Controller & Keyboard -->
        <section style="padding:18px 20px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
          <h3 style="margin:0 0 10px;font-size:14px;text-transform:uppercase;letter-spacing:0.05em;color:var(--muted)">Controller &amp; Keyboard</h3>
          ${renderRowSetting('input.analog', 'Analog Gamepad Sticks & Triggers')}
          ${renderRowSetting('input.wheel', 'Steering Wheel Support')}
          ${renderRowSetting('input.steering_linearity', 'Steering Linearity (-2 to 2)')}

          ${keyGroups.map(([groupTitle, items]) => `
            <div style="margin-top:14px">
              <div class="eyebrow" style="margin-bottom:6px">${esc(groupTitle)}</div>
              <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px">
                ${items.map(([key, label]) => `
                  <div style="display:flex;align-items:center;justify-content:space-between;padding:6px 10px;background:rgba(255,255,255,0.02);border:1px solid var(--studio-border);border-radius:6px">
                    <span style="font-size:12px">${esc(label)}</span>
                    <button type="button" class="studio-btn" data-bind="${esc(key)}" style="min-width:72px;justify-content:center;font-family:var(--mono);font-weight:700;padding:3px 8px">
                      ${esc(draft[key] ?? '')}
                    </button>
                  </div>
                `).join('')}
              </div>
            </div>
          `).join('')}
        </section>
      </div>
    </div>`;
}

function renderColorSelect(key) {
  const current = Number(getSaveVal(key));
  const hex = paintColors[current] || '#333';
  return `
    <div style="display:inline-flex;align-items:center;gap:6px">
      <span style="width:14px;height:14px;border-radius:3px;background:${hex};border:1px solid rgba(255,255,255,0.35);display:inline-block"></span>
      <select data-field="${esc(key)}" style="padding:4px 6px;border-radius:5px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-size:12px;font-family:var(--mono)">
        ${paintColors.map((_, i) => `<option value="${i}" ${current === i ? 'selected' : ''}>Color #${i + 1}</option>`).join('')}
      </select>
    </div>`;
}

function renderSavesView() {
  const cars = getCarNamesList();
  const extraUnlocked = Number(getSaveVal('extraGrandPrixUnlocked')) === 1;

  return `
    <div style="max-width:1040px;margin:0 auto;padding:24px;display:flex;flex-direction:column;gap:18px">
      <!-- Top Save File Selector Bar -->
      <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;padding:12px 16px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
        <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
          <span style="font-weight:700;font-size:12px;color:var(--muted);text-transform:uppercase">Save File:</span>
          ${saveList.length ? `
            <select id="save-file-select" style="min-width:220px;padding:6px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-weight:600;font-size:12px">
              ${saveList.map(s => `<option value="${esc(s.id)}" ${String(selectedSaveId) === String(s.id) ? 'selected' : ''}>${esc(s.team || 'UNNAMED')} · ${Number(s.money || 0).toLocaleString('en-US')} GP</option>`).join('')}
            </select>
          ` : ''}
          <button type="button" class="studio-btn" data-action="open-save">📂 Open File / Memory Card…</button>
          <div style="display:inline-flex;align-items:center;gap:4px">
            <select id="new-save-region" style="padding:6px 8px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-size:12px">
              ${['PAL', 'NTSC-U', 'NTSC-J'].map(r => `<option value="${r}" ${newSaveRegion === r ? 'selected' : ''}>${r}</option>`).join('')}
            </select>
            <button type="button" class="studio-btn" data-action="new-save">➕ New Save</button>
          </div>
        </div>

        ${save && !save.card ? `
          <button type="button" class="studio-btn primary" data-action="save-copy" ${state?.running ? 'disabled' : ''}>
            💾 Save Changes${saveDirty ? ' *' : ''}
          </button>
        ` : ''}
      </div>

      ${save?.card ? `
        <section style="padding:18px 20px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
          <h3 style="margin:0 0 10px;font-size:14px">Select a Memory Card Slot (${esc(save.name)})</h3>
          <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:8px">
            ${save.entries.map(e => `
              <button type="button" class="studio-btn" data-card="${e.index}" style="justify-content:space-between;padding:10px 12px">
                <b>Slot #${e.index + 1} · ${esc(e.team)}</b>
                <span class="muted">${esc(e.name)}</span>
              </button>
            `).join('') || '<p class="muted">No Rage Racer saves found on this card.</p>'}
          </div>
        </section>
      ` : !save ? `
        <div style="padding:40px;text-align:center;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
          <p class="muted" style="margin:0 0 12px">No save loaded. Click <b>Open File / Memory Card…</b> or <b>New Save</b> above.</p>
        </div>
      ` : `
        <!-- 1. Team, Credits & Career Progression (Single Clean Card) -->
        <section style="padding:20px 22px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px;display:flex;flex-direction:column;gap:16px">
          <div style="display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap;padding-bottom:14px;border-bottom:1px solid rgba(255,255,255,0.06)">
            <div style="display:flex;align-items:center;gap:14px">
              ${renderTeamLogoSvg()}
              <div>
                <label for="team-name-input" class="eyebrow" style="display:block;margin-bottom:4px">Team Name &amp; Emblem</label>
                <div style="display:flex;align-items:center;gap:8px">
                  <input id="team-name-input" data-team maxlength="7" value="${esc(edits.team ?? save.team ?? '')}"
                    style="width:120px;padding:6px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono);font-weight:700;font-size:14px">
                  <button type="button" class="studio-btn" data-action="edit-logo" title="Edit 16×16 Team Emblem">🎨 Edit Emblem…</button>
                </div>
              </div>
            </div>

            <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap">
              <button type="button" class="studio-btn" data-cheat="max-money">💰 Max Credits</button>
              <button type="button" class="studio-btn" data-cheat="unlock-all">🏆 Unlock All Classes &amp; Modes</button>
            </div>
          </div>

          <div style="display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:14px">
            <label style="display:flex;flex-direction:column;gap:6px;font-size:12px">
              <span class="muted">Grand Prix Credits (GP)</span>
              <input data-field="Grand Prix / Credits" type="number" min="0" max="999999999" value="${esc(getSaveVal('Grand Prix / Credits'))}"
                style="width:100%;padding:7px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono);font-weight:700">
            </label>
            <label style="display:flex;flex-direction:column;gap:6px;font-size:12px">
              <span class="muted">Grand Prix Class (1–5)</span>
              <input data-field="Grand Prix / Class" type="number" min="0" max="5" value="${esc(getSaveVal('Grand Prix / Class'))}"
                style="width:100%;padding:7px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono)">
            </label>
            <label style="display:flex;flex-direction:column;gap:6px;font-size:12px">
              <span class="muted">Extra GP Credits</span>
              <input data-field="Extra Grand Prix / Credits" type="number" min="0" max="999999999" value="${esc(getSaveVal('Extra Grand Prix / Credits'))}"
                style="width:100%;padding:7px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono);font-weight:700">
            </label>
            <label style="display:flex;flex-direction:column;gap:6px;font-size:12px">
              <span class="muted">Extra GP Mirror Mode</span>
              <select data-field="extraGrandPrixUnlocked" style="width:100%;padding:7px 10px;border-radius:6px;background:#090b10;color:var(--text);border:1px solid var(--studio-border)">
                <option value="0" ${!extraUnlocked ? 'selected' : ''}>Locked</option>
                <option value="1" ${extraUnlocked ? 'selected' : ''}>Unlocked</option>
              </select>
            </label>
          </div>
        </section>

        <!-- 2. Garage Table (All 13 Cars in One Clean Table) -->
        <section style="padding:20px 22px;background:var(--studio-panel);border:1px solid var(--studio-border);border-radius:10px">
          <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:14px">
            <div style="display:flex;align-items:center;gap:14px;flex-wrap:wrap">
              <h3 style="margin:0;font-size:14px;text-transform:uppercase;letter-spacing:0.05em;color:var(--muted)">Garage Cars</h3>
              <div class="studio-seg">
                ${['Grand Prix', 'Extra Grand Prix', 'Time Attack'].map(m => `
                  <button type="button" class="${garageMode === m ? 'active' : ''}" data-garage-mode="${esc(m)}">${esc(m)}</button>
                `).join('')}
              </div>
            </div>
            <button type="button" class="studio-btn" data-cheat="own-all">🚗 Mark All 13 Cars Owned</button>
          </div>

          <table class="asset-table" style="width:100%;font-size:13px">
            <thead>
              <tr>
                <th style="width:50px">#</th>
                <th>Car</th>
                <th>Owned</th>
                <th>Grade</th>
                <th>Tires</th>
                <th>Gearbox</th>
                <th>Body Paint</th>
                <th>Stripe Paint</th>
              </tr>
            </thead>
            <tbody>
              ${Array.from({length: 13}, (_, c) => {
                const pad = String(c).padStart(2, '0');
                const prefix = `${garageMode} / Car ${pad}`;
                if (!getFieldObj(`${prefix} / Owned`)) return '';
                const owned = Number(getSaveVal(`${prefix} / Owned`)) === 1;
                const trans = Number(getSaveVal(`${prefix} / Transmission`));
                return `
                  <tr>
                    <td class="muted" style="font-family:var(--mono)">#${pad}</td>
                    <td><strong>${esc(cars[c] || `Car ${pad}`)}</strong></td>
                    <td>
                      <input type="checkbox" data-owned-field="${esc(prefix + ' / Owned')}" ${owned ? 'checked' : ''}
                        style="width:16px;height:16px;accent-color:var(--accent);cursor:pointer">
                    </td>
                    <td>
                      <input data-field="${esc(prefix + ' / Grade')}" type="number" min="0" max="5" value="${esc(getSaveVal(`${prefix} / Grade`))}"
                        style="width:62px;padding:5px 8px;border-radius:5px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono)">
                    </td>
                    <td>
                      <input data-field="${esc(prefix + ' / Tires')}" type="number" min="0" max="5" value="${esc(getSaveVal(`${prefix} / Tires`))}"
                        style="width:62px;padding:5px 8px;border-radius:5px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-family:var(--mono)">
                    </td>
                    <td>
                      <select data-field="${esc(prefix + ' / Transmission')}" style="padding:5px 8px;border-radius:5px;background:#090b10;color:var(--text);border:1px solid var(--studio-border);font-size:12px">
                        <option value="0" ${trans === 0 ? 'selected' : ''}>AT</option>
                        <option value="1" ${trans === 1 ? 'selected' : ''}>MT</option>
                      </select>
                    </td>
                    <td>${renderColorSelect(`${prefix} / Paint 1`)}</td>
                    <td>${renderColorSelect(`${prefix} / Paint 2`)}</td>
                  </tr>`;
              }).join('')}
            </tbody>
          </table>
        </section>
      `}
    </div>`;
}

function render() {
  const root = $('#studio-app');
  if (!root || !state) return;

  root.innerHTML = `
    <header class="studio-menubar">
      <div class="studio-menubar-left">
        <div class="studio-seg">
          <button type="button" data-open-studio="car">🚗 Car Editor</button>
          <button type="button" data-open-studio="track">🗺️ Track Editor</button>
          <button type="button" class="${page === 'saves' ? 'active' : ''}" data-page="saves">💾 Save Editor</button>
          <button type="button" class="${page === 'settings' ? 'active' : ''}" data-page="settings">⚙️ Game Settings</button>
        </div>
      </div>

      <div class="studio-menubar-right">
        <button type="button" class="studio-btn primary" data-action="play" ${!state.ready || state.running || state.busy ? 'disabled' : ''}>
          ${state.running ? 'Game Running…' : '▷ Play Game'}
        </button>
      </div>
    </header>

    <div style="flex:1;min-height:0;overflow-y:auto">
      ${page === 'saves' ? renderSavesView() : renderSettingsView()}
    </div>

    <footer class="studio-statusbar">
      <span>${state.ready ? `Ready · Disc (${esc(state.disc?.region || '')})` : 'No disc loaded'}</span>
      <span>${page === 'saves' ? (saveDirty ? 'Unsaved save file edits' : 'Save Editor') : 'Settings auto-saved'}</span>
    </footer>`;
}

document.addEventListener('click', e => {
  const b = e.target.closest('button');
  if (!b) return;
  e.preventDefault();

  (async () => {
    try {
      if (b.dataset.action === 'close') { $('#dialog').close(); return; }
      if (b.dataset.openStudio) { await request('openStudio', b.dataset.openStudio); return; }
      if (b.dataset.page) {
        page = b.dataset.page;
        if (page === 'saves') {
          if (!saveList.length) saveList = await request('saves');
          if (!save && saveList.length) {
            selectedSaveId = String(saveList[0].id);
            save = await request('openSave', saveList[0].id);
            edits = {};
            saveDirty = false;
          }
        }
        render();
        return;
      }
      if (b.dataset.action === 'defaults') {
        draft = Object.fromEntries(schema.map(f => [f.key, f.default]));
        state = await request('settings', draft);
        render();
        toast('Restored default game settings');
        return;
      }
      if (b.dataset.bind) {
        const key = b.dataset.bind;
        const dialog = $('#dialog');
        dialog.innerHTML = `
          <h2>Press a key for ${esc(key.replace('input.', '').toUpperCase())}</h2>
          <p class="muted" data-key-hint>Press any keyboard key, or Escape to cancel.</p>
          <footer><button type="button" data-action="close">Cancel</button></footer>`;
        const capture = async ev => {
          if (ev.key === 'Escape') return;
          ev.preventDefault();
          ev.stopPropagation();
          const name = bindingName(ev.code);
          if (!name) {
            dialog.querySelector('[data-key-hint]').textContent = 'Unsupported key — try another key.';
            return;
          }
          draft[key] = name;
          dialog.close();
          state = await request('settings', draft);
          render();
          toast(`Bound ${key.replace('input.', '').toUpperCase()} to ${name}`);
        };
        dialog.addEventListener('keydown', capture);
        dialog.addEventListener('close', () => dialog.removeEventListener('keydown', capture), {once: true});
        dialog.showModal();
        return;
      }
      if (b.dataset.action === 'choose-game') {
        await request('chooseGame');
        return;
      }
      if (b.dataset.action === 'play') {
        await request('play');
        return;
      }

      // Save Editor actions
      if (b.dataset.card !== undefined) {
        save = await request('cardEntry', Number(b.dataset.card));
        edits = {};
        saveDirty = false;
        render();
        return;
      }
      if (b.dataset.action === 'open-save') {
        const data = await request('openSave');
        if (data) {
          selectedSaveId = '';
          save = data;
          edits = {};
          saveDirty = false;
          saveList = await request('saves');
          render();
        }
        return;
      }
      if (b.dataset.action === 'new-save') {
        const regionSel = $('#new-save-region');
        if (regionSel) newSaveRegion = regionSel.value;
        const data = await request('newSave', newSaveRegion);
        if (data) {
          save = data;
          edits = {};
          saveDirty = false;
          saveList = await request('saves');
          selectedSaveId = saveList.length ? String(saveList[saveList.length - 1].id) : '';
          render();
          toast(`Created new ${newSaveRegion} save`);
        }
        return;
      }
      if (b.dataset.action === 'save-copy') {
        if (!save) return;
        const data = await request('saveCopy', edits);
        if (data) {
          save = data;
          edits = {};
          saveDirty = false;
          saveList = await request('saves');
          render();
          toast('Save updated with valid PS1 checksums');
        }
        return;
      }
      if (b.dataset.garageMode) {
        garageMode = b.dataset.garageMode;
        render();
        return;
      }
      if (b.dataset.cheat) {
        if (!save) return;
        if (b.dataset.cheat === 'max-money') {
          edits['Grand Prix / Credits'] = 99999999;
          edits['Extra Grand Prix / Credits'] = 99999999;
          saveDirty = true;
          render();
          toast('Credits set to 99,999,999 GP');
        } else if (b.dataset.cheat === 'unlock-all') {
          edits['extraGrandPrixUnlocked'] = 1;
          edits['Grand Prix / Class'] = 5;
          edits['Grand Prix / Highest class'] = 5;
          edits['Extra Grand Prix / Class'] = 5;
          edits['Extra Grand Prix / Highest class'] = 5;
          for (const mode of ['Grand Prix', 'Extra Grand Prix', 'Time Attack']) {
            for (let c = 0; c < 13; c++) {
              const key = `${mode} / Car ${String(c).padStart(2, '0')} / Owned`;
              if (getFieldObj(key)) edits[key] = 1;
            }
          }
          saveDirty = true;
          render();
          toast('Unlocked Extra GP, Class 5 & all 13 cars');
        } else if (b.dataset.cheat === 'own-all') {
          for (let c = 0; c < 13; c++) {
            const key = `${garageMode} / Car ${String(c).padStart(2, '0')} / Owned`;
            if (getFieldObj(key)) edits[key] = 1;
          }
          saveDirty = true;
          render();
          toast(`All 13 cars unlocked in ${garageMode}`);
        }
        return;
      }
      if (b.dataset.action === 'edit-logo') {
        if (!save?.logo) return;
        const logo = {
          pixels: edits['logo.pixels'] ?? save.logo.pixels,
          colors: save.logo.colors.map((c, i) => {
            const v = edits['logo.color.' + i];
            return v ? {rgb: '#' + v.slice(0, 6), transparent: v[6] === '1'} : {...c};
          })
        };
        mountLogo(
          $('#dialog'),
          logo,
          result => {
            if (result.pixels !== logo.pixels) edits['logo.pixels'] = result.pixels;
            result.colors.forEach((c, i) => {
              if (c.rgb !== logo.colors[i].rgb || c.transparent !== logo.colors[i].transparent) {
                edits['logo.color.' + i] = c.rgb.slice(1) + (c.transparent ? '1' : '0');
              }
            });
            saveDirty = Object.keys(edits).length > 0;
            $('#dialog').close();
            render();
          },
          () => $('#dialog').close(),
          changed => { modalDirty = changed; }
        );
        $('#dialog').showModal();
        return;
      }
    } catch (err) {
      showError(err);
    }
  })();
});

document.addEventListener('change', e => {
  (async () => {
    try {
      if (e.target.dataset.setting) {
        const key = e.target.dataset.setting;
        draft[key] = e.target.type === 'checkbox' ? e.target.checked : e.target.value;
        state = await request('settings', draft);
        toast('Setting saved');
        return;
      }
      if (e.target.id === 'save-file-select') {
        selectedSaveId = String(e.target.value);
        save = await request('openSave', selectedSaveId);
        edits = {};
        saveDirty = false;
        render();
        return;
      }
      if (e.target.id === 'new-save-region') {
        newSaveRegion = e.target.value;
        return;
      }
      if (e.target.dataset.ownedField) {
        edits[e.target.dataset.ownedField] = e.target.checked ? 1 : 0;
        saveDirty = true;
        const saveBtn = document.querySelector('[data-action="save-copy"]');
        if (saveBtn) saveBtn.textContent = '💾 Save Changes *';
        const statusEl = document.querySelector('.studio-statusbar span:last-child');
        if (statusEl && page === 'saves') statusEl.textContent = 'Unsaved save file edits';
        return;
      }
      if (e.target.dataset.field) {
        edits[e.target.dataset.field] = e.target.value;
        saveDirty = true;
        const saveBtn = document.querySelector('[data-action="save-copy"]');
        if (saveBtn) saveBtn.textContent = '💾 Save Changes *';
        const statusEl = document.querySelector('.studio-statusbar span:last-child');
        if (statusEl && page === 'saves') statusEl.textContent = 'Unsaved save file edits';
      }
    } catch (err) {
      showError(err);
    }
  })();
});

document.addEventListener('input', e => {
  if (e.target.hasAttribute('data-team')) {
    edits.team = e.target.value;
    saveDirty = true;
    return;
  }
  if (e.target.dataset.field) {
    edits[e.target.dataset.field] = e.target.value;
    saveDirty = true;
  }
});

api.onState(value => {
  state = value;
  if (value.settings) draft = {...value.settings};
  render();
});

(async () => {
  try {
    state = await request('snapshot');
    draft = {...(state.settings || {})};
    saveList = await request('saves').catch(() => []);
    if (saveList.length) {
      selectedSaveId = String(saveList[0].id);
      save = await request('openSave', saveList[0].id).catch(() => null);
    }
    render();
  } catch (err) {
    showError(err);
  }
})();
