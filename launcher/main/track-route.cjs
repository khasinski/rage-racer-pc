const fs = require('node:fs/promises');

function clampInt16(v) {
  return Math.max(-32768, Math.min(32767, Math.round(Number(v) || 0)));
}

function clampInt32(v) {
  return Math.max(-2147483648, Math.min(2147483647, Math.round(Number(v) || 0)));
}

function normalizeAngleDeltaRad(rad) {
  let d = rad;
  while (d > Math.PI) d -= 2 * Math.PI;
  while (d < -Math.PI) d += 2 * Math.PI;
  return d;
}

function recalculatePointGeometry(basePoints, nextPoints) {
  const n = basePoints.length;
  const out = nextPoints.map(p => ({ ...p }));
  for (let i = 0; i < n; i++) {
    const b0 = basePoints[i], b1 = basePoints[(i + 1) % n];
    const d0 = out[i], d1 = out[(i + 1) % n];
    const moved = d0.x !== b0.x || d0.y !== b0.y || d0.z !== b0.z || d1.x !== b1.x || d1.y !== b1.y || d1.z !== b1.z;
    if (!moved) continue;
    const origDx = b1.x - b0.x, origDz = b1.z - b0.z;
    const newDx = d1.x - d0.x, newDz = d1.z - d0.z;
    const origChord = Math.hypot(origDx, origDz);
    const newChord = Math.hypot(newDx, newDz);
    if (d0.length === b0.length) {
      const ratio = origChord > 1 ? newChord / origChord : 1;
      d0.length = Math.max(1, Math.min(32767, Math.round((b0.length || 64) * ratio)));
    }
    if (d0.angle === b0.angle && newChord > 0.5 && origChord > 0.5) {
      const dRad = normalizeAngleDeltaRad(Math.atan2(newDz, newDx) - Math.atan2(origDz, origDx));
      const dUnits = Math.round((dRad * 4096) / (2 * Math.PI));
      const raw = (b0.angle || 0) + dUnits;
      d0.angle = b0.angle < 0 ? (((raw + 2048) % 4096 + 4096) % 4096 - 2048) : ((raw % 4096 + 4096) % 4096);
    }
    if (d0.pitch === b0.pitch) {
      const origPitch = Math.atan2((b0.y || 0) - (b1.y || 0), Math.max(1, origChord));
      const newPitch = Math.atan2((d0.y || 0) - (d1.y || 0), Math.max(1, newChord));
      const dPitch = Math.round((normalizeAngleDeltaRad(newPitch - origPitch) * 4096) / (2 * Math.PI));
      d0.pitch = clampInt16((b0.pitch || 0) + dPitch);
    }
  }
  return out;
}

function trackPrefix(bank) {
  if (!Number.isInteger(bank) || bank < 88 || bank > 134 || bank % 2) throw Error('Invalid track bank');
  return `track.${['big', 'mid', 'hi', 'oval'][((bank - 88) / 2) % 4]}${Math.floor((bank - 88) / 8) + 1}`;
}

async function saveRoadBatch(service, bank, entries, revision) {
  service.requireReady();
  const prefix = trackPrefix(bank) + '.';
  if (!Array.isArray(entries) || !entries.length || entries.length > 10000) throw Error('Invalid road batch edits');
  const roadFields = ['x', 'y', 'z', 'angle', 'pitch', 'cross_slope', 'left_width', 'right_width', 'length', 'arc'];
  const arcFields = ['x', 'z'];
  const changes = [];
  for (const entry of entries) {
    const type = entry?.type === 'arc' ? 'arc' : 'road';
    const index = Number(entry?.index);
    const allowed = type === 'arc' ? arcFields : roadFields;
    if (!Number.isInteger(index) || index < 0 || index > 10000 || !entry?.values || typeof entry.values !== 'object') {
      throw Error('Invalid road batch entry');
    }
    for (const [rawField, rawVal] of Object.entries(entry.values)) {
      const field = { leftWidth: 'left_width', rightWidth: 'right_width', crossSlope: 'cross_slope' }[rawField] || rawField;
      if (!allowed.includes(field)) throw Error('Unsupported road field: ' + field);
      const value = Math.round(Number(rawVal));
      if (!Number.isSafeInteger(value)) throw Error('Invalid value for ' + field);
      changes.push({ kind: 'track', id: `${prefix}${type}.${index}`, field, value });
    }
  }
  if (!changes.length) throw Error('No road changes to save');
  return service.saveTrackChanges(bank, changes, revision);
}

function buildRouteObj(road, trackId, bank) {
  const points = road?.points || [];
  const arcs = road?.arcs || [];
  const lines = [
    `# Rage Racer Track Route OBJ`,
    `# format: rage-racer-route-v1`,
    `# track: ${trackId} (bank ${bank})`,
    `# points: ${points.length}, arcs: ${arcs.length}`,
    `# Move centerline vertices in 3D (o road_centerline) and re-import this .obj to reshape the driveable route.`
  ];
  for (let i = 0; i < arcs.length; i++) {
    lines.push(`# rr_arc ${i} ${arcs[i].x} ${arcs[i].z}`);
  }
  lines.push('o road_centerline');
  for (let i = 0; i < points.length; i++) {
    const p = points[i];
    lines.push(`v ${p.x} ${p.y} ${p.z}`);
  }
  if (points.length > 1) {
    lines.push('l ' + Array.from({ length: points.length }, (_, i) => i + 1).concat([1]).join(' '));
  }
  lines.push('o road_ribbon');
  const baseIdx = points.length;
  for (let i = 0; i < points.length; i++) {
    const prev = points[(i - 1 + points.length) % points.length];
    const curr = points[i];
    const next = points[(i + 1) % points.length];
    const tx = next.x - prev.x, tz = next.z - prev.z;
    const len = Math.hypot(tx, tz) || 1;
    const nx = -tz / len, nz = tx / len;
    const lw = Math.max(20, Number(curr.leftWidth) || 0) * 4;
    const rw = Math.max(20, Number(curr.rightWidth) || 0) * 4;
    const slope = Number(curr.crossSlope) || 0;
    const ly = Math.round(curr.y + (slope * lw) / 512);
    const ry = Math.round(curr.y - (slope * rw) / 512);
    lines.push(`v ${Math.round(curr.x + nx * lw)} ${ly} ${Math.round(curr.z + nz * lw)}`);
    lines.push(`v ${Math.round(curr.x - nx * rw)} ${ry} ${Math.round(curr.z - nz * rw)}`);
  }
  for (let i = 0; i < points.length; i++) {
    const next = (i + 1) % points.length;
    const l0 = baseIdx + i * 2 + 1, r0 = baseIdx + i * 2 + 2;
    const l1 = baseIdx + next * 2 + 1, r1 = baseIdx + next * 2 + 2;
    lines.push(`f ${l0} ${r0} ${r1} ${l1}`);
  }
  return lines.join('\n') + '\n';
}

async function exportTrackRoute(service, bank, destination) {
  service.requireReady();
  const trackId = trackPrefix(bank);
  const scene = await service.trackScene(bank);
  if (destination.toLowerCase().endsWith('.obj')) {
    await fs.writeFile(destination, buildRouteObj(scene.road, trackId, bank), 'utf8');
    return destination;
  }
  const payload = {
    format: 'rage-racer-route-v1',
    bank,
    trackId,
    points: scene.road.points,
    arcs: scene.road.arcs
  };
  await fs.writeFile(destination, JSON.stringify(payload, null, 2) + '\n', 'utf8');
  return destination;
}

function diffRoadAndArcs(baseRoad, nextPoints, nextArcs) {
  const entries = [];
  const basePts = baseRoad.points || [];
  const recalcPts = recalculatePointGeometry(basePts, nextPoints);
  const fieldMap = [
    ['x', 'x'], ['y', 'y'], ['z', 'z'],
    ['angle', 'angle'], ['pitch', 'pitch'],
    ['crossSlope', 'cross_slope'],
    ['leftWidth', 'left_width'], ['rightWidth', 'right_width'],
    ['length', 'length'], ['arc', 'arc']
  ];
  for (let i = 0; i < Math.min(basePts.length, recalcPts.length); i++) {
    const b = basePts[i], n = recalcPts[i];
    const values = {};
    for (const [prop, field] of fieldMap) {
      const altProp = prop === 'crossSlope' ? 'cross_slope' : prop === 'leftWidth' ? 'left_width' : prop === 'rightWidth' ? 'right_width' : prop;
      const rawVal = n[prop] !== undefined ? n[prop] : n[altProp];
      if (rawVal === undefined) continue;
      const val = ['x', 'z'].includes(prop) ? clampInt32(rawVal) : prop === 'length' ? Math.max(1, Math.min(32767, Math.round(Number(rawVal) || 1))) : prop === 'arc' ? Math.max(0, Math.min(32767, Math.round(Number(rawVal) || 0))) : clampInt16(rawVal);
      if (val !== b[prop]) values[field] = val;
    }
    if (Object.keys(values).length) entries.push({ type: 'road', index: i, values });
  }
  const baseArcs = baseRoad.arcs || [];
  if (Array.isArray(nextArcs)) {
    for (let i = 0; i < Math.min(baseArcs.length, nextArcs.length); i++) {
      const b = baseArcs[i], n = nextArcs[i];
      const values = {};
      if (n?.x !== undefined && clampInt32(n.x) !== b.x) values.x = clampInt32(n.x);
      if (n?.z !== undefined && clampInt32(n.z) !== b.z) values.z = clampInt32(n.z);
      if (Object.keys(values).length) entries.push({ type: 'arc', index: i, values });
    }
  }
  return entries;
}

async function importTrackRoute(service, bank, sourceFile, revision) {
  service.requireReady();
  const scene = await service.trackScene(bank);
  if (revision !== undefined && scene.revision !== revision) {
    throw Error('Project changed; reload before importing route');
  }
  const text = await fs.readFile(sourceFile, 'utf8');
  const basePts = scene.road.points || [];
  let nextPoints = basePts.map(p => ({ ...p }));
  let nextArcs = (scene.road.arcs || []).map(a => ({ ...a }));

  if (sourceFile.toLowerCase().endsWith('.obj') || text.trimStart().startsWith('#') || text.trimStart().startsWith('v ')) {
    const verts = [];
    let currentObj = '';
    const centerVerts = [];
    for (const rawLine of text.split(/\r?\n/)) {
      const line = rawLine.trim();
      if (line.startsWith('o ') || line.startsWith('g ')) {
        currentObj = line.slice(2).trim();
      } else if (line.startsWith('# rr_arc ')) {
        const parts = line.split(/\s+/);
        const idx = Number(parts[2]);
        if (nextArcs[idx]) {
          nextArcs[idx].x = clampInt32(parts[3]);
          nextArcs[idx].z = clampInt32(parts[4]);
        }
      } else if (line.startsWith('v ')) {
        const parts = line.split(/\s+/);
        const v = { x: clampInt32(parts[1]), y: clampInt16(parts[2]), z: clampInt32(parts[3]) };
        verts.push(v);
        if (currentObj === 'road_centerline') centerVerts.push(v);
      }
    }
    const sourceVerts = centerVerts.length === basePts.length ? centerVerts : verts.slice(0, basePts.length);
    if (sourceVerts.length < basePts.length) {
      throw Error(`Route OBJ has ${sourceVerts.length} vertices, expected ${basePts.length} road points`);
    }
    for (let i = 0; i < basePts.length; i++) {
      nextPoints[i].x = sourceVerts[i].x;
      nextPoints[i].y = sourceVerts[i].y;
      nextPoints[i].z = sourceVerts[i].z;
    }
  } else {
    const data = JSON.parse(text);
    const pts = Array.isArray(data) ? data : data?.points || data?.road?.points;
    if (!Array.isArray(pts) || pts.length !== basePts.length) {
      throw Error(`Route JSON must contain ${basePts.length} road points`);
    }
    nextPoints = basePts.map((b, i) => ({ ...b, ...pts[i] }));
    if (Array.isArray(data?.arcs || data?.road?.arcs)) {
      nextArcs = data.arcs || data.road.arcs;
    }
  }

  const entries = diffRoadAndArcs(scene.road, nextPoints, nextArcs);
  if (!entries.length) return service.project();
  return saveRoadBatch(service, bank, entries, scene.revision);
}

module.exports = {
  recalculatePointGeometry,
  saveRoadBatch,
  exportTrackRoute,
  importTrackRoute,
  buildRouteObj,
  diffRoadAndArcs
};
