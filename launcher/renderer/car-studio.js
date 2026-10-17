import {installMaterialEditor} from './material-editor.mjs';
import {carImportForm, carPartName} from './car-workshop.mjs';
import {surfaceDialog} from './tracks.mjs';
import {displayedShiftSpeed, storedShiftSpeed} from './car-speed.mjs';
import {mountModelPreview} from './model-preview.js?v=17';

const $ = s => document.querySelector(s);
const api = window.launcher;
const esc = v => String(v ?? '').replace(/[&<>"']/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));

const COURSE_NAMES = ['Mythical Coast', 'Over Pass City', 'Lakeside Gate', 'The Extreme Oval'];

let state = null;
let project = null;
let carParts = [];
let carSection = 'all'; // 'all' | 'player' | 'rival'
let rivalClassFilter = 0; // 0 = all classes, 1..6 = specific class
let rivalCourseFilter = -1; // -1 = all courses, 0..3 = specific course
let carSearch = '';
let carGradeFilter = 'all';
let selectedCar = '';
let inlineCarPart = 'whole';
let inspectorCollapsed = false;
let currentCarSpec = null;
let currentCarSpecBank = -1;
let currentCarSpecRev = -1;
let activePreviewKey = '';
let loadingPreviewKey = '';
let previewToken = 0;
let previewBusy = false;
let savingBusy = false;
let booting = true;
let previewPromise = null;
let toastTimer = null;
let formDirty = false;
let testTrackBank = 88; // Class 1 · Mythical Coast default
const openAccordions = new Set(['fitment', 'identity', 'engine', 'gearbox', 'handling']);

const DEFAULT_FITMENT = Object.freeze({
  frontWidth: 0,
  frontY: 0,
  frontZ: 0,
  frontScale: 100,
  rearWidth: 0,
  rearY: 0,
  rearZ: 0,
  rearScale: 100,
  bodyY: 0,
  bodyZ: 0,
  bodyScale: 100
});
let fitmentDraft = {...DEFAULT_FITMENT};
let lastWholeCarData = null;

function getProjectFitment(car) {
  if (!car) return {...DEFAULT_FITMENT};
  const bodyKey = `car.${car.rival ? 'rival' : 'player'}.${car.bank}.part.${car.part}`;
  const entry = project?.changes?.find(c => c.kind === 'car' && c.id === bodyKey && c.field === 'fitment');
  const arr = entry?.value;
  if (Array.isArray(arr) && arr.length === 11) {
    return {
      frontWidth: arr[0],
      frontY: arr[1],
      frontZ: arr[2],
      frontScale: arr[3],
      rearWidth: arr[4],
      rearY: arr[5],
      rearZ: arr[6],
      rearScale: arr[7],
      bodyY: arr[8],
      bodyZ: arr[9],
      bodyScale: arr[10]
    };
  }
  return {...DEFAULT_FITMENT};
}

function hasPendingFitment() {
  const {car} = resolveCatalogBodies();
  const saved = getProjectFitment(car);
  return Object.keys(DEFAULT_FITMENT).some(k => Number(fitmentDraft[k]) !== Number(saved[k]));
}

function syncFitmentFromProject() {
  const {car} = resolveCatalogBodies();
  fitmentDraft = getProjectFitment(car);
}

function resetFitmentDraft() {
  fitmentDraft = {...DEFAULT_FITMENT};
}

function computeFittedVertices(base, partRanges, targetFitment, bakedFitment = DEFAULT_FITMENT) {
  const next = Array.from(base);
  const fScale = Math.max(0.25, Math.min(3, Number(targetFitment.frontScale || 100) / 100)) / Math.max(0.25, Math.min(3, Number(bakedFitment.frontScale || 100) / 100));
  const rScale = Math.max(0.25, Math.min(3, Number(targetFitment.rearScale || 100) / 100)) / Math.max(0.25, Math.min(3, Number(bakedFitment.rearScale || 100) / 100));
  const bScale = Math.max(0.25, Math.min(3, Number(targetFitment.bodyScale || 100) / 100)) / Math.max(0.25, Math.min(3, Number(bakedFitment.bodyScale || 100) / 100));
  const fWidth = (Number(targetFitment.frontWidth) || 0) - (Number(bakedFitment.frontWidth) || 0);
  const fY = (Number(targetFitment.frontY) || 0) - (Number(bakedFitment.frontY) || 0);
  const fZ = (Number(targetFitment.frontZ) || 0) - (Number(bakedFitment.frontZ) || 0);
  const rWidth = (Number(targetFitment.rearWidth) || 0) - (Number(bakedFitment.rearWidth) || 0);
  const rY = (Number(targetFitment.rearY) || 0) - (Number(bakedFitment.rearY) || 0);
  const rZ = (Number(targetFitment.rearZ) || 0) - (Number(bakedFitment.rearZ) || 0);
  const bY = (Number(targetFitment.bodyY) || 0) - (Number(bakedFitment.bodyY) || 0);
  const bZ = (Number(targetFitment.bodyZ) || 0) - (Number(bakedFitment.bodyZ) || 0);

  const rangeCenter = (start, count, pred) => {
    let minX = Infinity, minY = Infinity, minZ = Infinity;
    let maxX = -Infinity, maxY = -Infinity, maxZ = -Infinity;
    let hit = 0;
    for (let i = 0; i < count; i++) {
      const at = (start + i) * 9;
      const x = base[at], y = base[at + 1], z = base[at + 2];
      if (pred && !pred(x, y, z)) continue;
      if (x < minX) minX = x;
      if (x > maxX) maxX = x;
      if (y < minY) minY = y;
      if (y > maxY) maxY = y;
      if (z < minZ) minZ = z;
      if (z > maxZ) maxZ = z;
      hit++;
    }
    return hit ? [(minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2] : [0, 0, 0];
  };

  for (const range of partRanges || []) {
    const {role, startVertex, vertexCount, scale} = range;
    if (role === 'body') {
      const [cx, cy, cz] = rangeCenter(startVertex, vertexCount);
      for (let i = 0; i < vertexCount; i++) {
        const at = (startVertex + i) * 9;
        next[at] = cx + (base[at] - cx) * bScale;
        next[at + 1] = cy + (base[at + 1] - cy) * bScale + bY;
        next[at + 2] = cz + (base[at + 2] - cz) * bScale + bZ;
      }
    } else if (role === 'front') {
      const [cx, cy, cz] = rangeCenter(startVertex, vertexCount);
      const xDir = (scale?.[0] ?? 1) < 0 ? -1 : 1;
      for (let i = 0; i < vertexCount; i++) {
        const at = (startVertex + i) * 9;
        next[at] = cx + (base[at] - cx) * fScale + xDir * fWidth;
        next[at + 1] = cy + (base[at + 1] - cy) * fScale + fY;
        next[at + 2] = cz + (base[at + 2] - cz) * fScale + fZ;
      }
    } else if (role === 'rear') {
      const [cxL, cyL, czL] = rangeCenter(startVertex, vertexCount, x => x < 0);
      const [cxR, cyR, czR] = rangeCenter(startVertex, vertexCount, x => x >= 0);
      for (let i = 0; i < vertexCount; i++) {
        const at = (startVertex + i) * 9;
        const x = base[at], y = base[at + 1], z = base[at + 2];
        if (x < 0) {
          next[at] = cxL + (x - cxL) * rScale - rWidth;
          next[at + 1] = cyL + (y - cyL) * rScale + rY;
          next[at + 2] = czL + (z - czL) * rScale + rZ;
        } else {
          next[at] = cxR + (x - cxR) * rScale + rWidth;
          next[at + 1] = cyR + (y - cyR) * rScale + rY;
          next[at + 2] = czR + (z - czR) * rScale + rZ;
        }
      }
    }
  }
  return next;
}

function applyFitmentToViewport() {
  const host = $('#car-studio-viewport');
  if (!host || typeof host.updateVertices !== 'function' || !lastWholeCarData?.baseVertices || !lastWholeCarData?.partRanges) {
    return;
  }
  const next = computeFittedVertices(
    lastWholeCarData.baseVertices,
    lastWholeCarData.partRanges,
    fitmentDraft,
    lastWholeCarData.bakedFitment || DEFAULT_FITMENT
  );
  host.updateVertices(next);
}

async function request(name, ...args) {
  const res = await api[name](...args);
  if (!res.ok) throw Error(res.error);
  return res.value;
}

function toast(text) {
  const el = $('#toast');
  if (!el) return;
  el.textContent = text;
  el.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.hidden = true; }, 5000);
}

function showError(e) {
  const dialog = $('#dialog');
  if (!dialog) return;
  dialog.innerHTML = `<h2>Could not complete the operation</h2><p>${esc(e.message || e)}</p><footer><button type="button" data-action="close">Close</button></footer>`;
  if (!dialog.open) dialog.showModal();
}

function closeMenus() {
  for (const m of document.querySelectorAll('details.studio-menu[open]')) m.open = false;
}

async function cancelPreview() {
  const job = previewPromise;
  if (!job) return;
  const token = ++previewToken;
  await api.cancel('preview').catch(() => {});
  await job.catch(() => {});
  if (previewPromise === job) previewPromise = null;
  if (token === previewToken) {previewBusy = false; loadingPreviewKey = '';}
}

function slotNumberForCar(c, rivalIndex = 0) {
  if (!c.rival) {
    return c.bank >= 200 ? 32 + Math.floor((c.bank - 200) / 2) : Math.floor((c.bank - 10) / 2);
  }
  return 32 + rivalIndex;
}

function carGradeBadge(c) {
  if (c.rival) {
    const cls = Math.floor((c.bank - 88) / 8) + 1;
    return `C${cls}`;
  }
  if (c.customSlot || c.bank >= 200) return 'NEW';
  const match = /\bG(\d)\b/.exec(c.name || '');
  if (match) return 'G' + match[1];
  const slot = Math.floor((c.bank - 10) / 2);
  return slot >= 30 ? 'SP' : 'G' + Math.min(5, Math.max(1, Math.floor(slot / 6) + 1));
}

function resolveCatalogBodies() {
  const playerBodies = carParts
    .filter(c => !c.rival && c.part === 0)
    .sort((a, b) => a.bank - b.bank)
    .map(c => ({...c, globalSlot: c.bank >= 200 ? 32 + Math.floor((c.bank - 200) / 2) : Math.floor((c.bank - 10) / 2)}));

  const customMaxSlot = playerBodies.reduce((m, c) => Math.max(m, c.globalSlot), 31);
  const rivalBodies = carParts
    .filter(c => c.rival && c.part % 5 === 0)
    .sort((a, b) => a.bank - b.bank || a.part - b.part)
    .map((c, idx) => {
      const cls = Math.floor((c.bank - 88) / 8) + 1;
      const courseIdx = ((c.bank - 88) / 2) % 4;
      return {
        ...c,
        globalSlot: customMaxSlot + 1 + idx,
        rivalClass: cls,
        rivalCourse: courseIdx,
        rivalCourseName: COURSE_NAMES[courseIdx] || 'Course'
      };
    });

  const allBodies = [...playerBodies, ...rivalBodies];
  const car = allBodies.find(c => c.key === selectedCar) || playerBodies[0] || allBodies[0] || null;
  return {playerBodies, rivalBodies, allBodies, car};
}

async function ensureCarSpec() {
  const {car} = resolveCatalogBodies();
  if (!car || car.rival) {
    currentCarSpec = null;
    currentCarSpecBank = -1;
    return;
  }
  const rev = project?.revision ?? 0;
  if (currentCarSpec && currentCarSpecBank === car.bank && currentCarSpecRev === rev) return;
  try {
    currentCarSpec = await request('carSpec', car.bank);
    currentCarSpecBank = car.bank;
    currentCarSpecRev = rev;
  } catch (_) {
    currentCarSpec = null;
  }
}

function collectPendingSpecEdits() {
  const form = $('#car-spec-form');
  if (!form) return null;
  const edits = Object.fromEntries(
    [...form.querySelectorAll('[data-spec]')]
      .filter(i => i.value !== (i.dataset.original ?? i.defaultValue))
      .map(i => [i.dataset.spec, i.dataset.text !== undefined ? i.value : i.dataset.speed ? storedShiftSpeed(Number(i.value)) : Number(i.value)])
  );
  return Object.keys(edits).length ? {bank: Number(form.dataset.bank), edits} : null;
}

async function saveAndApplyAll() {
  const form = $('#car-spec-form');
  if (form && !form.reportValidity()) return false;
  savingBusy = true;
  try {
    if (previewPromise) await cancelPreview();
    const {car} = resolveCatalogBodies();
    if (car && hasPendingFitment()) {
      project = await request('saveCarFitment', car.bank, car.part, Boolean(car.rival), fitmentDraft, project?.revision ?? 0);
      syncFitmentFromProject();
      activePreviewKey = '';
    }
    const pending = collectPendingSpecEdits();
    if (pending) {
      project = await request('saveCarSpec', pending.bank, pending.edits, project?.revision ?? 0);
    }
    project = await request('applyProject', project?.revision ?? 0);
    formDirty = false;
    carParts = (await request('cars')).parts;
    currentCarSpecBank = -1;
    await ensureCarSpec();
    syncFitmentFromProject();
  } finally {
    savingBusy = false;
  }
  render();
  return true;
}

function computeMasterFitmentValues() {
  return {
    trackWidth: Math.round(((Number(fitmentDraft.frontWidth) || 0) + (Number(fitmentDraft.rearWidth) || 0)) / 2),
    wheelbase: Math.round(((Number(fitmentDraft.frontZ) || 0) - (Number(fitmentDraft.rearZ) || 0)) / 2),
    wheelY: Math.round(((Number(fitmentDraft.frontY) || 0) + (Number(fitmentDraft.rearY) || 0)) / 2),
    wheelScale: Math.round(((Number(fitmentDraft.frontScale) || 100) + (Number(fitmentDraft.rearScale) || 100)) / 2)
  };
}

function syncFitmentInputsFromDraft(skipEl = null) {
  for (const key of Object.keys(DEFAULT_FITMENT)) {
    const val = String(fitmentDraft[key] ?? DEFAULT_FITMENT[key]);
    const rng = document.querySelector(`[data-fitment-range="${key}"]`);
    const num = document.querySelector(`[data-fitment-num="${key}"]`);
    if (rng && rng !== skipEl) rng.value = val;
    if (num && num !== skipEl) num.value = val;
  }
  const master = computeMasterFitmentValues();
  for (const [mKey, mVal] of Object.entries(master)) {
    const val = String(mVal);
    const rng = document.querySelector(`[data-fitment-master-range="${mKey}"]`);
    const num = document.querySelector(`[data-fitment-master-num="${mKey}"]`);
    if (rng && rng !== skipEl) rng.value = val;
    if (num && num !== skipEl) num.value = val;
  }
}

function renderFitmentAccordion() {
  const openAttr = openAccordions.has('fitment') ? ' open' : '';
  const pending = hasPendingFitment();
  const master = computeMasterFitmentValues();
  const masterRow = (mKey, label, min, max, step = 1, def = 0, unit = '') => {
    const val = master[mKey] ?? def;
    const numMin = unit === '%' ? 25 : min * 3;
    const numMax = unit === '%' ? 300 : max * 3;
    return `
      <div class="studio-fitment-row">
        <div class="studio-fitment-row-head">
          <label class="studio-scrub-label" for="fit-mnum-${mKey}" title="Drag horizontally on label to scrub value">${esc(label)}${unit ? ` <small class="muted">(${unit})</small>` : ''}</label>
          <div class="studio-fitment-row-controls">
            <input id="fit-mnum-${mKey}" type="number" min="${numMin}" max="${numMax}" step="1" value="${val}" data-fitment-master-num="${mKey}">
            <button type="button" class="field-reset" data-fitment-master-reset="${mKey}" data-default="${def}" title="Reset to ${def}">↺</button>
          </div>
        </div>
        <input id="fit-mrng-${mKey}" type="range" min="${min}" max="${max}" step="${step}" value="${val}" data-fitment-master-range="${mKey}">
      </div>`;
  };
  const row = (key, label, min, max, step = 1, unit = '') => {
    const val = fitmentDraft[key] ?? DEFAULT_FITMENT[key];
    const def = DEFAULT_FITMENT[key];
    const numMin = unit === '%' ? 25 : min * 3;
    const numMax = unit === '%' ? 300 : max * 3;
    return `
      <div class="studio-fitment-row">
        <div class="studio-fitment-row-head">
          <label class="studio-scrub-label" for="fit-num-${key}" title="Drag horizontally on label to scrub value">${esc(label)}${unit ? ` <small class="muted">(${unit})</small>` : ''}</label>
          <div class="studio-fitment-row-controls">
            <input id="fit-num-${key}" type="number" min="${numMin}" max="${numMax}" step="1" value="${val}" data-fitment-num="${key}">
            <button type="button" class="field-reset" data-fitment-reset="${key}" data-default="${def}" title="Reset to ${def}">↺</button>
          </div>
        </div>
        <input id="fit-rng-${key}" type="range" min="${min}" max="${max}" step="${step}" value="${val}" data-fitment-range="${key}">
      </div>`;
  };
  return `
    <details class="studio-accordion" data-accordion="fitment"${openAttr}>
      <summary>
        <span>🛞 Wheel &amp; Body Fitment</span>
        <span class="studio-accordion-badge">${pending ? 'Modified *' : '3D Pose'}</span>
      </summary>
      <div class="studio-accordion-body">
        <p class="muted" style="margin:0 0 6px;font-size:11.5px;line-height:1.4">
          Adjust wheel track width, wheelbase, ride height, and diameter with live 3D preview. Toggle <b>👻 Base Cage</b> (hotkey <b>O</b>) in the 3D viewport to compare against original disc wheel arches.
        </p>
        <div class="studio-fitment-group">
          <div class="studio-fitment-group-title">
            <span>All 4 Wheels (Linked)</span>
            <small class="muted">Front + Rear</small>
          </div>
          ${masterRow('trackWidth', '↔ Track Width (L/R Stance X)', -250, 250, 1, 0)}
          ${masterRow('wheelbase', '↕ Wheelbase (Front/Rear Axle Z)', -300, 300, 1, 0)}
          ${masterRow('wheelY', '⇕ Wheel Height (Vertical Y)', -200, 200, 1, 0)}
          ${masterRow('wheelScale', '🛞 Wheel Diameter (All 4)', 50, 200, 1, 100, '%')}
        </div>
        <div class="studio-fitment-group">
          <div class="studio-fitment-group-title">
            <span>Car Body Shell</span>
            <small class="muted">Part #1</small>
          </div>
          ${row('bodyScale', '🚗 Body Scale', 50, 160, 1, '%')}
          ${row('bodyY', '⇕ Ride Height (Body Y)', -250, 250, 1)}
          ${row('bodyZ', '↕ Longitudinal Offset (Body Z)', -300, 300, 1)}
        </div>
        <div class="studio-fitment-group">
          <div class="studio-fitment-group-title">
            <span>Front Axle (Independent)</span>
            <small class="muted">Part #3</small>
          </div>
          ${row('frontWidth', '↔ Front Track Width (X)', -250, 250, 1)}
          ${row('frontZ', '↕ Front Axle Offset (Z)', -350, 350, 1)}
          ${row('frontY', '⇕ Front Wheel Height (Y)', -250, 250, 1)}
          ${row('frontScale', '🛞 Front Wheel Diameter', 50, 200, 1, '%')}
        </div>
        <div class="studio-fitment-group">
          <div class="studio-fitment-group-title">
            <span>Rear Axle (Independent)</span>
            <small class="muted">Part #4</small>
          </div>
          ${row('rearWidth', '↔ Rear Track Width (X)', -250, 250, 1)}
          ${row('rearZ', '↕ Rear Axle Offset (Z)', -350, 350, 1)}
          ${row('rearY', '⇕ Rear Wheel Height (Y)', -250, 250, 1)}
          ${row('rearScale', '🛞 Rear Wheel Diameter', 50, 200, 1, '%')}
        </div>
        <div class="row between" style="gap:6px;margin-top:4px">
          <select id="copy-fitment-source" style="flex:1;font-size:11px;padding:3px 6px" title="Copy wheel & body fitment sliders from another car slot">
            ${resolveCatalogBodies().allBodies.filter(c => c.key !== selectedCar).map(c => `<option value="${esc(c.key)}">Slot #${c.globalSlot} (${carGradeBadge(c)}) · ${esc(c.name)}</option>`).join('')}
          </select>
          <button type="button" class="studio-btn" data-action="copy-fitment-from" title="Copy wheel & body fitment from selected car">📋 Copy Fitment</button>
        </div>
        <div class="row between" style="gap:8px;margin-top:4px">
          <button type="button" class="studio-btn" data-action="reset-fitment">↺ Reset Sliders</button>
          <button type="button" class="studio-btn primary" data-action="apply-fitment">✓ Apply Fitment to Mesh</button>
        </div>
      </div>
    </details>`;
}

function getLiveSpecValue(fields, key, fallback = 0) {
  const domInput = document.getElementById(`spec-${key}`);
  if (domInput) {
    const v = Number(domInput.value);
    if (Number.isFinite(v)) return domInput.dataset.speed ? storedShiftSpeed(v) : v;
  }
  const f = fields?.find(x => x.key === key);
  return Number(f?.value ?? fallback);
}

function renderTorqueChart(fields, revLimit, redline) {
  const liveRevLimit = getLiveSpecValue(fields, 'revLimit', revLimit || 9000);
  const liveRedline = getLiveSpecValue(fields, 'redline', redline || 7500);
  const band = Array.from({length: 16}, (_, i) => getLiveSpecValue(fields, 'torqueBand' + i, i * 500));
  const curve = Array.from({length: 16}, (_, i) => getLiveSpecValue(fields, 'torqueCurve' + i, 0));
  const maxRpm = Math.max(...band, Number(liveRevLimit || 9000), 1000);
  const maxTorque = Math.max(...curve, 220, 100);
  const w = 320, h = 132, padL = 32, padR = 10, padT = 14, padB = 22;
  const plotW = w - padL - padR, plotH = h - padT - padB;
  const pts = band.map((rpm, i) => {
    const x = padL + (Math.max(0, rpm) / maxRpm) * plotW;
    const y = padT + plotH - (Math.max(0, curve[i]) / maxTorque) * plotH;
    return `${x.toFixed(1)},${y.toFixed(1)}`;
  });
  const areaPts = `${padL},${padT + plotH} ${pts.join(' ')} ${pts.at(-1)?.split(',')[0] || padL},${padT + plotH}`;
  const redX = liveRedline ? padL + (Math.min(maxRpm, Number(liveRedline)) / maxRpm) * plotW : null;
  return `
    <div id="studio-torque-chart-wrap" style="background:#0d0f14;border:1px solid #262b35;border-radius:6px;padding:8px">
      <div class="row between" style="margin-bottom:4px">
        <small style="color:#94a3b8;font-weight:700" title="Drag yellow points up/down to sculpt torque curve (Shift+Drag to adjust RPM band)">TORQUE CURVE <span style="color:#38bdf8;font-weight:600">(Drag Points)</span></small>
        <small style="color:#cbd5e1">Peak: <b>${Math.max(...curve)}</b> · Max: <b>${maxRpm} RPM</b></small>
      </div>
      <svg id="studio-torque-svg" class="studio-torque-svg" viewBox="0 0 ${w} ${h}" data-max-rpm="${maxRpm}" data-max-torque="${maxTorque}" data-pad-l="${padL}" data-pad-r="${padR}" data-pad-t="${padT}" data-pad-b="${padB}" data-w="${w}" data-h="${h}" style="width:100%;height:auto;display:block;user-select:none;touch-action:none" role="img" aria-label="Interactive Engine Torque Curve">
        <line x1="${padL}" y1="${padT + plotH}" x2="${w - padR}" y2="${padT + plotH}" stroke="#273244" stroke-width="1"/>
        <line x1="${padL}" y1="${padT}" x2="${padL}" y2="${padT + plotH}" stroke="#273244" stroke-width="1"/>
        ${redX ? `<line x1="${redX.toFixed(1)}" y1="${padT}" x2="${redX.toFixed(1)}" y2="${padT + plotH}" stroke="#ef4444" stroke-dasharray="3,3" stroke-width="1.5"/>` : ''}
        <polygon points="${areaPts}" fill="rgba(249,115,22,0.18)"/>
        <polyline points="${pts.join(' ')}" fill="none" stroke="#f97316" stroke-width="2"/>
        ${band.map((rpm, i) => {
          const [x, y] = pts[i].split(',');
          return `<circle cx="${x}" cy="${y}" r="3.8" fill="#fbbf24" data-torque-node="${i}"><title>Node #${i}: ${rpm} RPM → ${curve[i]} Torque (Drag up/down, Shift+Drag for RPM)</title></circle>`;
        }).join('')}
        <text x="${padL}" y="${h - 5}" fill="#64748b" font-size="9">0</text>
        <text x="${w - padR}" y="${h - 5}" fill="#64748b" font-size="9" text-anchor="end">${maxRpm} RPM</text>
      </svg>
    </div>`;
}

function renderGearboxChart(fields) {
  const topGear = Math.min(6, Math.max(4, Math.round(getLiveSpecValue(fields, 'topGear', 6))));
  const gears = [];
  let maxSpeed = 240;
  for (let g = 1; g <= topGear; g++) {
    const rawDn = getLiveSpecValue(fields, 'downshift' + g, 0);
    const rawUp = getLiveSpecValue(fields, 'upshift' + g, 0);
    const dn = g === 1 ? 0 : Number(displayedShiftSpeed(rawDn));
    const up = Number(displayedShiftSpeed(rawUp));
    const ratio = getLiveSpecValue(fields, 'gearRatio' + g, 0);
    const spanEnd = up > dn ? up : Math.max(dn + 38, g * 45);
    if (spanEnd > maxSpeed) maxSpeed = spanEnd;
    gears.push({g, dn, up: spanEnd, rawUp: up, ratio, isTop: g === topGear});
  }
  maxSpeed = Math.max(180, Math.ceil(maxSpeed / 20) * 20);
  const w = 320, h = 22 + topGear * 18, padL = 28, padR = 46, padT = 6;
  const plotW = w - padL - padR;
  return `
    <div id="studio-gearbox-chart-wrap" style="background:#0d0f14;border:1px solid #262b35;border-radius:6px;padding:8px">
      <div class="row between" style="margin-bottom:4px">
        <small style="color:#94a3b8;font-weight:700">GEAR SPEED RANGES (${topGear}-SPEED)</small>
        <small style="color:#cbd5e1">0 – ${maxSpeed} km/h</small>
      </div>
      <svg viewBox="0 0 ${w} ${h}" style="width:100%;height:auto;display:block" role="img" aria-label="Gearbox shift speed bands">
        ${gears.map((item, idx) => {
          const y = padT + idx * 18;
          const x1 = padL + Math.max(0, Math.min(1, item.dn / maxSpeed)) * plotW;
          const x2 = padL + Math.max(0, Math.min(1, item.up / maxSpeed)) * plotW;
          const barW = Math.max(6, x2 - x1);
          const color = item.isTop ? '#38bdf8' : '#22c55e';
          return `
            <text x="4" y="${y + 10}" fill="#94a3b8" font-size="9.5" font-weight="700">G${item.g}</text>
            <rect x="${padL}" y="${y + 2}" width="${plotW}" height="10" rx="3" fill="#161b24"/>
            <rect x="${x1.toFixed(1)}" y="${y + 2}" width="${barW.toFixed(1)}" height="10" rx="3" fill="${color}" fill-opacity="0.72" stroke="${color}" stroke-width="0.8">
              <title>G${item.g}: ${item.dn.toFixed(0)} → ${item.rawUp > 0 ? item.rawUp.toFixed(0) : 'Top'} km/h · Ratio ${item.ratio}</title>
            </rect>
            <text x="${w - 4}" y="${y + 10}" fill="#cbd5e1" font-size="8.5" text-anchor="end">${item.dn.toFixed(0)}–${item.rawUp > 0 ? item.rawUp.toFixed(0) : 'MAX'}</text>
          `;
        }).join('')}
      </svg>
    </div>`;
}

function renderAccordionInspector(car, parts, spec) {
  const openAttr = id => openAccordions.has(id) ? ' open' : '';
  const fieldInput = f => {
    if (!f) return '';
    if (f.type === 'text') {
      return `<input id="spec-${f.key}" data-spec="${f.key}" data-text data-original="${esc(f.value)}" type="text" maxlength="${f.maxLength}" value="${esc(f.value)}" required>`;
    }
    const speed = /^(downshift|upshift)/.test(f.key);
    const val = speed ? displayedShiftSpeed(f.value) : f.value;
    const min = speed ? displayedShiftSpeed(f.min) : f.min;
    const max = speed ? displayedShiftSpeed(f.max) : f.max;
    return `<input id="spec-${f.key}" data-spec="${f.key}" data-original="${val}" ${speed ? 'data-speed="true" ' : ''}type="number" min="${min}" max="${max}" step="${speed ? '0.01' : '1'}" value="${val}" required>`;
  };
  const fieldBox = (key, label) => {
    const f = spec?.fields?.find(x => x.key === key);
    if (!f) return '';
    const modified = f.baseValue !== undefined && f.value !== f.baseValue;
    const speed = /^(downshift|upshift)/.test(f.key);
    const base = f.type === 'text' ? f.baseValue : (speed ? displayedShiftSpeed(f.baseValue) : f.baseValue);
    const scrubClass = f.type === 'text' ? '' : ' class="studio-scrub-label" title="Drag horizontally on label to scrub value"';
    return `
      <div class="studio-field">
        <label for="spec-${f.key}"${scrubClass}>
          <span>${esc(label)} ${f.origin?`<small data-origin="${esc(f.origin)}">${esc(f.origin)}</small>`:''}</span>
          ${modified ? `<button type="button" class="field-reset" data-reset-field="${f.key}" data-base-value="${esc(base)}" title="Reset to ${esc(base)}">↺</button>` : ''}
        </label>
        ${fieldInput(f)}
      </div>`;
  };

  if (car.rival) {
    return `
      <div class="studio-accordion-scroll">
        ${renderFitmentAccordion()}
        <details class="studio-accordion" data-accordion="identity" open>
          <summary><span>🏷️ Custom Race / Rival Car Slot</span><span class="studio-accordion-badge">Slot #${car.globalSlot}</span></summary>
          <div class="studio-accordion-body">
            <p class="muted" style="margin:0 0 8px">
              <b>${esc(car.name)}</b> (Class ${car.rivalClass} · ${esc(car.rivalCourseName)}, Bank ${car.bank}).
            </p>
            <p class="muted" style="margin:0">
              Use <b>Wheel &amp; Body Fitment</b> above to adjust wheel positions, or use the <b>Left Tool Dock</b> to isolate any of the 5 mesh parts or edit its textures.
            </p>
          </div>
        </details>
      </div>`;
  }

  if (!spec?.fields?.length) {
    return `
      <div class="studio-accordion-scroll">
        ${renderFitmentAccordion()}
        <div style="padding:16px;color:#94a3b8">Loading car parameters…</div>
      </div>`;
  }

  const manual = spec.fields.find(f => f.key === 'manualOnly');
  const revLimit = spec.fields.find(f => f.key === 'revLimit')?.value;
  const redline = spec.fields.find(f => f.key === 'redline')?.value;
  const topGearVal = Number(spec.fields.find(f => f.key === 'topGear')?.value || 6);
  const physicsWarnings = [];
  if (Number(redline) > Number(revLimit)) physicsWarnings.push(`Redline (${redline} RPM) exceeds Rev Limit (${revLimit} RPM)`);
  for (let g = 2; g <= Math.min(6, topGearVal - 1); g++) {
    const dn = Number(spec.fields.find(f => f.key === 'downshift' + g)?.value ?? 0);
    const up = Number(spec.fields.find(f => f.key === 'upshift' + g)?.value ?? 0);
    if (up > 0 && dn >= up) {
      physicsWarnings.push(`Gear G${g} downshift speed ≥ upshift speed`);
      break;
    }
  }
  const donorPlayerCars = typeof resolveCatalogBodies === 'function'
    ? resolveCatalogBodies().playerBodies.filter(c => c.bank !== car.bank)
    : [];

  return `
    <form id="car-spec-form" class="studio-accordion-scroll" data-bank="${car.bank}" data-revision="${spec.revision}">
      <!-- WHEEL & BODY FITMENT -->
      ${renderFitmentAccordion()}

      ${physicsWarnings.length ? `
        <div style="margin:0 10px 6px;padding:6px 8px;background:rgba(245,158,11,0.14);border:1px solid #f59e0b;border-radius:6px">
          <small style="color:#fde68a;font-weight:700;display:block">⚠️ Physics Sanity Warnings</small>
          ${physicsWarnings.map(w => `<small style="color:#fef3c7;display:block">• ${esc(w)}</small>`).join('')}
        </div>` : ''}

      <!-- IDENTITY, SHOP & CLASS -->
      <details class="studio-accordion" data-accordion="identity"${openAttr('identity')}>
        <summary>
          <span>🏷️ Identity, Shop &amp; Class</span>
          <span class="studio-accordion-badge">Slot #${car.globalSlot} · ${carGradeBadge(car)}</span>
        </summary>
        <div class="studio-accordion-body">
          ${fieldBox('name', 'Car Name (max 31 chars)')}
          <div class="studio-grid-2">
            ${fieldBox('manufacturer', 'Manufacturer')}
            ${fieldBox('className', 'Class Badge')}
          </div>
          <div class="studio-grid-3">
            ${fieldBox('unlockClass', 'Unlock (0-5)')}
            ${fieldBox('price', 'Price (Cr)')}
            ${fieldBox('upgradePrice', 'Upgrade (Cr)')}
          </div>
          ${donorPlayerCars.length ? `
            <div style="padding-top:6px;border-top:1px solid #262b35">
              <small style="color:#94a3b8;font-weight:700;display:block;margin-bottom:4px">CLONE PHYSICS SETUP FROM ANOTHER CAR</small>
              <div class="row between" style="gap:6px">
                <select id="copy-physics-source" style="flex:1;font-size:11px;padding:3px 6px" title="Choose donor car to copy Engine, Gearbox & Handling parameters from">
                  ${donorPlayerCars.map(c => `<option value="${c.bank}">Slot #${c.globalSlot} (${carGradeBadge(c)}) · ${esc(c.name)}</option>`).join('')}
                </select>
                <button type="button" class="studio-btn" data-action="copy-physics-from" title="Copy Engine, Torque Curve, Gearbox & Handling from selected car">📋 Copy Physics</button>
              </div>
            </div>` : ''}
        </div>
      </details>

      <!-- ENGINE & TORQUE CURVE -->
      <details class="studio-accordion" data-accordion="engine"${openAttr('engine')}>
        <summary>
          <span>⚙️ Engine &amp; Torque Curve</span>
          <span class="studio-accordion-badge">${revLimit || 0} RPM</span>
        </summary>
        <div class="studio-accordion-body">
          ${renderTorqueChart(spec.fields, revLimit, redline)}
          <div class="studio-grid-3">
            ${fieldBox('revLimit', 'Rev Limit')}
            ${fieldBox('redline', 'Redline RPM')}
            ${fieldBox('automaticAccelerationScale', 'Auto Accel ‰')}
          </div>
          <details class="car-advanced">
            <summary>Edit 16-Point RPM &amp; Torque Curve Table</summary>
            <div class="table-scroll compact-table-wrap" style="max-height:200px;margin-top:6px">
              <table class="asset-table compact-table">
                <thead><tr><th>#</th><th>RPM Band</th><th>Torque</th><th>Loss RPM</th><th>Loss</th></tr></thead>
                <tbody>
                  ${Array.from({length: 16}, (_, i) => `
                    <tr>
                      <th>${i}</th>
                      <td>${fieldInput(spec.fields.find(f => f.key === 'torqueBand' + i))}</td>
                      <td>${fieldInput(spec.fields.find(f => f.key === 'torqueCurve' + i))}</td>
                      <td>${fieldInput(spec.fields.find(f => f.key === 'torqueLossRpm' + i)) || '—'}</td>
                      <td>${fieldInput(spec.fields.find(f => f.key === 'torqueLossValue' + i)) || '—'}</td>
                    </tr>`).join('')}
                </tbody>
              </table>
            </div>
          </details>
        </div>
      </details>

      <!-- GEARBOX & SHIFT SPEEDS -->
      <details class="studio-accordion" data-accordion="gearbox"${openAttr('gearbox')}>
        <summary>
          <span>🕹️ Gearbox &amp; Shift Speeds</span>
          <span class="studio-accordion-badge">${spec.fields.find(f => f.key === 'topGear')?.value || 6}-Speed</span>
        </summary>
        <div class="studio-accordion-body">
          ${typeof renderGearboxChart === 'function' ? renderGearboxChart(spec.fields) : ''}
          <div class="studio-grid-2">
            ${manual ? `
              <div class="studio-field">
                <label for="spec-manualOnly"><span>Transmission</span></label>
                <select id="spec-manualOnly" data-spec="manualOnly" data-original="${manual.value}">
                  <option value="0" ${!manual.value ? 'selected' : ''}>AT and MT</option>
                  <option value="1" ${manual.value ? 'selected' : ''}>Manual Only</option>
                </select>
              </div>` : ''}
            ${fieldBox('topGear', 'Forward Gears (4–6)')}
          </div>
          <div class="table-scroll compact-table-wrap">
            <table class="asset-table compact-table">
              <thead><tr><th>G</th><th>Down (km/h)</th><th>Up (km/h)</th><th>Ratio</th><th>Torque</th><th>Load</th></tr></thead>
              <tbody>
                ${Array.from({length: 6}, (_, i) => `
                  <tr>
                    <th>G${i + 1}</th>
                    <td>${fieldInput(spec.fields.find(f => f.key === 'downshift' + (i + 1)))}</td>
                    <td>${fieldInput(spec.fields.find(f => f.key === 'upshift' + (i + 1)))}</td>
                    <td>${fieldInput(spec.fields.find(f => f.key === 'gearRatio' + (i + 1)))}</td>
                    <td>${fieldInput(spec.fields.find(f => f.key === 'torqueScale' + i))}</td>
                    <td>${fieldInput(spec.fields.find(f => f.key === 'gearLoad' + i))}</td>
                  </tr>`).join('')}
              </tbody>
            </table>
          </div>
          <details class="car-advanced"><summary>Neutral gear ratio</summary>${fieldBox('gearRatio0', 'Neutral ratio (index 0)')}</details>
        </div>
      </details>

      <!-- HANDLING, GRIP & AERO -->
      <details class="studio-accordion" data-accordion="handling"${openAttr('handling')}>
        <summary>
          <span>🏎️ Handling, Grip &amp; Aero</span>
          <span class="studio-accordion-badge">Grip ${spec.fields.find(f => f.key === 'baseSteeringGrip')?.value ?? ''}</span>
        </summary>
        <div class="studio-accordion-body">
          <div class="studio-grid-2">
            ${fieldBox('baseSteeringGrip', 'Base Steering Grip')}
            ${fieldBox('steeringGripResponse', 'Grip Response')}
            ${fieldBox('referenceTurnRadius', 'Turn Radius Ref')}
            ${fieldBox('speedDragDivisor', 'Aero Drag Divisor')}
            ${fieldBox('steerResponse', 'AI Steer Divisor')}
            ${fieldBox('negconSteeringAssistScale', 'neGcon Assist ‰')}
          </div>
        </div>
      </details>
    </form>`;
}

function openLoadCarModal() {
  const {playerBodies, rivalBodies, allBodies, car} = resolveCatalogBodies();
  const grades = ['all', 'G1', 'G2', 'G3', 'G4', 'G5', 'SP', 'NEW'];
  const sourceList = carSection === 'player' ? playerBodies : carSection === 'rival' ? rivalBodies : allBodies;
  const filtered = sourceList.filter(c => {
    if (!c.rival && carSection === 'player' && carGradeFilter !== 'all' && carGradeBadge(c) !== carGradeFilter) return false;
    if (c.rival) {
      if (rivalClassFilter > 0 && c.rivalClass !== rivalClassFilter) return false;
      if (rivalCourseFilter >= 0 && c.rivalCourse !== rivalCourseFilter) return false;
    }
    if (carSearch && !String(c.name || '').toLowerCase().includes(carSearch.toLowerCase())) return false;
    return true;
  });

  const dialog = $('#dialog');
  dialog.innerHTML = `
    <div style="min-width:720px;max-width:880px">
      <div class="row between wrap">
        <div>
          <div class="eyebrow">FILE → LOAD CAR</div>
          <h2 style="margin:2px 0">Select a Car Slot (${allBodies.length} Total Slots)</h2>
        </div>
        <div class="row" style="gap:6px">
          <button type="button" class="studio-btn ${carSection === 'all' ? 'primary' : ''}" data-modal-car-section="all">All Slots (${allBodies.length})</button>
          <button type="button" class="studio-btn ${carSection === 'player' ? 'primary' : ''}" data-modal-car-section="player">Player &amp; Expansion (${playerBodies.length})</button>
          <button type="button" class="studio-btn ${carSection === 'rival' ? 'primary' : ''}" data-modal-car-section="rival">Custom Race / Rival (${rivalBodies.length})</button>
        </div>
      </div>
      <div class="row wrap" style="margin-top:10px;gap:8px">
        <input type="search" id="modal-car-search" placeholder="Search across ${allBodies.length} car slots…" value="${esc(carSearch)}" style="flex:1;min-width:200px">
        ${carSection === 'player' ? `
          <div class="row" style="gap:4px">
            ${grades.map(g => `<button type="button" class="studio-chip ${carGradeFilter === g ? 'active' : ''}" data-modal-grade="${g}">${g === 'all' ? 'All' : g}</button>`).join('')}
          </div>` : ''}
        ${carSection === 'rival' || carSection === 'all' ? `
          <label class="row" style="gap:4px;font-size:12px">Class
            <select data-modal-rival="class">
              <option value="0" ${rivalClassFilter === 0 ? 'selected' : ''}>All Classes (1–6)</option>
              ${Array.from({length: 6}, (_, i) => `<option value="${i + 1}" ${rivalClassFilter === i + 1 ? 'selected' : ''}>Class ${i + 1}</option>`).join('')}
            </select>
          </label>
          <label class="row" style="gap:4px;font-size:12px">Course
            <select data-modal-rival="course">
              <option value="-1" ${rivalCourseFilter === -1 ? 'selected' : ''}>All 4 Courses</option>
              ${COURSE_NAMES.map((n, i) => `<option value="${i}" ${rivalCourseFilter === i ? 'selected' : ''}>${n}</option>`).join('')}
            </select>
          </label>` : ''}
      </div>
      <div class="studio-browser-grid">
        ${filtered.map(c => {
          const canRestore = !c.customSlot && (c.rival || c.bank <= 72);
          return `
          <div class="studio-browser-card ${car && c.key === car.key ? 'active' : ''}" data-pick-car="${esc(c.key)}" role="button" tabindex="0">
            <div class="row between" style="width:100%">
              <span class="car-slot-badge">${carGradeBadge(c)}</span>
              <small>Slot #${c.globalSlot} · Bank ${c.bank}</small>
            </div>
            <strong style="font-size:13.5px">${esc(c.name)}</strong>
            <div class="row between" style="width:100%;margin-top:2px;gap:6px">
              <small>${c.rival ? `Class ${c.rivalClass} · ${esc(c.rivalCourseName)}` : c.customSlot ? 'Expansion Slot · Isolated 3D + Physics' : 'Player Slot · Full Physics + 3D'}</small>
              ${canRestore ? `<button type="button" class="studio-btn" data-restore-car-key="${esc(c.key)}" style="padding:2px 7px;font-size:10.5px" title="Restore original version from game disc">↺ Restore Original</button>` : ''}
              ${c.customSlot ? `<button type="button" class="studio-btn" data-delete-car-key="${esc(c.key)}" style="padding:2px 7px;font-size:10.5px;border-color:rgba(248,113,113,0.45);color:#fca5a5" title="Delete this custom expansion car slot">🗑 Delete Slot</button>` : ''}
            </div>
          </div>`;
        }).join('') || '<p class="muted">No cars match this filter.</p>'}
      </div>
      <footer class="row end">
        <button type="button" class="studio-btn" data-action="close">Close</button>
      </footer>
    </div>`;
  if (!dialog.open) dialog.showModal();
  const searchEl = $('#modal-car-search');
  if (searchEl) {
    searchEl.focus();
    searchEl.setSelectionRange(searchEl.value.length, searchEl.value.length);
  }
}

function openNewCarModal() {
  const {playerBodies, car} = resolveCatalogBodies();
  const builtInPlayers = playerBodies.filter(c => c.bank >= 10 && c.bank <= 72);
  const currentBank = (!car || car.rival || car.bank > 72) ? (builtInPlayers[0]?.bank || 10) : car.bank;
  const nextSlot = Math.max(31, ...playerBodies.map(c => c.globalSlot)) + 1;
  const dialog = $('#dialog');
  dialog.innerHTML = `
    <div style="min-width:540px;max-width:640px">
      <div class="eyebrow">FILE → NEW CAR</div>
      <h2 style="margin:2px 0 6px">Add a New Car or Replace an Existing Slot</h2>
      <p class="muted" style="margin:0 0 12px">You can create a brand-new expansion car slot (Slot #${nextSlot}+) or overwrite any existing car slot, copying physics from any donor car.</p>
      <form id="new-car-form">
        <div class="studio-grid-2" style="margin-bottom:10px">
          <div class="studio-field">
            <label for="new-car-target">Target Car Slot</label>
            <select id="new-car-target" name="targetBank" required>
              <option value="new" selected>➕ Add New Expansion Car Slot (Slot #${nextSlot})</option>
              <optgroup label="Existing Player &amp; Expansion Slots">
                ${playerBodies.map(c => `<option value="${c.bank}">Overwrite Slot #${c.globalSlot} (${carGradeBadge(c)}) — ${esc(c.name)}</option>`).join('')}
              </optgroup>
            </select>
          </div>
          <div class="studio-field">
            <label for="new-car-source">Copy Base Physics &amp; Mesh From</label>
            <select id="new-car-source" name="sourceBank" required>
              ${builtInPlayers.map(c => `<option value="${c.bank}" ${c.bank === currentBank ? 'selected' : ''}>Slot #${c.globalSlot} — ${esc(c.name)}</option>`).join('')}
            </select>
          </div>
        </div>
        <div class="studio-grid-3" style="margin-bottom:10px">
          <div class="studio-field">
            <label for="new-car-name">New Car Name</label>
            <input id="new-car-name" name="name" type="text" maxlength="31" value="CUSTOM GT" required>
          </div>
          <div class="studio-field">
            <label for="new-car-mfg">Manufacturer</label>
            <input id="new-car-mfg" name="manufacturer" type="text" maxlength="31" value="AGE" required>
          </div>
          <div class="studio-field">
            <label for="new-car-class">Class Badge</label>
            <input id="new-car-class" name="className" type="text" maxlength="31" value="PROTO" required>
          </div>
        </div>
        <div class="studio-grid-3" style="margin-bottom:12px">
          <div class="studio-field">
            <label for="new-car-unlock">Unlock Level (0 = Start)</label>
            <input id="new-car-unlock" name="unlockClass" type="number" min="0" max="5" value="0" required>
          </div>
          <div class="studio-field">
            <label for="new-car-price">Price (Credits)</label>
            <input id="new-car-price" name="price" type="number" min="0" max="2147483647" value="15000" required>
          </div>
          <div class="studio-field">
            <label for="new-car-upgrade">Upgrade Price</label>
            <input id="new-car-upgrade" name="upgradePrice" type="number" min="0" max="2147483647" value="30000" required>
          </div>
        </div>
        <label style="display:flex;align-items:center;gap:8px;margin-bottom:14px;font-size:12px;color:#e2e8f0">
          <input type="checkbox" name="importGlbAfter">
          <span>Immediately open <b>Import Complete 3D Car (.GLB / .glTF)</b> after creating</span>
        </label>
        <footer class="row between">
          <button type="button" class="studio-btn" data-action="close">Cancel</button>
          <button type="submit" class="studio-btn primary">✨ Create Car</button>
        </footer>
      </form>
    </div>`;
  if (!dialog.open) dialog.showModal();
}

function summarizeProjectChanges() {
  const {allBodies} = resolveCatalogBodies();
  const groups = new Map();
  for (const c of project?.changes || []) {
    if (c.kind === 'car') {
      const m = /^car\.(player|rival)\.(\d+)(?:\.part\.(\d+))?/.exec(String(c.id || ''));
      if (m) {
        const isRival = m[1] === 'rival';
        const bank = Number(m[2]);
        const rawPart = m[3] !== undefined ? Number(m[3]) : 0;
        const bodyPart = isRival ? Math.floor(rawPart / 5) * 5 : 0;
        const bodyKey = `car.${isRival ? 'rival' : 'player'}.${bank}.part.${bodyPart}`;
        const carObj = allBodies.find(x => x.key === bodyKey);
        if (!groups.has(bodyKey)) {
          groups.set(bodyKey, {
            id: bodyKey,
            kind: 'car',
            title: carObj ? `Slot #${carObj.globalSlot} (${carGradeBadge(carObj)}) · ${carObj.name}` : `Car Bank ${bank}${isRival ? ` · Part ${bodyPart}` : ''}`,
            subtitle: carObj?.rival ? `Rival · Class ${carObj.rivalClass} · ${carObj.rivalCourseName}` : (carObj?.customSlot ? 'Custom Expansion Slot' : `Player Car · Bank ${bank}`),
            carKey: carObj?.key || bodyKey,
            bank,
            rival: isRival,
            bodyPart,
            customSlot: Boolean(carObj?.customSlot || (!isRival && bank >= 200)),
            canRestore: Boolean(!carObj?.customSlot && (isRival || bank <= 72)),
            tags: new Set(),
            count: 0
          });
        }
        const g = groups.get(bodyKey);
        g.count++;
        if (c.field === 'spec') g.tags.add('Physics / Spec');
        else if (c.field === 'fitment') g.tags.add('3D Wheel/Body Fitment');
        else if (c.field === 'mesh' || c.field === 'model') g.tags.add('3D Mesh');
        else if (c.field === 'texture' || c.field === 'material') g.tags.add('Textures / Surfaces');
        else g.tags.add(c.field || 'Modified');
      }
    } else if (c.kind === 'track') {
      const m = /^track\.(\d+)/.exec(String(c.id || ''));
      const bank = m ? Number(m[1]) : Number(c.bank || 88);
      const trackKey = `track.${bank}`;
      if (!groups.has(trackKey)) {
        const grade = Math.floor((bank - 88) / 8) + 1;
        const courseIdx = Math.floor(((bank - 88) % 8) / 2);
        groups.set(trackKey, {
          id: trackKey,
          kind: 'track',
          title: `Class ${grade} · ${COURSE_NAMES[courseIdx] || 'Course'}`,
          subtitle: `Track Bank ${bank}`,
          bank,
          canRestore: true,
          tags: new Set(),
          count: 0
        });
      }
      const g = groups.get(trackKey);
      g.count++;
      g.tags.add(c.field || 'Track Edit');
    }
  }
  return [...groups.values()];
}

function openProjectChangesModal() {
  const groups = summarizeProjectChanges();
  const totalEdits = project?.changes?.length || 0;
  const dialog = $('#dialog');
  dialog.innerHTML = `
    <div style="min-width:640px;max-width:780px">
      <div class="row between wrap">
        <div>
          <div class="eyebrow">EDIT → PROJECT CHANGES OUTLINER</div>
          <h2 style="margin:2px 0">Modified Assets in Project (${groups.length} Asset${groups.length === 1 ? '' : 's'} · ${totalEdits} Edit${totalEdits === 1 ? '' : 's'})</h2>
        </div>
        <div class="row" style="gap:6px">
          <button type="button" class="studio-btn" data-project-history="undo" ${!project?.canUndo ? 'disabled' : ''}>↶ Undo</button>
          <button type="button" class="studio-btn" data-project-history="redo" ${!project?.canRedo ? 'disabled' : ''}>↷ Redo</button>
        </div>
      </div>
      <p class="muted" style="margin:6px 0 12px;font-size:12px">
        Inspect every modified car and track in your mod project. Jump directly to any asset or revert individual assets back to their original disc version.
      </p>
      <div style="max-height:54vh;overflow-y:auto;padding-right:4px">
        ${groups.length ? groups.map(g => `
          <div class="studio-outliner-card">
            <div>
              <div class="row" style="gap:8px;align-items:center">
                <span class="car-slot-badge">${g.kind === 'car' ? 'CAR' : 'TRACK'}</span>
                <strong style="font-size:13px">${esc(g.title)}</strong>
                <small class="muted">${esc(g.subtitle)}</small>
              </div>
              <div class="row wrap" style="gap:5px;margin-top:6px">
                ${[...g.tags].map(t => `<span class="studio-accordion-badge" style="background:rgba(56,189,248,0.15);color:#bae6fd">${esc(t)}</span>`).join('')}
                <small class="muted" style="margin-left:4px">(${g.count} change entry${g.count === 1 ? '' : 'ies'})</small>
              </div>
            </div>
            <div class="row" style="gap:6px;flex-shrink:0">
              ${g.kind === 'car' ? `
                <button type="button" class="studio-btn primary" data-pick-car="${esc(g.carKey)}">Open Car</button>
                ${g.canRestore ? `<button type="button" class="studio-btn" data-restore-car-key="${esc(g.carKey)}" title="Revert this car to its original disc state">↺ Revert to Disc</button>` : ''}
                ${g.customSlot ? `<button type="button" class="studio-btn" data-delete-car-key="${esc(g.carKey)}" style="border-color:rgba(248,113,113,0.45);color:#fca5a5">🗑 Delete Slot</button>` : ''}
              ` : `
                <button type="button" class="studio-btn primary" data-open-studio="track">Open Track Studio</button>
              `}
            </div>
          </div>
        `).join('') : '<div class="card" style="text-align:center;padding:24px"><p class="muted" style="margin:0">No project edits recorded yet — all cars and tracks match the original game disc.</p></div>'}
      </div>
      <footer class="row between" style="margin-top:14px">
        <small class="muted">Project Revision #${project?.revision ?? 0}</small>
        <button type="button" class="studio-btn" data-action="close">Close</button>
      </footer>
    </div>`;
  if (!dialog.open) dialog.showModal();
}

let paletteQuery = '';
function openCommandPaletteModal(query = paletteQuery) {
  paletteQuery = query;
  const {allBodies, car} = resolveCatalogBodies();
  const parts = car ? carParts.filter(c => c.bank === car.bank && c.rival === car.rival && c.part >= car.part && c.part < car.part + 5) : [];
  const items = [
    {group: 'Action', label: '💾 Save & Apply All Changes to Game', hint: 'Ctrl+S', attr: 'data-action="save-all"'},
    {group: 'Action', label: `🏁 Start Test Race with ${car?.name || 'Current Car'}`, hint: 'Launch Race', attr: 'data-action="test-race"'},
    {group: 'Action', label: `📦 Project Changes Outliner (${project?.changes?.length || 0} edits)`, hint: 'Inspect / Revert', attr: 'data-open-project-changes="true"'},
    {group: 'Action', label: '✨ Create New Car / Expansion Slot…', hint: 'Ctrl+N', attr: 'data-open-new-car="true"'},
    {group: 'Action', label: '🚗 Browse Car Catalog Modal…', hint: 'Ctrl+O', attr: 'data-open-load-car="true"'},
    {group: 'Action', label: '🎨 Open Car Surface & Texture Sheet…', hint: 'TIM / Clut', attr: `data-car-materials="${esc(inlineCarPart === 'whole' ? (car?.key || '') : inlineCarPart)}"`},
    {group: 'Studio', label: '🗺️ Switch to Track Editor Studio', hint: '18 Course Banks', attr: 'data-open-studio="track"'},
    {group: 'Studio', label: '💾 Switch to Save Editor', hint: 'Garage & Credits', attr: 'data-open-studio="saves"'},
    {group: '3D Part', label: '🚗 Show Complete 5-Part Car Assembly', hint: 'Whole Car', attr: 'data-inline-part="whole"'},
    ...parts.map(p => ({
      group: '3D Part',
      label: `🔧 Isolate: ${carPartName(car, p.part)}`,
      hint: p.key,
      attr: `data-inline-part="${esc(p.key)}"`
    })),
    ...allBodies.map(c => ({
      group: c.rival ? 'Rival Car' : (c.customSlot ? 'Expansion Car' : 'Player Car'),
      label: `Slot #${c.globalSlot} (${carGradeBadge(c)}) · ${c.name}`,
      hint: c.rival ? `Class ${c.rivalClass} · ${c.rivalCourseName}` : `Bank ${c.bank}`,
      attr: `data-pick-car="${esc(c.key)}"`
    }))
  ];
  const q = String(paletteQuery || '').trim().toLowerCase();
  const filtered = (q ? items.filter(it => `${it.group} ${it.label} ${it.hint}`.toLowerCase().includes(q)) : items).slice(0, 45);
  const dialog = $('#dialog');
  const renderList = list => list.map((it, idx) => `
    <button type="button" class="studio-palette-item ${idx === 0 ? 'selected' : ''}" ${it.attr}>
      <div>
        <span class="car-slot-badge" style="margin-right:6px">${esc(it.group)}</span>
        <strong>${esc(it.label)}</strong>
      </div>
      <small class="muted">${esc(it.hint)}</small>
    </button>
  `).join('') || '<p class="muted" style="padding:12px">No matching commands or car slots.</p>';

  if (dialog.open && $('#studio-command-palette-input')) {
    const listEl = $('#studio-command-palette-list');
    if (listEl) listEl.innerHTML = renderList(filtered);
    return;
  }

  dialog.innerHTML = `
    <div style="min-width:600px;max-width:720px">
      <div class="row between">
        <div class="eyebrow">COMMAND PALETTE (CTRL+K)</div>
        <small class="muted">Use [ and ] in studio to step slots · 1/3/7/9/W/G/B/O/T/L in 3D viewport</small>
      </div>
      <input type="search" id="studio-command-palette-input" placeholder="Jump to any car slot, isolate 3D mesh part, or run studio action…" value="${esc(paletteQuery)}" style="width:100%;margin-top:8px;padding:9px 12px;font-size:13.5px">
      <div id="studio-command-palette-list" class="studio-palette-list">
        ${renderList(filtered)}
      </div>
      <footer class="row between" style="margin-top:10px">
        <small class="muted">Press Enter to run first match · Esc to close</small>
        <button type="button" class="studio-btn" data-action="close">Close</button>
      </footer>
    </div>`;
  if (!dialog.open) dialog.showModal();
  const inp = $('#studio-command-palette-input');
  if (inp) {
    inp.focus();
    inp.setSelectionRange(inp.value.length, inp.value.length);
  }
}

async function mountCarViewport() {
  if (previewPromise) await cancelPreview();
  const token = ++previewToken;
  const host = $('#car-studio-viewport');
  if (!host || host.querySelector('canvas')) return;
  const {car} = resolveCatalogBodies();
  if (!car) return;
  const targetKey = `${car.key}:${inlineCarPart}:${project?.revision ?? 0}`;
  if (loadingPreviewKey === targetKey || activePreviewKey === targetKey) return;
  loadingPreviewKey = targetKey;
  const savedView = host._savedView || host._viewState || null;
  host.innerHTML = '<div style="color:#94a3b8;font-size:13px">Loading 3D Car Viewport…</div>';
  previewBusy = true;
  const job = inlineCarPart === 'whole'
    ? request('previewWholeCar', car.bank, car.part, car.rival)
    : request('previewCar', inlineCarPart);
  previewPromise = job;
  try {
    const data = await job;
    if (token !== previewToken || !host.isConnected) return;
    activePreviewKey = targetKey;
    let ghostVertices = null;
    if (inlineCarPart === 'whole' && Array.isArray(data?.vertices)) {
      const baseVertices = Float32Array.from(data.vertices);
      const partRanges = data.partRanges || [];
      const bakedFitment = data.fitment || {...DEFAULT_FITMENT};
      lastWholeCarData = {baseVertices, partRanges, bakedFitment};
      ghostVertices = computeFittedVertices(baseVertices, partRanges, DEFAULT_FITMENT, bakedFitment);
    } else {
      lastWholeCarData = null;
    }
    const height = Math.max(420, host.clientHeight || window.innerHeight - 66);
    await mountModelPreview(host, data, {inline: true, height, viewState: savedView, ghostVertices});
    if (inlineCarPart === 'whole' && hasPendingFitment()) {
      applyFitmentToViewport();
    }
  } catch (error) {
    if (token === previewToken && host.isConnected) {
      activePreviewKey = '';
      if (/Wait for the current operation|Another operation|Operation canceled/.test(error.message)) {
        host.textContent = 'Waiting for the editor…';
        setTimeout(() => {if (host.isConnected && token === previewToken) mountCarViewport();}, 150);
      } else {
        host.innerHTML = '<p>Could not load preview: '+esc(error.message)+'</p><button type="button" data-retry-preview>Retry preview</button>';
        host.querySelector('[data-retry-preview]').addEventListener('click',mountCarViewport,{once:true});
      }
    }
  } finally {
    if (previewPromise === job) previewPromise = null;
    if (loadingPreviewKey === targetKey) loadingPreviewKey = '';
    if (token === previewToken) {
      previewBusy = false;
      if (state) state.busy = null;
    }
  }
}

function render() {
  const root = $('#studio-app');
  if (!root || !state) return;

  if (!state.ready) {
    root.innerHTML = `
      <header class="studio-menubar">
        <div class="studio-menubar-left"><span class="studio-brand-badge">RAGE CAR STUDIO</span></div>
        <div class="studio-menubar-right"><button type="button" class="studio-btn" data-open-studio="classic">Classic Launcher</button></div>
      </header>
      <div style="display:flex;align-items:center;justify-content:center;height:100%;padding:32px">
        <div class="card" style="max-width:520px;text-align:center">
          <h2>Load Your Rage Racer Disc Image First</h2>
          <p class="muted">Select your legally obtained CUE, CHD, or Track 01 BIN disc image to extract and edit 3D cars.</p>
          <button type="button" class="studio-btn primary" data-action="choose-game">Choose Disc Image…</button>
        </div>
      </div>
      <footer class="studio-statusbar"><span>No disc loaded</span><span>Rage Car Studio</span></footer>`;
    return;
  }

  const {allBodies, car} = resolveCatalogBodies();
  if (!car) {
    root.innerHTML = `<div style="padding:32px">No cars found in catalog.</div>`;
    return;
  }
  if (selectedCar !== car.key) {
    selectedCar = car.key;
    inlineCarPart = 'whole';
    fitmentDraft = getProjectFitment(car);
  }
  const parts = carParts.filter(c => c.bank === car.bank && c.rival === car.rival && c.part >= car.part && c.part < car.part + 5);
  const spec = (!car.rival && currentCarSpecBank === car.bank) ? currentCarSpec : null;
  const prevViewport = $('#car-studio-viewport');
  const prevViewState = prevViewport?._viewState || prevViewport?._savedView || null;
  const isDirty = Boolean(project?.dirty || formDirty || hasPendingFitment());
  const canAct = !state.busy && !state.running;

  const trackOptions = [];
  for (let grade = 1; grade <= 6; grade++) {
    for (let course = 0; course < 4; course++) {
      if (course === 3 && grade < 3) continue;
      const b = 88 + (grade - 1) * 8 + course * 2;
      trackOptions.push({bank: b, label: `Class ${grade} · ${COURSE_NAMES[course]}`});
    }
  }

  root.innerHTML = `
    <!-- 1. TOP MENU BAR (Canonical File / Edit / View / Window + Single Save + Direct Test in Race) -->
    <header class="studio-menubar">
      <div class="studio-menubar-left">
        <div class="studio-seg">
          <button type="button" class="active" data-open-studio="car">🚗 Car Editor</button>
          <button type="button" data-open-studio="track">🗺️ Track Editor</button>
          <button type="button" data-open-studio="saves">💾 Save Editor</button>
          <button type="button" data-open-studio="settings">⚙️ Game Settings</button>
        </div>

        <details class="studio-menu">
          <summary>File ▾</summary>
          <div class="studio-menu-dropdown">
            <button type="button" data-open-new-car><span>✨ New Car…</span><small>Ctrl+N</small></button>
            <button type="button" data-open-load-car><span>🚗 Load Car (${allBodies.length} Slots)…</span><small>Ctrl+O</small></button>
            <button type="button" data-open-command-palette><span>🔍 Command Palette…</span><small>Ctrl+K</small></button>
            <hr>
            <button type="button" data-import-set="${esc(car.key)}" data-format="glb"><span>📥 Import Complete 3D Car (.GLB / .glTF)…</span></button>
            <button type="button" data-import-set="${esc(car.key)}"><span>📁 Import Car Set Folder (.OBJ + .PNG)…</span></button>
            <button type="button" data-export-set="${esc(car.key)}"><span>📤 Export Complete 3D Car (.GLB / .OBJ)…</span></button>
            <hr>
            ${!car.customSlot && (car.rival || car.bank <= 72) ? `<button type="button" data-action="restore-original-car" ${!canAct ? 'disabled' : ''}><span>↺ Restore Original Version</span></button>` : ''}
            ${car.customSlot ? `<button type="button" data-action="delete-custom-car" ${!canAct ? 'disabled' : ''}><span>🗑 Delete Custom Slot</span></button>` : ''}
            <button type="button" data-action="save-all" ${!canAct ? 'disabled' : ''}><span>💾 Save Changes</span><small>Ctrl+S</small></button>
          </div>
        </details>

        <details class="studio-menu">
          <summary>Edit ▾</summary>
          <div class="studio-menu-dropdown">
            <button type="button" data-project-history="undo" ${!canAct || !project?.canUndo ? 'disabled' : ''}><span>↶ Undo</span><small>Ctrl+Z</small></button>
            <button type="button" data-project-history="redo" ${!canAct || !project?.canRedo ? 'disabled' : ''}><span>↷ Redo</span><small>Ctrl+Y</small></button>
            <hr>
            <button type="button" data-open-project-changes><span>📦 Project Changes Outliner (${project?.changes?.length || 0})…</span></button>
            ${!car.customSlot && (car.rival || car.bank <= 72) ? `
              <hr>
              <button type="button" data-action="restore-original-car" ${!canAct ? 'disabled' : ''}><span>↺ Restore Original Version</span></button>
            ` : ''}
            ${car.customSlot ? `
              <hr>
              <button type="button" data-action="delete-custom-car" ${!canAct ? 'disabled' : ''}><span>🗑 Delete Custom Slot</span></button>
            ` : ''}
          </div>
        </details>

        <details class="studio-menu">
          <summary>View ▾</summary>
          <div class="studio-menu-dropdown">
            <button type="button" data-inline-part="whole"><span>● Whole Car Assembly (All 5 Parts)</span></button>
            <hr>
            ${parts.map(p => `<button type="button" data-inline-part="${esc(p.key)}"><span>Isolate: ${esc(carPartName(car, p.part))}</span></button>`).join('')}
            <hr>
            <button type="button" data-car-materials="${esc(inlineCarPart === 'whole' ? car.key : inlineCarPart)}"><span>🎨 Surface &amp; Texture Sheet…</span></button>
            <button type="button" data-toggle-inspector><span>${inspectorCollapsed ? 'Show Inspector Panel' : 'Hide Inspector Panel'}</span></button>
          </div>
        </details>
      </div>

      <div class="studio-menubar-center">
        <button type="button" class="studio-doc-pill" data-open-load-car title="Click to switch car (${allBodies.length} available slots, or use [ and ] keys)">
          <span class="studio-doc-dot ${isDirty ? 'dirty' : ''}"></span>
          <span>Slot #${car.globalSlot} · ${carGradeBadge(car)} · <b>${esc(car.name)}</b> ▾</span>
        </button>
        <button type="button" class="studio-btn" data-open-command-palette title="Open Quick Command Palette (Ctrl+K)">🔍 Ctrl+K</button>
      </div>

      <div class="studio-menubar-right">
        <div class="studio-race-launcher" title="${car.rival ? 'Rival car model belongs to this specific course pack' : 'Choose track and launch game directly into a race with this car'}">
          <span>Track:</span>
          <select data-test-race-track ${car.rival ? 'disabled' : ''}>
            ${trackOptions.map(t => `<option value="${t.bank}" ${t.bank === (car.rival ? car.bank : testTrackBank) ? 'selected' : ''}>${esc(t.label)}</option>`).join('')}
          </select>
          <button type="button" class="studio-race-btn" data-action="test-race" ${!canAct ? 'disabled' : ''}>🏁 Start Race</button>
        </div>
        ${!car.customSlot && (car.rival || car.bank <= 72) ? `
          <button type="button" class="studio-btn" data-action="restore-original-car" ${!canAct ? 'disabled' : ''} title="Restore original version of this car from the game disc">↺ Restore Original Version</button>
        ` : ''}
        ${car.customSlot ? `
          <button type="button" class="studio-btn" data-action="delete-custom-car" ${!canAct ? 'disabled' : ''} style="border-color:rgba(248,113,113,0.45);color:#fca5a5" title="Delete this custom expansion car slot">🗑 Delete Custom Slot</button>
        ` : ''}
        <button type="button" class="studio-btn primary" data-action="save-all" ${!canAct ? 'disabled' : ''} title="Save car parameters and apply project changes to game (Ctrl+S)">💾 Save${isDirty ? ' *' : ''}</button>
      </div>
    </header>

    <!-- 2. MAIN WORKSPACE (Left Part Picker + Full-Screen 3D Viewport + Right Accordion Inspector) -->
    <div class="studio-workspace has-partdock ${inspectorCollapsed ? 'inspector-collapsed' : ''}">
      <!-- Left Vertical Part Picker: Full Names for 3D Mesh Parts & Textures -->
      <aside class="studio-tooldock studio-partdock" aria-label="3D Viewport Part Tools">
        <div class="studio-partdock-title">3D Mesh Parts</div>
        <button type="button" class="studio-tool-btn ${inlineCarPart === 'whole' ? 'active' : ''}" data-inline-part="whole" title="Show Complete 5-Part Car Assembly">
          <span>🚗 Complete Car (${parts.length})</span>
        </button>
        <div class="studio-tool-sep"></div>
        ${parts.map((p, idx) => `
          <button type="button" class="studio-tool-btn ${inlineCarPart === p.key ? 'active' : ''}" data-inline-part="${esc(p.key)}" title="Isolate ${esc(carPartName(car, p.part))}">
            <span>${idx === 2 || idx === 3 ? '🛞' : '🔧'} ${esc(carPartName(car, p.part))}</span>
          </button>`).join('')}
        <div class="studio-tool-sep"></div>
        <div class="studio-partdock-title">Materials</div>
        <button type="button" class="studio-tool-btn" data-car-materials="${esc(inlineCarPart === 'whole' ? car.key : inlineCarPart)}" title="Inspect & Replace Car Textures / Surfaces">
          <span>🎨 Textures &amp; Surfaces</span>
        </button>
      </aside>

      <!-- Full-Screen 3D Viewport -->
      <main class="studio-viewport">
        <!-- Row 1: Top Viewport Bar -->
        <div class="studio-hud-top">
          <div class="studio-hud-top-left">
            <span class="car-slot-badge">${carGradeBadge(car)}</span>
            <strong style="font-size:12px">Slot #${car.globalSlot} · ${esc(car.name)}</strong>
            <small style="color:#94a3b8">· ${inlineCarPart === 'whole' ? (car.original?'Original disc model':'Complete 5-Part Assembly') : esc(carPartName(car, parts.find(p => p.key === inlineCarPart)?.part ?? 0))}</small>
          </div>
          <div class="studio-hud-top-right">
            ${inlineCarPart !== 'whole' ? `
              <button type="button" class="studio-btn" data-import-car="${esc(inlineCarPart)}">⬆ Replace Part Mesh…</button>
              <button type="button" class="studio-btn" data-export-car="${esc(inlineCarPart)}" data-format="glb">⬇ Export Part .GLB</button>
            ` : ''}
            <button type="button" class="studio-btn" data-toggle-inspector>${inspectorCollapsed ? '◀ Inspector' : 'Inspector ▶'}</button>
          </div>
        </div>

        <!-- Row 2: Center 3D WebGL Canvas Host -->
        <div id="car-studio-viewport" class="studio-stage-canvas" data-inline-car="${esc(car.key)}" data-inline-bank="${car.bank}" data-inline-body="${car.part}" data-inline-rival="${car.rival}"></div>
      </main>

      <!-- Right Collapsible Accordion Inspector -->
      <aside class="studio-inspector" ${inspectorCollapsed ? 'hidden' : ''} aria-label="Car Parameters Inspector">
        <div class="studio-inspector-header">
          <span>CAR INSPECTOR</span>
          <button type="button" class="studio-btn" data-collapse-all-accordions style="padding:3px 8px;font-size:10px">Collapse / Expand</button>
        </div>
        ${renderAccordionInspector(car, parts, spec)}
      </aside>
    </div>

    <!-- 3. BOTTOM STATUS BAR -->
    <footer class="studio-statusbar">
      <span style="display:inline-flex;align-items:center;gap:8px">
        <span>${state.busy && !previewBusy ? `Busy: ${esc(state.busy)}` : `Ready · ${esc(state.disc?.region || '')} · ${allBodies.length} Car Slots`}</span>
        <button type="button" class="studio-status-edits-btn" data-open-project-changes title="Open Project Changes Outliner to inspect or revert modified cars & tracks">📦 ${project?.changes?.length || 0} Project Edit(s)</button>
        <span>· ${isDirty ? 'Unsaved changes' : 'All changes saved'}</span>
      </span>
      <span>[ / ] Prev/Next Car · Ctrl+K Palette · Orbit / Shift+Pan / Wheel Zoom</span>
    </footer>`;

  const nextViewport = $('#car-studio-viewport');
  if (nextViewport && prevViewState) {
    nextViewport._savedView = prevViewState;
  }
  const nextKey = `${car.key}:${inlineCarPart}:${project?.revision ?? 0}`;
  if (prevViewport && nextViewport && (activePreviewKey === nextKey || loadingPreviewKey === nextKey)) {
    nextViewport.replaceWith(prevViewport);
  } else {
    activePreviewKey = '';
    loadingPreviewKey = '';
    if (!car.rival && currentCarSpecBank !== car.bank) {
      ensureCarSpec().then(() => {
        if (currentCarSpecBank === car.bank) render();
      });
      return;
    }
    mountCarViewport();
  }
}

async function stepCarSlot(delta) {
  const {allBodies, car} = resolveCatalogBodies();
  if (!allBodies.length || !car) return;
  const idx = allBodies.findIndex(c => c.key === car.key);
  const nextIdx = (idx + delta + allBodies.length) % allBodies.length;
  const picked = allBodies[nextIdx];
  if (!picked) return;
  selectedCar = picked.key;
  inlineCarPart = 'whole';
  formDirty = false;
  if (picked.rival) testTrackBank = picked.bank;
  fitmentDraft = getProjectFitment(picked);
  await ensureCarSpec();
  render();
}

document.addEventListener('keydown', e => {
  if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 's') {
    e.preventDefault();
    saveAndApplyAll().then(ok => { if (ok) toast('All car changes saved and applied to game'); }).catch(showError);
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'o') {
    e.preventDefault();
    openLoadCarModal();
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'n') {
    e.preventDefault();
    openNewCarModal();
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'k') {
    e.preventDefault();
    openCommandPaletteModal('');
  } else if (e.key === 'Enter' && e.target.id === 'studio-command-palette-input') {
    e.preventDefault();
    const first = $('#studio-command-palette-list .studio-palette-item');
    if (first) first.click();
  } else if (!e.metaKey && !e.ctrlKey && !e.altKey && (e.key === '[' || e.key === ']')) {
    const tag = e.target?.tagName;
    if (tag === 'INPUT' || tag === 'SELECT' || tag === 'TEXTAREA' || e.target?.isContentEditable || $('#dialog')?.open) return;
    e.preventDefault();
    stepCarSlot(e.key === ']' ? 1 : -1).catch(showError);
  }
});

// Interactive Direct-Manipulation Torque Curve SVG Node Dragging & Horizontal Label Scrubbing
let activeTorqueDrag = null;
let activeLabelScrub = null;

document.addEventListener('pointerdown', e => {
  const node = e.target.closest?.('circle[data-torque-node]');
  if (node) {
    const svg = node.closest('#studio-torque-svg');
    if (!svg) return;
    e.preventDefault();
    node.classList.add('dragging');
    node.setPointerCapture?.(e.pointerId);
    activeTorqueDrag = {
      pointerId: e.pointerId,
      index: Number(node.dataset.torqueNode),
      svg,
      node
    };
    return;
  }

  const label = e.target.closest?.('label.studio-scrub-label');
  if (label && !e.target.closest('button')) {
    const forId = label.getAttribute('for');
    const input = forId ? document.getElementById(forId) : label.parentElement?.querySelector('input[type="number"]');
    if (input && input.type === 'number' && !input.disabled) {
      e.preventDefault();
      label.setPointerCapture?.(e.pointerId);
      const stepAttr = Number(input.step);
      const step = Number.isFinite(stepAttr) && stepAttr > 0 ? stepAttr : 1;
      activeLabelScrub = {
        pointerId: e.pointerId,
        input,
        startX: e.clientX,
        startVal: Number(input.value) || 0,
        step,
        min: input.min !== '' ? Number(input.min) : -Infinity,
        max: input.max !== '' ? Number(input.max) : Infinity
      };
    }
  }
});

document.addEventListener('pointermove', e => {
  if (activeTorqueDrag && e.pointerId === activeTorqueDrag.pointerId) {
    const {svg, index} = activeTorqueDrag;
    const rect = svg.getBoundingClientRect();
    if (!rect.width || !rect.height) return;
    const w = Number(svg.dataset.w) || 320;
    const h = Number(svg.dataset.h) || 132;
    const padL = Number(svg.dataset.padL) || 32;
    const padR = Number(svg.dataset.padR) || 10;
    const padT = Number(svg.dataset.padT) || 14;
    const padB = Number(svg.dataset.padB) || 22;
    const maxRpm = Number(svg.dataset.maxRpm) || 9000;
    const maxTorque = Number(svg.dataset.maxTorque) || 220;
    const plotW = w - padL - padR;
    const plotH = h - padT - padB;
    const svgX = ((e.clientX - rect.left) / rect.width) * w;
    const svgY = ((e.clientY - rect.top) / rect.height) * h;

    const normY = Math.max(0, Math.min(1, 1 - (svgY - padT) / plotH));
    const torqueInput = document.getElementById(`spec-torqueCurve${index}`);
    if (torqueInput) {
      const tMax = torqueInput.max !== '' ? Number(torqueInput.max) : 255;
      const tMin = torqueInput.min !== '' ? Number(torqueInput.min) : 0;
      const newTorque = Math.max(tMin, Math.min(tMax, Math.round(normY * maxTorque)));
      torqueInput.value = String(newTorque);
      torqueInput.dispatchEvent(new Event('input', {bubbles: true}));
    }

    if (e.shiftKey) {
      const normX = Math.max(0, Math.min(1, (svgX - padL) / plotW));
      const rpmInput = document.getElementById(`spec-torqueBand${index}`);
      if (rpmInput) {
        const rMax = rpmInput.max !== '' ? Number(rpmInput.max) : 30000;
        const rMin = rpmInput.min !== '' ? Number(rpmInput.min) : 0;
        const newRpm = Math.max(rMin, Math.min(rMax, Math.round((normX * maxRpm) / 50) * 50));
        rpmInput.value = String(newRpm);
        rpmInput.dispatchEvent(new Event('input', {bubbles: true}));
      }
    }
    return;
  }

  if (activeLabelScrub && e.pointerId === activeLabelScrub.pointerId) {
    const {input, startX, startVal, step, min, max} = activeLabelScrub;
    const dx = e.clientX - startX;
    const mult = e.shiftKey ? 10 : (e.altKey ? 0.2 : 1);
    const raw = startVal + Math.round(dx / 3) * step * mult;
    const clamped = Math.max(min, Math.min(max, raw));
    const formatted = step < 1 ? clamped.toFixed(2) : String(Math.round(clamped));
    if (input.value !== formatted) {
      input.value = formatted;
      input.dispatchEvent(new Event('input', {bubbles: true}));
    }
  }
});

const endPointerInteraction = e => {
  if (activeTorqueDrag && e.pointerId === activeTorqueDrag.pointerId) {
    activeTorqueDrag.node?.classList?.remove('dragging');
    activeTorqueDrag = null;
  }
  if (activeLabelScrub && e.pointerId === activeLabelScrub.pointerId) {
    activeLabelScrub = null;
  }
};
document.addEventListener('pointerup', endPointerInteraction);
document.addEventListener('pointercancel', endPointerInteraction);

document.addEventListener('toggle', e => {
  const menu = e.target.closest?.('details.studio-menu');
  if (menu && menu.open) {
    for (const other of document.querySelectorAll('details.studio-menu[open]')) {
      if (other !== menu) other.open = false;
    }
    return;
  }
  const acc = e.target.closest?.('details.studio-accordion');
  if (acc && acc.dataset.accordion && acc.isConnected) {
    if (acc.open) openAccordions.add(acc.dataset.accordion);
    else openAccordions.delete(acc.dataset.accordion);
  }
}, true);

document.addEventListener('click', e => {
  const clickedMenu = e.target.closest('details.studio-menu');
  for (const other of document.querySelectorAll('details.studio-menu[open]')) {
    if (other !== clickedMenu) other.open = false;
  }
  const b = e.target.closest('button, [data-pick-car]');
  if (!b || b.type === 'reset') return;
  if (b.tagName === 'BUTTON' && (!b.type || b.type === 'submit')) {
    if (b.closest('form') && !b.dataset.action && !b.dataset.resetField && !b.dataset.fitmentReset && !b.dataset.fitmentMasterReset) return;
  }
  e.preventDefault();
  closeMenus();

  (async () => {
    try {
      if (b.dataset.action === 'close') { $('#dialog').close(); return; }
      if (b.dataset.action === 'choose-game') { await request('chooseGame'); return; }
      if (b.dataset.openProjectChanges !== undefined) { openProjectChangesModal(); return; }
      if (b.dataset.openCommandPalette !== undefined) { openCommandPaletteModal(''); return; }
      if (b.dataset.action === 'save-all') {
        if ($('#dialog')?.open) $('#dialog').close();
        if (await saveAndApplyAll()) toast('All car changes saved and applied to game');
        return;
      }
      if (b.dataset.action === 'delete-custom-car' || b.dataset.deleteCarKey) {
        const {allBodies, car: activeCar} = resolveCatalogBodies();
        const target = b.dataset.deleteCarKey
          ? allBodies.find(c => c.key === b.dataset.deleteCarKey)
          : activeCar;
        if (!target || !target.customSlot) return;
        const deletedName = target.name;
        const wasActive = activeCar && activeCar.key === target.key;
        project = await request('deleteCustomCar', target.bank, project?.revision);
        carParts = (await request('cars')).parts;
        currentCarSpecBank = -1;
        activePreviewKey = '';
        loadingPreviewKey = '';
        formDirty = false;
        if (wasActive) {
          const nextBodies = resolveCatalogBodies();
          selectedCar = nextBodies.playerBodies[0]?.key || '';
        }
        syncFitmentFromProject();
        await ensureCarSpec();
        if (b.dataset.deleteCarKey && $('#dialog')?.open) {
          if ($('#dialog').querySelector('.studio-outliner-card')) openProjectChangesModal();
          else openLoadCarModal();
        }
        render();
        toast(`Deleted custom car slot ${deletedName}`);
        return;
      }
      if (b.dataset.action === 'restore-original-car' || b.dataset.restoreCarKey) {
        const {allBodies, car: activeCar} = resolveCatalogBodies();
        const target = b.dataset.restoreCarKey
          ? allBodies.find(c => c.key === b.dataset.restoreCarKey)
          : activeCar;
        if (!target || target.customSlot || (!target.rival && target.bank > 72)) return;
        project = await request('restoreOriginalCar', target.bank, target.rival ? target.part : 0, Boolean(target.rival), project?.revision);
        carParts = (await request('cars')).parts;
        currentCarSpecBank = -1;
        activePreviewKey = '';
        loadingPreviewKey = '';
        formDirty = false;
        syncFitmentFromProject();
        await ensureCarSpec();
        if (b.dataset.restoreCarKey && $('#dialog')?.open) {
          if ($('#dialog').querySelector('.studio-outliner-card')) openProjectChangesModal();
          else openLoadCarModal();
        }
        render();
        toast(`Restored original disc version of ${target.name}`);
        return;
      }
      if (b.dataset.action === 'copy-fitment-from') {
        const srcKey = $('#copy-fitment-source')?.value;
        if (!srcKey) return;
        const {allBodies} = resolveCatalogBodies();
        const donor = allBodies.find(c => c.key === srcKey);
        if (!donor) return;
        fitmentDraft = getProjectFitment(donor);
        syncFitmentInputsFromDraft();
        applyFitmentToViewport();
        const saveBtn = document.querySelector('[data-action="save-all"].primary');
        if (saveBtn) saveBtn.textContent = '💾 Save *';
        toast(`Copied wheel & body fitment from ${donor.name}`);
        return;
      }
      if (b.dataset.action === 'copy-physics-from') {
        const donorBank = Number($('#copy-physics-source')?.value);
        if (!donorBank) return;
        const {car, playerBodies} = resolveCatalogBodies();
        if (!car || car.rival) return;
        const donor = playerBodies.find(c => c.bank === donorBank);
        const donorSpec = await request('carSpec', donorBank);
        const identityKeys = new Set(['name', 'manufacturer', 'className', 'unlockClass', 'price', 'upgradePrice']);
        const edits = {};
        for (const f of donorSpec?.fields || []) {
          if (identityKeys.has(f.key)) continue;
          edits[f.key] = f.value;
        }
        project = await request('saveCarSpec', car.bank, edits, project?.revision ?? 0);
        currentCarSpecBank = -1;
        await ensureCarSpec();
        render();
        toast(`Copied Engine, Gearbox & Handling setup from ${donor?.name || 'Bank ' + donorBank}`);
        return;
      }
      if (b.dataset.action === 'apply-fitment') {
        const {car} = resolveCatalogBodies();
        if (!car) return;
        if (await saveAndApplyAll()) {
          toast('Wheel & body fitment saved and applied to game');
        }
        return;
      }
      if (b.dataset.action === 'reset-fitment') {
        resetFitmentDraft();
        applyFitmentToViewport();
        render();
        return;
      }
      if (b.dataset.fitmentMasterReset) {
        const mKey = b.dataset.fitmentMasterReset;
        if (mKey === 'trackWidth') { fitmentDraft.frontWidth = 0; fitmentDraft.rearWidth = 0; }
        else if (mKey === 'wheelbase') { fitmentDraft.frontZ = 0; fitmentDraft.rearZ = 0; }
        else if (mKey === 'wheelY') { fitmentDraft.frontY = 0; fitmentDraft.rearY = 0; }
        else if (mKey === 'wheelScale') { fitmentDraft.frontScale = 100; fitmentDraft.rearScale = 100; }
        syncFitmentInputsFromDraft();
        applyFitmentToViewport();
        const saveBtn = document.querySelector('[data-action="save-all"].primary');
        if (saveBtn) saveBtn.textContent = '💾 Save *';
        return;
      }
      if (b.dataset.fitmentReset) {
        const key = b.dataset.fitmentReset;
        if (key in DEFAULT_FITMENT) {
          fitmentDraft[key] = DEFAULT_FITMENT[key];
          syncFitmentInputsFromDraft();
          applyFitmentToViewport();
          const saveBtn = document.querySelector('[data-action="save-all"].primary');
          if (saveBtn) saveBtn.textContent = '💾 Save *';
        }
        return;
      }
      if (b.dataset.action === 'test-race') {
        if ($('#dialog')?.open) $('#dialog').close();
        const {car} = resolveCatalogBodies();
        if (!car) return;
        if (formDirty || project?.dirty || hasPendingFitment()) {
          const ok = await saveAndApplyAll();
          if (!ok) return;
        }
        toast(`Launching race with ${car.name}…`);
        await request('play', {
          carBank: car.bank,
          rival: Boolean(car.rival),
          rivalBody: car.rival ? car.part : undefined,
          trackBank: car.rival ? car.bank : testTrackBank,
          mode: 'time-attack'
        });
        return;
      }
      if (b.dataset.openStudio) {
        if ($('#dialog')?.open) $('#dialog').close();
        await cancelPreview();
        await request('openStudio', b.dataset.openStudio);
        return;
      }
      if (b.dataset.toggleInspector !== undefined) { inspectorCollapsed = !inspectorCollapsed; render(); return; }
      if (b.dataset.collapseAllAccordions !== undefined) {
        if (openAccordions.size > 0) openAccordions.clear();
        else ['fitment', 'identity', 'engine', 'gearbox', 'handling'].forEach(k => openAccordions.add(k));
        render();
        return;
      }
      if (b.dataset.openLoadCar !== undefined) { openLoadCarModal(); return; }
      if (b.dataset.openNewCar !== undefined) { openNewCarModal(); return; }
      if (b.dataset.modalCarSection) {
        carSection = b.dataset.modalCarSection;
        openLoadCarModal();
        return;
      }
      if (b.dataset.modalGrade) {
        carGradeFilter = b.dataset.modalGrade;
        openLoadCarModal();
        return;
      }
      if (b.dataset.pickCar) {
        selectedCar = b.dataset.pickCar;
        inlineCarPart = 'whole';
        formDirty = false;
        const picked = resolveCatalogBodies().car;
        if (picked?.rival) testTrackBank = picked.bank;
        fitmentDraft = getProjectFitment(picked);
        $('#dialog').close();
        await ensureCarSpec();
        render();
        return;
      }
      if (b.dataset.inlinePart) {
        if ($('#dialog')?.open) $('#dialog').close();
        inlineCarPart = b.dataset.inlinePart;
        activePreviewKey = '';
        render();
        return;
      }
      if (b.dataset.resetField) {
        const input = b.closest('form')?.querySelector(`[data-spec="${b.dataset.resetField}"]`);
        if (input && b.dataset.baseValue !== undefined) {
          input.value = b.dataset.baseValue;
          input.dispatchEvent(new Event('input', {bubbles: true}));
        }
        return;
      }
      if (b.dataset.projectHistory) {
        project = await request('projectHistory', b.dataset.projectHistory, project.revision);
        carParts = (await request('cars')).parts;
        currentCarSpecBank = -1;
        activePreviewKey = '';
        formDirty = false;
        syncFitmentFromProject();
        await ensureCarSpec();
        if ($('#dialog')?.open && $('#dialog').querySelector('.studio-outliner-card')) {
          openProjectChangesModal();
        }
        render();
        return;
      }
      if (b.dataset.exportSet) {
        const folder = await request('exportCarSet', b.dataset.exportSet);
        if (folder) toast('Complete car exported to ' + folder);
        return;
      }
      if (b.dataset.importSet) {
        const result = await request('importCarSet', b.dataset.importSet, b.dataset.format);
        if (result?.mapping) {
          const dialog = $('#dialog'), token = result.mapping.token;
          dialog.innerHTML = carImportForm(result.mapping);
          dialog.addEventListener('close', () => { request('cancelCarImport', token).catch(() => {}); }, {once: true});
          dialog.showModal();
        } else if (result) {
          project = await request('project');
          syncFitmentFromProject();
          activePreviewKey = '';
          render();
          toast('3D Car imported into project — click Save to apply to game');
        }
        return;
      }
      if (b.dataset.previewImport !== undefined) {
        const form = b.closest('#car-import-form'), host = form.querySelector('[data-import-preview]');
        await mountModelPreview(host, await request('previewCarImport', form.dataset.token, Number(b.dataset.previewImport)), {embedded: true});
        return;
      }
      if (b.dataset.targetMaterial !== undefined) {
        const form = b.closest('#car-import-form'), select = form.querySelector('[data-import-material="' + b.dataset.targetMaterial + '"]');
        if (!select.value) { toast('Choose a target material first'); return; }
        const value = select.value, data = await request('carImportTexture', form.dataset.token, Number(value));
        if (!form.isConnected || select.value !== value) return;
        const host = form.querySelector('[data-target-swatch="' + b.dataset.targetMaterial + '"]');
        host.hidden = false;
        host.innerHTML = (data.png ? '<img style="width:96px;height:96px;object-fit:contain;image-rendering:pixelated" alt="' + esc(data.label) + '" src="data:image/png;base64,' + data.png + '">' : '<span style="display:inline-block;width:96px;height:96px;background:' + data.color + '"></span>') + '<small>' + esc(data.note) + '</small>';
        return;
      }
      if (b.dataset.importCar) {
        if (await request('importCar', b.dataset.importCar)) {
          project = await request('project');
          syncFitmentFromProject();
          activePreviewKey = '';
          render();
          toast('Part mesh imported into project');
        }
        return;
      }
      if (b.dataset.exportCar) {
        const folder = await request('exportCar', b.dataset.exportCar, b.dataset.format || 'obj');
        if (folder) toast('Part exported to ' + folder);
        return;
      }
      if (b.dataset.carMaterials) {
        if (previewPromise) await cancelPreview();
        let surfaces;
        for(let attempt=0;attempt<50;attempt++){try{surfaces=await request('carMaterials',b.dataset.carMaterials);break;}catch(error){if(!/Wait for the current operation|Another operation/.test(error.message))throw error;await new Promise(r=>setTimeout(r,100));}}
        if(!surfaces)throw Error('The editor is busy. Try opening surfaces again.');
        $('#dialog').innerHTML = surfaceDialog(surfaces);
        if (!$('#dialog').open) $('#dialog').showModal();
        return;
      }
      if (b.dataset.importTrackTexture !== undefined || b.dataset.exportTrackTexture !== undefined || b.dataset.resetTrackTexture !== undefined) {
        const host = b.closest('[data-track-surfaces]'), id = host.dataset.trackSurfaces;
        const source = b.dataset.importTrackTexture ?? b.dataset.exportTrackTexture ?? b.dataset.resetTrackTexture;
        if (b.dataset.exportTrackTexture !== undefined) {
          if (await request('exportCarTexture', id, source)) toast('Texture PNG exported');
          return;
        }
        const result = await request(b.dataset.importTrackTexture !== undefined ? 'importCarTexture' : 'resetCarTexture', id, source, Number(host.dataset.revision));
        if (result) {
          project = result;
          activePreviewKey = '';
          render();
          $('#dialog').innerHTML = surfaceDialog(await request('carMaterials', id));
          toast('Car texture updated in project');
        }
        return;
      }
    } catch (err) {
      showError(err);
    }
  })();
});

document.addEventListener('input', e => {
  if (e.target.id === 'studio-command-palette-input') {
    openCommandPaletteModal(e.target.value);
    return;
  }
  if (e.target.id === 'modal-car-search') {
    carSearch = e.target.value;
    openLoadCarModal();
    return;
  }
  const masterKey = e.target.dataset?.fitmentMasterRange || e.target.dataset?.fitmentMasterNum;
  if (masterKey) {
    const val = Number(e.target.value);
    if (Number.isFinite(val)) {
      if (masterKey === 'trackWidth') {
        fitmentDraft.frontWidth = val;
        fitmentDraft.rearWidth = val;
      } else if (masterKey === 'wheelbase') {
        fitmentDraft.frontZ = val;
        fitmentDraft.rearZ = -val;
      } else if (masterKey === 'wheelY') {
        fitmentDraft.frontY = val;
        fitmentDraft.rearY = val;
      } else if (masterKey === 'wheelScale') {
        fitmentDraft.frontScale = val;
        fitmentDraft.rearScale = val;
      }
      syncFitmentInputsFromDraft(e.target);
      if (inlineCarPart !== 'whole') {
        inlineCarPart = 'whole';
        activePreviewKey = '';
        render();
        return;
      }
      applyFitmentToViewport();
      const saveBtn = document.querySelector('[data-action="save-all"].primary');
      if (saveBtn) saveBtn.textContent = '💾 Save *';
      const dot = document.querySelector('.studio-doc-dot');
      if (dot) dot.classList.add('dirty');
    }
    return;
  }
  const fitKey = e.target.dataset?.fitmentRange || e.target.dataset?.fitmentNum;
  if (fitKey && fitKey in DEFAULT_FITMENT) {
    const val = Number(e.target.value);
    if (Number.isFinite(val)) {
      fitmentDraft[fitKey] = val;
      syncFitmentInputsFromDraft(e.target);
      if (inlineCarPart !== 'whole') {
        inlineCarPart = 'whole';
        activePreviewKey = '';
        render();
        return;
      }
      applyFitmentToViewport();
      const saveBtn = document.querySelector('[data-action="save-all"].primary');
      if (saveBtn) saveBtn.textContent = '💾 Save *';
      const dot = document.querySelector('.studio-doc-dot');
      if (dot) dot.classList.add('dirty');
    }
    return;
  }
  if (e.target.closest('#car-spec-form')) {
    formDirty = true;
    const saveBtn = document.querySelector('[data-action="save-all"].primary');
    if (saveBtn) saveBtn.textContent = '💾 Save *';
    const dot = document.querySelector('.studio-doc-dot');
    if (dot) dot.classList.add('dirty');
    const specKey = e.target.dataset?.spec || '';
    if (/^(torqueBand|torqueCurve|revLimit|redline)/.test(specKey) && currentCarSpec?.fields) {
      const wrap = $('#studio-torque-chart-wrap');
      if (wrap) {
        const dragIdx = activeTorqueDrag?.index;
        wrap.outerHTML = renderTorqueChart(currentCarSpec.fields);
        if (dragIdx !== undefined) {
          const newNode = document.querySelector(`#studio-torque-svg circle[data-torque-node="${dragIdx}"]`);
          const newSvg = $('#studio-torque-svg');
          if (newNode && newSvg && activeTorqueDrag) {
            newNode.classList.add('dragging');
            activeTorqueDrag.svg = newSvg;
            activeTorqueDrag.node = newNode;
          }
        }
      }
    } else if (/^(downshift|upshift|gearRatio|topGear)/.test(specKey) && currentCarSpec?.fields) {
      const gWrap = $('#studio-gearbox-chart-wrap');
      if (gWrap) gWrap.outerHTML = renderGearboxChart(currentCarSpec.fields);
    }
  }
});

document.addEventListener('change', e => {
  if (e.target.matches('[data-test-race-track]')) {
    testTrackBank = Number(e.target.value) || 88;
    document.querySelectorAll('[data-test-race-track]').forEach(el => { el.value = String(testTrackBank); });
    return;
  }
  if (e.target.dataset.modalRival) {
    if (e.target.dataset.modalRival === 'class') rivalClassFilter = Number(e.target.value);
    else rivalCourseFilter = Number(e.target.value);
    openLoadCarModal();
  }
});

document.addEventListener('submit', e => {
  e.preventDefault();
  (async () => {
    try {
      if (e.target.id === 'new-car-form') {
        const fd = new FormData(e.target);
        const rawTarget = fd.get('targetBank');
        const targetBank = rawTarget === 'new' ? 'new' : Number(rawTarget);
        const sourceBank = Number(fd.get('sourceBank'));
        const options = {
          targetBank,
          sourceBank,
          name: String(fd.get('name') || '').trim(),
          manufacturer: String(fd.get('manufacturer') || '').trim(),
          className: String(fd.get('className') || '').trim(),
          unlockClass: Number(fd.get('unlockClass')),
          price: Number(fd.get('price')),
          upgradePrice: Number(fd.get('upgradePrice')),
        };
        const importGlbAfter = fd.get('importGlbAfter') === 'on';
        const res = await request('createNewCar', options, project?.revision ?? 0);
        project = res;
        const createdBank = res.createdBank ?? (targetBank === 'new' ? 200 : targetBank);
        carParts = (await request('cars')).parts;
        carSection = 'all';
        const created = carParts.find(p => !p.rival && p.bank === createdBank && p.part === 0);
        if (created) selectedCar = created.key;
        currentCarSpecBank = -1;
        formDirty = false;
        syncFitmentFromProject();
        await ensureCarSpec();
        $('#dialog').close();
        activePreviewKey = '';
        render();
        const slotNum = createdBank >= 200 ? 32 + (createdBank - 200) / 2 : (createdBank - 10) / 2;
        toast(`Created "${options.name}" in Slot #${slotNum}`);
        if (importGlbAfter && created) {
          const result = await request('importCarSet', created.key, 'glb');
          if (result?.mapping) {
            const dialog = $('#dialog'), token = result.mapping.token;
            dialog.innerHTML = carImportForm(result.mapping);
            dialog.addEventListener('close', () => { request('cancelCarImport', token).catch(() => {}); }, {once: true});
            dialog.showModal();
          } else if (result) {
            project = await request('project');
            syncFitmentFromProject();
            activePreviewKey = '';
            render();
          }
        }
        return;
      }
      if (e.target.id === 'car-spec-form') {
        if (await saveAndApplyAll()) toast('All car changes saved and applied to game');
        return;
      }
      if (e.target.id === 'car-import-form') {
        const form = e.target;
        if (!form.reportValidity()) return;
        project = await request('completeCarImport', form.dataset.token, {
          nodes: [...form.querySelectorAll('[data-import-node]')].map(n => ({node: Number(n.dataset.importNode), instance: Number(n.value)})),
          materials: [...form.querySelectorAll('[data-import-material]')].map(m => ({material: Number(m.dataset.importMaterial), word: Number(m.value)}))
        });
        syncFitmentFromProject();
        activePreviewKey = '';
        $('#dialog').close();
        render();
        toast('3D Car imported into project — click Save to apply to game');
      }
    } catch (err) {
      showError(err);
    }
  })();
});

api.onState(async value => {
  state = previewBusy ? {...value, busy: null} : value;
  project = value.project;
  if (booting || savingBusy) return;
  if (value.mcpUpdated && state.ready) {
    carParts = (await request('cars')).parts;
    currentCarSpec = null;
    await ensureCarSpec();
    syncFitmentFromProject();
    activePreviewKey = '';
  }
  if (!formDirty && !hasPendingFitment()) render();
});

(async () => {
  try {
    state = await request('snapshot');
    project = state.project;
    if (state.ready) {
      carParts = (await request('cars')).parts;
      project = await request('project');
      syncFitmentFromProject();
      await ensureCarSpec();
    }
  } catch (err) {
    showError(err);
  } finally {
    booting = false;
    render();
  }
})();

installMaterialEditor({request,onError:showError,onBegin:async()=>{savingBusy=true;await cancelPreview();},onEnd:()=>{savingBusy=false;render();},onChanged:async p=>{project=p;activePreviewKey='';render();}});
