import {installMaterialEditor} from './material-editor.mjs';
import {
  recalculateRoadGeometry,
  deformRoadPoints,
  shapeRoadSection,
  diffRoadDraft,
  build3dTrackView,
  surfaceDialog
} from './tracks.mjs';
import {mountModelPreview} from './model-preview.js?v=17';

const $ = s => document.querySelector(s);
const api = window.launcher;
const esc = v => String(v ?? '').replace(/[&<>"']/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));

let state = null;
let project = null;
let trackList = [];
let trackBank = 88;
let trackScene = null;
let trackPoint = 0;
let trackObject = 0;
let terrainCell = 0;
let terrainBrushMode = false;
let terrainBrushModel = 0;
let aiSeries = 0; // 0 = Forward, 1 = Reverse
let aiHintIndex = 0;
let aiSpeedIndex = 0;
let startSlotIndex = 0;
let cameraTable = 0;
let cameraIndex = 0;
let introIndex = 0;
let envCueIndex = 0;
let stageView = '3d'; // Default to '3d' so the user immediately sees the 3D Track & Terrain models! ('2d' | '3d' | 'tile3d')
let activeTool = 'move'; // 'select' | 'move' | 'terrain' | 'scenery'
let roadRadius = 6;
let roadDraft = null;
let trackLayers = {road: true, terrain: true, course: true, ai: true, markers: true, isolate: false};
let inspectorCollapsed = false;
let mapViewBox = [0, 0, 1000, 1000];
let mapViewBank = 88;
let cachedTrackPreview = null;
let cachedTrackPreviewKey = '';
let active3dKey = '';
let loading3dKey = '';
let saved3dViewState = {key: '', state: null};
let previewToken = 0;
let previewBusy = false;
let materialBusy = false;
let booting = true;
let previewPromise = null;
let modalDirty = false;
let toastTimer = null;
let testCarBank = 10; // Default test car (Slot #0 · ERRISO G1)
let playerCarOptions = [];
let selected3dEntity = null;
const openAccordions = new Set(['terrain', 'road', 'scenery', 'scenery-models']);

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
  dialog.innerHTML = `<h2>Could not complete the operation</h2><p>${esc(e.message || e)}</p><footer><button type="button" class="studio-btn" data-action="close">Close</button></footer>`;
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
  if (token === previewToken) {previewBusy = false; loading3dKey = '';}
}

function trackAssetPrefix(bank) {
  if (bank >= 136) return `track.custom${(bank - 136) / 2 + 1}`;
  return `track.${['big', 'mid', 'hi', 'oval'][((bank - 88) / 2) % 4]}${Math.floor((bank - 88) / 8) + 1}`;
}

function trackGradeBadge(t) {
  if (!t) return 'Class 1';
  if (t.origin === 'custom' || t.grade === 'Custom') {
    return t.slot ? `Custom #${t.slot}` : 'Custom';
  }
  return `Class ${t.grade}`;
}

function getTerrainCellModel(cells, idx) {
  const raw = cells?.[idx];
  if (raw === undefined || raw === null) return 1023;
  if (typeof raw === 'number') return raw & 1023;
  return Number(raw.model ?? raw.tile ?? 1023) & 1023;
}

function getTerrainModelsCount(scene) {
  return Number(scene?.terrain?.models ?? scene?.terrainModels ?? 0);
}

function findNearestPopulatedTerrainCell(scene, targetPt) {
  const cells = scene?.terrain?.cells || [];
  if (!cells.length) return 0;
  const col0 = Math.max(0, Math.min(31, Math.floor((Number(targetPt?.x) || 32768) / 2048)));
  const zr0 = Math.max(0, Math.min(31, Math.floor((Number(targetPt?.z) || 32768) / 2048)));
  const row0 = 31 - zr0;
  const exactIdx = row0 * 32 + col0;
  if (getTerrainCellModel(cells, exactIdx) !== 1023) return exactIdx;

  let bestIdx = exactIdx;
  let bestDist = Infinity;
  for (let idx = 0; idx < cells.length; idx++) {
    if (getTerrainCellModel(cells, idx) === 1023) continue;
    const c = idx & 31;
    const r = idx >> 5;
    const d = Math.hypot(c - col0, r - row0);
    if (d < bestDist) {
      bestDist = d;
      bestIdx = idx;
    }
  }
  return bestIdx;
}

function ensureRoadDraft() {
  if (!trackScene?.road) return null;
  if (!roadDraft) {
    roadDraft = {
      points: trackScene.road.points.map(p => ({...p})),
      arcs: (trackScene.road.arcs || []).map(a => ({...a}))
    };
  }
  return roadDraft;
}

function track3dKey(bank) {
  if (stageView === 'tile3d') {
    const model = getTerrainCellModel(trackScene?.terrain?.cells, terrainCell);
    return `tile3d:${bank}:${trackScene?.revision ?? 0}:${terrainCell}:${model}`;
  }
  const ptsLen = (roadDraft?.points || trackScene?.road?.points || []).length;
  return `3d:${bank}:${trackScene?.revision ?? 0}:${ptsLen}:${trackLayers.road ? 1 : 0}:${trackLayers.terrain ? 1 : 0}:${trackLayers.course ? 1 : 0}:${trackLayers.isolate ? `iso-${trackPoint}-${roadRadius}` : 'full'}`;
}

function computeTrackBounds(points, objects, terrain) {
  const xs = [...points.map(p => p.x), ...objects.map(o => o.x)];
  const zs = [...points.map(p => p.z), ...objects.map(o => o.z)];
  if (terrain?.cells?.length === 1024) {
    for (let idx = 0; idx < 1024; idx++) {
      if (getTerrainCellModel(terrain.cells, idx) === 1023) continue;
      const col = idx & 31;
      const row = idx >> 5;
      const zr = 31 - row;
      xs.push(col * 2048, (col + 1) * 2048);
      zs.push(zr * 2048, (zr + 1) * 2048);
    }
  }
  const minX = xs.length ? Math.min(...xs) : 0;
  const maxX = xs.length ? Math.max(...xs) : 65536;
  const minZ = zs.length ? Math.min(...zs) : 0;
  const maxZ = zs.length ? Math.max(...zs) : 65536;
  const pad = 45;
  const size = 1000;
  const scale = (size - pad * 2) / Math.max(maxX - minX, maxZ - minZ, 1);
  return {minX, maxX, minZ, maxZ, pad, size, scale};
}

function renderFullViewportSvg() {
  if (!trackScene?.road?.points?.length) {
    return `<div style="color:#94a3b8;padding:24px">No road data in this track scene.</div>`;
  }
  const basePoints = trackScene.road.points;
  const points = roadDraft?.points || basePoints;
  const arcs = roadDraft?.arcs || trackScene.road.arcs || [];
  const visibleObjects = (trackScene.objects || []).map((o, i) => ({...o, i})).filter(o => o.model >= 0);
  const bounds = computeTrackBounds(basePoints, visibleObjects, trackScene.terrain);
  const {minX, maxX, minZ, maxZ, pad, scale} = bounds;
  const sx = x => (pad + (x - minX) * scale).toFixed(1);
  const sy = z => (pad + (maxZ - z) * scale).toFixed(1);
  const n = points.length;
  const sel = Math.min(Math.max(0, trackPoint), n - 1);

  const inNeighborhood = idx => {
    const d = Math.min(Math.abs(idx - sel), n - Math.abs(idx - sel));
    return d <= roadRadius;
  };

  // Precompute road normals for width & AI lateral offset rendering
  const normals = [];
  for (let i = 0; i < n; i++) {
    const prev = points[(i - 1 + n) % n];
    const next = points[(i + 1) % n];
    const dx = next.x - prev.x, dz = next.z - prev.z;
    const len = Math.hypot(dx, dz) || 1;
    normals.push([-dz / len, dx / len]);
  }

  // 1. 32x32 Terrain Segments Grid Overlay in exact world coordinates (col * 2048, (31 - row) * 2048)
  let terrainGridSvg = '';
  if (trackLayers.terrain && trackScene.terrain?.cells?.length === 1024) {
    const cells = trackScene.terrain.cells;
    const tiles = [];
    for (let r = 0; r < 32; r++) {
      for (let c = 0; c < 32; c++) {
        const idx = r * 32 + c;
        const model = getTerrainCellModel(cells, idx);
        const hasMesh = model !== 1023;
        const isSelTile = idx === terrainCell;
        if (!hasMesh && !isSelTile && activeTool !== 'terrain') continue;
        const zr = 31 - r;
        const x0 = Number(sx(c * 2048));
        const y0 = Number(sy(zr * 2048));
        const x1 = Number(sx((c + 1) * 2048));
        const y1 = Number(sy((zr + 1) * 2048));
        const tx = Math.min(x0, x1);
        const ty = Math.min(y0, y1);
        const tw = Math.max(2, Math.abs(x1 - x0));
        const th = Math.max(2, Math.abs(y1 - y0));
        const fill = isSelTile
          ? 'rgba(56,189,248,0.28)'
          : hasMesh
            ? 'rgba(34,197,94,0.12)'
            : 'rgba(255,255,255,0.02)';
        const stroke = isSelTile
          ? '#38bdf8'
          : hasMesh
            ? 'rgba(34,197,94,0.42)'
            : 'rgba(255,255,255,0.06)';
        const sw = isSelTile ? '2.2' : '0.9';
        tiles.push(
          `<g data-select-terrain-cell="${idx}" style="cursor:pointer">` +
          `<rect x="${tx.toFixed(1)}" y="${ty.toFixed(1)}" width="${tw.toFixed(1)}" height="${th.toFixed(1)}" fill="${fill}" stroke="${stroke}" stroke-width="${sw}"/>` +
          (hasMesh || isSelTile
            ? `<text x="${(tx + 3).toFixed(1)}" y="${(ty + 11).toFixed(1)}" fill="${isSelTile ? '#e0f2fe' : 'rgba(134,239,172,0.85)'}" font-size="8.5" font-weight="${isSelTile ? '700' : '600'}" pointer-events="none">[${c},${r}]${hasMesh ? `·M${model}` : ''}</text>`
            : '') +
          `<title>Terrain Segment [Col ${c}, Row ${r}] (Cell #${idx}) · ${hasMesh ? '3D Terrain Model #' + model : 'Empty (1023)'} — Click to select</title>` +
          `</g>`
        );
      }
    }
    terrainGridSvg = `<g class="terrain-grid-layer">${tiles.join('')}</g>`;
  }

  // 2. Road Ribbon Layer (True World Width L/R + Banking + Centerline + Nodes)
  let roadLayerSvg = '';
  if (trackLayers.road) {
    const leftEdge = [];
    const rightEdge = [];
    for (let i = 0; i < n; i++) {
      const p = points[i];
      const [nx, nz] = normals[i];
      const lw = Math.max(120, Number(p.leftWidth || 320));
      const rw = Math.max(120, Number(p.rightWidth || 320));
      leftEdge.push([p.x + nx * lw, p.z + nz * lw]);
      rightEdge.push([p.x - nx * rw, p.z - nz * rw]);
    }

    const fullQuads = [];
    const neighborQuads = [];
    for (let i = 0; i < n; i++) {
      const next = (i + 1) % n;
      const isNear = inNeighborhood(i) || inNeighborhood(next);
      if (trackLayers.isolate && !isNear) continue;
      const poly = `${sx(leftEdge[i][0])},${sy(leftEdge[i][1])} ${sx(leftEdge[next][0])},${sy(leftEdge[next][1])} ${sx(rightEdge[next][0])},${sy(rightEdge[next][1])} ${sx(rightEdge[i][0])},${sy(rightEdge[i][1])}`;
      if (isNear) {
        const slope = Number(points[i].crossSlope || 0);
        const fill = slope > 8 ? 'rgba(56,189,248,0.48)' : slope < -8 ? 'rgba(249,115,22,0.48)' : 'rgba(239,68,68,0.42)';
        neighborQuads.push(`<polygon points="${poly}" fill="${fill}" stroke="#fb923c" stroke-width="1.2"/>`);
      } else {
        fullQuads.push(`<polygon points="${poly}" fill="rgba(56,189,248,0.28)" stroke="rgba(56,189,248,0.65)" stroke-width="0.9"/>`);
      }
    }

    const closedBasePts = basePoints.length ? [...basePoints, basePoints[0]] : [];
    const closedPts = points.length ? [...points, points[0]] : [];
    const baseLine = !trackLayers.isolate
      ? `<polyline fill="none" stroke="rgba(148,163,184,0.4)" stroke-width="2" stroke-dasharray="4 4" points="${closedBasePts.map(p => `${sx(p.x)},${sy(p.z)}`).join(' ')}"/>`
      : '';
    const roadTrackCasing = !trackLayers.isolate
      ? `<polyline fill="none" stroke="rgba(15,23,42,0.85)" stroke-width="5.5" stroke-linejoin="round" stroke-linecap="round" points="${closedPts.map(p => `${sx(p.x)},${sy(p.z)}`).join(' ')}"/>` +
        `<polyline fill="none" stroke="#38bdf8" stroke-width="3.2" stroke-linejoin="round" stroke-linecap="round" opacity="0.92" points="${closedPts.map(p => `${sx(p.x)},${sy(p.z)}`).join(' ')}"/>`
      : '';
    const centerLine = !trackLayers.isolate
      ? `<polyline fill="none" stroke="#f8fafc" stroke-width="1.6" stroke-dasharray="6 5" opacity="0.95" points="${closedPts.map(p => `${sx(p.x)},${sy(p.z)}`).join(' ')}"/>`
      : '';

    const neighborPts = [];
    for (let off = -roadRadius; off <= roadRadius; off++) {
      const p = points[(sel + off + n) % n];
      neighborPts.push(`${sx(p.x)},${sy(p.z)}`);
    }
    const neighborLine = `<polyline fill="none" stroke="#f97316" stroke-width="3.4" points="${neighborPts.join(' ')}"/>`;

    const arcNodes = arcs.map((a, idx) =>
      `<circle data-drag-arc="${idx}" cx="${sx(a.x)}" cy="${sy(a.z)}" r="4.5" fill="#38bdf8" stroke="#0f172a" stroke-width="1.2" style="cursor:move">` +
      `<title>Curve Center #${idx + 1} (Drag to adjust curve center)</title></circle>`
    ).join('');

    const stride = n > 240 ? Math.ceil(n / 180) : 1;
    const nodes = points.map((p, i) => {
      const isSel = i === sel;
      const isNear = inNeighborhood(i);
      if (trackLayers.isolate && !isNear) return '';
      if (!isSel && !isNear && i % stride !== 0) return '';
      const r = isSel ? 8.5 : isNear ? 5.2 : 3.4;
      const fill = isSel ? '#ef4444' : isNear ? '#fb923c' : '#38bdf8';
      const stroke = isSel ? '#ffffff' : '#0f172a';
      return (
        `<circle data-track-point="${i}" cx="${sx(p.x)}" cy="${sy(p.z)}" r="${r}" fill="${fill}" stroke="${stroke}" stroke-width="${isSel ? 2.4 : 1.1}" style="cursor:pointer">` +
        `<title>Road Segment #${i + 1} (X:${p.x}, Y:${p.y}, Z:${p.z}) — Drag to reshape route</title></circle>` +
        (isSel ? `<text x="${(Number(sx(p.x)) + 11).toFixed(1)}" y="${(Number(sy(p.z)) + 4).toFixed(1)}" fill="#ffffff" font-size="11" font-weight="700" pointer-events="none">★ Seg #${i + 1}</text>` : '')
      );
    }).join('');

    roadLayerSvg = `<g class="road-ribbon-layer">${roadTrackCasing}${fullQuads.join('')}${baseLine}${centerLine}${neighborQuads.join('')}${neighborLine}${arcNodes}${nodes}</g>`;
  }

  // 3. AI Racing Line Overlay
  let aiLayerSvg = '';
  const hintsList = trackScene.ai?.hints?.[aiSeries];
  if (trackLayers.ai && Array.isArray(hintsList) && hintsList.length) {
    const biasBySeg = new Float32Array(n);
    for (const h of hintsList) {
      const s = Math.max(0, Math.min(n - 1, Number(h.start) || 0));
      const e = Math.max(s, Math.min(n - 1, Number(h.end) || s));
      const midOff = ((Number(h.min_offset) || 0) + (Number(h.max_offset) || 0)) * 0.5 + (Number(h.adjustment) || 0) * 2;
      for (let i = s; i <= e; i++) biasBySeg[i] = midOff * 4;
    }
    const aiPts = points.map((p, i) => {
      const [nx, nz] = normals[i];
      const off = biasBySeg[i] || 0;
      return `${sx(p.x + nx * off)},${sy(p.z + nz * off)}`;
    });
    const hintMarkers = hintsList.map((h, hIdx) => {
      const midSeg = Math.max(0, Math.min(n - 1, Math.round(((Number(h.start) || 0) + (Number(h.end) || 0)) / 2)));
      const p = points[midSeg];
      if (!p) return '';
      const [nx, nz] = normals[midSeg];
      const off = (((Number(h.min_offset) || 0) + (Number(h.max_offset) || 0)) * 0.5 + (Number(h.adjustment) || 0) * 2) * 4;
      const cx = sx(p.x + nx * off), cy = sy(p.z + nz * off);
      const isAct = hIdx === aiHintIndex;
      return `<circle data-select-ai-hint="${hIdx}" cx="${cx}" cy="${cy}" r="${isAct ? 6.5 : 3.8}" fill="${isAct ? '#10b981' : '#34d399'}" stroke="#064e3b" stroke-width="1.4" style="cursor:pointer"><title>AI Hint #${hIdx + 1} (Seg ${h.start}–${h.end}, Offset ${h.min_offset}..${h.max_offset}, Adj ${h.adjustment}) — Click to edit</title></circle>`;
    }).join('');
    aiLayerSvg = `<g class="ai-line-layer"><polyline fill="none" stroke="#10b981" stroke-width="2.2" stroke-dasharray="8 4" opacity="0.88" points="${aiPts.join(' ')}"/>${hintMarkers}</g>`;
  }

  // 4. Draggable 3D Scenery Objects Layer
  let sceneryLayerSvg = '';
  if (trackLayers.course) {
    const selPt = points[sel];
    const items = visibleObjects.map(o => {
      if (trackLayers.isolate && selPt && Math.hypot(o.x - selPt.x, o.z - selPt.z) > 8500) return '';
      const isSelObj = o.i === trackObject;
      const x = Number(sx(o.x)), y = Number(sy(o.z));
      const size = isSelObj ? 12 : 8;
      const fill = isSelObj ? 'rgba(250,204,21,0.35)' : 'rgba(192,132,252,0.28)';
      const stroke = isSelObj ? '#facc15' : '#c084fc';
      return (
        `<g data-map-object="${o.i}" transform="translate(${x.toFixed(1)},${y.toFixed(1)})" style="cursor:move">` +
        `<rect x="${-size / 2}" y="${-size / 2}" width="${size}" height="${size}" rx="2" fill="${fill}" stroke="${stroke}" stroke-width="${isSelObj ? 2 : 1.2}"/>` +
        (isSelObj ? `<text x="9" y="4" fill="#fef08a" font-size="10" font-weight="700" pointer-events="none">🌲 Obj #${o.i + 1} (M${o.model})</text>` : '') +
        `<title>Scenery Object #${o.i + 1} (Model ${o.model}) — Drag to move on map</title>` +
        `</g>`
      );
    }).join('');
    sceneryLayerSvg = `<g class="scenery-objects-layer">${items}</g>`;
  }

  // 5. Start Grid Slots (P1..P12) & TV Cameras Layer
  let markersLayerSvg = '';
  if (trackLayers.markers) {
    const startsList = trackScene.ai?.starts?.[aiSeries] || [];
    const startItems = startsList.map((st, sIdx) => {
      const isAct = sIdx === startSlotIndex;
      const x = Number(sx(st.x)), y = Number(sy(st.z));
      return (
        `<g data-map-start="${sIdx}" transform="translate(${x.toFixed(1)},${y.toFixed(1)})" style="cursor:move">` +
        `<circle r="${isAct ? 7.5 : 5}" fill="${isAct ? '#f43f5e' : '#fb7185'}" stroke="#fff" stroke-width="${isAct ? 2 : 1}"/>` +
        `<text x="8" y="3" fill="#fda4af" font-size="9" font-weight="700" pointer-events="none">P${sIdx + 1}</text>` +
        `<title>Start Grid P${sIdx + 1} (X:${st.x}, Z:${st.z}, Seg #${(st.point ?? 0) + 1}) — Drag on map to move</title>` +
        `</g>`
      );
    }).join('');
    const camsList = trackScene.cameras?.[cameraTable] || [];
    const camItems = camsList.map((cam, cIdx) => {
      const seg = Math.max(0, Math.min(n - 1, Number(cam.section) || 0));
      const pt = points[seg];
      if (!pt) return '';
      const ox = Number(cam.offset?.[0] || 0), oz = Number(cam.offset?.[2] || 0);
      const x = Number(sx(pt.x + ox)), y = Number(sy(pt.z + oz));
      const isAct = cIdx === cameraIndex;
      return (
        `<g data-map-camera="${cIdx}" transform="translate(${x.toFixed(1)},${y.toFixed(1)})" style="cursor:pointer">` +
        `<polygon points="0,-7 6,5 -6,5" fill="${isAct ? '#38bdf8' : '#0284c7'}" stroke="#e0f2fe" stroke-width="1.2"/>` +
        (isAct ? `<text x="8" y="3" fill="#bae6fd" font-size="9.5" font-weight="700" pointer-events="none">🎥 Cam #${cIdx + 1}</text>` : '') +
        `<title>TV Camera #${cIdx + 1} (Seg #${seg + 1}, Mode ${cam.mode}) — Click to inspect</title>` +
        `</g>`
      );
    }).join('');
    markersLayerSvg = `<g class="markers-layer">${camItems}${startItems}</g>`;
  }

  return `
    <svg id="studio-track-map" class="studio-track-svg" viewBox="${mapViewBox.map(v => v.toFixed(1)).join(' ')}"
      data-map-min-x="${minX}" data-map-max-x="${maxX}" data-map-min-z="${minZ}" data-map-max-z="${maxZ}" data-map-scale="${scale}" data-map-pad="${pad}"
      role="img" aria-label="Interactive full-screen track map">
      <defs>
        <pattern id="studioGrid" width="50" height="50" patternUnits="userSpaceOnUse">
          <path d="M 50 0 L 0 0 0 50" fill="none" stroke="rgba(255,255,255,0.04)" stroke-width="0.8"/>
        </pattern>
      </defs>
      <rect x="-2000" y="-2000" width="6000" height="6000" fill="url(#studioGrid)"/>
      ${terrainGridSvg}
      ${roadLayerSvg}
      ${aiLayerSvg}
      ${sceneryLayerSvg}
      ${markersLayerSvg}
    </svg>`;
}

function renderElevationMini(points, selectedIndex) {
  if (!points?.length) return '';
  const n = points.length;
  const sel = Math.min(Math.max(0, selectedIndex), n - 1);
  const ys = points.map(p => p.y);
  const rawMinY = Math.min(...ys);
  const rawMaxY = Math.max(...ys);
  const midY = (rawMinY + rawMaxY) / 2;
  const halfSpan = Math.max(60, (rawMaxY - rawMinY) * 0.6);
  const minY = Math.round(midY - halfSpan);
  const maxY = Math.round(midY + halfSpan);
  const spanY = Math.max(120, maxY - minY);
  const w = 310, h = 76, padX = 12, padY = 10;
  const plotW = w - padX * 2, plotH = h - padY * 2;
  const sx = i => (padX + ((n - 1 - i) / Math.max(1, n - 1)) * plotW).toFixed(1);
  // In PS1 GTE space, -Y is UP and +Y is DOWN, so smaller y belongs at the top (padY) of the SVG:
  const sy = y => (padY + ((y - minY) / spanY) * plotH).toFixed(1);
  const poly = points.map((p, i) => `${sx(i)},${sy(p.y)}`).join(' ');
  const cur = points[sel];
  const baseCurY = trackScene?.road?.points?.[sel]?.y ?? cur.y;
  const deltaElev = baseCurY - cur.y;
  const deltaStr = deltaElev === 0 ? '±0' : (deltaElev > 0 ? `+${deltaElev}` : `${deltaElev}`);
  return `
    <div style="background:#0d0f14;border:1px solid #262b35;border-radius:6px;padding:7px 9px">
      <div class="row between" style="margin-bottom:3px">
        <small style="color:#94a3b8;font-weight:700">ELEVATION (DRAG DOT · SHIFT=FINE)</small>
        <small style="color:#38bdf8">Elev: <b data-elev-label>${-cur.y} (${deltaStr})</b></small>
      </div>
      <svg id="studio-elev-svg" viewBox="0 0 ${w} ${h}" data-elev-min-y="${minY}" data-elev-max-y="${maxY}" style="width:100%;height:68px;display:block">
        <polyline points="${poly}" fill="none" stroke="#38bdf8" stroke-width="1.8"/>
        <circle data-elev-drag="${sel}" cx="${sx(sel)}" cy="${sy(cur.y)}" r="6" fill="#ef4444" stroke="#fff" stroke-width="1.8" style="cursor:ns-resize"/>
      </svg>
    </div>`;
}

function renderTrackAccordion() {
  const openAttr = id => openAccordions.has(id) ? ' open' : '';
  const prefix = trackAssetPrefix(trackBank);
  const basePoints = trackScene?.road?.points || [];
  const points = roadDraft?.points || basePoints;
  const n = points.length;
  const sel = Math.min(Math.max(0, trackPoint), Math.max(0, n - 1));
  const diff = roadDraft && trackScene?.road ? diffRoadDraft(trackScene.road, roadDraft) : {entries: [], pointCount: 0, arcCount: 0};

  // Terrain cell info
  const cells = trackScene?.terrain?.cells || [];
  const activeCell = Math.min(Math.max(0, terrainCell), Math.max(0, (cells.length || 1024) - 1));
  const cellModel = getTerrainCellModel(cells, activeCell);
  const cellCol = activeCell & 31;
  const cellRow = activeCell >> 5;
  const terrainModelsCount = getTerrainModelsCount(trackScene);
  const populatedTileCount = cells.reduce((acc, _, i) => acc + (getTerrainCellModel(cells, i) !== 1023 ? 1 : 0), 0);

  // Scenery object info
  const objs = trackScene?.objects || [];
  const activeObjIdx = Math.min(Math.max(0, trackObject), Math.max(0, objs.length - 1));
  const obj = objs[activeObjIdx];
  const courseModelsCount = trackScene?.courseModels || 0;

  // AI Racing Line & Speeds info
  const hints = trackScene?.ai?.hints?.[aiSeries] || [];
  const activeHintIdx = Math.min(Math.max(0, aiHintIndex), Math.max(0, hints.length - 1));
  const curHint = hints[activeHintIdx] || {start: 0, end: 10, minOffset: -400, maxOffset: 400, adjustment: 0};
  const hintMinOffset = curHint.min_offset ?? curHint.minOffset ?? -400;
  const hintMaxOffset = curHint.max_offset ?? curHint.maxOffset ?? 400;
  const speeds = trackScene?.ai?.speeds?.[aiSeries] || [];
  const activeSpeedIdx = Math.min(Math.max(0, aiSpeedIndex), Math.max(0, speeds.length - 1));
  const curSpeed = speeds[activeSpeedIdx] || {progress: 0, pitch: 0, targets: [160, 180, 200, 220]};
  const speedTargets = curSpeed.targets || [curSpeed.target_0 ?? 160, curSpeed.target_1 ?? 180, curSpeed.target_2 ?? 200, curSpeed.target_3 ?? 220];

  // Start Grid & Cameras info
  const starts = trackScene?.ai?.starts?.[aiSeries] || [];
  const activeStartIdx = Math.min(Math.max(0, startSlotIndex), Math.max(0, starts.length - 1));
  const curStart = starts[activeStartIdx] || {x: points[0]?.x ?? 0, z: points[0]?.z ?? 0, point: 0};
  const allCameras = (trackScene?.cameras || []).flatMap((tbl, tIdx) => (tbl || []).map((c, cIdx) => ({...c, tableIndex: tIdx, localIndex: cIdx})));
  const activeCamFlatIdx = Math.min(Math.max(0, cameraIndex), Math.max(0, allCameras.length - 1));
  const curCam = allCameras[activeCamFlatIdx] || null;
  const camOffsets = curCam ? (curCam.offset || curCam.offsets || [curCam.offset_0 ?? 0, curCam.offset_1 ?? -300, curCam.offset_2 ?? 0]) : [0, -300, 0];

  // Atmosphere & Intro Flyover info
  const envCues = trackScene?.environment?.cues || [];
  const activeEnvIdx = Math.min(Math.max(0, envCueIndex), Math.max(0, envCues.length - 1));
  const curCue = envCues[activeEnvIdx] || null;
  const cueColors = curCue ? (curCue.colors || [0x1e1b4b, 0xf97316, 0xfde047, 0xa87848]) : [0x1e1b4b, 0xf97316, 0xfde047, 0xa87848];
  const hexColor = c => '#' + (Math.max(0, Number(c) || 0) & 0xffffff).toString(16).padStart(6, '0');
  const allIntroFrames = (trackScene?.intro || []).flatMap((tbl, tIdx) =>
    Array.isArray(tbl) ? tbl.map((f, fIdx) => ({...f, tableIndex: tIdx, localIndex: fIdx})) : (tbl ? [{...tbl, tableIndex: 0, localIndex: tIdx}] : [])
  );
  const activeIntroIdx = Math.min(Math.max(0, introIndex), Math.max(0, allIntroFrames.length - 1));
  const curIntro = allIntroFrames[activeIntroIdx] || null;

  // Selected Road Segment & Track Sanity Check
  const curPt = points[sel] || {x: 0, y: 0, z: 0, leftWidth: 600, rightWidth: 600, crossSlope: 0};
  const ptLeftW = curPt.leftWidth ?? curPt.left_width ?? 600;
  const ptRightW = curPt.rightWidth ?? curPt.right_width ?? 600;
  const ptSlope = curPt.crossSlope ?? curPt.cross_slope ?? 0;

  let narrowSegIdx = -1;
  let steepStepIdx = -1;
  let offRoadStartCount = 0;
  for (let i = 0; i < n; i++) {
    const p = points[i];
    const lw = p.leftWidth ?? p.left_width ?? 600;
    const rw = p.rightWidth ?? p.right_width ?? 600;
    if (narrowSegIdx < 0 && lw + rw < 280) narrowSegIdx = i;
    const nextP = points[(i + 1) % n];
    if (steepStepIdx < 0 && nextP && Math.abs((nextP.y ?? 0) - (p.y ?? 0)) > 360) steepStepIdx = i;
  }
  for (const st of starts) {
    const rp = points[Math.min(Math.max(0, st.point ?? 0), Math.max(0, n - 1))];
    if (rp) {
      const maxW = ((rp.leftWidth ?? 600) + (rp.rightWidth ?? 600)) * 1.45;
      if (Math.hypot((st.x ?? 0) - rp.x, (st.z ?? 0) - rp.z) > maxW) offRoadStartCount++;
    }
  }

  return `
    <div class="studio-accordion-scroll">
      <!-- ROAD ROUTE (DRAG & DROP + PRECISION SEGMENT FIELDS) -->
      <details class="studio-accordion" data-accordion="road"${openAttr('road')}>
        <summary>
          <span>🛣️ Road Route (Seg #${sel + 1}/${n})</span>
          <span class="studio-accordion-badge">${diff.entries.length ? `${diff.pointCount} Edited` : `±${roadRadius} Falloff`}</span>
        </summary>
        <div class="studio-accordion-body">
          <small style="color:#94a3b8;line-height:1.35">Drag road points on the <b>2D Map</b> (or <b>Shift+Drag</b> on the road in <b>3D</b>) to shape the track:</small>

          ${narrowSegIdx >= 0 || steepStepIdx >= 0 || offRoadStartCount > 0 ? `
            <div style="padding:6px 8px;background:rgba(245,158,11,0.14);border:1px solid #f59e0b;border-radius:6px;display:flex;flex-direction:column;gap:4px">
              <small style="color:#fde68a;font-weight:700">⚠️ Track Sanity Warnings</small>
              ${narrowSegIdx >= 0 ? `<div class="row between"><small style="color:#fef3c7">Very narrow road at Seg #${narrowSegIdx + 1}</small><button type="button" class="studio-btn" data-jump-track-point="${narrowSegIdx}" style="padding:1px 6px;font-size:10px">Jump</button></div>` : ''}
              ${steepStepIdx >= 0 ? `<div class="row between"><small style="color:#fef3c7">Sharp vertical step at Seg #${steepStepIdx + 1}</small><button type="button" class="studio-btn" data-jump-track-point="${steepStepIdx}" style="padding:1px 6px;font-size:10px">Jump</button></div>` : ''}
              ${offRoadStartCount > 0 ? `<div class="row between"><small style="color:#fef3c7">${offRoadStartCount} Start Grid slot(s) off-road</small><button type="button" class="studio-btn" data-snap-start-grid style="padding:1px 6px;font-size:10px">Fix Grid</button></div>` : ''}
            </div>` : ''}

          <div class="row between" style="gap:4px">
            <small style="color:#cbd5e1;font-weight:600">Drag Radius (±Segs):</small>
            <div class="row" style="gap:3px">
              ${[1, 3, 6, 12, 20].map(r => `<button type="button" class="studio-chip ${roadRadius === r ? 'active' : ''}" data-road-radius="${r}" style="padding:2px 7px">${r}</button>`).join('')}
            </div>
          </div>

          ${diff.entries.length ? `
            <div style="display:flex;align-items:center;justify-content:space-between;padding:6px 8px;background:rgba(239,68,68,0.16);border:1px solid #ef4444;border-radius:6px">
              <small style="color:#fecaca;font-weight:700">${diff.pointCount} segment(s) reshaped</small>
              <div class="row" style="gap:4px">
                <button type="button" class="studio-btn" data-reset-road-draft>↺ Discard</button>
                <button type="button" class="studio-btn primary" data-save-road-draft>✓ Apply Route</button>
              </div>
            </div>` : ''}

          <div class="studio-grid-3">
            <button type="button" class="studio-btn" data-shape-road="smooth:0">〰 Smooth</button>
            <button type="button" class="studio-btn" data-shape-road="straighten:0">📏 Straighten</button>
            <button type="button" class="studio-btn" data-shape-road="level:0">⛰️ Level Y</button>
            <button type="button" class="studio-btn" data-shape-road="bank:-24">↙ Bank L</button>
            <button type="button" class="studio-btn" data-shape-road="bank:24">↘ Bank R</button>
            <button type="button" class="studio-btn" data-shape-road="widen:40">↔ Widen</button>
          </div>

          ${renderElevationMini(points, sel)}

          <form id="road-point-form" data-bank="${trackBank}" data-index="${sel}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:6px;border-top:1px solid #262b35">
            <small style="color:#94a3b8;font-weight:700;display:block;margin-bottom:4px">SEGMENT #${sel + 1} WIDTH, BANKING &amp; HEIGHT</small>
            <div class="studio-grid-2" style="gap:6px">
              <div class="studio-field">
                <label>Left Width</label>
                <input data-road-field="left_width" data-original="${ptLeftW}" type="number" min="80" max="4096" step="10" value="${ptLeftW}" required>
              </div>
              <div class="studio-field">
                <label>Right Width</label>
                <input data-road-field="right_width" data-original="${ptRightW}" type="number" min="80" max="4096" step="10" value="${ptRightW}" required>
              </div>
              <div class="studio-field">
                <label>Camber / Slope</label>
                <input data-road-field="cross_slope" data-original="${ptSlope}" type="number" min="-1024" max="1024" step="8" value="${ptSlope}" required>
              </div>
              <div class="studio-field">
                <label>Raw GTE Y</label>
                <input data-road-field="y" data-original="${curPt.y}" type="number" step="1" value="${curPt.y}" required>
              </div>
            </div>
          </form>
        </div>
      </details>

      <!-- TERRAIN UNITS (32x32 + PAINT BRUSH + INDIVIDUAL 3D MODEL IMPORT/EXPORT) -->
      <details class="studio-accordion" data-accordion="terrain"${openAttr('terrain')}>
        <summary>
          <span>⛰️ Terrain Units [${cellCol}, ${cellRow}]</span>
          <span class="studio-accordion-badge">${terrainBrushMode ? '🖌 Brush On' : (cellModel === 1023 ? 'Empty' : `Unit #${cellModel}`)}</span>
        </summary>
        <div class="studio-accordion-body">
          <div style="display:flex;align-items:center;justify-content:space-between;gap:6px;padding:6px 8px;background:#0d0f14;border:1px solid ${terrainBrushMode ? '#22c55e' : '#262b35'};border-radius:6px">
            <button type="button" class="studio-chip ${terrainBrushMode ? 'active' : ''}" data-toggle-terrain-brush title="Toggle Terrain Paint Brush — click or drag across the 32×32 grid or 2D map to paint Terrain Units">
              🖌 ${terrainBrushMode ? 'Paint Brush: ON' : 'Paint Brush: OFF'}
            </button>
            <select data-terrain-brush-model aria-label="Brush Terrain Unit Model" style="flex:1;max-width:175px;font-size:11px;padding:3px 6px">
              <option value="1023" ${terrainBrushModel === 1023 ? 'selected' : ''}>Eraser · Empty (1023)</option>
              ${Array.from({length: terrainModelsCount}, (_, m) => `<option value="${m}" ${m === terrainBrushModel ? 'selected' : ''}>Brush: Unit #${m}</option>`).join('')}
            </select>
          </div>

          ${cells.length === 1024 ? `
            <div>
              <div class="row between" style="margin-bottom:4px">
                <small style="color:#94a3b8;font-weight:700">32×32 TERRAIN GRID (${populatedTileCount} TILES)</small>
                <small style="color:#86efac">${terrainBrushMode ? 'Click/drag to paint' : 'Click tile to select'}</small>
              </div>
              <div class="studio-terrain-matrix" role="grid" aria-label="32 by 32 terrain segments">
                ${cells.map((_, idx) => {
                  const m = getTerrainCellModel(cells, idx);
                  const has = m !== 1023;
                  const act = idx === activeCell;
                  return `<button type="button" class="${has ? 'has-mesh' : ''} ${act ? 'active' : ''}" data-select-terrain-cell="${idx}" title="Col ${idx & 31}, Row ${idx >> 5} · ${has ? 'Terrain Unit #' + m : 'Empty'}"></button>`;
                }).join('')}
              </div>
            </div>` : ''}

          <form id="terrain-cell-form" data-bank="${trackBank}" data-index="${activeCell}" data-revision="${trackScene?.revision ?? 0}">
            <div class="studio-field">
              <label>Terrain Unit Model for [${cellCol}, ${cellRow}]</label>
              <select data-terrain-model data-original="${cellModel}">
                <option value="1023" ${cellModel === 1023 ? 'selected' : ''}>Empty Terrain Tile (1023)</option>
                ${Array.from({length: terrainModelsCount}, (_, m) => `<option value="${m}" ${m === cellModel ? 'selected' : ''}>Terrain Unit #${m}</option>`).join('')}
              </select>
            </div>
          </form>

          ${cellModel !== 1023 ? `
            <div style="padding-top:6px;border-top:1px solid #262b35">
              <small style="color:#94a3b8;font-weight:700;display:block;margin-bottom:5px">3D MESH &amp; TEXTURES · TERRAIN UNIT #${cellModel}</small>
              <div class="studio-grid-2" style="gap:5px;margin-bottom:5px">
                <button type="button" class="studio-btn" data-import-track-part="${prefix}.terrain" data-part="${cellModel}" data-revision="${trackScene?.revision ?? 0}" title="Import custom 3D mesh (.GLB or .OBJ) into Terrain Unit #${cellModel}">📥 Import .GLB/.OBJ</button>
                <button type="button" class="studio-btn" data-track-textures="${prefix}.terrain" data-part="${cellModel}" title="Inspect and replace PNG textures on Terrain Unit #${cellModel}">🎨 Textures</button>
              </div>
              <div class="studio-grid-2" style="gap:5px">
                <button type="button" class="studio-btn" data-export-track-part="${prefix}.terrain" data-part="${cellModel}" data-format="glb" title="Export Terrain Unit #${cellModel} as .GLB for Blender">📤 Export .GLB</button>
                <button type="button" class="studio-btn" data-export-track-part="${prefix}.terrain" data-part="${cellModel}" data-format="obj" title="Export Terrain Unit #${cellModel} as .OBJ + textures">📤 Export .OBJ</button>
              </div>
            </div>` : ''}
        </div>
      </details>

      <!-- DECORATIONS (SCENERY + INDIVIDUAL 3D MODEL IMPORT/EXPORT) -->
      <details class="studio-accordion" data-accordion="scenery"${openAttr('scenery')}>
        <summary>
          <span>🌲 Decorations (Scenery)</span>
          <span class="studio-accordion-badge">${objs.filter(o => o.model >= 0).length} Placed</span>
        </summary>
        <div class="studio-accordion-body">
          <small style="color:#94a3b8;line-height:1.35">Drag a model onto the <b>2D Map</b> to place it, click a card to preview in 3D, or click <b>+</b> to drop at Seg #${sel + 1}:</small>
          <div class="studio-scenery-grid">
            ${Array.from({length: courseModelsCount}, (_, m) => `
              <div class="studio-scenery-card ${obj?.model === m ? 'active' : ''}" draggable="true" data-scenery-prefab="${m}" title="Click to preview 3D Model #${m} · Drag onto 2D map to place">
                <span style="font-weight:600">🌲 Model #${m}</span>
                <button type="button" class="studio-btn" data-place-scenery-model="${m}" title="Place Model #${m} at Road Segment #${sel + 1}" style="padding:1px 6px;font-size:10px">+</button>
              </div>`).join('')}
          </div>

          ${obj ? `
            <form id="track-object-form" data-bank="${trackBank}" data-index="${activeObjIdx}" data-revision="${trackScene?.revision ?? 0}" style="margin-top:6px;padding-top:8px;border-top:1px solid #262b35">
              <div class="studio-field" style="margin-bottom:6px">
                <label>Selected Placed Decoration (Drag on Map / Shift+Drag in 3D)</label>
                <select data-select-object>
                  ${objs.map((o, i) => `<option value="${i}" ${i === activeObjIdx ? 'selected' : ''}>Decoration #${i + 1} · ${o.model < 0 ? 'Hidden' : `Model #${o.model}`}</option>`).join('')}
                </select>
              </div>
              <div class="studio-grid-2" style="margin-bottom:8px">
                <div class="studio-field">
                  <label>Decoration Model</label>
                  <select data-object-field="model" data-original="${obj.model}">
                    <option value="-1" ${obj.model < 0 ? 'selected' : ''}>Hidden (-1)</option>
                    ${Array.from({length: courseModelsCount}, (_, m) => `<option value="${m}" ${m === obj.model ? 'selected' : ''}>Model #${m}</option>`).join('')}
                  </select>
                </div>
                <div class="studio-field">
                  <label>Rotation (°)</label>
                  <input data-object-field="yaw" data-original="${(obj.yaw * 360 / 4096).toFixed(2)}" type="number" min="-360" max="360" step="1" value="${(obj.yaw * 360 / 4096).toFixed(0)}" required>
                </div>
              </div>
              <div class="studio-grid-2" style="margin-bottom:8px">
                <button type="button" class="studio-btn" data-duplicate-track-object="${activeObjIdx}">⎘ Duplicate</button>
                <button type="button" class="studio-btn" data-remove-track-object="${activeObjIdx}">🗑 Delete</button>
              </div>
              ${obj.model >= 0 ? `
                <div style="padding-top:6px;border-top:1px solid #262b35">
                  <small style="color:#94a3b8;font-weight:700;display:block;margin-bottom:5px">3D MESH &amp; TEXTURES · DECORATION MODEL #${obj.model}</small>
                  <div class="studio-grid-2" style="gap:5px;margin-bottom:5px">
                    <button type="button" class="studio-btn" data-import-track-part="${prefix}.course" data-part="${obj.model}" data-revision="${trackScene?.revision ?? 0}" title="Import custom 3D mesh (.GLB or .OBJ) into Decoration Model #${obj.model}">📥 Import .GLB/.OBJ</button>
                    <button type="button" class="studio-btn" data-track-textures="${prefix}.course" data-part="${obj.model}" title="Inspect and replace PNG textures on Decoration Model #${obj.model}">🎨 Textures</button>
                  </div>
                  <div class="studio-grid-2" style="gap:5px">
                    <button type="button" class="studio-btn" data-export-track-part="${prefix}.course" data-part="${obj.model}" data-format="glb" title="Export Decoration Model #${obj.model} as .GLB for Blender">📤 Export .GLB</button>
                    <button type="button" class="studio-btn" data-export-track-part="${prefix}.course" data-part="${obj.model}" data-format="obj" title="Export Decoration Model #${obj.model} as .OBJ + textures">📤 Export .OBJ</button>
                  </div>
                </div>` : ''}
            </form>` : '<small class="muted">No placed decorations.</small>'}
        </div>
      </details>

      <!-- AI RACING LINE & SPEEDS -->
      <details class="studio-accordion" data-accordion="ai"${openAttr('ai')}>
        <summary>
          <span>🤖 AI Racing Line &amp; Speeds</span>
          <span class="studio-accordion-badge">${aiSeries === 0 ? 'Forward' : 'Reverse'}</span>
        </summary>
        <div class="studio-accordion-body">
          <div class="row between" style="gap:6px">
            <small style="color:#94a3b8;font-weight:600">Direction Series:</small>
            <div class="row" style="gap:4px">
              <button type="button" class="studio-chip ${aiSeries === 0 ? 'active' : ''}" data-ai-series="0">Forward (0)</button>
              <button type="button" class="studio-chip ${aiSeries === 1 ? 'active' : ''}" data-ai-series="1">Reverse (1)</button>
            </div>
          </div>

          <button type="button" class="studio-btn" data-recenter-ai-hints style="width:100%;justify-content:center" title="Recenter all 30 AI lateral corridor hints around the road centerline after reshaping a track">
            ⚡ Recenter All AI Hints to Road Center
          </button>

          ${hints.length ? `
            <form id="ai-hint-form" data-bank="${trackBank}" data-series="${aiSeries}" data-index="${activeHintIdx}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:6px;border-top:1px solid #262b35">
              <div class="studio-field" style="margin-bottom:6px">
                <label>AI Racing Line Hint (#1..${hints.length})</label>
                <select data-select-ai-hint>
                  ${hints.map((h, i) => `<option value="${i}" ${i === activeHintIdx ? 'selected' : ''}>Hint #${i + 1} · Segs ${h.start}–${h.end} (Offset ${h.min_offset ?? h.minOffset ?? 0}..${h.max_offset ?? h.maxOffset ?? 0})</option>`).join('')}
                </select>
              </div>
              <div class="studio-grid-2" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>Start Seg</label>
                  <input data-ai-hint-field="start" data-original="${curHint.start}" type="number" min="0" max="${Math.max(0, n - 1)}" value="${curHint.start}" required>
                </div>
                <div class="studio-field">
                  <label>End Seg</label>
                  <input data-ai-hint-field="end" data-original="${curHint.end}" type="number" min="0" max="${Math.max(0, n - 1)}" value="${curHint.end}" required>
                </div>
                <div class="studio-field">
                  <label>Min Lateral Offset</label>
                  <input data-ai-hint-field="min_offset" data-original="${hintMinOffset}" type="number" min="-4096" max="4096" step="10" value="${hintMinOffset}" required>
                </div>
                <div class="studio-field">
                  <label>Max Lateral Offset</label>
                  <input data-ai-hint-field="max_offset" data-original="${hintMaxOffset}" type="number" min="-4096" max="4096" step="10" value="${hintMaxOffset}" required>
                </div>
              </div>
              <div class="studio-field">
                <label>Steering Adjustment</label>
                <input data-ai-hint-field="adjustment" data-original="${curHint.adjustment}" type="number" min="-2048" max="2048" step="5" value="${curHint.adjustment}" required>
              </div>
            </form>` : '<small class="muted">No AI hints loaded for this track.</small>'}

          ${speeds.length ? `
            <form id="ai-speed-form" data-bank="${trackBank}" data-series="${aiSeries}" data-index="${activeSpeedIdx}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:8px;border-top:1px solid #262b35">
              <div class="studio-field" style="margin-bottom:6px">
                <label>AI Target Speed Zone (#1..${speeds.length})</label>
                <select data-select-ai-speed>
                  ${speeds.map((s, i) => {
                    const t = s.targets || [s.target_0 ?? 0, s.target_1 ?? 0, s.target_2 ?? 0, s.target_3 ?? 0];
                    return `<option value="${i}" ${i === activeSpeedIdx ? 'selected' : ''}>Zone #${i + 1} · Seg ${s.progress} (${t.join('/')} km/h)</option>`;
                  }).join('')}
                </select>
              </div>
              <div class="studio-grid-2" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>Road Seg (Progress)</label>
                  <input data-ai-speed-field="progress" data-original="${curSpeed.progress}" type="number" min="0" max="${Math.max(0, n - 1)}" value="${curSpeed.progress}" required>
                </div>
                <div class="studio-field">
                  <label>Pitch Hint</label>
                  <input data-ai-speed-field="pitch" data-original="${curSpeed.pitch}" type="number" min="-2048" max="2048" value="${curSpeed.pitch}" required>
                </div>
                <div class="studio-field">
                  <label>Class 1 Speed</label>
                  <input data-ai-speed-field="target_0" data-original="${speedTargets[0]}" type="number" min="20" max="400" value="${speedTargets[0]}" required>
                </div>
                <div class="studio-field">
                  <label>Class 2 Speed</label>
                  <input data-ai-speed-field="target_1" data-original="${speedTargets[1]}" type="number" min="20" max="400" value="${speedTargets[1]}" required>
                </div>
                <div class="studio-field">
                  <label>Class 3 Speed</label>
                  <input data-ai-speed-field="target_2" data-original="${speedTargets[2]}" type="number" min="20" max="400" value="${speedTargets[2]}" required>
                </div>
                <div class="studio-field">
                  <label>Class 4/SP Speed</label>
                  <input data-ai-speed-field="target_3" data-original="${speedTargets[3]}" type="number" min="20" max="400" value="${speedTargets[3]}" required>
                </div>
              </div>
            </form>` : ''}
        </div>
      </details>

      <!-- START GRID & CAMERAS -->
      <details class="studio-accordion" data-accordion="markers"${openAttr('markers')}>
        <summary>
          <span>🏁 Start Grid &amp; Cameras</span>
          <span class="studio-accordion-badge">P${activeStartIdx + 1} · ${allCameras.length} Cams</span>
        </summary>
        <div class="studio-accordion-body">
          <button type="button" class="studio-btn" data-snap-start-grid style="width:100%;justify-content:center" title="Automatically align all 12 starting grid slots (P1..P12) in a staggered 2-column formation behind Road Segment #${sel + 1}">
            ⚡ Snap All 12 Start Slots to Road (Seg #${sel + 1})
          </button>

          ${starts.length ? `
            <form id="start-slot-form" data-bank="${trackBank}" data-series="${aiSeries}" data-index="${activeStartIdx}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:6px;border-top:1px solid #262b35">
              <div class="studio-field" style="margin-bottom:6px">
                <label>Starting Grid Slot (Drag P1..P12 on 2D Map)</label>
                <select data-select-start-slot>
                  ${starts.map((s, i) => `<option value="${i}" ${i === activeStartIdx ? 'selected' : ''}>Grid P${i + 1} · Seg #${(s.point ?? 0) + 1} (X:${s.x}, Z:${s.z})</option>`).join('')}
                </select>
              </div>
              <div class="studio-grid-3" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>World X</label>
                  <input data-start-field="x" data-original="${curStart.x}" type="number" step="10" value="${curStart.x}" required>
                </div>
                <div class="studio-field">
                  <label>World Z</label>
                  <input data-start-field="z" data-original="${curStart.z}" type="number" step="10" value="${curStart.z}" required>
                </div>
                <div class="studio-field">
                  <label>Road Seg</label>
                  <input data-start-field="point" data-original="${curStart.point}" type="number" min="0" max="${Math.max(0, n - 1)}" value="${curStart.point}" required>
                </div>
              </div>
              <button type="button" class="studio-btn" data-snap-start-slot="${activeStartIdx}" style="width:100%;justify-content:center">
                📍 Move P${activeStartIdx + 1} to Selected Road Seg #${sel + 1}
              </button>
            </form>` : '<small class="muted">No start grid slots loaded.</small>'}

          ${curCam ? `
            <form id="camera-form" data-bank="${trackBank}" data-camera-id="${curCam.id}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:8px;border-top:1px solid #262b35">
              <div class="studio-field" style="margin-bottom:6px">
                <label>TV / Replay Camera (${allCameras.length} Total)</label>
                <select data-select-camera>
                  ${allCameras.map((c, i) => `<option value="${i}" ${i === activeCamFlatIdx ? 'selected' : ''}>Cam #${c.id} (Table ${c.tableIndex + 1}) · Seg #${(c.section ?? 0) + 1} · Mode ${c.mode}</option>`).join('')}
                </select>
              </div>
              <div class="studio-grid-3" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>Road Seg</label>
                  <input data-camera-field="section" data-original="${curCam.section}" type="number" min="0" max="${Math.max(0, n - 1)}" value="${curCam.section}" required>
                </div>
                <div class="studio-field">
                  <label>Mode</label>
                  <input data-camera-field="mode" data-original="${curCam.mode}" type="number" min="0" max="255" value="${curCam.mode}" required>
                </div>
                <div class="studio-field">
                  <label>Duration</label>
                  <input data-camera-field="duration" data-original="${curCam.duration}" type="number" min="1" max="4096" value="${curCam.duration}" required>
                </div>
                <div class="studio-field">
                  <label>Offset X</label>
                  <input data-camera-field="offset_0" data-original="${camOffsets[0]}" type="number" step="10" value="${camOffsets[0]}" required>
                </div>
                <div class="studio-field">
                  <label>Offset Y</label>
                  <input data-camera-field="offset_1" data-original="${camOffsets[1]}" type="number" step="10" value="${camOffsets[1]}" required>
                </div>
                <div class="studio-field">
                  <label>Offset Z</label>
                  <input data-camera-field="offset_2" data-original="${camOffsets[2]}" type="number" step="10" value="${camOffsets[2]}" required>
                </div>
              </div>
              <button type="button" class="studio-btn" data-snap-camera-seg="${curCam.id}" style="width:100%;justify-content:center">
                📹 Move Cam #${curCam.id} to Selected Road Seg #${sel + 1}
              </button>
            </form>` : ''}
        </div>
      </details>

      <!-- ATMOSPHERE, SKY & INTRO CAMERA -->
      <details class="studio-accordion" data-accordion="atmosphere"${openAttr('atmosphere')}>
        <summary>
          <span>🌅 Atmosphere, Sky &amp; Intro Camera</span>
          <span class="studio-accordion-badge">${envCues.length} Cues · ${allIntroFrames.length} Intro</span>
        </summary>
        <div class="studio-accordion-body">
          ${curCue ? `
            <form id="environment-form" data-bank="${trackBank}" data-revision="${trackScene?.revision ?? 0}">
              <div class="studio-field" style="margin-bottom:6px">
                <label>Sky, Horizon &amp; Fog Cue (#1..${envCues.length})</label>
                <select data-select-env-cue>
                  ${envCues.map((c, i) => `<option value="${i}" ${i === activeEnvIdx ? 'selected' : ''}>Cue #${i + 1} · Time ${c.time ?? 0} · Dur ${c.duration ?? 0}</option>`).join('')}
                </select>
              </div>
              <div style="height:20px;border-radius:5px;border:1px solid #262b35;margin-bottom:6px;background:linear-gradient(180deg, ${hexColor(cueColors[0])} 0%, ${hexColor(cueColors[1])} 55%, ${hexColor(cueColors[2])} 100%)" title="Sky / Horizon / Fog Gradient Preview"></div>
              <div class="studio-grid-3" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>Sky Top</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="color_0" data-original="${hexColor(cueColors[0])}" type="color" value="${hexColor(cueColors[0])}">
                </div>
                <div class="studio-field">
                  <label>Horizon</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="color_1" data-original="${hexColor(cueColors[1])}" type="color" value="${hexColor(cueColors[1])}">
                </div>
                <div class="studio-field">
                  <label>Fog / Ground</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="color_2" data-original="${hexColor(cueColors[2])}" type="color" value="${hexColor(cueColors[2])}">
                </div>
              </div>
              <div class="studio-grid-3" style="margin-bottom:4px">
                <div class="studio-field">
                  <label>Time</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="time" data-original="${curCue.time ?? 0}" type="number" min="0" max="65535" value="${curCue.time ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>Duration</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="duration" data-original="${curCue.duration ?? 0}" type="number" min="0" max="65535" value="${curCue.duration ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>Mode</label>
                  <input data-env-type="env" data-env-index="${curCue.index ?? activeEnvIdx}" data-env-field="mode" data-original="${curCue.mode ?? 0}" type="number" min="0" max="255" value="${curCue.mode ?? 0}" required>
                </div>
              </div>
            </form>` : '<small class="muted">No environment cues loaded.</small>'}

          ${curIntro ? `
            <form id="intro-camera-form" data-bank="${trackBank}" data-index="${curIntro.id ?? curIntro.index ?? activeIntroIdx}" data-revision="${trackScene?.revision ?? 0}" style="padding-top:8px;border-top:1px solid #262b35">
              <div class="row between" style="margin-bottom:6px;gap:6px">
                <small style="color:#94a3b8;font-weight:700">RACE INTRO FLYOVER (${allIntroFrames.length} KEYFRAMES)</small>
                <button type="button" class="studio-btn" data-align-intro-camera style="padding:2px 7px;font-size:10.5px" title="Automatically align all intro flyover camera keyframes along the road approaching Seg #${sel + 1}">⚡ Align Flyover to Road</button>
              </div>
              <div class="studio-field" style="margin-bottom:6px">
                <label>Intro Keyframe (#1..${allIntroFrames.length})</label>
                <select data-select-intro-frame>
                  ${allIntroFrames.map((f, i) => `<option value="${i}" ${i === activeIntroIdx ? 'selected' : ''}>Keyframe #${i + 1} (Table ${(f.tableIndex ?? 0) + 1}) · ID ${f.id ?? i} · Frame ${f.frame ?? 0}</option>`).join('')}
                </select>
              </div>
              <div class="studio-grid-3" style="margin-bottom:6px">
                <div class="studio-field">
                  <label>World X</label>
                  <input data-intro-field="x" data-original="${curIntro.x ?? 0}" type="number" step="10" value="${curIntro.x ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>World Y</label>
                  <input data-intro-field="y" data-original="${curIntro.y ?? 0}" type="number" step="10" value="${curIntro.y ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>World Z</label>
                  <input data-intro-field="z" data-original="${curIntro.z ?? 0}" type="number" step="10" value="${curIntro.z ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>Frame</label>
                  <input data-intro-field="frame" data-original="${curIntro.frame ?? 0}" type="number" min="0" max="65535" value="${curIntro.frame ?? 0}" required>
                </div>
                <div class="studio-field">
                  <label>Duration</label>
                  <input data-intro-field="duration" data-original="${curIntro.duration ?? 15}" type="number" min="1" max="65535" value="${curIntro.duration ?? 15}" required>
                </div>
              </div>
              <button type="button" class="studio-btn" data-snap-intro-frame="${curIntro.id ?? curIntro.index ?? activeIntroIdx}" style="width:100%;justify-content:center">
                🎬 Move Keyframe #${activeIntroIdx + 1} Above Selected Road Seg #${sel + 1}
              </button>
            </form>` : ''}
        </div>
      </details>
    </div>`;
}

function openLoadTrackModal() {
  const dialog = $('#dialog');
  dialog.innerHTML = `
    <div style="min-width:640px;max-width:780px">
      <div class="row between" style="align-items:flex-start;margin-bottom:8px">
        <div>
          <div class="eyebrow">FILE → LOAD TRACK</div>
          <h2 style="margin:2px 0 0">Select a Track &amp; Class Layout (${trackList.length} Tracks)</h2>
        </div>
        <button type="button" class="studio-btn primary" data-action="import-r4-tracks" title="Import all 8 tracks from Ridge Racer Type 4 (R4.BIN, CUE, or Track 01 BIN)">🏁 Import R4 Tracks…</button>
      </div>
      <div class="studio-browser-grid" style="max-height:65vh;overflow-y:auto;padding-right:4px">
        ${trackList.map(t => `
          <div class="studio-browser-card ${t.bank === trackBank ? 'active' : ''}" data-pick-track="${t.bank}" role="button" tabindex="0">
            <div class="row between" style="width:100%">
              <span class="car-slot-badge">${esc(trackGradeBadge(t))}</span>
              <small>Bank ${t.bank}</small>
            </div>
            <strong style="font-size:13.5px">${esc(t.name)}</strong>
            <div class="row between" style="width:100%;margin-top:2px;gap:6px">
              <small>${t.origin === 'custom' ? 'Custom Track · Driveable in Custom / GP' : t.available ? 'Driveable in Class ' + t.grade : 'Bonus / Unlockable'}</small>
              <button type="button" class="studio-btn" data-restore-track-bank="${t.bank}" style="padding:2px 7px;font-size:10.5px" title="Restore original version">↺ Restore</button>
            </div>
          </div>`).join('')}
      </div>
      <footer class="row between" style="margin-top:12px">
        <button type="button" class="studio-btn" data-action="import-r4-tracks">🏁 Import Ridge Racer Type 4 Tracks (R4.BIN / Disc)…</button>
        <button type="button" class="studio-btn" data-action="close">Close</button>
      </footer>
    </div>`;
  if (!dialog.open) dialog.showModal();
}

function summarizeTrackProjectChanges() {
  const groups = new Map();
  for (const c of project?.changes || []) {
    if (c.kind === 'track') {
      const m = /^track\.(?:(big|mid|hi|oval)([1-6])|custom(\d+)|(\d+))/.exec(String(c.id || ''));
      const bank = m
        ? (m[1] ? 88 + (Number(m[2]) - 1) * 8 + ['big', 'mid', 'hi', 'oval'].indexOf(m[1]) * 2
          : m[3] ? 136 + (Number(m[3]) - 1) * 2
          : Number(m[4]))
        : Number(c.bank || trackBank);
      const key = `track.${bank}`;
      if (!groups.has(key)) {
        const tr = trackList.find(t => t.bank === bank);
        groups.set(key, {
          id: key,
          kind: 'track',
          bank,
          title: tr ? `${trackGradeBadge(tr)} · ${tr.name}` : `Track Bank ${bank}`,
          subtitle: `Track Bank ${bank}`,
          tags: new Set(),
          count: 0
        });
      }
      const g = groups.get(key);
      g.count++;
      g.tags.add(c.field || 'Track Edit');
    } else if (c.kind === 'car') {
      const m = /^car\.(player|rival)\.(\d+)/.exec(String(c.id || ''));
      const bank = m ? Number(m[2]) : 10;
      const isRival = m && m[1] === 'rival';
      const key = `car.${isRival ? 'rival' : 'player'}.${bank}`;
      if (!groups.has(key)) {
        groups.set(key, {
          id: key,
          kind: 'car',
          bank,
          title: `${isRival ? 'Rival' : 'Player'} Car · Bank ${bank}`,
          subtitle: isRival ? 'Course Rival Pack' : 'Player / Expansion Slot',
          tags: new Set(),
          count: 0
        });
      }
      const g = groups.get(key);
      g.count++;
      g.tags.add(c.field || 'Car Edit');
    }
  }
  return [...groups.values()];
}

function openProjectChangesModal() {
  const groups = summarizeTrackProjectChanges();
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
        Inspect every modified track and car in your mod project. Jump directly to any track or revert individual tracks back to their original disc version.
      </p>
      <div style="max-height:54vh;overflow-y:auto;padding-right:4px">
        ${groups.length ? groups.map(g => `
          <div class="studio-outliner-card">
            <div>
              <div class="row" style="gap:8px;align-items:center">
                <span class="car-slot-badge">${g.kind === 'track' ? 'TRACK' : 'CAR'}</span>
                <strong style="font-size:13px">${esc(g.title)}</strong>
                <small class="muted">${esc(g.subtitle)}</small>
              </div>
              <div class="row wrap" style="gap:5px;margin-top:6px">
                ${[...g.tags].map(t => `<span class="studio-accordion-badge" style="background:rgba(56,189,248,0.15);color:#bae6fd">${esc(t)}</span>`).join('')}
                <small class="muted" style="margin-left:4px">(${g.count} change entry${g.count === 1 ? '' : 'ies'})</small>
              </div>
            </div>
            <div class="row" style="gap:6px;flex-shrink:0">
              ${g.kind === 'track' ? `
                <button type="button" class="studio-btn primary" data-pick-track="${g.bank}">Open Track</button>
                <button type="button" class="studio-btn" data-restore-track-bank="${g.bank}" title="Revert this track to its original disc state">↺ Revert to Disc</button>
              ` : `
                <button type="button" class="studio-btn primary" data-open-studio="car">Open Car Studio</button>
              `}
            </div>
          </div>
        `).join('') : '<div class="card" style="text-align:center;padding:24px"><p class="muted" style="margin:0">No project edits recorded yet — all tracks and cars match the original game disc.</p></div>'}
      </div>
      <footer class="row between" style="margin-top:14px">
        <small class="muted">Project Revision #${project?.revision ?? 0}</small>
        <button type="button" class="studio-btn" data-action="close">Close</button>
      </footer>
    </div>`;
  if (!dialog.open) dialog.showModal();
}

let trackPaletteQuery = '';
function openCommandPaletteModal(query = trackPaletteQuery) {
  trackPaletteQuery = query;
  const currentTrack = trackList.find(t => t.bank === trackBank) || trackList[0] || {name: 'Mythical Coast', grade: 1, bank: 88};
  const items = [
    {group: 'Action', label: '💾 Save & Apply All Track Changes to Game', hint: 'Ctrl+S', attr: 'data-action="save-all"'},
    {group: 'Action', label: `🏁 Start Test Race on ${trackGradeBadge(currentTrack)} · ${currentTrack.name}`, hint: 'Launch Race', attr: 'data-action="test-race"'},
    {group: 'Action', label: '🏁 Import All 8 Tracks from Ridge Racer Type 4 (R4.BIN / Disc)…', hint: 'Custom #1–#8', attr: 'data-action="import-r4-tracks"'},
    {group: 'Action', label: `📦 Project Changes Outliner (${project?.changes?.length || 0} edits)`, hint: 'Inspect / Revert', attr: 'data-open-project-changes="true"'},
    {group: 'Action', label: '🗺️ Browse Track Catalog Modal…', hint: 'Ctrl+O', attr: 'data-open-load-track="true"'},
    {group: 'Action', label: '↺ Restore Current Track to Original Disc Version', hint: `Bank ${trackBank}`, attr: 'data-action="restore-original-track"'},
    {group: 'Viewport', label: '🎥 Switch to 3D Full Track Viewport', hint: 'WebGL 3D', attr: 'data-stage-view="3d"'},
    {group: 'Viewport', label: '🗺️ Switch to 2D Top-Down Track Map (Drag & Drop)', hint: 'SVG Map', attr: 'data-stage-view="2d"'},
    {group: 'Viewport', label: `⛰️ Switch to 3D Terrain Tile Inspector [${terrainCell & 31}, ${terrainCell >> 5}]`, hint: 'Tile 3D', attr: 'data-stage-view="tile3d"'},
    {group: 'Studio', label: '🚗 Switch to Car Editor Studio', hint: '120+ Car Slots', attr: 'data-open-studio="car"'},
    {group: 'Studio', label: '💾 Switch to Save Editor', hint: 'Garage & Credits', attr: 'data-open-studio="saves"'},
    ...trackList.map(t => ({
      group: trackGradeBadge(t),
      label: `${t.name} (${trackGradeBadge(t)})`,
      hint: `Bank ${t.bank}`,
      attr: `data-pick-track="${t.bank}"`
    }))
  ];
  const q = String(trackPaletteQuery || '').trim().toLowerCase();
  const filtered = (q ? items.filter(it => `${it.group} ${it.label} ${it.hint}`.toLowerCase().includes(q)) : items).slice(0, 40);
  const dialog = $('#dialog');
  const renderList = list => list.map((it, idx) => `
    <button type="button" class="studio-palette-item ${idx === 0 ? 'selected' : ''}" ${it.attr}>
      <div>
        <span class="car-slot-badge" style="margin-right:6px">${esc(it.group)}</span>
        <strong>${esc(it.label)}</strong>
      </div>
      <small class="muted">${esc(it.hint)}</small>
    </button>
  `).join('') || '<p class="muted" style="padding:12px">No matching commands or tracks.</p>';

  if (dialog.open && $('#studio-command-palette-input')) {
    const listEl = $('#studio-command-palette-list');
    if (listEl) listEl.innerHTML = renderList(filtered);
    return;
  }

  dialog.innerHTML = `
    <div style="min-width:600px;max-width:720px">
      <div class="row between">
        <div class="eyebrow">COMMAND PALETTE (CTRL+K)</div>
        <small class="muted">Use [ and ] in studio to step road segments · 1/3/7/9/W/G/B/T/L in 3D viewport</small>
      </div>
      <input type="search" id="studio-command-palette-input" placeholder="Jump to any track bank, switch 2D/3D viewport, or run studio action…" value="${esc(trackPaletteQuery)}" style="width:100%;margin-top:8px;padding:9px 12px;font-size:13.5px">
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

async function mount3dTrackViewport() {
  if (materialBusy) return;
  if (stageView !== '3d' && stageView !== 'tile3d') return;
  if (!trackScene?.road?.points?.length) return;
  if (previewPromise) await cancelPreview();
  const token = ++previewToken;
  const host = $('#track-studio-3d-host');
  if (!host || host.querySelector('canvas')) return;
  const targetKey = track3dKey(trackBank);
  if (loading3dKey === targetKey || active3dKey === targetKey) return;
  loading3dKey = targetKey;
  let job = null;

  try {
    const height = Math.max(420, host.clientHeight || window.innerHeight - 140);

    // Mode A: 3D Terrain Tile Preview for selected Terrain Segment [Col, Row]
    if (stageView === 'tile3d') {
      const cells = trackScene?.terrain?.cells || [];
      const model = getTerrainCellModel(cells, terrainCell);
      const col = terrainCell & 31;
      const row = terrainCell >> 5;
      if (model === 1023) {
        active3dKey = targetKey;
        host.innerHTML = `
          <div style="display:flex;flex-direction:column;align-items:center;justify-content:center;height:100%;gap:10px;padding:24px;text-align:center">
            <strong style="font-size:14px">Terrain Segment [Col ${col}, Row ${row}] is Empty (Model 1023)</strong>
            <span style="color:#94a3b8;max-width:440px">Click a green cell in the 32×32 Terrain Matrix on the right to inspect its 3D terrain mesh.</span>
            <div class="row" style="gap:8px">
              <button type="button" class="studio-btn primary" data-stage-view="3d">🌍 View Full 3D Track</button>
            </div>
          </div>`;
        return;
      }
      host.innerHTML = `<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#94a3b8;font-size:13px">Loading 3D Terrain Tile [Col ${col}, Row ${row}] (Model #${model})…</div>`;
      previewBusy = true;
      const prefix = trackAssetPrefix(trackBank);
      job = request('previewTrackPart', `${prefix}.terrain`, model);
      previewPromise = job;
      const tileData = await job;
      if (token !== previewToken || !host.isConnected) return;
      active3dKey = targetKey;
      await mountModelPreview(host, tileData, {inline: true, height});
      return;
    }

    // Mode B: Full 3D Track Scene (3D Terrain Models + 3D Road Ribbon + 3D Scenery Objects)
    const rawKey = `${trackBank}:${trackScene?.revision ?? 0}`;
    let rawData = cachedTrackPreviewKey === rawKey ? cachedTrackPreview : null;
    if (!rawData) {
      host.innerHTML = '<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#94a3b8;font-size:13px">Loading Full 3D Track &amp; Terrain Models…</div>';
      previewBusy = true;
      job = request('trackPreview', trackBank);
      previewPromise = job;
      rawData = await job;
      if (token !== previewToken || !host.isConnected) return;
      cachedTrackPreview = rawData;
      cachedTrackPreviewKey = rawKey;
    }
    active3dKey = targetKey;
    const {data: layeredData, frame} = build3dTrackView(rawData, trackScene, roadDraft || trackScene?.road, trackPoint, roadRadius, trackLayers);
    await mountModelPreview(host, layeredData, {
      inline: true,
      height,
      frame,
      selectedEntity: selected3dEntity || {bank: 'road', point: trackPoint, radius: roadRadius},
      onMoveEntity: async (entity, delta) => {
        if (entity.bank === 'road' && Number.isInteger(entity.point)) {
          const draftObj = ensureRoadDraft();
          if (draftObj && trackScene?.road?.points) {
            draftObj.points = deformRoadPoints(trackScene.road.points, draftObj.points, entity.point, {dx: delta.x, dz: -delta.z}, roadRadius);
            trackPoint = entity.point;
          }
          selected3dEntity = {bank: 'road', point: entity.point, radius: roadRadius};
          active3dKey = track3dKey(trackBank);
          render();
          toast(`Moved road segment #${entity.point + 1} in 3D (±${roadRadius}); click Save Route to commit`);
          return build3dTrackView(cachedTrackPreview, trackScene, roadDraft || trackScene?.road, trackPoint, roadRadius, trackLayers).data;
        }
        const object = trackScene.objects[entity.object];
        project = await request('saveTrackObject', trackBank, entity.object, {x: object.x + delta.x, z: object.z - delta.z}, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        const preview = await request('trackPreview', trackBank);
        cachedTrackPreview = preview;
        cachedTrackPreviewKey = `${trackBank}:${trackScene?.revision ?? 0}`;
        selected3dEntity = {bank: 'course', object: entity.object};
        active3dKey = track3dKey(trackBank);
        render();
        toast('Scenery object moved in 3D');
        return build3dTrackView(preview, trackScene, roadDraft || trackScene?.road, trackPoint, roadRadius, trackLayers).data;
      },
      onSelectEntity: entity => {
        if (entity.bank === 'road' && Number.isInteger(entity.point)) {
          trackPoint = entity.point;
          selected3dEntity = {bank: 'road', point: entity.point, radius: roadRadius};
          openAccordions.add('road');
          render();
          toast(`Selected Road Segment #${entity.point + 1} in 3D (±${roadRadius} neighbors)`);
          return;
        }
        if (entity.bank === 'course' && Number.isInteger(entity.object)) {
          const now = Date.now();
          const isDbl = trackObject === entity.object && (now - (window._lastSceneryClickAt || 0)) < 450;
          window._lastSceneryClickAt = now;
          trackObject = entity.object;
          selected3dEntity = {bank: 'course', object: entity.object};
          openAccordions.add('scenery');
          render();
          const obj = trackScene?.objects?.[entity.object];
          if (isDbl && obj && obj.model >= 0) {
            const prefix = trackAssetPrefix(trackBank);
            request('previewTrackPart', `${prefix}.course`, obj.model)
              .then(data => mountModelPreview($('#dialog'), {...data, previewName: `Scenery Object #${entity.object + 1} · Course Model #${obj.model}`}))
              .catch(showError);
          }
          return;
        }
        if (entity.bank === 'terrain' && Number.isInteger(entity.cell)) {
          terrainCell = entity.cell;
          selected3dEntity = {bank: 'terrain', cell: entity.cell};
          openAccordions.add('terrain');
          render();
          toast(`Selected 3D Terrain Segment [Col ${entity.cell & 31}, Row ${entity.cell >> 5}] (Model #${getTerrainCellModel(trackScene?.terrain?.cells, entity.cell)})`);
        }
      }
    });
  } catch (err) {
    if (token === previewToken && host.isConnected) {
      active3dKey = '';
      if (/Wait for the current operation|Another operation|Operation canceled/.test(err?.message || '')) {
        host.innerHTML = '<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#94a3b8;font-size:13px">Waiting for the editor…</div>';
        setTimeout(() => {if (host.isConnected && token === previewToken) mount3dTrackViewport();}, 150);
      } else {
        host.innerHTML = `<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#94a3b8">3D track scene unavailable: ${esc(err?.message || err)}</div>`;
      }
    }
  } finally {
    if (job && previewPromise === job) previewPromise = null;
    if (loading3dKey === targetKey) loading3dKey = '';
    if (token === previewToken) {
      previewBusy = false;
      if (state) state.busy = null;
    }
  }
}

function bind2dMapEvents() {
  const svg = $('#studio-track-map');
  if (!svg) return;
  if (mapViewBank !== trackBank) {
    mapViewBox = [0, 0, 1000, 1000];
    mapViewBank = trackBank;
  }
  const applyVb = () => svg.setAttribute('viewBox', mapViewBox.map(v => v.toFixed(1)).join(' '));
  applyVb();

  const svgToWorld = (clientX, clientY) => {
    const rect = svg.getBoundingClientRect();
    const [vx, vy, vw, vh] = mapViewBox;
    const sx = vx + ((clientX - rect.left) / Math.max(1, rect.width)) * vw;
    const sy = vy + ((clientY - rect.top) / Math.max(1, rect.height)) * vh;
    const minX = Number(svg.dataset.mapMinX || 0);
    const maxZ = Number(svg.dataset.mapMaxZ || 65536);
    const scale = Number(svg.dataset.mapScale || 1);
    const pad = Number(svg.dataset.mapPad || 45);
    return {
      sx,
      sy,
      worldX: Math.round((sx - pad) / scale + minX),
      worldZ: Math.round(maxZ - (sy - pad) / scale)
    };
  };

  svg.addEventListener('wheel', e => {
    e.preventDefault();
    const rect = svg.getBoundingClientRect();
    const mx = (e.clientX - rect.left) / Math.max(1, rect.width);
    const my = (e.clientY - rect.top) / Math.max(1, rect.height);
    const factor = e.deltaY < 0 ? 0.82 : 1.22;
    const [vx, vy, vw, vh] = mapViewBox;
    const nw = Math.max(80, Math.min(2200, vw * factor));
    const nh = Math.max(80, Math.min(2200, vh * factor));
    mapViewBox = [vx + (vw - nw) * mx, vy + (vh - nh) * my, nw, nh];
    applyVb();
  }, {passive: false});

  let panStart = null;
  let pointDrag = null;
  let arcDrag = null;
  let objDrag = null;
  let startDrag = null;
  let brushCells = null;

  svg.addEventListener('pointerdown', e => {
    // 1. Curve Arc Center drag
    const arcEl = e.target.closest('[data-drag-arc]');
    if (arcEl) {
      const draftObj = ensureRoadDraft();
      if (!draftObj) return;
      arcDrag = {arcIdx: Number(arcEl.dataset.dragArc), el: arcEl, moved: false, startX: e.clientX, startY: e.clientY};
      svg.setPointerCapture(e.pointerId);
      e.preventDefault();
      return;
    }

    // 1b. Start Grid Slot drag (P1..P12) on 2D map
    const startEl = e.target.closest('[data-map-start]');
    if (startEl) {
      const sIdx = Number(startEl.dataset.mapStart);
      startSlotIndex = sIdx;
      openAccordions.add('markers');
      const starts = trackScene?.ai?.starts?.[aiSeries] || [];
      const orig = starts[sIdx] || {x: 0, z: 0, point: 0};
      startDrag = {sIdx, el: startEl, moved: false, startX: e.clientX, startY: e.clientY, worldX: orig.x, worldZ: orig.z};
      svg.setPointerCapture(e.pointerId);
      e.preventDefault();
      return;
    }

    // 1c. TV Camera marker click on 2D map
    const camEl = e.target.closest('[data-map-camera]');
    if (camEl) {
      const camId = Number(camEl.dataset.mapCamera);
      const allCameras = (trackScene?.cameras || []).flatMap((tbl, tIdx) => (tbl || []).map((c, cIdx) => ({...c, tableIndex: tIdx, localIndex: cIdx})));
      const foundIdx = allCameras.findIndex(c => c.id === camId);
      if (foundIdx >= 0) {
        cameraIndex = foundIdx;
        if (Number.isInteger(allCameras[foundIdx].section)) trackPoint = allCameras[foundIdx].section;
      }
      openAccordions.add('markers');
      render();
      toast(`Selected TV Camera #${camId}`);
      e.preventDefault();
      return;
    }

    // 2. Scenery Object drag on 2D map
    const objEl = e.target.closest('[data-map-object]');
    if (objEl) {
      const objIdx = Number(objEl.dataset.mapObject);
      trackObject = objIdx;
      selected3dEntity = {bank: 'course', object: objIdx};
      openAccordions.add('scenery');
      const orig = trackScene?.objects?.[objIdx];
      if (!orig) return;
      objDrag = {objIdx, el: objEl, moved: false, startX: e.clientX, startY: e.clientY, worldX: orig.x, worldZ: orig.z};
      svg.setPointerCapture(e.pointerId);
      e.preventDefault();
      return;
    }

    // 3. Road Point drag on 2D map
    const ptEl = e.target.closest('circle[data-track-point]');
    if (ptEl) {
      const idx = Number(ptEl.dataset.trackPoint);
      trackPoint = idx;
      selected3dEntity = {bank: 'road', point: idx, radius: roadRadius};
      openAccordions.add('road');
      const draftObj = ensureRoadDraft();
      if (!draftObj) return;
      pointDrag = {idx, el: ptEl, moved: false, startX: e.clientX, startY: e.clientY, snapshot: draftObj.points.map(p => ({...p}))};
      svg.setPointerCapture(e.pointerId);
      e.preventDefault();
      return;
    }

    // 4. Terrain Tile click or Paint Brush stroke on 2D map
    const tileEl = e.target.closest('[data-select-terrain-cell]');
    if (tileEl) {
      const cellIdx = Number(tileEl.dataset.selectTerrainCell);
      terrainCell = cellIdx;
      openAccordions.add('terrain');
      if (terrainBrushMode) {
        brushCells = new Set([cellIdx]);
        tileEl.setAttribute('fill', terrainBrushModel === 1023 ? 'rgba(15,23,42,0.18)' : 'rgba(34,197,94,0.45)');
        svg.setPointerCapture(e.pointerId);
        e.preventDefault();
        return;
      }
      render();
      return;
    }

    // 5. Pan map
    panStart = {x: e.clientX, y: e.clientY, vb: [...mapViewBox]};
    svg.setPointerCapture(e.pointerId);
  });

  svg.addEventListener('pointermove', e => {
    if (brushCells) {
      const hit = document.elementFromPoint(e.clientX, e.clientY)?.closest?.('[data-select-terrain-cell]');
      if (hit && svg.contains(hit)) {
        const cellIdx = Number(hit.dataset.selectTerrainCell);
        terrainCell = cellIdx;
        brushCells.add(cellIdx);
        hit.setAttribute('fill', terrainBrushModel === 1023 ? 'rgba(15,23,42,0.18)' : 'rgba(34,197,94,0.45)');
      }
      return;
    }
    if (startDrag) {
      if (Math.hypot(e.clientX - startDrag.startX, e.clientY - startDrag.startY) < 3 && !startDrag.moved) return;
      startDrag.moved = true;
      const {sx, sy, worldX, worldZ} = svgToWorld(e.clientX, e.clientY);
      startDrag.el.setAttribute('transform', `translate(${sx.toFixed(1)},${sy.toFixed(1)})`);
      startDrag.worldX = worldX;
      startDrag.worldZ = worldZ;
      return;
    }
    if (arcDrag) {
      if (Math.hypot(e.clientX - arcDrag.startX, e.clientY - arcDrag.startY) < 3 && !arcDrag.moved) return;
      arcDrag.moved = true;
      const {sx, sy, worldX, worldZ} = svgToWorld(e.clientX, e.clientY);
      arcDrag.el.setAttribute('cx', sx.toFixed(1));
      arcDrag.el.setAttribute('cy', sy.toFixed(1));
      const draftObj = ensureRoadDraft();
      if (draftObj?.arcs?.[arcDrag.arcIdx]) {
        draftObj.arcs[arcDrag.arcIdx].x = worldX;
        draftObj.arcs[arcDrag.arcIdx].z = worldZ;
      }
      return;
    }
    if (objDrag) {
      if (Math.hypot(e.clientX - objDrag.startX, e.clientY - objDrag.startY) < 3 && !objDrag.moved) return;
      objDrag.moved = true;
      const {sx, sy, worldX, worldZ} = svgToWorld(e.clientX, e.clientY);
      objDrag.el.setAttribute('transform', `translate(${sx.toFixed(1)},${sy.toFixed(1)})`);
      objDrag.worldX = worldX;
      objDrag.worldZ = worldZ;
      return;
    }
    if (pointDrag) {
      if (Math.hypot(e.clientX - pointDrag.startX, e.clientY - pointDrag.startY) < 3 && !pointDrag.moved) return;
      pointDrag.moved = true;
      const {sx, sy, worldX, worldZ} = svgToWorld(e.clientX, e.clientY);
      pointDrag.el.setAttribute('cx', sx.toFixed(1));
      pointDrag.el.setAttribute('cy', sy.toFixed(1));
      const basePt = pointDrag.snapshot[pointDrag.idx];
      const dx = worldX - basePt.x, dz = worldZ - basePt.z;
      const draftObj = ensureRoadDraft();
      if (draftObj && trackScene?.road?.points) {
        draftObj.points = deformRoadPoints(trackScene.road.points, pointDrag.snapshot, pointDrag.idx, {dx, dz}, roadRadius);
      }
      return;
    }
    if (!panStart) return;
    const rect = svg.getBoundingClientRect();
    const dx = ((e.clientX - panStart.x) / Math.max(1, rect.width)) * panStart.vb[2];
    const dy = ((e.clientY - panStart.y) / Math.max(1, rect.height)) * panStart.vb[3];
    mapViewBox = [panStart.vb[0] - dx, panStart.vb[1] - dy, panStart.vb[2], panStart.vb[3]];
    applyVb();
  });

  const finishPointer = () => {
    if (brushCells) {
      const painted = [...brushCells];
      brushCells = null;
      if (painted.length) {
        (async () => {
          try {
            const entries = painted.map(index => ({type: 'cell', index, values: {model: terrainBrushModel}}));
            project = await request('saveTrackBatch', trackBank, entries, trackScene.revision);
            trackScene = await request('trackScene', trackBank);
            cachedTrackPreviewKey = '';
            active3dKey = '';
            render();
            toast(`Painted ${painted.length} terrain tile(s) with ${terrainBrushModel === 1023 ? 'Empty (1023)' : 'Unit #' + terrainBrushModel}`);
          } catch (err) {
            showError(err);
          }
        })();
      }
      return;
    }
    if (startDrag) {
      const {sIdx, moved, worldX, worldZ} = startDrag;
      startDrag = null;
      if (moved) {
        const pts = trackScene?.road?.points || [];
        let bestPt = 0;
        let bestDist = Infinity;
        pts.forEach((p, idx) => {
          const d = Math.hypot(p.x - worldX, p.z - worldZ);
          if (d < bestDist) { bestDist = d; bestPt = idx; }
        });
        (async () => {
          try {
            project = await request('saveStartSlot', trackBank, aiSeries * 12 + sIdx, {x: worldX, z: worldZ, point: bestPt}, trackScene.revision);
            trackScene = await request('trackScene', trackBank);
            render();
            toast(`Moved Start Grid P${sIdx + 1} to (X:${worldX}, Z:${worldZ}, Seg #${bestPt + 1})`);
          } catch (err) {
            showError(err);
          }
        })();
      } else {
        render();
      }
      return;
    }
    if (arcDrag) {
      const moved = arcDrag.moved;
      arcDrag = null;
      if (moved) render();
      return;
    }
    if (objDrag) {
      const {objIdx, moved, worldX, worldZ} = objDrag;
      objDrag = null;
      if (moved) {
        (async () => {
          try {
            project = await request('saveTrackObject', trackBank, objIdx, {x: worldX, z: worldZ}, trackScene.revision);
            trackScene = await request('trackScene', trackBank);
            cachedTrackPreviewKey = '';
            render();
            toast(`Moved Scenery Object #${objIdx + 1} to (X:${worldX}, Z:${worldZ})`);
          } catch (err) {
            showError(err);
          }
        })();
      } else {
        render();
      }
      return;
    }
    if (pointDrag) {
      const {idx, moved} = pointDrag;
      pointDrag = null;
      trackPoint = idx;
      render();
      if (moved) toast(`Reshaped road segment #${idx + 1} (±${roadRadius} neighbors)`);
      return;
    }
    panStart = null;
  };

  svg.addEventListener('pointerup', finishPointer);
  svg.addEventListener('pointercancel', () => { panStart = null; pointDrag = null; arcDrag = null; objDrag = null; startDrag = null; brushCells = null; });

  // Double-click on a Scenery Item on the 2D Map opens its 3D Model Preview
  svg.addEventListener('dblclick', e => {
    const objEl = e.target.closest('[data-map-object]');
    if (!objEl) return;
    e.preventDefault();
    e.stopPropagation();
    const objIdx = Number(objEl.dataset.mapObject);
    trackObject = objIdx;
    const obj = trackScene?.objects?.[objIdx];
    if (!obj || obj.model < 0) return;
    const prefix = trackAssetPrefix(trackBank);
    request('previewTrackPart', `${prefix}.course`, obj.model)
      .then(data => mountModelPreview($('#dialog'), {...data, previewName: `Scenery Object #${objIdx + 1} · Course Model #${obj.model}`}))
      .catch(showError);
  });

  // HTML5 Drag & Drop from Right Scenery Models Library onto the 2D Map
  svg.addEventListener('dragover', e => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  });
  svg.addEventListener('drop', e => {
    e.preventDefault();
    const modelStr = e.dataTransfer.getData('text/scenery-model');
    if (!modelStr) return;
    const model = Number(modelStr);
    if (!Number.isInteger(model) || model < 0) return;
    const {worldX, worldZ} = svgToWorld(e.clientX, e.clientY);
    const pts = trackScene?.road?.points || [];
    let bestPt = trackPoint;
    let bestDist = Infinity;
    pts.forEach((p, idx) => {
      const d = Math.hypot(p.x - worldX, p.z - worldZ);
      if (d < bestDist) { bestDist = d; bestPt = idx; }
    });
    const y = pts[bestPt]?.y ?? 0;
    (async () => {
      try {
        project = await request('addTrackObject', trackBank, model, bestPt, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        const newIdx = trackScene.objects.length - 1;
        project = await request('saveTrackObject', trackBank, newIdx, {x: worldX, y, z: worldZ}, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        trackObject = newIdx;
        openAccordions.add('scenery');
        cachedTrackPreviewKey = '';
        render();
        toast(`Dropped Course Model #${model} at (X:${worldX}, Y:${y}, Z:${worldZ})`);
      } catch (err) {
        showError(err);
      }
    })();
  });
}

function bindTerrainMatrixEvents() {
  const matrix = $('.studio-terrain-matrix');
  if (!matrix || !terrainBrushMode) return;
  let painting = null;

  matrix.addEventListener('pointerdown', e => {
    const btn = e.target.closest('[data-select-terrain-cell]');
    if (!btn) return;
    const idx = Number(btn.dataset.selectTerrainCell);
    terrainCell = idx;
    painting = new Set([idx]);
    btn.classList.toggle('has-mesh', terrainBrushModel !== 1023);
    matrix.setPointerCapture(e.pointerId);
    e.preventDefault();
    e.stopPropagation();
  });

  matrix.addEventListener('pointermove', e => {
    if (!painting) return;
    const hit = document.elementFromPoint(e.clientX, e.clientY)?.closest?.('[data-select-terrain-cell]');
    if (hit && matrix.contains(hit)) {
      const idx = Number(hit.dataset.selectTerrainCell);
      terrainCell = idx;
      painting.add(idx);
      hit.classList.toggle('has-mesh', terrainBrushModel !== 1023);
    }
  });

  const finishPaint = () => {
    if (!painting) return;
    const painted = [...painting];
    painting = null;
    if (!painted.length) return;
    (async () => {
      try {
        const entries = painted.map(index => ({type: 'cell', index, values: {model: terrainBrushModel}}));
        project = await request('saveTrackBatch', trackBank, entries, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        cachedTrackPreviewKey = '';
        active3dKey = '';
        render();
        toast(`Painted ${painted.length} terrain tile(s) with ${terrainBrushModel === 1023 ? 'Empty (1023)' : 'Unit #' + terrainBrushModel}`);
      } catch (err) {
        showError(err);
      }
    })();
  };

  matrix.addEventListener('pointerup', finishPaint);
  matrix.addEventListener('pointercancel', () => { painting = null; });
}

function bindElevationEvents() {
  const elevSvg = $('#studio-elev-svg');
  if (!elevSvg) return;
  let elevDrag = null;
  elevSvg.addEventListener('pointerdown', e => {
    const h = e.target.closest('[data-elev-drag]');
    if (!h) return;
    const draftObj = ensureRoadDraft();
    if (!draftObj) return;
    elevDrag = {idx: Number(h.dataset.elevDrag), dot: h, startY: e.clientY, moved: false, snapshot: draftObj.points.map(p => ({...p}))};
    elevSvg.setPointerCapture(e.pointerId);
    e.preventDefault();
  });
  elevSvg.addEventListener('pointermove', e => {
    if (!elevDrag) return;
    if (Math.abs(e.clientY - elevDrag.startY) < 2 && !elevDrag.moved) return;
    elevDrag.moved = true;
    // Dragging UP (e.clientY < startY) produces negative dy in GTE space (-Y is UP in 3D/game).
    // Use 1.0 GTE unit per pixel (or 0.25 with Shift) so tiny drags produce gentle, realistic slopes:
    const sensitivity = e.shiftKey ? 0.25 : 1.0;
    const dy = Math.round((e.clientY - elevDrag.startY) * sensitivity);
    const draftObj = ensureRoadDraft();
    if (draftObj && trackScene?.road?.points) {
      draftObj.points = deformRoadPoints(trackScene.road.points, elevDrag.snapshot, elevDrag.idx, {dy}, roadRadius);
      const pts = draftObj.points;
      const n = pts.length;
      const ys = pts.map(p => p.y);
      const rawMinY = Math.min(...ys), rawMaxY = Math.max(...ys);
      const midY = (rawMinY + rawMaxY) / 2;
      const halfSpan = Math.max(60, (rawMaxY - rawMinY) * 0.6);
      const minY = Math.round(midY - halfSpan);
      const maxY = Math.round(midY + halfSpan);
      const spanY = Math.max(120, maxY - minY);
      const plotW = 310 - 24, plotH = 76 - 20;
      const sx = i => (12 + ((n - 1 - i) / Math.max(1, n - 1)) * plotW).toFixed(1);
      const sy = y => (10 + ((y - minY) / spanY) * plotH).toFixed(1);
      const polyEl = elevSvg.querySelector('polyline');
      if (polyEl) polyEl.setAttribute('points', pts.map((p, i) => `${sx(i)},${sy(p.y)}`).join(' '));
      if (elevDrag.dot && pts[elevDrag.idx]) elevDrag.dot.setAttribute('cy', sy(pts[elevDrag.idx].y));
      const lbl = document.querySelector('[data-elev-label]');
      if (lbl && pts[elevDrag.idx]) {
        const baseY = trackScene.road.points[elevDrag.idx]?.y ?? pts[elevDrag.idx].y;
        const deltaElev = baseY - pts[elevDrag.idx].y;
        const deltaStr = deltaElev === 0 ? '±0' : (deltaElev > 0 ? `+${deltaElev}` : `${deltaElev}`);
        lbl.textContent = `${-pts[elevDrag.idx].y} (${deltaStr})`;
      }
    }
  });
  elevSvg.addEventListener('pointerup', () => {
    if (!elevDrag) return;
    const {idx, moved} = elevDrag;
    elevDrag = null;
    if (moved) { active3dKey = ''; render(); toast(`Adjusted elevation around segment #${idx + 1}`); }
  });
}

function render() {
  const root = $('#studio-app');
  if (!root || !state) return;

  if (!state.ready) {
    root.innerHTML = `
      <header class="studio-menubar">
        <div class="studio-menubar-left"><span class="studio-brand-badge">🗺️ RAGE TRACK STUDIO</span></div>
        <div class="studio-menubar-right"><button type="button" class="studio-btn" data-open-studio="classic">⚙️ Settings &amp; Saves</button></div>
      </header>
      <div style="display:flex;align-items:center;justify-content:center;height:100%;padding:32px">
        <div class="card" style="max-width:520px;text-align:center">
          <h2>Load Your Rage Racer Disc Image First</h2>
          <p class="muted">Select your legally obtained CUE, CHD, or Track 01 BIN disc image to open and edit tracks.</p>
          <button type="button" class="studio-btn primary" data-action="choose-game">Choose Disc Image…</button>
        </div>
      </div>
      <footer class="studio-statusbar"><span>No disc loaded</span><span>Rage Track Studio</span></footer>`;
    return;
  }

  const currentTrack = trackList.find(t => t.bank === trackBank) || trackList[0] || {name: 'Mythical Coast', grade: 1, bank: 88};
  const prefix = trackAssetPrefix(trackBank);
  const pts = roadDraft?.points || trackScene?.road?.points || [];
  const n = pts.length;
  const sel = Math.min(Math.max(0, trackPoint), Math.max(0, n - 1));
  const prev3dHost = (stageView === '3d' || stageView === 'tile3d') ? $('#track-studio-3d-host') : null;
  if (prev3dHost?._viewState && prev3dHost.dataset.cameraScope) {
    saved3dViewState = {key: prev3dHost.dataset.cameraScope, state: prev3dHost._viewState};
  }
  const isDirty = Boolean(project?.dirty || roadDraft || modalDirty);
  const canAct = !state.busy && !state.running;
  const courseModelsCount = trackScene?.courseModels || 0;
  const activeTileModel = getTerrainCellModel(trackScene?.terrain?.cells, terrainCell);

  const carSelectOptions = playerCarOptions.length
    ? playerCarOptions
    : [
        {bank: 10, label: 'Slot #0 · ERRISO G1'},
        {bank: 18, label: 'Slot #4 · VULCANO G1'},
        {bank: 28, label: 'Slot #9 · PEGASO G1'},
        {bank: 38, label: 'Slot #14 · ABEILLE G2'},
        {bank: 46, label: 'Slot #18 · ESPERANZA G2'},
        {bank: 56, label: 'Slot #23 · GECKO G3'},
        {bank: 68, label: 'Slot #29 · BAYONET G4'},
        {bank: 72, label: 'Slot #31 · SQUALDON SP'}
      ];

  root.innerHTML = `
    <!-- 1. TOP MENU BAR (Canonical File / Edit / Layers & View / Window + Direct Test in Race + Single Save) -->
    <header class="studio-menubar">
      <div class="studio-menubar-left">
        <div class="studio-seg">
          <button type="button" data-open-studio="car">🚗 Car Editor</button>
          <button type="button" class="active" data-open-studio="track">🗺️ Track Editor</button>
          <button type="button" data-open-studio="saves">💾 Save Editor</button>
          <button type="button" data-open-studio="settings">⚙️ Game Settings</button>
        </div>

        <details class="studio-menu">
          <summary>File ▾</summary>
          <div class="studio-menu-dropdown">
            <button type="button" data-open-load-track><span>🗺️ Load Track…</span><small>Ctrl+O</small></button>
            <button type="button" data-action="import-r4-tracks" ${!canAct ? 'disabled' : ''}><span>🏁 Import R4 Tracks (R4.BIN / Disc)…</span></button>
            <button type="button" data-open-command-palette><span>🔍 Command Palette…</span><small>Ctrl+K</small></button>
            <hr>
            <button type="button" data-import-route data-revision="${trackScene?.revision ?? 0}"><span>📥 Import Track Route (.JSON / .OBJ)…</span></button>
            <button type="button" data-export-route="json"><span>📤 Export Track Route (.JSON)…</span></button>
            <button type="button" data-export-route="obj"><span>📤 Export 3D Road Ribbon (.OBJ)…</span></button>
            <hr>
            <button type="button" data-import-track-mesh="${prefix}.course" data-revision="${trackScene?.revision ?? 0}"><span>📦 Import Scenery Mesh Bank (.RMESH)…</span></button>
            <button type="button" data-export-track-mesh="${prefix}.course"><span>📦 Export Scenery Mesh Bank (.RMESH)…</span></button>
            <button type="button" data-import-track-mesh="${prefix}.terrain" data-revision="${trackScene?.revision ?? 0}"><span>⛰️ Import Terrain Mesh Bank (.RMESH)…</span></button>
            <button type="button" data-export-track-mesh="${prefix}.terrain"><span>⛰️ Export Terrain Mesh Bank (.RMESH)…</span></button>
            <hr>
            <button type="button" data-action="restore-original-track" ${!canAct ? 'disabled' : ''}><span>↺ Restore Original Version</span></button>
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
            <hr>
            <button type="button" data-reset-road-draft ${!roadDraft ? 'disabled' : ''}><span>↺ Discard Unsaved Route Drag</span></button>
            <button type="button" data-action="restore-original-track" ${!canAct ? 'disabled' : ''}><span>↺ Restore Original Version</span></button>
          </div>
        </details>

        <details class="studio-menu">
          <summary>View ▾</summary>
          <div class="studio-menu-dropdown">
            <button type="button" data-stage-view="3d"><span>🎥 3D Full Track</span></button>
            <button type="button" data-stage-view="tile3d"><span>⛰️ 3D Terrain Unit ([${terrainCell & 31}, ${terrainCell >> 5}])</span></button>
            <button type="button" data-stage-view="2d"><span>🗺️ 2D Map (Drag &amp; Drop)</span></button>
            <hr>
            <button type="button" data-toggle-layer="terrain"><span>${trackLayers.terrain ? '✓ ' : ''}👁 Terrain Layer (32×32)</span></button>
            <button type="button" data-toggle-layer="road"><span>${trackLayers.road ? '✓ ' : ''}👁 Road Ribbon Layer</span></button>
            <button type="button" data-toggle-layer="course"><span>${trackLayers.course ? '✓ ' : ''}👁 Decorations Layer</span></button>
            <button type="button" data-toggle-layer="ai"><span>${trackLayers.ai ? '✓ ' : ''}👁 AI Racing Line (2D)</span></button>
            <button type="button" data-toggle-layer="markers"><span>${trackLayers.markers ? '✓ ' : ''}👁 Start Grid &amp; Cameras (2D)</span></button>
          </div>
        </details>
      </div>

      <div class="studio-menubar-center">
        <button type="button" class="studio-doc-pill" data-open-load-track title="Click to switch track (File -> Load Track)">
          <span class="studio-doc-dot ${isDirty ? 'dirty' : ''}"></span>
          <span>${esc(trackGradeBadge(currentTrack))} · <b>${esc(currentTrack.name)}</b> (${n} segs) ▾</span>
        </button>
        <button type="button" class="studio-btn" data-action="import-r4-tracks" ${!canAct ? 'disabled' : ''} title="Import all 8 tracks from Ridge Racer Type 4 (R4.BIN, CUE, or Track 01 BIN)">🏁 Import R4 Tracks…</button>
        <button type="button" class="studio-btn" data-open-command-palette title="Open Quick Command Palette (Ctrl+K)">🔍 Ctrl+K</button>
      </div>

      <div class="studio-menubar-right">
        <div class="studio-race-launcher" title="Choose a car and launch Rage Racer directly into a race on this track">
          <span>🚗 Car:</span>
          <select data-test-race-car aria-label="Select car for race test">
            ${carSelectOptions.map(c => `<option value="${c.bank}" ${c.bank === testCarBank ? 'selected' : ''}>${esc(c.label)}</option>`).join('')}
          </select>
          <button type="button" class="studio-race-btn" data-action="test-race" ${!canAct ? 'disabled' : ''}>🏁 Start Race on This Track</button>
        </div>
        <button type="button" class="studio-btn" data-action="restore-original-track" ${!canAct ? 'disabled' : ''} title="Restore original version of this track from the game disc">↺ Restore Original Version</button>
        <button type="button" class="studio-btn primary" data-action="save-all" ${!canAct ? 'disabled' : ''} title="Save reshaped route and apply all track changes to game (Ctrl+S)">💾 Save${isDirty ? ' *' : ''}</button>
      </div>
    </header>

    <!-- 2. MAIN WORKSPACE (Full Viewport + Right Inspector) -->
    <div class="studio-workspace ${inspectorCollapsed ? 'inspector-collapsed' : ''}">
      <main class="studio-viewport">
        <!-- Row 1: Top Viewport Bar (Viewport Mode + Layer Visibility) -->
        <div class="studio-hud-top">
          <div class="studio-hud-top-left">
            <button type="button" class="studio-chip ${stageView === '3d' ? 'active' : ''}" data-stage-view="3d">🎥 3D Full Track</button>
            <button type="button" class="studio-chip ${stageView === '2d' ? 'active' : ''}" data-stage-view="2d">🗺️ 2D Map (Drag &amp; Drop)</button>
            <button type="button" class="studio-chip ${stageView === 'tile3d' ? 'active' : ''}" data-stage-view="tile3d">⛰️ 3D Terrain Unit [${terrainCell & 31},${terrainCell >> 5}]${activeTileModel !== 1023 ? ` (#${activeTileModel})` : ''}</button>
            <button type="button" class="studio-chip ${stageView === '3d' && Boolean(prev3dHost?._viewState?.driveMode || saved3dViewState?.state?.driveMode) ? 'active' : ''}" data-toggle-drive-view title="Toggle 1st-Person Driver Camera at Road Seg #${sel + 1} (Hotkey: V, use [ / ] or Up/Down arrows to drive along track)">🏎️ Drive View (Seg #${sel + 1})</button>
            ${stageView !== 'tile3d' ? `
              <span style="width:1px;height:16px;background:#334155;margin:0 2px"></span>
              <button type="button" class="studio-chip ${trackLayers.terrain ? 'layer-on' : ''}" data-toggle-layer="terrain">👁 Terrain</button>
              <button type="button" class="studio-chip ${trackLayers.road ? 'layer-on' : ''}" data-toggle-layer="road">👁 Road</button>
              <button type="button" class="studio-chip ${trackLayers.course ? 'layer-on' : ''}" data-toggle-layer="course">👁 Decorations</button>
              ${stageView === '2d' ? `
                <button type="button" class="studio-chip ${trackLayers.ai ? 'layer-on' : ''}" data-toggle-layer="ai">👁 AI Line</button>
                <button type="button" class="studio-chip ${trackLayers.markers ? 'layer-on' : ''}" data-toggle-layer="markers">👁 Grid &amp; Cams</button>
              ` : ''}
            ` : ''}
          </div>
          <div class="studio-hud-top-right">
            ${stageView === '2d' ? `
              <button type="button" class="studio-btn" data-map-zoom="in" title="Zoom In">+</button>
              <button type="button" class="studio-btn" data-map-zoom="out" title="Zoom Out">−</button>
              <button type="button" class="studio-btn" data-map-zoom="reset" title="Fit Entire Track">Fit Track</button>
            ` : stageView === '3d' ? `
              <small style="color:#94a3b8">Click Road / Terrain / Decoration · Shift+Drag move · Right-Drag pan · <b>V</b> Drive View · <b>C</b> Focus Seg</small>
            ` : ''}
          </div>
        </div>

        <!-- Row 2: Full-Screen Stage Canvas (3D WebGL Host or 2D SVG Map) -->
        ${stageView === '2d'
          ? `<div class="studio-stage-canvas">${renderFullViewportSvg()}</div>`
          : `<div id="track-studio-3d-host" class="studio-stage-canvas" data-inline-track="${trackBank}" data-camera-scope="${trackBank}:${stageView === 'tile3d' ? 'tile:' + terrainCell : '3d'}"></div>`}
      </main>

      <!-- Right Inspector -->
      <aside class="studio-inspector" ${inspectorCollapsed ? 'hidden' : ''} aria-label="Track Inspector">
        ${renderTrackAccordion()}
      </aside>
    </div>

    <!-- 3. BOTTOM STATUS BAR -->
    <footer class="studio-statusbar">
      <span style="display:inline-flex;align-items:center;gap:8px">
        <span>Ready · ${esc(currentTrack.name)} (${esc(trackGradeBadge(currentTrack))}) · View: ${stageView.toUpperCase()} · Road Seg #${sel + 1} · Terrain [${terrainCell & 31}, ${terrainCell >> 5}] (${activeTileModel === 1023 ? 'Empty' : 'Model #' + activeTileModel})</span>
        <button type="button" class="studio-status-edits-btn" data-open-project-changes title="Open Project Changes Outliner to inspect or revert modified tracks & cars">📦 ${project?.changes?.length || 0} Project Edit(s)</button>
      </span>
      <span>[ / ] Prev/Next Seg · Ctrl+K Palette · Orbit 3D / Pan 2D · Scroll Zoom</span>
    </footer>`;

  // Mark numeric field labels in the Track Inspector as scrubbable
  for (const field of root.querySelectorAll('.studio-field')) {
    const lbl = field.querySelector('label');
    const numInp = field.querySelector('input[type="number"]');
    if (lbl && numInp) {
      lbl.classList.add('studio-scrub-label');
      if (!lbl.title) lbl.title = 'Drag horizontally on label to scrub value';
    }
  }

  bindElevationEvents();
  bindTerrainMatrixEvents();

  if (stageView === '2d') {
    bind2dMapEvents();
  } else {
    const next3dHost = $('#track-studio-3d-host');
    const nextKey = track3dKey(trackBank);
    if (prev3dHost && next3dHost && (active3dKey === nextKey || loading3dKey === nextKey)) {
      next3dHost.replaceWith(prev3dHost);
      prev3dHost.setSelectedEntity?.(selected3dEntity || {bank: 'road', point: trackPoint, radius: roadRadius});
    } else {
      if (next3dHost && saved3dViewState.state && next3dHost.dataset.cameraScope === saved3dViewState.key) {
        next3dHost._viewState = saved3dViewState.state;
      }
      active3dKey = '';
      loading3dKey = '';
      mount3dTrackViewport();
    }
  }
}

document.addEventListener('dragstart', e => {
  const chip = e.target.closest?.('[data-scenery-prefab]');
  if (!chip) return;
  e.dataTransfer.setData('text/scenery-model', chip.dataset.sceneryPrefab);
  e.dataTransfer.effectAllowed = 'copy';
});

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

  const placeSceneryBtn = e.target.closest('[data-place-scenery-model]');
  if (placeSceneryBtn) {
    e.preventDefault();
    e.stopPropagation();
    const model = Number(placeSceneryBtn.dataset.placeSceneryModel);
    (async () => {
      try {
        project = await request('addTrackObject', trackBank, model, trackPoint, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        trackObject = Math.max(0, (trackScene?.objects?.length || 1) - 1);
        openAccordions.add('scenery');
        cachedTrackPreviewKey = '';
        render();
        toast(`Placed Scenery Course Model #${model} at Road Segment #${trackPoint + 1}`);
      } catch (err) {
        showError(err);
      }
    })();
    return;
  }

  const prefab = e.target.closest('[data-scenery-prefab]');
  if (prefab) {
    const prefix = trackAssetPrefix(trackBank);
    const modelId = Number(prefab.dataset.sceneryPrefab);
    request('previewTrackPart', `${prefix}.course`, modelId)
      .then(data => mountModelPreview($('#dialog'), {...data, previewName: `Scenery Course Model #${modelId}`}))
      .catch(showError);
    return;
  }

  const b = e.target.closest('button, [data-pick-track]');
  if (!b || b.type === 'reset') return;
  if (b.tagName === 'BUTTON' && (!b.type || b.type === 'submit')) {
    if (b.closest('form') && !b.dataset.action) return;
  }
  e.preventDefault();
  closeMenus();

  (async () => {
    try {
      if (b.dataset.action === 'close') { $('#dialog').close(); return; }
      if (b.dataset.action === 'choose-game') { await request('chooseGame'); return; }
      if (b.dataset.action === 'play') { await request('play'); return; }
      if (b.dataset.action === 'import-r4-tracks') {
        if ($('#dialog')?.open) $('#dialog').close();
        await cancelPreview();
        const res = await request('importR4Tracks');
        if (!res) return;
        project = res.project || (await request('project').catch(() => project));
        const catalog = await request('tracks');
        trackList = catalog.tracks || trackList;
        trackBank = res.firstBank || 136;
        trackPoint = 0;
        trackObject = 0;
        roadDraft = null;
        cachedTrackPreview = null;
        cachedTrackPreviewKey = '';
        active3dKey = '';
        loading3dKey = '';
        trackScene = await request('trackScene', trackBank);
        const pts = trackScene?.road?.points || [];
        terrainCell = findNearestPopulatedTerrainCell(trackScene, pts[0]);
        render();
        toast(`Imported ${res.imported?.length || 8} Ridge Racer Type 4 tracks into Custom #1–#${res.imported?.length || 8}!`);
        return;
      }
      if (b.dataset.openProjectChanges !== undefined) { openProjectChangesModal(); return; }
      if (b.dataset.openCommandPalette !== undefined) { openCommandPaletteModal(''); return; }
      if (b.dataset.action === 'restore-original-track' || b.dataset.restoreTrackBank) {
        const targetBank = b.dataset.restoreTrackBank ? Number(b.dataset.restoreTrackBank) : trackBank;
        const targetTrack = trackList.find(t => t.bank === targetBank) || {name: `Track ${targetBank}`, grade: targetBank >= 136 ? 'Custom' : Math.floor((targetBank - 88) / 8) + 1};
        project = await request('restoreOriginalTrack', targetBank, project?.revision);
        if (targetBank === trackBank) {
          roadDraft = null;
          cachedTrackPreview = null;
          cachedTrackPreviewKey = '';
          active3dKey = '';
          loading3dKey = '';
          trackScene = await request('trackScene', trackBank);
        }
        if (b.dataset.restoreTrackBank && $('#dialog')?.open) {
          if ($('#dialog').querySelector('.studio-outliner-card')) openProjectChangesModal();
          else openLoadTrackModal();
        }
        render();
        toast(`Restored original version of ${trackGradeBadge(targetTrack)} · ${targetTrack.name}`);
        return;
      }
      if (b.dataset.openStudio) {
        if ($('#dialog')?.open) $('#dialog').close();
        await cancelPreview();
        await request('openStudio', b.dataset.openStudio);
        return;
      }
      if (b.dataset.toggleDriveView !== undefined) {
        const host = $('#track-studio-3d-host');
        if (stageView === '3d' && host?.setDriveView) {
          const on = host.setDriveView();
          b.classList.toggle('active', Boolean(on));
        } else {
          stageView = '3d';
          saved3dViewState = {
            key: `${trackBank}:3d`,
            state: {...(saved3dViewState.state || {}), driveMode: true}
          };
          active3dKey = '';
          render();
        }
        return;
      }
      if (b.dataset.openLoadTrack !== undefined) { openLoadTrackModal(); return; }
      if (b.dataset.pickTrack) {
        trackBank = Number(b.dataset.pickTrack);
        trackPoint = 0;
        trackObject = 0;
        roadDraft = null;
        cachedTrackPreview = null;
        cachedTrackPreviewKey = '';
        active3dKey = '';
        $('#dialog').close();
        trackScene = await request('trackScene', trackBank);
        const pts = trackScene?.road?.points || [];
        terrainCell = findNearestPopulatedTerrainCell(trackScene, pts[0]);
        render();
        return;
      }
      if (b.dataset.toggleInspector !== undefined) { inspectorCollapsed = !inspectorCollapsed; render(); return; }
      if (b.dataset.collapseAllAccordions !== undefined) {
        if (openAccordions.size > 0) openAccordions.clear();
        else ['terrain', 'road', 'scenery', 'ai', 'markers', 'atmosphere'].forEach(k => openAccordions.add(k));
        render();
        return;
      }
      if (b.dataset.jumpTrackPoint !== undefined) {
        trackPoint = Number(b.dataset.jumpTrackPoint) || 0;
        selected3dEntity = {bank: 'road', point: trackPoint, radius: roadRadius};
        if (trackLayers.isolate) active3dKey = '';
        render();
        toast(`Jumped to Road Segment #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.alignIntroCamera !== undefined) {
        const allFrames = (trackScene?.intro || []).flatMap(tbl => Array.isArray(tbl) ? tbl : (tbl ? [tbl] : []));
        const pts = trackScene?.road?.points || [];
        if (!allFrames.length || !pts.length) return;
        const nPts = pts.length;
        const entries = allFrames.map((f, idx) => {
          const t = allFrames.length > 1 ? idx / (allFrames.length - 1) : 1;
          const segIdx = (trackPoint - Math.round((1 - t) * 14) + nPts * 4) % nPts;
          const pEye = pts[segIdx];
          return {
            type: 'intro',
            index: f.id ?? f.index ?? idx,
            values: {
              x: pEye.x,
              y: (pEye.y ?? 0) - Math.round(320 - t * 140),
              z: pEye.z
            }
          };
        });
        project = await request('saveTrackBatch', trackBank, entries, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Aligned all ${allFrames.length} Race Intro Flyover camera keyframes to road approaching Seg #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.snapIntroFrame !== undefined) {
        const frameId = Number(b.dataset.snapIntroFrame);
        const pts = trackScene?.road?.points || [];
        const pEye = pts[trackPoint] || pts[0];
        if (!pEye) return;
        project = await request('saveIntro', trackBank, frameId, {
          x: pEye.x,
          y: (pEye.y ?? 0) - 240,
          z: pEye.z
        }, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Moved Intro Keyframe #${introIndex + 1} above Road Seg #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.toggleTerrainBrush !== undefined) {
        terrainBrushMode = !terrainBrushMode;
        if (terrainBrushMode) {
          const curModel = getTerrainCellModel(trackScene?.terrain?.cells, terrainCell);
          if (curModel !== 1023) terrainBrushModel = curModel;
        }
        render();
        toast(terrainBrushMode ? `Terrain Paint Brush ON (Unit #${terrainBrushModel})` : 'Terrain Paint Brush OFF');
        return;
      }
      if (b.dataset.aiSeries !== undefined) {
        aiSeries = Number(b.dataset.aiSeries) || 0;
        render();
        return;
      }
      if (b.dataset.recenterAiHints !== undefined) {
        const hints = trackScene?.ai?.hints?.[aiSeries] || [];
        const pts = trackScene?.road?.points || [];
        if (!hints.length || !pts.length) return;
        const entries = hints.map((h, i) => {
          const seg = pts[Math.min(Math.max(0, h.start), pts.length - 1)] || {leftWidth: 600, rightWidth: 600};
          const halfW = Math.max(120, Math.round(((seg.leftWidth || 600) + (seg.rightWidth || 600)) * 0.35));
          return {
            type: 'ai-hint',
            index: aiSeries * 30 + i,
            values: {min_offset: -halfW, max_offset: halfW, adjustment: 0}
          };
        });
        project = await request('saveTrackBatch', trackBank, entries, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Recentered all ${hints.length} AI racing line hints to road centerline`);
        return;
      }
      if (b.dataset.snapStartGrid !== undefined) {
        const pts = trackScene?.road?.points || [];
        if (!pts.length) return;
        const nPts = pts.length;
        const entries = Array.from({length: 12}, (_, slot) => {
          const ptIdx = (trackPoint - slot * 2 + nPts * 4) % nPts;
          const nextIdx = (ptIdx + 1) % nPts;
          const p0 = pts[ptIdx], p1 = pts[nextIdx];
          const dx = p1.x - p0.x, dz = p1.z - p0.z;
          const len = Math.max(1, Math.hypot(dx, dz));
          const nx = -dz / len, nz = dx / len;
          const side = (slot % 2 === 0 ? -1 : 1) * 180;
          return {
            type: 'start',
            index: aiSeries * 12 + slot,
            values: {
              x: Math.round(p0.x + nx * side),
              z: Math.round(p0.z + nz * side),
              point: ptIdx
            }
          };
        });
        project = await request('saveTrackBatch', trackBank, entries, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Snapped all 12 Start Grid positions (P1..P12) along road from Seg #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.snapStartSlot !== undefined) {
        const slot = Number(b.dataset.snapStartSlot);
        const pts = trackScene?.road?.points || [];
        const pt = pts[trackPoint];
        if (!pt) return;
        project = await request('saveStartSlot', trackBank, aiSeries * 12 + slot, {x: pt.x, z: pt.z, point: trackPoint}, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Moved Start Grid P${slot + 1} to Road Seg #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.snapCameraSeg !== undefined) {
        const camId = Number(b.dataset.snapCameraSeg);
        project = await request('saveCamera', trackBank, camId, {section: trackPoint}, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast(`Moved TV Camera #${camId} to Road Seg #${trackPoint + 1}`);
        return;
      }
      if (b.dataset.studioTool) {
        activeTool = b.dataset.studioTool;
        if (activeTool === 'terrain') trackLayers.terrain = true;
        render();
        return;
      }
      if (b.dataset.stageView) {
        if ($('#dialog')?.open) $('#dialog').close();
        stageView = b.dataset.stageView;
        if (stageView === 'tile3d') {
          const cells = trackScene?.terrain?.cells || [];
          if (getTerrainCellModel(cells, terrainCell) === 1023) {
            const pts = roadDraft?.points || trackScene?.road?.points || [];
            terrainCell = findNearestPopulatedTerrainCell(trackScene, pts[trackPoint]);
          }
          openAccordions.add('terrain');
        }
        active3dKey = '';
        render();
        return;
      }
      if (b.dataset.selectTerrainCell !== undefined) {
        terrainCell = Number(b.dataset.selectTerrainCell);
        selected3dEntity = {bank: 'terrain', cell: terrainCell};
        openAccordions.add('terrain');
        if (stageView === 'tile3d') active3dKey = '';
        render();
        return;
      }
      if (b.dataset.toggleLayer) {
        const l = b.dataset.toggleLayer;
        if (Object.hasOwn(trackLayers, l)) {
          trackLayers = {...trackLayers, [l]: !trackLayers[l]};
          active3dKey = '';
          render();
        }
        return;
      }
      if (b.dataset.roadRadius !== undefined) {
        roadRadius = Number(b.dataset.roadRadius);
        selected3dEntity = {bank: 'road', point: trackPoint, radius: roadRadius};
        if (trackLayers.isolate) active3dKey = '';
        render();
        return;
      }
      if (b.dataset.stepPoint) {
        const pts = trackScene?.road?.points || [];
        if (!pts.length) return;
        trackPoint = (trackPoint + Number(b.dataset.stepPoint) + pts.length) % pts.length;
        selected3dEntity = {bank: 'road', point: trackPoint, radius: roadRadius};
        if (trackLayers.isolate) active3dKey = '';
        render();
        return;
      }
      if (b.dataset.stepObject) {
        const objs = trackScene?.objects || [];
        if (!objs.length) return;
        trackObject = (trackObject + Number(b.dataset.stepObject) + objs.length) % objs.length;
        selected3dEntity = {bank: 'course', object: trackObject};
        render();
        return;
      }
      if (b.dataset.stepTerrain) {
        const [dc, dr] = b.dataset.stepTerrain.split(',').map(Number);
        const col = ((terrainCell & 31) + dc + 32) % 32;
        const row = ((terrainCell >> 5) + dr + 32) % 32;
        terrainCell = row * 32 + col;
        selected3dEntity = {bank: 'terrain', cell: terrainCell};
        trackLayers.terrain = true;
        openAccordions.add('terrain');
        if (stageView === 'tile3d') active3dKey = '';
        render();
        return;
      }
      if (b.dataset.shapeRoad) {
        const [op, amt] = b.dataset.shapeRoad.split(':');
        const draftObj = ensureRoadDraft();
        if (!draftObj || !trackScene?.road?.points) return;
        draftObj.points = shapeRoadSection(trackScene.road.points, draftObj.points, trackPoint, roadRadius, op, Number(amt) || 0);
        active3dKey = '';
        render();
        return;
      }
      if (b.dataset.resetRoadDraft !== undefined) {
        roadDraft = null;
        active3dKey = '';
        render();
        toast('Discarded unsaved route drag edits');
        return;
      }
      if (b.dataset.saveRoadDraft !== undefined) {
        if (!roadDraft || !trackScene?.road) return;
        const {entries, pointCount, arcCount} = diffRoadDraft(trackScene.road, roadDraft);
        if (!entries.length) { roadDraft = null; render(); return; }
        project = await request('saveRoadBatch', trackBank, entries, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        roadDraft = null;
        active3dKey = '';
        cachedTrackPreviewKey = '';
        render();
        toast(`Saved ${pointCount} road segment(s)${arcCount ? ` and ${arcCount} curve center(s)` : ''} to project`);
        return;
      }
      if (b.dataset.exportRoute) {
        const file = await request('exportTrackRoute', trackBank, b.dataset.exportRoute);
        if (file) toast('Route exported to ' + file);
        return;
      }
      if (b.dataset.importRoute !== undefined) {
        const result = await request('importTrackRoute', trackBank, Number(b.dataset.revision));
        if (result) {
          project = result;
          trackScene = await request('trackScene', trackBank);
          roadDraft = null;
          active3dKey = '';
          cachedTrackPreviewKey = '';
          render();
          toast('Track route imported into project');
        }
        return;
      }
      if (b.dataset.mapZoom) {
        const svg = $('#studio-track-map');
        if (!svg) return;
        const [vx, vy, vw, vh] = mapViewBox;
        if (b.dataset.mapZoom === 'reset') mapViewBox = [0, 0, 1000, 1000];
        else if (b.dataset.mapZoom === 'focus') {
          const selEl = svg.querySelector(`circle[data-track-point="${trackPoint}"]`);
          if (selEl) {
            const cx = Number(selEl.getAttribute('cx')), cy = Number(selEl.getAttribute('cy'));
            mapViewBox = [cx - 180, cy - 180, 360, 360];
          }
        } else {
          const factor = b.dataset.mapZoom === 'in' ? 0.75 : 1.33;
          const nw = Math.max(80, Math.min(2200, vw * factor)), nh = Math.max(80, Math.min(2200, vh * factor));
          mapViewBox = [vx + (vw - nw) / 2, vy + (vh - nh) / 2, nw, nh];
        }
        svg.setAttribute('viewBox', mapViewBox.map(v => v.toFixed(1)).join(' '));
        return;
      }
      if (b.dataset.previewTrackPart) {
        await mountModelPreview($('#dialog'), await request('previewTrackPart', b.dataset.previewTrackPart, Number(b.dataset.part)));
        return;
      }
      if (b.dataset.exportTrackPart) {
        const folder = await request('exportTrackPart', b.dataset.exportTrackPart, Number(b.dataset.part), b.dataset.format || 'obj');
        if (folder) toast('Exported 3D track model to ' + folder);
        return;
      }
      if (b.dataset.importTrackPart) {
        const result = await request('importTrackPart', b.dataset.importTrackPart, Number(b.dataset.part), Number(b.dataset.revision));
        if (result) {
          project = result;
          trackScene = await request('trackScene', trackBank);
          cachedTrackPreviewKey = '';
          active3dKey = '';
          render();
          toast('3D track model imported into project');
        }
        return;
      }
      if (b.dataset.exportTrackMesh) {
        if (await request('exportTrackMesh', b.dataset.exportTrackMesh)) toast('Native track mesh bank exported');
        return;
      }
      if (b.dataset.importTrackMesh) {
        const result = await request('importTrackMesh', b.dataset.importTrackMesh, Number(b.dataset.revision));
        if (result) {
          project = result;
          trackScene = await request('trackScene', trackBank);
          cachedTrackPreviewKey = '';
          active3dKey = '';
          render();
          toast('Native track mesh bank imported');
        }
        return;
      }
      if (b.dataset.trackTextures) {
        const data = await request('trackTextures', b.dataset.trackTextures, Number(b.dataset.part));
        $('#dialog').innerHTML = surfaceDialog(data);
        $('#dialog').showModal();
        return;
      }
      if (b.dataset.importTrackTexture !== undefined || b.dataset.exportTrackTexture !== undefined || b.dataset.resetTrackTexture !== undefined) {
        const host = b.closest('[data-track-surfaces]'), id = host.dataset.trackSurfaces, part = Number(host.dataset.part);
        const source = Number(b.dataset.importTrackTexture ?? b.dataset.exportTrackTexture ?? b.dataset.resetTrackTexture);
        if (b.dataset.exportTrackTexture !== undefined) {
          if (await request('exportTrackTexture', id, part, source)) toast('Track texture PNG exported');
          return;
        }
        const result = await request(b.dataset.importTrackTexture !== undefined ? 'importTrackTexture' : 'resetTrackTexture', id, part, source, Number(host.dataset.revision));
        if (result) {
          project = result;
          trackScene = await request('trackScene', trackBank);
          cachedTrackPreviewKey = '';
          active3dKey = '';
          render();
          $('#dialog').innerHTML = surfaceDialog(await request('trackTextures', id, part));
          toast('Track surface texture updated in project');
        }
        return;
      }
      if (b.dataset.duplicateTrackObject !== undefined || b.dataset.removeTrackObject !== undefined) {
        const dup = b.dataset.duplicateTrackObject !== undefined;
        const idx = Number(dup ? b.dataset.duplicateTrackObject : b.dataset.removeTrackObject);
        project = await request(dup ? 'duplicateTrackObject' : 'removeTrackObject', trackBank, idx, trackScene.revision);
        trackScene = await request('trackScene', trackBank);
        if (dup) trackObject = trackScene.objects.length - 1;
        cachedTrackPreviewKey = '';
        active3dKey = '';
        render();
        toast(dup ? 'Scenery object duplicated' : 'Scenery object removed');
        return;
      }
      if (b.dataset.projectHistory) {
        project = await request('projectHistory', b.dataset.projectHistory, project.revision);
        roadDraft = null;
        trackScene = await request('trackScene', trackBank);
        cachedTrackPreviewKey = '';
        active3dKey = '';
        if ($('#dialog')?.open && $('#dialog').querySelector('.studio-outliner-card')) {
          openProjectChangesModal();
        }
        render();
        return;
      }
      if (b.dataset.action === 'save-all' || b.dataset.action === 'apply-project') {
        if ($('#dialog')?.open) $('#dialog').close();
        if (roadDraft && trackScene?.road) {
          const {entries} = diffRoadDraft(trackScene.road, roadDraft);
          if (entries.length) {
            project = await request('saveRoadBatch', trackBank, entries, trackScene.revision);
            roadDraft = null;
          }
        }
        project = await request('applyProject', project.revision);
        trackScene = await request('trackScene', trackBank);
        render();
        toast('All track changes saved and applied to game');
        return;
      }
      if (b.dataset.action === 'test-race') {
        if ($('#dialog')?.open) $('#dialog').close();
        if (roadDraft && trackScene?.road) {
          const {entries} = diffRoadDraft(trackScene.road, roadDraft);
          if (entries.length) {
            project = await request('saveRoadBatch', trackBank, entries, trackScene.revision);
            roadDraft = null;
          }
        }
        if (project?.dirty) {
          project = await request('applyProject', project.revision);
          trackScene = await request('trackScene', trackBank);
          render();
        }
        const currentTrack = trackList.find(t => t.bank === trackBank);
        toast(`Launching race on ${currentTrack?.name || 'track'}…`);
        await request('play', {
          carBank: testCarBank,
          trackBank,
          mode: 'time-attack',
          trackPoint: trackPoint > 0 ? trackPoint : undefined
        });
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
  }
});

let activeTrackLabelScrub = null;
document.addEventListener('pointerdown', e => {
  const label = e.target.closest?.('label.studio-scrub-label');
  if (label && !e.target.closest('button')) {
    const forId = label.getAttribute('for');
    const input = forId ? document.getElementById(forId) : label.parentElement?.querySelector('input[type="number"]');
    if (input && input.type === 'number' && !input.disabled) {
      e.preventDefault();
      label.setPointerCapture?.(e.pointerId);
      const stepAttr = Number(input.step);
      const step = Number.isFinite(stepAttr) && stepAttr > 0 ? stepAttr : 1;
      activeTrackLabelScrub = {
        pointerId: e.pointerId,
        input,
        startX: e.clientX,
        startVal: Number(input.value) || 0,
        step,
        min: input.min !== '' ? Number(input.min) : -Infinity,
        max: input.max !== '' ? Number(input.max) : Infinity,
        changed: false
      };
    }
  }
});

document.addEventListener('pointermove', e => {
  if (activeTrackLabelScrub && e.pointerId === activeTrackLabelScrub.pointerId) {
    const {input, startX, startVal, step, min, max} = activeTrackLabelScrub;
    const dx = e.clientX - startX;
    const mult = e.shiftKey ? 10 : (e.altKey ? 0.2 : 1);
    const raw = startVal + Math.round(dx / 3) * step * mult;
    const clamped = Math.max(min, Math.min(max, raw));
    const formatted = step < 1 ? clamped.toFixed(2) : String(Math.round(clamped));
    if (input.value !== formatted) {
      input.value = formatted;
      activeTrackLabelScrub.changed = true;
      input.dispatchEvent(new Event('input', {bubbles: true}));
    }
  }
});

const endTrackLabelScrub = e => {
  if (activeTrackLabelScrub && e.pointerId === activeTrackLabelScrub.pointerId) {
    const {input, changed} = activeTrackLabelScrub;
    activeTrackLabelScrub = null;
    if (changed) {
      input.dispatchEvent(new Event('change', {bubbles: true}));
    }
  }
};
document.addEventListener('pointerup', endTrackLabelScrub);
document.addEventListener('pointercancel', endTrackLabelScrub);

document.addEventListener('keydown', e => {
  if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 's') {
    e.preventDefault();
    const saveBtn = document.querySelector('[data-action="save-all"].primary');
    saveBtn?.click();
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'o') {
    e.preventDefault();
    openLoadTrackModal();
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'k') {
    e.preventDefault();
    openCommandPaletteModal('');
  } else if (e.key === 'Enter' && e.target.id === 'studio-command-palette-input') {
    e.preventDefault();
    const first = $('#studio-command-palette-list .studio-palette-item');
    if (first) first.click();
  } else if (!e.metaKey && !e.ctrlKey && !e.altKey) {
    const tag = e.target?.tagName;
    if (tag === 'INPUT' || tag === 'SELECT' || tag === 'TEXTAREA' || e.target?.isContentEditable || $('#dialog')?.open) return;
    const k = e.key.toLowerCase();
    if (e.key === '[' || e.key === ']') {
      const pts = trackScene?.road?.points || [];
      if (!pts.length) return;
      e.preventDefault();
      trackPoint = (trackPoint + (e.key === ']' ? 1 : -1) + pts.length) % pts.length;
      selected3dEntity = {bank: 'road', point: trackPoint, radius: roadRadius};
      if (trackLayers.isolate) active3dKey = '';
      render();
    } else if (k === 'v' && e.target?.tagName !== 'CANVAS') {
      e.preventDefault();
      const btn = document.querySelector('[data-toggle-drive-view]');
      if (btn) btn.click();
    } else if (k === 'c' && stageView === '3d' && e.target?.tagName !== 'CANVAS') {
      e.preventDefault();
      $('#track-studio-3d-host')?.focusSelection?.();
    }
  }
});

document.addEventListener('change', e => {
  if (e.target.matches('[data-test-race-car]')) {
    testCarBank = Number(e.target.value) || 10;
    document.querySelectorAll('[data-test-race-car]').forEach(el => { el.value = String(testCarBank); });
    return;
  }
  if (e.target.matches('[data-terrain-brush-model]')) {
    terrainBrushModel = Number(e.target.value);
    terrainBrushMode = true;
    render();
    return;
  }
  if (e.target.matches('[data-track-scrubber]')) {
    trackPoint = Number(e.target.value);
    selected3dEntity = {bank: 'road', point: trackPoint, radius: roadRadius};
    if (trackLayers.isolate) active3dKey = '';
    render();
    return;
  }
  if (e.target.matches('[data-select-object]')) {
    trackObject = Number(e.target.value);
    selected3dEntity = {bank: 'course', object: trackObject};
    render();
    return;
  }
  if (e.target.matches('[data-select-ai-hint]')) {
    aiHintIndex = Number(e.target.value) || 0;
    const h = trackScene?.ai?.hints?.[aiSeries]?.[aiHintIndex];
    if (h && Number.isInteger(h.start)) trackPoint = h.start;
    render();
    return;
  }
  if (e.target.matches('[data-select-ai-speed]')) {
    aiSpeedIndex = Number(e.target.value) || 0;
    const s = trackScene?.ai?.speeds?.[aiSeries]?.[aiSpeedIndex];
    if (s && Number.isInteger(s.progress)) trackPoint = s.progress;
    render();
    return;
  }
  if (e.target.matches('[data-select-start-slot]')) {
    startSlotIndex = Number(e.target.value) || 0;
    const st = trackScene?.ai?.starts?.[aiSeries]?.[startSlotIndex];
    if (st && Number.isInteger(st.point)) trackPoint = st.point;
    render();
    return;
  }
  if (e.target.matches('[data-select-camera]')) {
    cameraIndex = Number(e.target.value) || 0;
    const allCameras = (trackScene?.cameras || []).flatMap(tbl => tbl || []);
    const cam = allCameras[cameraIndex];
    if (cam && Number.isInteger(cam.section)) trackPoint = cam.section;
    render();
    return;
  }
  if (e.target.matches('[data-select-env-cue]')) {
    envCueIndex = Number(e.target.value) || 0;
    const cue = trackScene?.environment?.cues?.[envCueIndex];
    if (cue && Number.isInteger(cue.progress)) trackPoint = cue.progress;
    render();
    return;
  }
  if (e.target.matches('[data-select-intro-frame]')) {
    introIndex = Number(e.target.value) || 0;
    render();
    return;
  }
  if (e.target.matches('[data-road-field],[data-terrain-model],[data-object-field],[data-ai-hint-field],[data-ai-speed-field],[data-start-field],[data-camera-field],[data-env-field],[data-intro-field]')) {
    e.target.closest('form')?.requestSubmit();
  }
});

document.addEventListener('submit', e => {
  e.preventDefault();
  (async () => {
    try {
      if (e.target.id === 'road-point-form') {
        const idx = Number(e.target.dataset.index);
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-road-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.roadField, Number(i.value)])
        );
        if (roadDraft || ('y' in values && trackScene?.road?.points)) {
          const draftObj = ensureRoadDraft();
          const pt = draftObj?.points?.[idx];
          if (pt) {
            for (const [k, v] of Object.entries(values)) {
              pt[{left_width: 'leftWidth', right_width: 'rightWidth', cross_slope: 'crossSlope'}[k] || k] = v;
            }
            if ('y' in values && trackScene?.road?.points) {
              draftObj.points = recalculateRoadGeometry(trackScene.road.points, draftObj.points);
            }
          }
          const {entries} = diffRoadDraft(trackScene.road, draftObj);
          if (entries.length) project = await request('saveRoadBatch', Number(e.target.dataset.bank), entries, Number(e.target.dataset.revision));
          roadDraft = null;
        } else if (Object.keys(values).length) {
          project = await request('saveRoadPoint', Number(e.target.dataset.bank), idx, values, Number(e.target.dataset.revision));
        }
        trackScene = await request('trackScene', trackBank);
        cachedTrackPreviewKey = '';
        active3dKey = '';
        render();
        toast(`Saved Road Segment #${idx + 1}`);
        return;
      }
      if (e.target.id === 'terrain-cell-form') {
        const cell = Number(e.target.dataset.index);
        const modelEl = e.target.querySelector('[data-terrain-model]');
        if (modelEl && modelEl.value !== modelEl.dataset.original) {
          project = await request('saveTerrainCell', Number(e.target.dataset.bank), cell, {model: Number(modelEl.value)}, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          cachedTrackPreviewKey = '';
          active3dKey = '';
          render();
          toast(`Saved Terrain Segment [${cell & 31}, ${cell >> 5}]`);
        }
        return;
      }
      if (e.target.id === 'track-object-form') {
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-object-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.objectField, i.dataset.objectField === 'yaw' ? Math.round(Number(i.value) * 4096 / 360) : Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveTrackObject', Number(e.target.dataset.bank), Number(e.target.dataset.index), values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          cachedTrackPreviewKey = '';
          active3dKey = '';
          render();
          toast('Scenery object saved to project');
        }
        return;
      }
      if (e.target.id === 'ai-hint-form') {
        const series = Number(e.target.dataset.series) || 0;
        const idx = Number(e.target.dataset.index) || 0;
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-ai-hint-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.aiHintField, Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveAiHint', Number(e.target.dataset.bank), series * 30 + idx, values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast(`Saved AI Racing Line Hint #${idx + 1}`);
        }
        return;
      }
      if (e.target.id === 'ai-speed-form') {
        const series = Number(e.target.dataset.series) || 0;
        const idx = Number(e.target.dataset.index) || 0;
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-ai-speed-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.aiSpeedField, Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveAiSpeed', Number(e.target.dataset.bank), series * 48 + idx, values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast(`Saved AI Speed Zone #${idx + 1}`);
        }
        return;
      }
      if (e.target.id === 'start-slot-form') {
        const series = Number(e.target.dataset.series) || 0;
        const idx = Number(e.target.dataset.index) || 0;
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-start-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.startField, Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveStartSlot', Number(e.target.dataset.bank), series * 12 + idx, values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast(`Saved Start Grid P${idx + 1}`);
        }
        return;
      }
      if (e.target.id === 'camera-form') {
        const camId = Number(e.target.dataset.cameraId);
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-camera-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.cameraField, Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveCamera', Number(e.target.dataset.bank), camId, values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast(`Saved TV Camera #${camId}`);
        }
        return;
      }
      if (e.target.id === 'intro-camera-form') {
        const idx = Number(e.target.dataset.index) || 0;
        const values = Object.fromEntries(
          [...e.target.querySelectorAll('[data-intro-field]')]
            .filter(i => i.value !== i.dataset.original)
            .map(i => [i.dataset.introField, Number(i.value)])
        );
        if (Object.keys(values).length) {
          project = await request('saveIntro', Number(e.target.dataset.bank), idx, values, Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast(`Saved Intro Flyover Keyframe #${introIndex + 1}`);
        }
        return;
      }
      if (e.target.id === 'environment-form') {
        const grouped = new Map();
        for (const input of e.target.querySelectorAll('[data-env-field]')) {
          if (input.value === input.dataset.original) continue;
          const index = Number(input.dataset.envIndex), type = input.dataset.envType || 'env', key = type + ':' + index;
          if (!grouped.has(key)) grouped.set(key, {type, index, values: {}});
          grouped.get(key).values[input.dataset.envField] = input.type === 'color' ? parseInt(input.value.slice(1), 16) : Number(input.value);
        }
        if (grouped.size) {
          project = await request('saveEnvironment', Number(e.target.dataset.bank), [...grouped.values()], Number(e.target.dataset.revision));
          trackScene = await request('trackScene', trackBank);
          render();
          toast('Atmosphere & sky colors saved to project');
        }
        return;
      }
    } catch (err) {
      showError(err);
    }
  })();
});

api.onState(async value => {
  state = previewBusy ? {...value, busy: null} : value;
  project = value.project;
  if (booting || materialBusy) return;
  if (value.mcpUpdated && state.ready) {
    trackScene = await request('trackScene', trackBank);
    roadDraft = null;
    active3dKey = '';
  }
  if (!modalDirty && !roadDraft) render();
});

(async () => {
  try {
    state = await request('snapshot');
    project = state.project;
    if (state.ready) {
      await cancelPreview();
      trackList = (await request('tracks')).tracks;
      const carsRes = await request('cars').catch(() => ({parts: []}));
      playerCarOptions = (carsRes?.parts || [])
        .filter(c => !c.rival && c.part === 0)
        .sort((a, b) => a.bank - b.bank)
        .map(c => {
          const slot = c.bank >= 200 ? 32 + Math.floor((c.bank - 200) / 2) : Math.floor((c.bank - 10) / 2);
          return {bank: c.bank, label: `Slot #${slot} · ${c.name}`};
        });
      project = await request('project');
      for (let attempt = 0; attempt < 25; attempt++) {
        try {
          trackScene = await request('trackScene', trackBank);
          break;
        } catch (err) {
          if (!/Wait for the current operation|Another operation|Operation canceled/.test(err?.message || '')) throw err;
          await new Promise(r => setTimeout(r, 120));
        }
      }
      const pts = trackScene?.road?.points || [];
      terrainCell = findNearestPopulatedTerrainCell(trackScene, pts[0]);
    }
  } catch (err) {
    showError(err);
  } finally {
    booting = false;
    render();
  }
})();

installMaterialEditor({request,onError:showError,onBegin:async()=>{materialBusy=true;await cancelPreview();},onEnd:()=>{materialBusy=false;render();},onChanged:async p=>{project=p;trackScene=await request('trackScene',trackBank);cachedTrackPreviewKey='';active3dKey='';}});
