const escape=value=>String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

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

export function recalculateRoadGeometry(basePoints, draftPoints) {
  const n = basePoints?.length || 0;
  if (!n) return [];
  const out = draftPoints.map(p => ({ ...p }));
  for (let i = 0; i < n; i++) {
    const b0 = basePoints[i], b1 = basePoints[(i + 1) % n];
    const d0 = out[i], d1 = out[(i + 1) % n];
    const moved = d0.x !== b0.x || d0.y !== b0.y || d0.z !== b0.z || d1.x !== b1.x || d1.y !== b1.y || d1.z !== b1.z;
    if (!moved) continue;
    const origDx = b1.x - b0.x, origDz = b1.z - b0.z;
    const newDx = d1.x - d0.x, newDz = d1.z - d0.z;
    const origChord = Math.hypot(origDx, origDz);
    const newChord = Math.hypot(newDx, newDz);
    const ratio = origChord > 1 ? newChord / origChord : 1;
    d0.length = Math.max(1, Math.min(32767, Math.round((b0.length || 64) * ratio)));
    if (newChord > 0.5 && origChord > 0.5) {
      const dRad = normalizeAngleDeltaRad(Math.atan2(newDz, newDx) - Math.atan2(origDz, origDx));
      const dUnits = Math.round((dRad * 4096) / (2 * Math.PI));
      const raw = (b0.angle || 0) + dUnits;
      d0.angle = b0.angle < 0 ? (((raw + 2048) % 4096 + 4096) % 4096 - 2048) : ((raw % 4096 + 4096) % 4096);
    }
    // In PS1 GTE space, -Y is UP, so climbing uphill (p1.y < p0.y) has positive pitch:
    const origPitch = Math.atan2((b0.y || 0) - (b1.y || 0), Math.max(1, origChord));
    const newPitch = Math.atan2((d0.y || 0) - (d1.y || 0), Math.max(1, newChord));
    const dPitch = Math.round((normalizeAngleDeltaRad(newPitch - origPitch) * 4096) / (2 * Math.PI));
    d0.pitch = clampInt16((b0.pitch || 0) + dPitch);
  }
  return out;
}

export function deformRoadPoints(basePoints, sourcePoints, centerIndex, delta, radius = 6) {
  const n = sourcePoints?.length || 0;
  if (!n) return [];
  const next = sourcePoints.map(p => ({ ...p }));
  const r = Math.max(0, Math.min(Math.floor((n - 1) / 2), Number(radius) || 0));
  const dx = Number(delta?.dx) || 0;
  const dy = Number(delta?.dy) || 0;
  const dz = Number(delta?.dz) || 0;
  for (let offset = -r; offset <= r; offset++) {
    const idx = (centerIndex + offset + n) % n;
    const weight = r === 0 ? 1 : 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
    const pt = next[idx];
    if (dx) pt.x = clampInt32(pt.x + Math.round(dx * weight));
    if (dy) pt.y = clampInt16(pt.y + Math.round(dy * weight));
    if (dz) pt.z = clampInt32(pt.z + Math.round(dz * weight));
  }
  return recalculateRoadGeometry(basePoints, next);
}

export function shapeRoadSection(basePoints, currentPoints, centerIndex, radius = 6, operation = 'smooth-xz', amount = 0) {
  const n = currentPoints?.length || 0;
  if (!n) return [];
  const next = currentPoints.map(p => ({ ...p }));
  const r = Math.max(1, Math.min(Math.floor((n - 1) / 2), Number(radius) || 6));

  if (operation === 'smooth-xz' || operation === 'smooth') {
    for (let pass = 0; pass < 2; pass++) {
      const snap = next.map(p => ({ ...p }));
      for (let offset = -r; offset <= r; offset++) {
        const idx = (centerIndex + offset + n) % n;
        const prev = snap[(idx - 1 + n) % n];
        const curr = snap[idx];
        const succ = snap[(idx + 1) % n];
        const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
        const avgX = (prev.x + curr.x * 2 + succ.x) / 4;
        const avgZ = (prev.z + curr.z * 2 + succ.z) / 4;
        next[idx].x = clampInt32(curr.x + (avgX - curr.x) * w);
        next[idx].z = clampInt32(curr.z + (avgZ - curr.z) * w);
      }
    }
  } else if (operation === 'smooth-y' || operation === 'level') {
    for (let pass = 0; pass < 2; pass++) {
      const snap = next.map(p => ({ ...p }));
      for (let offset = -r; offset <= r; offset++) {
        const idx = (centerIndex + offset + n) % n;
        const prev = snap[(idx - 1 + n) % n];
        const curr = snap[idx];
        const succ = snap[(idx + 1) % n];
        const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
        const avgY = (prev.y + curr.y * 2 + succ.y) / 4;
        next[idx].y = clampInt16(curr.y + (avgY - curr.y) * w);
      }
    }
  } else if (operation === 'straighten') {
    const startIdx = (centerIndex - r + n) % n;
    const endIdx = (centerIndex + r + n) % n;
    const pA = next[startIdx], pB = next[endIdx];
    const span = r * 2;
    for (let step = 0; step <= span; step++) {
      const idx = (startIdx + step) % n;
      const t = span > 0 ? step / span : 0;
      next[idx].x = clampInt32(pA.x + (pB.x - pA.x) * t);
      next[idx].z = clampInt32(pA.z + (pB.z - pA.z) * t);
      if (step < span) next[idx].arc = 0;
    }
  } else if (operation === 'lateral') {
    const snap = next.map(p => ({ ...p }));
    for (let offset = -r; offset <= r; offset++) {
      const idx = (centerIndex + offset + n) % n;
      const prev = snap[(idx - 1 + n) % n];
      const succ = snap[(idx + 1) % n];
      const tx = succ.x - prev.x, tz = succ.z - prev.z;
      const len = Math.hypot(tx, tz) || 1;
      const nx = -tz / len, nz = tx / len;
      const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
      next[idx].x = clampInt32(snap[idx].x + nx * amount * w);
      next[idx].z = clampInt32(snap[idx].z + nz * amount * w);
    }
  } else if (operation === 'height') {
    for (let offset = -r; offset <= r; offset++) {
      const idx = (centerIndex + offset + n) % n;
      const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
      next[idx].y = clampInt16(next[idx].y + amount * w);
    }
  } else if (operation === 'width' || operation === 'widen') {
    for (let offset = -r; offset <= r; offset++) {
      const idx = (centerIndex + offset + n) % n;
      const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
      const deltaW = Math.round(amount * w);
      next[idx].leftWidth = Math.max(10, clampInt16((next[idx].leftWidth || 0) + deltaW));
      next[idx].rightWidth = Math.max(10, clampInt16((next[idx].rightWidth || 0) + deltaW));
    }
    return next;
  } else if (operation === 'bank') {
    for (let offset = -r; offset <= r; offset++) {
      const idx = (centerIndex + offset + n) % n;
      if (amount === 0) {
        next[idx].crossSlope = 0;
      } else {
        const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
        next[idx].crossSlope = clampInt16((next[idx].crossSlope || 0) + Math.round(amount * w));
      }
    }
    return next;
  }
  return recalculateRoadGeometry(basePoints, next);
}

export function diffRoadDraft(baseRoad, draftRoad) {
  const entries = [];
  let pointCount = 0, arcCount = 0;
  const basePts = baseRoad?.points || [];
  const draftPts = draftRoad?.points || [];
  const fieldPairs = [
    ['x', 'x'], ['y', 'y'], ['z', 'z'],
    ['angle', 'angle'], ['pitch', 'pitch'],
    ['crossSlope', 'cross_slope'],
    ['leftWidth', 'left_width'], ['rightWidth', 'right_width'],
    ['length', 'length'], ['arc', 'arc']
  ];
  for (let i = 0; i < Math.min(basePts.length, draftPts.length); i++) {
    const b = basePts[i], d = draftPts[i];
    const values = {};
    for (const [prop, field] of fieldPairs) {
      if (d[prop] !== undefined && d[prop] !== b[prop]) {
        values[field] = Math.round(Number(d[prop]));
      }
    }
    if (Object.keys(values).length) {
      entries.push({ type: 'road', index: i, values });
      pointCount++;
    }
  }
  const baseArcs = baseRoad?.arcs || [];
  const draftArcs = draftRoad?.arcs || [];
  for (let i = 0; i < Math.min(baseArcs.length, draftArcs.length); i++) {
    const b = baseArcs[i], d = draftArcs[i];
    const values = {};
    if (d?.x !== undefined && d.x !== b.x) values.x = Math.round(Number(d.x));
    if (d?.z !== undefined && d.z !== b.z) values.z = Math.round(Number(d.z));
    if (Object.keys(values).length) {
      entries.push({ type: 'arc', index: i, values });
      arcCount++;
    }
  }
  return { entries, pointCount, arcCount };
}

function nearRef3d(value, ref) {
  return ref + (((((Number(value) || 0) - ref) << 16) >> 16));
}

export function build3dTrackView(rawPreview, scene, effectiveRoad, selected = 0, radius = 6, layers = {}) {
  if (!rawPreview) return { data: null, frame: null };
  const showRoad = layers.road !== false;
  const showTerrain = layers.terrain !== false;
  const showCourse = layers.course !== false;
  const isolate = Boolean(layers.isolate);

  const road = effectiveRoad || scene?.road || { points: [], arcs: [] };
  const points = road.points || [];
  const n = points.length;
  const selIdx = n > 0 ? ((Number(selected) || 0) % n + n) % n : 0;
  const objects = scene?.objects || [];
  const span = Math.min(Math.floor(Math.max(0, n - 1) / 2), Math.max(4, (Number(radius) || 6) + 2));

  const activeSet = new Set();
  const focusPoints = [];
  if (n > 0) {
    const rBound = isolate ? span : Math.floor(n / 2);
    for (let off = -rBound; off <= rBound; off++) {
      const idx = ((selIdx + off) % n + n) % n;
      activeSet.add(idx);
      if (Math.abs(off) <= span) focusPoints.push(points[idx]);
    }
  }

  let minX = Infinity, maxX = -Infinity, minZ = Infinity, maxZ = -Infinity;
  for (const pt of (isolate && focusPoints.length ? focusPoints : points)) {
    minX = Math.min(minX, pt.x); maxX = Math.max(maxX, pt.x);
    minZ = Math.min(minZ, pt.z); maxZ = Math.max(maxZ, pt.z);
  }
  const margin = 4500;
  minX -= margin; maxX += margin;
  minZ -= margin; maxZ += margin;

  const baseVerts = rawPreview.vertices || [];
  const baseIndices = rawPreview.indices || [];
  const baseColors = rawPreview.colors || [];
  const baseEntities = rawPreview.entities || [];

  const filteredIndices = [];
  const filteredEntities = [];

  for (const ent of baseEntities) {
    if (ent.bank === 'terrain') {
      if (!showTerrain) continue;
      if (isolate && Number.isInteger(ent.cell) && n > 0) {
        const col = ent.cell & 31;
        const row = ent.cell >> 5;
        const cx = col * 2048 + 1024;
        const cz = (31 - row) * 2048 + 1024;
        if (cx < minX || cx > maxX || cz < minZ || cz > maxZ) continue;
      }
    } else if (ent.bank === 'course') {
      if (!showCourse) continue;
      if (isolate && Number.isInteger(ent.object) && n > 0) {
        const obj = objects[ent.object];
        if (obj && (obj.x < minX || obj.x > maxX || obj.z < minZ || obj.z > maxZ)) continue;
      }
    }
    const firstIndex = filteredIndices.length;
    const end = Math.min(baseIndices.length, ent.firstIndex + ent.indexCount);
    for (let k = ent.firstIndex; k < end; k++) {
      filteredIndices.push(baseIndices[k]);
    }
    filteredEntities.push({
      ...ent,
      firstIndex,
      indexCount: filteredIndices.length - firstIndex
    });
  }

  const extraVerts = [];
  const extraColors = [];
  let nextVertIndex = Math.floor(baseVerts.length / 9);

  if (showRoad && n >= 2) {
    const normals2d = computePointNormals(points);
    const ptCoords3d = points.map((pt, i) => {
      const { nx, nz } = normals2d[i];
      const lw = Math.max(60, Number(pt.leftWidth) || 320);
      const rw = Math.max(60, Number(pt.rightWidth) || 320);
      const slope = (Number(pt.crossSlope) || 0) / 128;
      const x3d = nearRef3d(pt.x, 32768);
      const y3d = -nearRef3d(pt.y, 0) + 2;
      const z3d = -nearRef3d(pt.z, 32768);
      const bankYL = Math.max(-160, Math.min(160, -slope * lw));
      const bankYR = Math.max(-160, Math.min(160, slope * rw));
      return {
        lx: x3d + nx * lw,
        ly: y3d + bankYL,
        lz: z3d - nz * lw,
        rx: x3d - nx * rw,
        ry: y3d + bankYR,
        rz: z3d + nz * rw
      };
    });

    const pushRoadVert = (x, y, z, ny, rgba) => {
      const idx = nextVertIndex++;
      extraVerts.push(x, y, z, 0, ny, 0, 65535, 0, 0);
      extraColors.push(rgba[0], rgba[1], rgba[2], rgba[3]);
      return idx;
    };

    for (let i = 0; i < n; i++) {
      if (isolate && !activeSet.has(i)) continue;
      const c0 = ptCoords3d[i];
      const c1 = ptCoords3d[(i + 1) % n];
      const dist = Math.min(Math.abs(i - selIdx), n - Math.abs(i - selIdx));
      const rgba = i === selIdx
        ? [250, 204, 21, 255]
        : dist === 1
        ? [249, 115, 22, 255]
        : dist <= radius
        ? [56, 189, 248, 255]
        : [14, 165, 233, 255];

      const firstIndex = filteredIndices.length;
      const v0 = pushRoadVert(c0.lx, c0.ly, c0.lz, 1, rgba);
      const v1 = pushRoadVert(c0.rx, c0.ry, c0.rz, 1, rgba);
      const v2 = pushRoadVert(c1.lx, c1.ly, c1.lz, 1, rgba);
      const v3 = pushRoadVert(c1.rx, c1.ry, c1.rz, 1, rgba);
      // Double-sided triangles so road ribbon is visible from any camera elevation
      filteredIndices.push(v0, v2, v1, v1, v2, v3, v0, v1, v2, v1, v3, v2);
      filteredEntities.push({
        bank: 'road',
        point: i,
        firstIndex,
        indexCount: 12
      });
    }
  }

  const allVerts = extraVerts.length ? baseVerts.concat(extraVerts) : baseVerts;
  const allColors = extraColors.length ? baseColors.concat(extraColors) : baseColors;

  let bMinX = Infinity, bMinY = Infinity, bMinZ = Infinity;
  let bMaxX = -Infinity, bMaxY = -Infinity, bMaxZ = -Infinity;
  if (isolate || (!showTerrain && !showCourse && extraVerts.length)) {
    for (let k = 0; k < filteredIndices.length; k++) {
      const vi = filteredIndices[k] * 9;
      const vx = allVerts[vi], vy = allVerts[vi + 1], vz = allVerts[vi + 2];
      if (vx === undefined) continue;
      bMinX = Math.min(bMinX, vx); bMaxX = Math.max(bMaxX, vx);
      bMinY = Math.min(bMinY, vy); bMaxY = Math.max(bMaxY, vy);
      bMinZ = Math.min(bMinZ, vz); bMaxZ = Math.max(bMaxZ, vz);
    }
  } else {
    for (let vi = 0; vi < allVerts.length; vi += 9) {
      const vx = allVerts[vi], vy = allVerts[vi + 1], vz = allVerts[vi + 2];
      bMinX = Math.min(bMinX, vx); bMaxX = Math.max(bMaxX, vx);
      bMinY = Math.min(bMinY, vy); bMaxY = Math.max(bMaxY, vy);
      bMinZ = Math.min(bMinZ, vz); bMaxZ = Math.max(bMaxZ, vz);
    }
  }
  const hasBounds = Number.isFinite(bMinX);
  const frame = hasBounds
    ? {
        center: [(bMinX + bMaxX) / 2, (bMinY + bMaxY) / 2, (bMinZ + bMaxZ) / 2],
        scale: 1.75 / Math.max(1, bMaxX - bMinX, bMaxY - bMinY, bMaxZ - bMinZ)
      }
    : null;

  const roadCameras = n >= 2 ? points.map((pt, i) => {
    const pAhead = points[(i + 2) % n] || points[(i + 1) % n] || pt;
    const x = nearRef3d(pt.x, 32768);
    const y = -nearRef3d(pt.y, 0) + 36;
    const z = -nearRef3d(pt.z, 32768);
    const tx = nearRef3d(pAhead.x, 32768);
    const ty = -nearRef3d(pAhead.y, 0) + 32;
    const tz = -nearRef3d(pAhead.z, 32768);
    const dx = tx - x, dy = ty - y, dz = tz - z;
    const horiz = Math.hypot(dx, dz) || 1;
    return {
      x,
      y,
      z,
      yaw: Math.atan2(dx, -dz),
      pitch: -Math.atan2(dy, horiz)
    };
  }) : [];

  return {
    data: {
      ...rawPreview,
      vertices: allVerts,
      colors: allColors,
      indices: filteredIndices,
      entities: filteredEntities,
      roadCameras,
      selectedRoadPoint: selIdx
    },
    frame
  };
}

// A geometric overview, not a replacement for the game's fixed-point physics.
// Both curve modes follow the shortest angular span; mode 2 reverses lateral
// sign in the runtime, not the centerline's angular direction.
export function roadPolyline(road){
  const points=road?.points||[],line=[];
  for(let i=0;i<points.length;i++){
    const p=points[i],next=points[(i+1)%points.length];line.push({x:p.x,z:p.z});
    const center=(p.arc&3)?road.arcs?.[(p.arc<<16>>16)>>4]:null;
    if(!center)continue;
    const dx=p.x-center.x,dz=p.z-center.z,nx=next.x-center.x,nz=next.z-center.z;
    const radius=Math.hypot(dx,dz),nextRadius=Math.hypot(nx,nz);
    if(!radius||!nextRadius)continue;
    const angle=Math.atan2(dz,dx);let span=Math.atan2(nz,nx)-angle;
    if(span>Math.PI)span-=2*Math.PI;else if(span<-Math.PI)span+=2*Math.PI;
    const steps=Math.ceil(Math.abs(span)/(Math.PI/32));
    for(let j=1;j<steps;j++){const t=j/steps,r=radius+(nextRadius-radius)*t,a=angle+span*t;line.push({x:center.x+Math.cos(a)*r,z:center.z+Math.sin(a)*r});}
  }
  if(points.length)line.push({x:points[0].x,z:points[0].z});
  return line;
}

export const trackTabs={
  road:'🛣️ Road & Spline',
  scenery:'🌲 Scenery & Terrain',
  atmosphere:'🌅 Sky & Atmosphere',
  race:'🏁 Race, AI & Cameras',
  all:'All Sections'
};

function computePointNormals(points) {
  const n = points.length;
  return points.map((curr, i) => {
    const prev = points[(i - 1 + n) % n];
    const next = points[(i + 1) % n];
    const tx = next.x - prev.x, tz = next.z - prev.z;
    const len = Math.hypot(tx, tz) || 1;
    return { nx: -tz / len, nz: tx / len, tx: tx / len, tz: tz / len };
  });
}

function buildRoadRibbonPolygon(points, positionFn) {
  if (points.length < 2) return '';
  const normals = computePointNormals(points);
  const leftSide = [], rightSide = [];
  for (let i = 0; i < points.length; i++) {
    const curr = points[i];
    const { nx, nz } = normals[i];
    const lw = Math.max(20, Number(curr.leftWidth) || 0) * 4;
    const rw = Math.max(20, Number(curr.rightWidth) || 0) * 4;
    const [lx, ly] = positionFn({x: curr.x + nx * lw, z: curr.z + nz * lw});
    const [rx, ry] = positionFn({x: curr.x - nx * rw, z: curr.z - nz * rw});
    leftSide.push(`${lx.toFixed(1)},${ly.toFixed(1)}`);
    rightSide.push(`${rx.toFixed(1)},${ry.toFixed(1)}`);
  }
  if (leftSide.length) {
    leftSide.push(leftSide[0]);
    rightSide.push(rightSide[0]);
  }
  return [...leftSide, ...rightSide.reverse()].join(' ');
}

function wrapIdx(i, n) {
  return n > 0 ? ((i % n) + n) % n : 0;
}

function buildLocalNeighborhoodMap(road, objects, terrain, selected, objectIndex, radius = 6, layers = {}) {
  const points = road?.points || [];
  const n = points.length;
  if (!n) return '';
  const showRoad = layers.road !== false;
  const showTerrain = layers.terrain !== false;
  const showCourse = layers.course !== false;
  const viewSpan = Math.min(Math.floor((n - 1) / 2), Math.max(5, radius + 3));
  const indices = [];
  for (let offset = -viewSpan; offset <= viewSpan; offset++) {
    indices.push({ idx: wrapIdx(selected + offset, n), offset });
  }
  const normals = computePointNormals(points);

  let minX = Infinity, maxX = -Infinity, minZ = Infinity, maxZ = -Infinity;
  const includePt = (x, z) => {
    minX = Math.min(minX, x); maxX = Math.max(maxX, x);
    minZ = Math.min(minZ, z); maxZ = Math.max(maxZ, z);
  };
  for (const { idx } of indices) {
    const pt = points[idx];
    const { nx, nz } = normals[idx] || { nx: 0, nz: 1 };
    const lw = Math.max(40, Number(pt.leftWidth) || 0) * 4;
    const rw = Math.max(40, Number(pt.rightWidth) || 0) * 4;
    includePt(pt.x + nx * lw, pt.z + nz * lw);
    includePt(pt.x - nx * rw, pt.z - nz * rw);
  }
  const padWorld = Math.max(600, Math.max(maxX - minX, maxZ - minZ) * 0.18);
  minX -= padWorld; maxX += padWorld;
  minZ -= padWorld; maxZ += padWorld;
  const spanWorld = Math.max(1, maxX - minX, maxZ - minZ);
  const scale = 500 / spanWorld;
  const pos = pt => [(pt.x - minX) * scale + 25, (pt.z - minZ) * scale + 25];

  const terrainOverlays = [];
  if (showTerrain && terrain?.cells?.length === 1024) {
    const cMin = Math.max(0, Math.floor(minX / 2048)), cMax = Math.min(31, Math.floor(maxX / 2048));
    const zRowMin = Math.max(0, Math.floor(minZ / 2048)), zRowMax = Math.min(31, Math.floor(maxZ / 2048));
    for (let col = cMin; col <= cMax; col++) {
      for (let zr = zRowMin; zr <= zRowMax; zr++) {
        const row = 31 - zr;
        const cellIdx = row * 32 + col;
        const model = terrain.cells[cellIdx] & 1023;
        const [x0, y0] = pos({ x: col * 2048, z: zr * 2048 });
        const [x1, y1] = pos({ x: (col + 1) * 2048, z: (zr + 1) * 2048 });
        terrainOverlays.push(
          `<g class="local-terrain-cell" data-select-terrain-cell="${cellIdx}"><rect x="${x0.toFixed(1)}" y="${y0.toFixed(1)}" width="${Math.max(1, x1 - x0).toFixed(1)}" height="${Math.max(1, y1 - y0).toFixed(1)}" fill="${model === 1023 ? 'none' : 'rgba(92,184,138,0.06)'}" stroke="rgba(144,199,211,0.14)" stroke-dasharray="2,3"/><text x="${(x0 + 4).toFixed(1)}" y="${(y0 + 12).toFixed(1)}" fill="rgba(154,167,188,0.5)" font-size="9">T(${col + 1},${row + 1})${model === 1023 ? '' : '·M' + model}</text></g>`
        );
      }
    }
  }

  const segQuads = [];
  if (showRoad) {
    for (let k = 0; k < indices.length - 1; k++) {
      const { idx: i0, offset: off0 } = indices[k];
      const { idx: i1 } = indices[k + 1];
      const p0 = points[i0], p1 = points[i1];
      const n0 = normals[i0], n1 = normals[i1];
      const lw0 = Math.max(20, Number(p0.leftWidth) || 0) * 4, rw0 = Math.max(20, Number(p0.rightWidth) || 0) * 4;
      const lw1 = Math.max(20, Number(p1.leftWidth) || 0) * 4, rw1 = Math.max(20, Number(p1.rightWidth) || 0) * 4;
      const [l0x, l0y] = pos({ x: p0.x + n0.nx * lw0, z: p0.z + n0.nz * lw0 });
      const [r0x, r0y] = pos({ x: p0.x - n0.nx * rw0, z: p0.z - n0.nz * rw0 });
      const [l1x, l1y] = pos({ x: p1.x + n1.nx * lw1, z: p1.z + n1.nz * lw1 });
      const [r1x, r1y] = pos({ x: p1.x - n1.nx * rw1, z: p1.z - n1.nz * rw1 });
      const isCurrentSeg = off0 === 0;
      const isImmediateNeighbor = off0 === -1 || off0 === 1;
      const inRadius = Math.abs(off0) <= radius;
      const fill = isCurrentSeg
        ? 'rgba(255,207,112,0.34)'
        : isImmediateNeighbor
        ? 'rgba(74,158,255,0.24)'
        : inRadius
        ? 'rgba(144,199,211,0.18)'
        : 'rgba(144,199,211,0.08)';
      const stroke = isCurrentSeg ? '#ffcf70' : isImmediateNeighbor ? '#4a9eff' : 'rgba(144,199,211,0.32)';
      const [c0x, c0y] = pos(p0), [c1x, c1y] = pos(p1);
      const mx = (c0x + c1x) / 2, my = (c0y + c1y) / 2;
      segQuads.push(
        `<g class="local-road-segment" data-track-point="${i0}"><polygon points="${l0x.toFixed(1)},${l0y.toFixed(1)} ${l1x.toFixed(1)},${l1y.toFixed(1)} ${r1x.toFixed(1)},${r1y.toFixed(1)} ${r0x.toFixed(1)},${r0y.toFixed(1)}" fill="${fill}" stroke="${stroke}" stroke-width="${isCurrentSeg ? '1.8' : '1'}"/><line x1="${c0x.toFixed(1)}" y1="${c0y.toFixed(1)}" x2="${c1x.toFixed(1)}" y2="${c1y.toFixed(1)}" stroke="${isCurrentSeg ? '#ffcf70' : '#90c7d3'}" stroke-width="${isCurrentSeg ? '2.5' : '1.4'}" stroke-dasharray="${isCurrentSeg ? 'none' : '3,2'}"/>${Math.abs(off0) <= 2 ? `<text x="${mx.toFixed(1)}" y="${(my - 5).toFixed(1)}" fill="${isCurrentSeg ? '#ffcf70' : '#c9d6ea'}" font-size="9.5" text-anchor="middle">L:${p0.length}</text>` : ''}</g>`
      );
    }
  }

  const p = points[selected];
  const nextPt = points[(selected + 1) % n];
  const arcIndex = p && (p.arc & 3) ? (p.arc << 16 >> 16) >> 4 : -1;
  const arc = road?.arcs?.[arcIndex];
  let arcOverlay = '';
  if (showRoad && arc && p && nextPt) {
    const [cx, cy] = pos(arc), [px, py] = pos(p), [nx, ny] = pos(nextPt);
    const clampedCx = Math.max(16, Math.min(534, cx)), clampedCy = Math.max(16, Math.min(534, cy));
    arcOverlay = `<g class="local-arc-overlay"><line x1="${clampedCx.toFixed(1)}" y1="${clampedCy.toFixed(1)}" x2="${px.toFixed(1)}" y2="${py.toFixed(1)}" stroke="#e8a838" stroke-dasharray="4,3" stroke-width="1.5"/><line x1="${clampedCx.toFixed(1)}" y1="${clampedCy.toFixed(1)}" x2="${nx.toFixed(1)}" y2="${ny.toFixed(1)}" stroke="#e8a838" stroke-dasharray="4,3" stroke-width="1.5"/><circle cx="${clampedCx.toFixed(1)}" cy="${clampedCy.toFixed(1)}" r="7" fill="#e8a838" stroke="#0d1016" stroke-width="2" data-drag-arc="${arcIndex}" style="cursor:grab"><title>Drag Curve Center #${arcIndex + 1} (${arc.x}, ${arc.z})</title></circle><text x="${(clampedCx + 10).toFixed(1)}" y="${(clampedCy + 4).toFixed(1)}" fill="#e8a838" font-size="9.5" font-weight="600">Arc #${arcIndex + 1}</text></g>`;
  }

  const nearbyObjects = showCourse
    ? objects
        .map((o, idx) => ({ o, idx }))
        .filter(({ o }) => o.model >= 0 && o.x >= minX && o.x <= maxX && o.z >= minZ && o.z <= maxZ)
        .map(({ o, idx }) => {
          const [ox, oy] = pos(o);
          const isSel = idx === objectIndex;
          return `<g data-map-object="${idx}" style="cursor:pointer"><rect x="${(ox - 5).toFixed(1)}" y="${(oy - 5).toFixed(1)}" width="10" height="10" rx="2" fill="${isSel ? '#4cd964' : '#4a9eff'}" stroke="#0d1016" stroke-width="1.2"/><text x="${(ox + 7).toFixed(1)}" y="${(oy + 3).toFixed(1)}" fill="${isSel ? '#4cd964' : '#8ab8ff'}" font-size="9">Obj #${idx + 1} (M${o.model + 1})</text><title>Scenery Object #${idx + 1} · Course Model ${o.model + 1} (${o.x}, ${o.y}, ${o.z})</title></g>`;
        })
        .join('')
    : '';

  const pointNodes = showRoad
    ? indices.map(({ idx, offset }) => {
        const pt = points[idx];
        const [x, y] = pos(pt);
        const isSel = offset === 0;
        const isAdj = offset === -1 || offset === 1;
        const inRadius = Math.abs(offset) <= radius;
        const r = isSel ? 8.5 : isAdj ? 6.5 : inRadius ? 5.2 : 3.8;
        const fill = isSel ? '#ffcf70' : isAdj ? '#4a9eff' : inRadius ? '#67e8f9' : '#90c7d3';
        const label = isSel ? `★ #${idx + 1}` : Math.abs(offset) <= 3 ? `#${idx + 1}` : '';
        return `<g class="local-point-node"><circle cx="${x.toFixed(1)}" cy="${y.toFixed(1)}" r="${r}" fill="${fill}" stroke="#0d1016" stroke-width="1.8" data-track-point="${idx}" data-local-point="${idx}" style="cursor:grab"><title>Point #${idx + 1} (${offset === 0 ? 'Selected' : (offset > 0 ? '+' + offset : offset)}) · X:${pt.x} Y:${pt.y} Z:${pt.z} · Drag to reshape</title></circle>${label ? `<text x="${(x + 9).toFixed(1)}" y="${(y - 6).toFixed(1)}" fill="${isSel ? '#ffcf70' : '#e2e8f0'}" font-size="${isSel ? '10.5' : '9'}" font-weight="${isSel ? '700' : '500'}" pointer-events="none">${label}</text>` : ''}</g>`;
      }).join('')
    : '';

  return `<svg class="local-road-map" viewBox="0 0 550 550" data-map-min-x="${minX}" data-map-min-z="${minZ}" data-map-scale="${scale}" role="group" aria-label="Local neighborhood road segments">${terrainOverlays.join('')}${segQuads.join('')}${arcOverlay}${nearbyObjects}${pointNodes}</svg>`;
}

function buildCrossSectionComparison(points, selected) {
  if (!points.length) return '';
  const n = points.length;
  const prev = points[wrapIdx(selected - 1, n)];
  const curr = points[wrapIdx(selected, n)];
  const next = points[wrapIdx(selected + 1, n)];
  const w = 240, h = 68, cx = w / 2, cy = h / 2 + 5;
  const maxHalf = Math.max(80, prev.leftWidth || 0, prev.rightWidth || 0, curr.leftWidth || 0, curr.rightWidth || 0, next.leftWidth || 0, next.rightWidth || 0);
  const scaleX = (w * 0.38) / maxHalf;
  const drawProfile = (pt, color, width, dash = '') => {
    const lw = (Number(pt.leftWidth) || 40) * scaleX;
    const rw = (Number(pt.rightWidth) || 40) * scaleX;
    const slope = (Number(pt.crossSlope) || 0) / 128;
    const ly = cy - Math.max(-22, Math.min(22, slope * lw * 1.4));
    const ry = cy + Math.max(-22, Math.min(22, slope * rw * 1.4));
    return `<line x1="${(cx - lw).toFixed(1)}" y1="${ly.toFixed(1)}" x2="${(cx + rw).toFixed(1)}" y2="${ry.toFixed(1)}" stroke="${color}" stroke-width="${width}" ${dash ? `stroke-dasharray="${dash}"` : ''} stroke-linecap="round"/>`;
  };
  return `
    <div class="cross-section-hud" title="Road cross-section and banking comparison (i-1 · ★ i · i+1)">
      <div class="row between"><small class="eyebrow">Cross-Section (i-1 · ★ i · i+1)</small><small class="muted">L:${curr.leftWidth}/R:${curr.rightWidth} · ∠${curr.crossSlope}</small></div>
      <svg class="cross-section-svg" viewBox="0 0 ${w} ${h}" role="img" aria-label="Road cross section and banking comparison">
        <line x1="10" y1="${cy}" x2="${w - 10}" y2="${cy}" stroke="rgba(144,199,211,0.16)" stroke-dasharray="2,2"/>
        <line x1="${cx}" y1="8" x2="${cx}" y2="${h - 6}" stroke="rgba(144,199,211,0.2)" stroke-dasharray="2,2"/>
        ${drawProfile(prev, 'rgba(144,199,211,0.45)', 1.8, '3,2')}
        ${drawProfile(next, 'rgba(74,158,255,0.55)', 1.8, '2,2')}
        ${drawProfile(curr, '#ffcf70', 3.2)}
        <circle cx="${cx}" cy="${cy}" r="3" fill="#ffcf70"/>
      </svg>
    </div>`;
}

function buildElevationChart(points, selected) {
  if (points.length < 2) return '';
  const w = 680, h = 76, padL = 32, padR = 12, padT = 12, padB = 14;
  const plotW = w - padL - padR, plotH = h - padT - padB;
  let minY = Infinity, maxY = -Infinity;
  for (const p of points) {
    const y = Number(p.y) || 0;
    minY = Math.min(minY, y);
    maxY = Math.max(maxY, y);
  }
  const spanY = Math.max(1, maxY - minY);
  const coords = points.map((p, i) => {
    const x = padL + (i / (points.length - 1)) * plotW;
    const y = padT + plotH - (((Number(p.y) || 0) - minY) / spanY) * plotH;
    return [x, y];
  });
  const poly = coords.map(([x, y]) => `${x.toFixed(1)},${y.toFixed(1)}`).join(' ');
  const area = `${padL},${padT + plotH} ${poly} ${(padL + plotW).toFixed(1)},${padT + plotH}`;
  const [selX, selY] = coords[Math.min(points.length - 1, Math.max(0, selected))] || [padL, padT];
  return `
    <div class="elevation-strip">
      <div class="row between"><small class="eyebrow">Elevation Profile (Height Y — Drag handle vertically to reshape)</small><small class="muted">Y: <strong>${points[selected]?.y ?? 0}</strong> (${minY} … ${maxY})</small></div>
      <svg class="elevation-chart" viewBox="0 0 ${w} ${h}" data-elev-min-y="${minY}" data-elev-max-y="${maxY}" role="group" aria-label="Track elevation profile">
        <polygon points="${area}" fill="rgba(144,199,211,0.14)"/>
        <polyline points="${poly}" fill="none" stroke="#90c7d3" stroke-width="2"/>
        <line x1="${selX.toFixed(1)}" y1="${padT}" x2="${selX.toFixed(1)}" y2="${padT + plotH}" stroke="#ffcf70" stroke-width="1.5" stroke-dasharray="3,2"/>
        <circle cx="${selX.toFixed(1)}" cy="${selY.toFixed(1)}" r="5.5" fill="#ffcf70" stroke="#0d1016" stroke-width="1.5" data-elev-drag="${selected}" style="cursor:ns-resize"/>
        ${coords.map(([x], i) => `<rect x="${(x - plotW / points.length / 2).toFixed(1)}" y="${padT}" width="${Math.max(2, plotW / points.length).toFixed(1)}" height="${plotH}" fill="transparent" data-track-point="${i}"><title>Point ${i + 1} · Y: ${points[i].y}</title></rect>`).join('')}
      </svg>
    </div>`;
}

function buildNeighborsComparisonDrawer(points, objects, terrain, selected) {
  const n = points.length;
  if (!n) return '';
  const maxOff = Math.min(2, Math.floor((n - 1) / 2));
  const offsets = [];
  for (let off = -maxOff; off <= maxOff; off++) offsets.push(off);
  const rows = offsets.map(off => {
    const idx = wrapIdx(selected + off, n);
    const pt = points[idx];
    const nextPt = points[wrapIdx(idx + 1, n)];
    const dy = (nextPt.y || 0) - (pt.y || 0);
    const roleLabel = off === 0 ? '★' : off < 0 ? `${off}` : `+${off}`;
    return `<tr class="${off === 0 ? 'neighbor-row-active' : ''}" data-track-point="${idx}" style="cursor:pointer">
      <td><strong>#${idx + 1}</strong> <small class="muted">(${roleLabel})</small></td>
      <td>${pt.x}, ${pt.z}</td>
      <td>${pt.y} <small class="muted">(${dy >= 0 ? '+' + dy : dy})</small></td>
      <td>${pt.leftWidth}/${pt.rightWidth}</td>
      <td>L:${pt.length} · ∠${pt.angle}</td>
    </tr>`;
  }).join('');

  const p = points[selected];
  const nearbyScenery = objects
    .map((o, i) => ({ o, i, dist: o.model >= 0 ? Math.round(Math.hypot(o.x - p.x, o.z - p.z)) : Infinity }))
    .filter(x => x.dist < 2500)
    .sort((a, b) => a.dist - b.dist)
    .slice(0, 3);

  const col = Math.max(0, Math.min(31, Math.floor(p.x / 2048)));
  const zr = Math.max(0, Math.min(31, Math.floor(p.z / 2048)));
  const row = 31 - zr;
  const cellIdx = row * 32 + col;
  const tModel = terrain?.cells?.length === 1024 ? (terrain.cells[cellIdx] & 1023) : 1023;
  const prevIdx = wrapIdx(selected - 1, n);
  const nextIdx = wrapIdx(selected + 1, n);

  return `
    <div class="inspector-neighbor-strip">
      <div class="neighbor-pill-switcher" role="group" aria-label="Element in Neighbor Context">
        <button type="button" data-track-point="${prevIdx}" title="Previous segment #${prevIdx + 1}">◀ #${prevIdx + 1}</button>
        <span class="neighbor-current-badge">★ Segment #${selected + 1} <small class="muted">/ ${n}</small></span>
        <button type="button" data-track-point="${nextIdx}" title="Next segment #${nextIdx + 1}">#${nextIdx + 1} ▶</button>
      </div>
      <div class="row wrap neighbor-links">
        <button type="button" class="chip-btn" data-jump-terrain-cell="${cellIdx}" title="Inspect terrain cell under this road segment">🗺️ Terrain (${col + 1},${row + 1})${tModel === 1023 ? '' : '·M' + tModel}</button>
        ${nearbyScenery.map(s => `<button type="button" class="chip-btn" data-map-object="${s.i}" title="Jump to nearby 3D scenery object (${s.dist}u away)">🌲 Obj #${s.i + 1}</button>`).join('')}
      </div>
      <details class="neighbor-table-drawer">
        <summary>Element in Neighbor Context (#${wrapIdx(selected - maxOff, n) + 1} … ★ #${selected + 1} … #${wrapIdx(selected + maxOff, n) + 1})</summary>
        <div class="table-scroll">
          <table class="asset-table neighbor-table">
            <thead>
              <tr><th>Segment</th><th>X, Z</th><th>Height Y</th><th>Width L/R</th><th>Len / Dir</th></tr>
            </thead>
            <tbody>${rows}</tbody>
          </table>
        </div>
      </details>
    </div>`;
}

export function trackWorkspace(tracks,bank,scene,selected=0,objectIndex=0,options={}) {
  const activeTab=Object.hasOwn(trackTabs,options.tab)?options.tab:'road';
  const stageView=options.view==='3d'?'3d':'2d';
  const raceSubtab=options.raceSubtab||'starts';
  const radius=Number.isInteger(options.roadRadius)?options.roadRadius:6;
  const activeTool=options.tool||'move';
  const layers={
    road:options.layers?.road!==false,
    terrain:options.layers?.terrain!==false,
    course:options.layers?.course!==false,
    isolate:Boolean(options.layers?.isolate)
  };
  const isRoadOnlyPreset=layers.road&&!layers.terrain&&!layers.course&&!layers.isolate;
  const isIsolatePreset=layers.road&&layers.isolate;
  const isFull3dPreset=stageView==='3d'&&layers.road&&layers.terrain&&layers.course&&!layers.isolate;

  const effectiveRoad=options.roadDraft||scene?.road;
  const draftDiff=options.roadDraft&&scene?.road?diffRoadDraft(scene.road,options.roadDraft):{entries:[],pointCount:0,arcCount:0};
  const paneHidden=id=>activeTab!=='all'&&activeTab!==id?' hidden':'';
  const track=tracks.find(t=>t.bank===bank);
  const points=effectiveRoad?.points||[];
  const objects=scene?.objects||[],object=objects[objectIndex];
  const p=points[selected];
  const arcIndex=p&&(p.arc&3)?(p.arc<<16>>16)>>4:-1,arc=effectiveRoad?.arcs?.[arcIndex];

  const layerBar=`
    <div class="viewport-header-bar row between wrap">
      <div class="row wrap">
        <div class="part-strip row" role="group" aria-label="Track stage mode">
          <button type="button" data-track-stage="2d" class="${stageView==='2d'?'active':''}">🗺️ 2D Route</button>
          <button type="button" data-track-stage="3d" class="${stageView==='3d'?'active':''}">🧊 3D Viewport</button>
        </div>
        <div class="layer-preset-strip row" role="group" aria-label="Quick view presets">
          <button type="button" data-layer-preset="road-only" class="${isRoadOnlyPreset?'active':''}" title="Show only the driveable road ribbon (hide terrain and objects)">🛣️ Road Only</button>
          <button type="button" data-layer-preset="isolated" class="${isIsolatePreset?'active':''}" title="Isolate selected segment and neighboring segments (±${radius})">🔍 Segment + Neighbors</button>
          <button type="button" data-layer-preset="full-3d" class="${isFull3dPreset?'active':''}" title="Show full 3D track model (road ribbon + terrain + course objects)">🌍 Full 3D Model</button>
        </div>
      </div>
      <div class="row wrap">
        <div class="layer-toggles-strip row" role="group" aria-label="View layers">
          <span class="eyebrow">Layers:</span>
          <button type="button" data-toggle-layer="road" class="layer-pill ${layers.road?'active':''}" aria-pressed="${layers.road}">👁 Road</button>
          <button type="button" data-toggle-layer="terrain" class="layer-pill ${layers.terrain?'active':''}" aria-pressed="${layers.terrain}">👁 Terrain 3D</button>
          <button type="button" data-toggle-layer="course" class="layer-pill ${layers.course?'active':''}" aria-pressed="${layers.course}">👁 Objects 3D</button>
          <button type="button" data-toggle-layer="isolate" class="layer-pill ${layers.isolate?'active':''}" aria-pressed="${layers.isolate}">🔍 Isolate ±${radius}</button>
        </div>
        <div class="route-io-compact row">
          <button type="button" data-export-route="json" title="Export driveable route (.json)">⬇ JSON</button>
          <button type="button" data-export-route="obj" title="Export 3D road ribbon for Blender (.obj)">⬇ OBJ</button>
          <button type="button" data-import-route="${bank}" data-revision="${scene?.revision}" title="Import modified route (.json or .obj)">⬆ Import</button>
          <button type="button" data-track-preview="${bank}" title="Open fullscreen 3D preview">⤢</button>
        </div>
      </div>
    </div>`;

  const radiusSelector=`<div class="row wrap" role="group" aria-label="Deformation falloff radius"><small class="muted">Falloff:</small>${[[0,'1 pt'],[3,'±3'],[6,'±6'],[12,'±12'],[20,'±20']].map(([r,lbl])=>`<button type="button" data-road-radius="${r}" class="${radius===r?'active':''}">${lbl}</button>`).join('')}</div>`;

  const toolOptionsContent = activeTool === 'select'
    ? `<div class="row wrap"><small class="muted">Select &amp; Navigate:</small><button type="button" data-step-point="-5">◀◀ -5</button><button type="button" data-step-point="-1">◀ Prev</button><button type="button" data-step-point="1">Next ▶</button><button type="button" data-step-point="5">+5 ▶▶</button><button type="button" data-toggle-layer="isolate" class="${layers.isolate?'active':''}">🔍 ${layers.isolate?'Exit Local Isolation':'Isolate Segment + Neighbors'}</button></div>`
    : activeTool === 'smooth'
    ? `<div class="row wrap"><small class="muted">Smooth &amp; Line (±${radius}):</small><button type="button" data-shape-road="smooth-xz:0">〰 Smooth Curve (X/Z)</button><button type="button" data-shape-road="straighten:0">📏 Straighten Section</button><button type="button" data-shape-road="smooth-y:0">〰 Smooth Elevation (Y)</button></div>`
    : activeTool === 'width'
    ? `<div class="row wrap"><small class="muted">Width &amp; Banking (±${radius}):</small><button type="button" data-shape-road="width:-16">Narrow (-16)</button><button type="button" data-shape-road="width:16">Widen (+16)</button><button type="button" data-shape-road="bank:-8">⟲ Bank L</button><button type="button" data-shape-road="bank:0">Flat (0)</button><button type="button" data-shape-road="bank:8">⟳ Bank R</button></div>`
    : `<div class="row wrap"><small class="muted">Move / Bend (±${radius}):</small><button type="button" data-shape-road="lateral:-400">◀◀ Left 400</button><button type="button" data-shape-road="lateral:-100">◀ Left 100</button><button type="button" data-shape-road="lateral:100">Right 100 ▶</button><button type="button" data-shape-road="lateral:400">Right 400 ▶▶</button><span class="divider-dot">·</span><button type="button" data-shape-road="height:-100">▼ Height -100</button><button type="button" data-shape-road="height:100">▲ Height +100</button></div>`;

  const draftActions = draftDiff.entries.length
    ? `<div class="row wrap" data-road-draft-banner><span class="pill warn-pill">Unsaved draft: ${draftDiff.pointCount} pts</span><button type="button" data-reset-road-draft>↺ Discard</button><button type="button" class="primary" data-save-road-draft="${bank}" data-revision="${scene?.revision}">💾 Save Route (${draftDiff.entries.length})</button></div>`
    : '';

  const toolOptionsBar = p ? `<div class="tool-options-bar row between wrap">${toolOptionsContent}<div class="row wrap">${radiusSelector}${draftActions}</div></div>` : '';

  const leftToolDock = p ? `
    <aside class="blender-tool-dock" role="toolbar" aria-label="Route editing tools">
      <button type="button" data-road-tool="select" class="tool-dock-btn ${activeTool==='select'?'active':''}" title="Select and inspect road segments">
        <span class="tool-icon">↖</span><small>Select</small>
      </button>
      <button type="button" data-road-tool="move" class="tool-dock-btn ${activeTool==='move'?'active':''}" title="Move / Bend route with proportional falloff">
        <span class="tool-icon">✥</span><small>Move</small>
      </button>
      <button type="button" data-road-tool="smooth" class="tool-dock-btn ${activeTool==='smooth'?'active':''}" title="Smooth curve or straighten section">
        <span class="tool-icon">〰</span><small>Smooth</small>
      </button>
      <button type="button" data-road-tool="width" class="tool-dock-btn ${activeTool==='width'?'active':''}" title="Adjust road width and camber banking">
        <span class="tool-icon">↔</span><small>Profile</small>
      </button>
    </aside>` : '';

  let map='<p class="muted">Choose a track to inspect its original road.</p>';
  if(points.length) {
    const samples=roadPolyline(effectiveRoad);
    let minX=Infinity,maxX=-Infinity,minZ=Infinity,maxZ=-Infinity;
    for(const pt of samples){minX=Math.min(minX,pt.x);maxX=Math.max(maxX,pt.x);minZ=Math.min(minZ,pt.z);maxZ=Math.max(maxZ,pt.z);}
    for(const o of objects){if(o.model>=0){minX=Math.min(minX,o.x);maxX=Math.max(maxX,o.x);minZ=Math.min(minZ,o.z);maxZ=Math.max(maxZ,o.z);}}
    const scale=620/Math.max(1,maxX-minX,maxZ-minZ);
    const position=pt=>[(pt.x-minX)*scale+40,(pt.z-minZ)*scale+40];
    const line=samples.map(pt=>position(pt).join(',')).join(' ');
    const ribbon=layers.road?buildRoadRibbonPolygon(points,position):'';
    const nextPt=points[wrapIdx(selected+1,points.length)];
    let arcOverlay='';
    if(layers.road&&arc&&p&&nextPt){
      const [cx,cy]=position(arc),[px,py]=position(p),[nx,ny]=position(nextPt);
      arcOverlay=`<g class="map-arc-overlay"><line x1="${cx}" y1="${cy}" x2="${px}" y2="${py}" stroke="#e8a838" stroke-dasharray="4,4" stroke-width="1.5"/><line x1="${cx}" y1="${cy}" x2="${nx}" y2="${ny}" stroke="#e8a838" stroke-dasharray="4,4" stroke-width="1.5"/><circle cx="${cx}" cy="${cy}" r="6" fill="#e8a838" stroke="#0d1016" stroke-width="2" data-drag-arc="${arcIndex}" style="cursor:grab"><title>Curve center ${arcIndex+1} (${arc.x}, ${arc.z})</title></circle></g>`;
    }
    const terrainGridFull=[];
    if(layers.terrain&&scene?.terrain?.cells?.length===1024){
      for(let idx=0;idx<1024;idx++){
        const m=scene.terrain.cells[idx]&1023;
        if(m===1023)continue;
        const col=idx&31,row=idx>>5,zr=31-row;
        const [x0,y0]=position({x:col*2048,z:zr*2048});
        const [x1,y1]=position({x:(col+1)*2048,z:(zr+1)*2048});
        terrainGridFull.push(`<rect x="${x0.toFixed(1)}" y="${y0.toFixed(1)}" width="${Math.max(1,x1-x0).toFixed(1)}" height="${Math.max(1,y1-y0).toFixed(1)}" fill="rgba(92,184,138,0.08)" stroke="rgba(144,199,211,0.12)" stroke-width="0.6" data-select-terrain-cell="${idx}"/>`);
      }
    }
    const objectMarkers=layers.course?objects.map((o,i)=>{
      if(o.model<0)return '';
      const [ox,oy]=position(o);
      const isSel=i===objectIndex;
      return `<rect x="${(ox-(isSel?5:3)).toFixed(1)}" y="${(oy-(isSel?5:3)).toFixed(1)}" width="${isSel?10:6}" height="${isSel?10:6}" rx="1.5" fill="${isSel?'#4cd964':'#4a9eff'}" stroke="#0d1016" stroke-width="1" data-map-object="${i}" role="button" aria-label="Scenery object ${i+1}"><title>Object ${i+1} · Model ${o.model+1}</title></rect>`;
    }).join(''):'';
    const startPos=points[0]?position(points[0]):null;
    const startMarker=layers.road&&startPos?`<circle cx="${startPos[0]}" cy="${startPos[1]}" r="9" fill="none" stroke="#ffffff" stroke-dasharray="3,3" stroke-width="2"/>`:'';
    const localContextSvg=buildLocalNeighborhoodMap(effectiveRoad,objects,scene?.terrain,selected,objectIndex,radius,layers);
    const fullCircuitSvg=`<svg class="track-map" viewBox="0 0 700 700" data-map-min-x="${minX}" data-map-min-z="${minZ}" data-map-scale="${scale}" role="group" aria-label="Current road — top view">${terrainGridFull.join('')}${ribbon?`<polygon points="${ribbon}" fill="rgba(144,199,211,0.16)" stroke="rgba(144,199,211,0.28)" stroke-width="1"/>`:''}${layers.road?`<polyline points="${line}" fill="none" stroke="#90c7d3" stroke-width="3"/>`:''}${arcOverlay}${startMarker}${objectMarkers}${layers.road?points.map((pt,i)=>{const [x,y]=position(pt);const dist=Math.min(Math.abs(i-selected),points.length-Math.abs(i-selected));const inR=dist<=radius;return `<circle cx="${x}" cy="${y}" r="${i===selected?7:inR?4.5:3}" fill="${i===selected?'#ffcf70':inR?'#67e8f9':'#90c7d3'}" data-track-point="${i}" tabindex="0" role="button" aria-label="Road point ${i+1}"/>`;}).join(''):''}</svg>`;
    const crossSection=buildCrossSectionComparison(points,selected);

    map=`
      <div class="cad-map-wrap">
        <div class="single-viewport-canvas">
          <div class="viewport-floating-controls row between wrap">
            <div class="row wrap">
              <span class="pill">${layers.isolate?`🔍 Isolated View: Segment #${selected+1} (±${radius})`:'🗺️ Full Circuit View (2D CAD)'}</span>
              ${!layers.isolate?`<button type="button" data-map-zoom="in" title="Zoom in">＋</button><button type="button" data-map-zoom="out" title="Zoom out">－</button><button type="button" data-map-zoom="reset" title="Reset view">Fit</button><button type="button" data-map-zoom="focus" title="Center on selected point">◎ Focus</button>`:''}
            </div>
            <small class="muted">Drag any point on the map to reshape the route</small>
          </div>
          <div class="primary-2d-stage">
            <div class="stage-map-pane" ${layers.isolate?'':'hidden'}>${localContextSvg}</div>
            <div class="stage-map-pane" ${layers.isolate?'hidden':''}>${fullCircuitSvg}</div>
            ${crossSection}
          </div>
        </div>
        <div class="scrubber-bar row between wrap">
          <div class="row">
            <button type="button" data-step-point="-5" title="Back 5 points">◀◀ -5</button>
            <button type="button" data-step-point="-1">◀ Prev</button>
            <button type="button" data-step-point="1">Next ▶</button>
            <button type="button" data-step-point="5" title="Forward 5 points">+5 ▶▶</button>
          </div>
          <input type="range" class="track-scrubber" data-track-scrubber min="0" max="${Math.max(0,points.length-1)}" value="${selected}" aria-label="Scrub along road points">
          <span class="pill">Point <strong>${selected+1}</strong> / ${points.length}</span>
        </div>
        ${buildElevationChart(points,selected)}
      </div>`;
  }

  const neighborsPanel=p?buildNeighborsComparisonDrawer(points,objects,scene?.terrain,selected):'';

  const roadInput=(field,label,min=-32768,max=32767)=>{
    const prop={left_width:'leftWidth',right_width:'rightWidth',cross_slope:'crossSlope'}[field]||field;
    const value=p[prop];
    return `<div class="setting"><label for="road-${field}">${label}</label><input id="road-${field}" data-road-field="${field}" type="number" step="1" min="${min}" max="${max}" value="${value}" data-original="${value}" required></div>`;
  };
  const roadPanel=p?`<form id="road-point-form" data-bank="${bank}" data-index="${selected}" data-revision="${scene.revision}"><p class="muted" hidden>Changes affect the driveable route, not the visible asphalt mesh. Keep both aligned before testing in game.</p><div class="compact-field-grid">${roadInput('x','Position X',-2147483648,2147483647)}${roadInput('y','Height')}${roadInput('z','Position Z',-2147483648,2147483647)}${roadInput('left_width','Left half-width')}${roadInput('right_width','Right half-width')}</div><details><summary>Segment direction &amp; curvature</summary><p class="muted">Heading and pitch use 4096 units per turn. Cross-slope uses 1/128. Length is stored separately from point coordinates; it is not recalculated. Arc is the original packed reference to an existing curve center.</p><div class="compact-field-grid">${roadInput('angle','Heading')}${roadInput('pitch','Pitch')}${roadInput('cross_slope','Cross-slope')}${roadInput('length','Segment length',1,32767)}${roadInput('arc','Packed curve reference',0,32767)}</div></details><footer><button type="submit" class="primary">Save point to project</button><button type="button" data-action="publish-mods">Save & apply project</button></footer></form>`:'';

  const arcPanel=arc?`<details class="inspector-subdrawer" open><summary>Curve center ${arcIndex+1}</summary><p class="muted">Shared by every segment referencing this center. Changing it affects the driveable curve, not scenery or asphalt geometry.</p><form id="road-arc-form" data-bank="${bank}" data-index="${arcIndex}" data-revision="${scene.revision}"><div class="compact-field-grid">${['x','z'].map(field=>`<div class="setting"><label for="arc-${field}">Center ${field.toUpperCase()}</label><input id="arc-${field}" data-arc-field="${field}" type="number" step="1" min="-2147483648" max="2147483647" value="${arc[field]}" data-original="${arc[field]}" required></div>`).join('')}</div><button type="submit" class="primary">Save curve center to project</button></form></details>`:'';

  const trackId=bank>=136?'track.custom'+((bank-136)/2+1):'track.'+['big','mid','hi','oval'][((bank-88)/2)%4]+(Math.floor((bank-88)/8)+1);
  const input=(field,label,min=-2147483648,max=2147483647)=>{
    const value=field==='yaw'?(((object.yaw+2048)%4096+4096)%4096-2048)*360/4096:object[field];
    return `<div class="setting"><label for="object-${field}">${label}</label><input id="object-${field}" data-object-field="${field}" type="number" step="${field==='yaw'?'any':'1'}" min="${min}" max="${max}" value="${value}" data-original="${value}" required></div>`;
  };
  const objectPanel=scene?`<section class="card"><div class="row between wrap"><h2>Placed 3D Scenery (${objects.length})</h2><div class="row"><button type="button" data-step-object="-1" ${objects.length?'':'disabled'}>◀</button><button type="button" data-step-object="1" ${objects.length?'':'disabled'}>▶</button></div></div><form id="track-add-object" class="toolbar-inline-form" data-bank="${bank}" data-point="${selected}" data-revision="${scene.revision}"><label>Scenery model <select name="model" required>${Array.from({length:Math.min(scene.courseModels||0,32768)},(_,i)=>`<option value="${i}" ${i===Math.max(0,object?.model||0)?'selected':''}>Course model ${i+1}</option>`).join('')}</select></label><button type="button" data-preview-add-model="${trackId}.course" ${scene.courseModels?'':'disabled'}>Preview chosen model</button><button type="submit" class="primary" ${scene.courseModels?'':'disabled'}>+ Add at road point ${selected+1}</button></form><p class="muted" hidden>New objects start at the selected road point. Duplicate keeps the original transform; move the copy in 3D or edit its coordinates. Remove keeps object IDs stable and can be undone.</p><label>Select placed object <select data-track-object>${objects.map((o,i)=>`<option value="${i}" ${i===objectIndex?'selected':''}>Object ${i+1}${o.model===-1?' · disabled':` · Course model ${o.model+1}`}</option>`).join('')}</select></label>${object?`<form id="track-object-form" data-bank="${bank}" data-index="${objectIndex}" data-revision="${scene.revision}"><div class="compact-field-grid">${input('x','Position X')}${input('y','Position Y (height)')}${input('z','Position Z')}${input('yaw','Rotation (degrees)',-180,180)}</div><details class="car-advanced"><summary>Model & rendering flags</summary><p class="muted">Model -1 hides the object. Flags: 1 = alt normals, 2 = alt env, 4 = env, 8 = blinking env.</p><div class="compact-field-grid">${input('model','Model',-1,32767)}${input('flags','Flags',0,15)}</div></details>${object.model>=0?`<div class="model-io-box"><div class="eyebrow">3D Mesh &amp; Textures (Course Model ${object.model+1})</div><div class="row wrap"><button type="button" data-preview-track-part="${trackId}.course" data-part="${object.model}">👁 Preview model</button><button type="button" data-track-textures="${trackId}.course" data-part="${object.model}">🎨 Textures</button><button type="button" data-export-track-part="${trackId}.course" data-part="${object.model}" data-format="glb">⬇ Export model GLB</button><button type="button" data-export-track-part="${trackId}.course" data-part="${object.model}" data-format="obj">⬇ Export OBJ</button><button type="button" class="primary" data-import-track-part="${trackId}.course" data-part="${object.model}" data-revision="${scene.revision}">⬆ Import model</button></div><p class="muted" hidden>Geometry is shared by every object using model ${object.model}. Each export creates a new folder named after the track and model; repeated exports add a number and preserve your earlier files. GLB contains embedded textures. Import accepts GLB, glTF or OBJ. Preserve material custom properties or rage_N names. Geometry changes affect modern rendering only; road physics is unchanged. OBJ export includes PNG textures. Import keeps geometry and textures together when track-model.json remains beside the OBJ; material textures are shared by other models too.</p></div>`:''}<footer>${object.model>=0?`<button type="button" data-duplicate-track-object="${objectIndex}">Duplicate</button><button type="button" data-remove-track-object="${objectIndex}">Remove</button>`:''}<button type="submit" class="primary">Save object to project</button><button type="button" data-action="publish-mods">Save & apply project</button></footer></form>`:''}</section>`:'';

  const geometryPanel=`<details class="card"><summary>Advanced · Native model banks</summary><p class="muted">Export or import the complete course object bank or terrain mesh bank. Individual models also support GLB with embedded textures and OBJ exchange above (modern renderer).</p>${[['course','Course objects'],['terrain','Terrain cells']].map(([name,label])=>`<div class="setting"><span><strong>${label}</strong> (${trackId}.${name})</span><div class="row wrap"><button data-export-track-mesh="${trackId}.${name}">⬇ Export native bank</button><button data-import-track-mesh="${trackId}.${name}" data-revision="${scene?.revision}">⬆ Import native bank</button></div></div>`).join('')}</details>`;

  const modeNav=`<nav class="tabs track-mode-tabs" aria-label="Track editor mode">${Object.entries(trackTabs).map(([id,label])=>`<button type="button" data-track-tab="${id}" class="${activeTab===id?'active':''}" aria-current="${activeTab===id?'page':'false'}">${label}</button>`).join('')}</nav>`;

  const stageCard=`
    <section class="card track-stage-card blender-workspace-card">
      ${layerBar}
      ${toolOptionsBar}
      <div class="blender-stage-body">
        ${leftToolDock}
        <div class="blender-viewport-main">
          <div data-track-stage-pane="2d" ${stageView==='2d'?'':'hidden'}>
            ${map}
            <p class="muted" hidden>Geometric top view of the current route, using stored curve centers and interpolated radii. Fixed-point physics may differ slightly. This is not a terrain or rendered road preview; switch to 3D track scene above to inspect terrain and objects.</p>
          </div>
          <div class="stage-3d-wrap" data-track-stage-pane="3d" ${stageView==='3d'?'':'hidden'}>
            <div class="viewport-floating-controls row between wrap">
              <span class="pill">🧊 3D View: ${isRoadOnlyPreset?'🛣️ Road Only':layers.isolate?`🔍 Segment #${selected+1} (±${radius})`:'🌍 Full 3D Model'}</span>
              <small class="muted">Click any road segment or object · <strong>Shift + Drag</strong> reshapes the route in 3D</small>
            </div>
            <div id="track-inline-preview" class="inline-stage" data-inline-track="${bank}" aria-label="Interactive 3D track scene"></div>
          </div>
        </div>
      </div>
    </section>`;

  const raceSubNav=`<nav class="tabs meta-subtabs" aria-label="Race & AI tools">${[['starts','Starting Grid'],['cameras','Track Cameras'],['intro','Intro Camera'],['ai','AI Line & Pace']].map(([id,label])=>`<button type="button" data-race-subtab="${id}" class="${raceSubtab===id?'active':''}">${label}</button>`).join('')}</nav>`;

  return `<div class="workspace-compact-header row between wrap"><div class="row wrap"><h1>${escape(track?.name||'Track workspace')}</h1><span class="muted">${points.length} road nodes · ${objects.length} placed objects · ${scene?.courseModels||0} course models</span></div><div class="row wrap"><label>Track &amp; Class <select data-track-select>${tracks.map(t=>`<option value="${t.bank}" ${t.bank===bank?'selected':''}>${escape(t.name)} · Class ${t.grade}${t.available?'':' · unavailable for racing'}</option>`).join('')}</select></label></div></div>${modeNav}<div class="track-studio-grid">${stageCard}<div class="track-inspector"><div class="track-pane" data-track-pane="road"${paneHidden('road')}><section class="card side-info"><div class="row between"><h2>Road point ${p?selected+1:'—'}</h2><span class="pill">of ${points.length}</span></div>${neighborsPanel}${roadPanel}${arcPanel}<details class="inspector-subdrawer"><summary>Source components</summary><ul>${(scene?.blocks||track?.blocks||[]).map(b=>`<li>${escape(b.role)} · ${b.size.toLocaleString('en-US')} bytes</li>`).join('')}</ul></details></section></div><div class="track-pane" data-track-pane="scenery"${paneHidden('scenery')}>${objectPanel}${terrainCellEditor(scene,bank)}${geometryPanel}</div><div class="track-pane" data-track-pane="atmosphere"${paneHidden('atmosphere')}>${environmentEditor(scene,bank)}</div><div class="track-pane" data-track-pane="race"${paneHidden('race')}>${raceSubNav}<div data-race-pane="starts" ${raceSubtab==='starts'||activeTab==='all'?'':'hidden'}>${startEditor(scene,bank)}</div><div data-race-pane="cameras" ${raceSubtab==='cameras'||activeTab==='all'?'':'hidden'}>${cameraEditor(scene,bank)}</div><div data-race-pane="intro" ${raceSubtab==='intro'||activeTab==='all'?'':'hidden'}>${introEditor(scene,bank)}</div><div data-race-pane="ai" ${raceSubtab==='ai'||activeTab==='all'?'':'hidden'}>${aiEditor(scene,bank)}${aiSpeedEditor(scene,bank)}</div></div></div></div>`;
}

export function terrainCellEditor(scene,bank,cell=0){
 const terrain=scene?.terrain;if(!terrain?.cells?.length)return '';
 if(!Number.isInteger(cell)||cell<0||cell>=terrain.cells.length)cell=0;
 const model=terrain.cells[cell]&1023,row=cell>>5,column=cell&31;
 const id=(bank>=136?'track.custom'+((bank-136)/2+1):'track.'+['big','mid','hi','oval'][((bank-88)/2)%4]+(Math.floor((bank-88)/8)+1))+'.terrain';
 const coordinate=(name,value)=>`<label>${name==='row'?'Row':'Column'} <select data-terrain-${name}>${Array.from({length:32},(_,i)=>`<option value="${i}" ${i===value?'selected':''}>${i+1}</option>`).join('')}</select></label>`;
 const actions=`<button type="button" data-track-textures="${id}" data-terrain-model-action data-part="${model}" ${model===1023?'hidden':''}>🎨 Textures</button>`+['preview','export','import'].map(action=>`<button type="button" data-${action}-track-part="${id}" data-format="glb" data-terrain-model-action data-part="${model}" data-revision="${scene.revision}" ${model===1023?'hidden':''}>${action==='preview'?'👁 Preview model':action==='export'?'⬇ Export model GLB':'⬆ Import model'}</button>`).join('');
 const visualGrid=terrain.cells.length===1024?`<div class="terrain-grid-wrap"><div class="row between"><small class="eyebrow">Interactive 32×32 Terrain Tile Map</small><small class="muted">Click any cell to inspect or replace its 3D terrain tile</small></div><div class="terrain-grid-map" role="grid" aria-label="32 by 32 terrain cell grid">${terrain.cells.map((raw,idx)=>{const m=raw&1023,empty=m===1023,active=idx===cell;const hue=(m*47)%360;return `<button type="button" class="terrain-tile ${empty?'empty':'filled'} ${active?'active':''}" data-select-terrain-cell="${idx}" style="${empty?'':`background:hsl(${hue},55%,38%)`}" title="Col ${(idx&31)+1}, Row ${(idx>>5)+1} · ${empty?'Empty':'Model '+m}"></button>`;}).join('')}</div></div>`:'';
 return `<section class="card" id="terrain-cell-editor" data-cell="${cell}"><h2>Terrain Grid &amp; 3D Tiles</h2><p class="muted" hidden>Choose any cell, including empty ones. Assigning a model changes only this cell; editing model geometry changes all cells sharing it. Each export creates a new folder named after the track and model; repeated exports add a number and preserve your earlier files. Visibility settings are preserved. GLB embeds textures; import accepts GLB, glTF or OBJ. Preserve material custom properties or rage_N names. Geometry changes affect modern rendering only, not road physics.</p>${visualGrid}<div class="row wrap">${coordinate('column',column)}${coordinate('row',row)}<span class="muted">Cell center: X ${column*2048+1024}, Z ${(31-row)*2048+1024} · Original grid row order</span></div><form id="terrain-cell-form" data-bank="${bank}" data-index="${cell}" data-revision="${scene.revision}"><label>Assigned Terrain Tile Model <select data-terrain-model data-original="${model}"><option value="1023" ${model===1023?'selected':''}>Empty cell</option>${Array.from({length:terrain.models},(_,i)=>`<option value="${i}" ${i===model?'selected':''}>Model ${i}</option>`).join('')}</select></label><div class="row wrap">${actions}</div><footer><button type="submit" class="primary">Save cell to project</button><button type="button" data-action="publish-mods">Save & apply project</button></footer></form></section>`;
}

function skyPalettes(palettes=[]){
 if(!palettes.length)return '';
 const hex=value=>'#'+value.toString(16).padStart(6,'0');
 return '<details><summary>Sky image palettes (5 palettes)</summary><p class="muted">These five palettes recolor the original sky images.</p>'+palettes.map((colors,index)=>'<details><summary>Palette '+(index+1)+'</summary><div style="height:24px;border-radius:4px;margin-bottom:8px;background:linear-gradient(to right,'+colors.flatMap((v,i)=>[hex(v)+' '+(i*100/colors.length)+'%',hex(v)+' '+((i+1)*100/colors.length)+'%']).join(',')+')" role="img" aria-label="Palette '+(index+1)+' color strip"></div><div class="row wrap">'+colors.map((value,color)=>'<label>Color '+(color+1)+'<input type="color" data-env-type="palette" data-env-index="'+index+'" data-env-field="color_'+color+'" value="'+hex(value)+'" data-original="'+hex(value)+'"></label>').join('')+'</div></details>').join('')+'</details>';
}
export function environmentEditor(scene,bank){
 const env=scene?.environment;if(!env)return '';
 const labels=['Fog','Sky top','Sky middle','Horizon','Sky bottom','Ground near top','Ground near bottom','Ground far top','Ground far bottom'];
 const hex=value=>'#'+Number(value||0).toString(16).padStart(6,'0');
 const color=(value,index,cue)=>`<label class="color-swatch-field"><span>${labels[index]}</span><input type="color" data-env-index="${cue}" data-env-field="color_${index}" value="${hex(value)}" data-original="${hex(value)}"></label>`;
 const number=(field,label,value,index,max,readonly=false)=>`<label>${label}<input type="number" data-env-index="${index}" data-env-field="${field}" value="${value}" data-original="${value}" min="0" max="${max}" step="1" required ${readonly?'readonly':''}></label>`;
 const timelineStrip=env.cues.length?`<div class="env-timeline-preview"><div class="eyebrow">Sky &amp; Horizon Timeline (${env.length.toLocaleString('en-US')} logic ticks)</div><div class="env-timeline-bar" style="background:linear-gradient(to right,${env.cues.map((c,i)=>`${hex(c.colors[3]||c.colors[1])} ${Math.round((c.time/Math.max(1,env.length))*100)}%`).join(',')})"></div></div>`:'';
 return `<section class="card"><h2>Atmosphere &amp; Time of Day</h2><p class="muted">Times and transitions use logic ticks, not rendering frames. Loop length: ${env.length.toLocaleString('en-US')} ticks. Sky texture row: ${env.skyRow+1}.</p>${timelineStrip}<form id="environment-form" data-bank="${bank}" data-revision="${scene.revision}"><details><summary>Loop & sky layout</summary><div class="compact-field-grid"><label>Loop length (logic ticks)<input type="number" data-env-type="environment" data-env-index="0" data-env-field="length" data-original="${env.length}" value="${env.length}" min="1" max="2147483647" step="1" required></label><label>Sky texture-row pair<select data-env-type="environment" data-env-index="0" data-env-field="sky_row" data-original="${env.skyRow}">${Array.from({length:4},(_,i)=>`<option value="${i}" ${env.skyRow===i?'selected':''}>Rows ${i+1} and ${i+2}</option>`).join('')}</select></label></div></details>${env.cues.map((cue,index)=>`<details ${index===0?'open':''}><summary>Color cue ${index+1} · tick ${cue.time}</summary><div class="color-swatch-grid">${cue.colors.slice(0,5).map((v,i)=>color(v,i,index)).join('')}</div><details><summary>Timing, palette & ground colors</summary><div class="compact-field-grid">${number('time','Start tick',cue.time,index,env.length-1,index===0)}${number('duration','Transition ticks',cue.duration,index,32767)}<label>Environment palette<select data-env-index="${index}" data-env-field="mode" data-original="${cue.mode}">${Array.from({length:5},(_,i)=>`<option value="${i}" ${cue.mode===i?'selected':''}>Palette ${i+1}</option>`).join('')}</select></label></div><div class="color-swatch-grid">${cue.colors.slice(5).map((v,i)=>color(v,i+5,index)).join('')}</div></details></details>`).join('')}${skyPalettes(env.palettes)}<div class="sticky-actions"><button type="reset">Discard unsaved colors</button><div class="row wrap"><button type="submit" class="primary">Save atmosphere to project</button><button type="button" data-action="publish-mods">Save & apply project</button></div></div></form></section>`;
}
export function aiEditor(scene,bank){
 if(!scene?.ai)return '';
 const input=(hint,key,label,min=-32768,max=32767)=>`<div class="setting"><label>${label}<input required type="number" step="1" min="${min}" max="${max}" data-ai-field="${key}" data-original="${hint[key]}" value="${hint[key]}"></label></div>`;
 return '<details class="card" open><summary>Rivals · Racing line</summary><p class="muted">The front four rivals receive a sideways correction while they are inside the chosen range and no nearby car blocks them. Offsets use game world units; they are not heights. Equal minimum and maximum disables the correction.</p>'+scene.ai.hints.map((hints,s)=>`<details ${s===0?'open':''}><summary>${s?'Reverse':'Forward'}</summary>${hints===null?'<p>This source table has an unsupported layout and cannot be edited yet.</p>':hints.map((hint,i)=>`<details><summary>Stretch ${i+1} · ${hint.start}–${hint.end}</summary><form class="ai-hint-form" data-bank="${bank}" data-index="${s*30+i}" data-revision="${scene.revision}"><div class="compact-field-grid">${input(hint,'min_offset','Minimum sideways offset')}${input(hint,'max_offset','Maximum sideways offset')}${input(hint,'adjustment','Sideways correction per simulation tick')}</div><details><summary>Stretch boundaries</summary><p class="muted">Progress uses internal track progress divided by 16. Entries must remain ordered and cannot overlap. The sentinel and unused entries are preserved.</p><div class="compact-field-grid">${input(hint,'start','Start progress',0)}${input(hint,'end','End progress',0)}</div></details><button type="submit" class="primary">Save racing line to project</button></form></details>`).join('')}</details>`).join('')+'</details>';
}
export function aiSpeedEditor(scene,bank){
 if(!scene?.ai?.speeds)return '';
 const input=(key,label,value)=>`<div class="setting"><label>${label}<input required type="number" step="1" min="-32768" max="32767" data-ai-field="${key}" data-original="${value}" value="${value}"></label></div>`;
 return '<details class="card" open><summary>Rivals · Pace keys</summary><p class="muted">The game interpolates these AI targets between progress keys and uses them to limit acceleration. These are internal tuning values, not kph or mph. Later rivals derive their target from rival 4 at a reduced scale.</p>'+scene.ai.speeds.map((keys,s)=>`<details ${s===0?'open':''}><summary>${s?'Reverse':'Forward'}</summary>${keys===null?'<p>This source table cannot be edited.</p>':keys.map((key,i)=>`<details><summary>Pace key ${i+1} · progress ${key.progress}</summary><form class="ai-hint-form" data-speed="true" data-bank="${bank}" data-index="${s*48+i}" data-revision="${scene.revision}"><div class="compact-field-grid">${key.targets.map((value,slot)=>input('target_'+slot,'Rival '+(slot+1)+' target',value)).join('')}</div><details><summary>Progress & slide pitch</summary><p class="muted">Progress uses internal track progress / 16. Keep authored key order; some originals have local reversals. -1 is the preserved terminator and cannot be assigned here. Pitch uses 4096 units per turn.</p><div class="compact-field-grid">${input('progress','Progress',key.progress)}${input('pitch','Slide pitch',key.pitch)}</div></details><button type="submit" class="primary">Save pace key to project</button></form></details>`).join('')}</details>`).join('')+'</details>';
}
export function startEditor(scene,bank){
 if(!scene?.ai?.starts?.length)return '';
 const input=(key,label,value)=>`<div class="setting"><label>${label}<input required type="number" step="1" min="-2147483648" max="2147483647" data-ai-field="${key}" data-original="${value}" value="${value}"></label></div>`;
 return '<details class="card" open><summary>Race · Starting placements</summary><p class="muted">Player and rival positions are authored separately for each direction. Height comes from the road. Keep X/Z on the driveable route; otherwise the game falls back to the selected road point. Editing an inactive slot does not enable it.</p>'+scene.ai.starts.map((slots,s)=>`<details ${s===0?'open':''}><summary>${s?'Reverse':'Forward'}</summary>${slots.map((slot,i)=>`<details><summary>${i?'Rival '+i:'Player'}${slot.seed===-1?' · inactive':''}</summary><form class="ai-hint-form" data-start="true" data-bank="${bank}" data-index="${s*12+i}" data-revision="${scene.revision}"><div class="compact-field-grid">${input('x','Position X',slot.x)}${input('z','Position Z',slot.z)}</div><details><summary>Road reference & progress seed</summary><label>Road point <select required data-ai-field="point" data-original="${slot.point}">${scene.road.points.map((_,n)=>`<option value="${n}" ${n===slot.point?'selected':''}>Point ${n+1}</option>`).join('')}</select></label><p class="muted">Progress seed ${slot.seed} is preserved. -1 means inactive; other values also choose initial lap-progress traversal. Enabling extra cars is not connected here.</p></details><button type="submit" class="primary">Save starting position to project</button></form></details>`).join('')}</details>`).join('')+'</details>';
}
export function cameraEditor(scene,bank){
 if(!scene?.cameras)return '';
 const modes=['In-car','Chase','Blended track node','Path around car','Sliding track node','Orbit'],titles=['GP forward','GP reverse','Default'];
 const input=(key,label,value,min=-2147483648,max=2147483647)=>`<div class="setting"><label>${label}<input required type="number" step="1" min="${min}" max="${max}" data-ai-field="${key}" data-original="${value}" value="${value}"></label></div>`;
 return '<details class="card" open><summary>Track cameras</summary><p class="muted">Camera lists used by the game, including replay views. Sections are rendering section references, not road-point numbers. Shared nodes affect every list that references them. The race starting camera is edited separately below.</p>'+scene.cameras.map((nodes,t)=>`<details ${t===0?'open':''}><summary>${titles[t]}</summary>${nodes===null?'<p>This source camera list cannot be edited.</p>':nodes.map((node,n)=>{const world=node.mode===2||node.mode===4,labels=world?['World X','World Y','World Z',node.mode===2?'Blend (per 10000)':'Car focus height']:['Pitch','Yaw','Roll','Distance'],used=scene.cameras.map((list,i)=>list?.some(x=>x.id===node.id)?titles[i]:null).filter(Boolean);return `<details><summary>View ${n+1} · ${modes[node.mode]} · section ${node.section}</summary><form class="ai-hint-form" data-camera="true" data-bank="${bank}" data-index="${node.id}" data-revision="${scene.revision}"><div class="compact-field-grid">${input('section','Rendering section',node.section,0,32767)}${input('duration','Transition duration (simulation ticks)',node.duration)}</div><details><summary>Camera type & parameters</summary><label>Type <select required data-ai-field="mode" data-original="${node.mode}">${modes.map((label,i)=>`<option value="${i}" ${i===node.mode?'selected':''}>${label}</option>`).join('')}</select></label><div class="compact-field-grid">${node.data.map((v,i)=>input('data_'+i,labels[i],v)).join('')}${node.offset.map((v,i)=>input('offset_'+i,(node.mode===4?'Destination':'Car-relative focus offset')+' '+['X','Y','Z'][i],v)).join('')}</div><p class="muted">Used by: ${used.join(', ')}.</p></details><button type="submit" class="primary">Save camera to project</button></form></details>`;}).join('')}</details>`).join('')+'</details>';
}
export function introEditor(scene,bank){
 if(!scene?.intro)return '';
 const titles=['Forward','Reverse'];
 const input=(field,label,value,min=-2147483648,max=2147483647)=>`<label>${label}<input required type="number" step="1" min="${min}" max="${max}" data-ai-field="${field}" data-original="${value}" value="${value}"></label>`;
 return '<details class="card" open><summary>Race · Starting camera</summary><p class="muted">Camera before the start of a single race, not the Grand Prix announcement. Positions use game units. The camera aims at the player. The terminal key switches to the in-car view with a fade.</p>'+scene.intro.map((keys,s)=>`<details ${s===0?'open':''}><summary>${titles[s]}</summary>${keys===null?'<p>This source script cannot be edited.</p>':keys.map((key,n)=>`<details><summary>Key ${n+1} · ${key.mode?'In-car transition':'Track position'}</summary><form class="ai-hint-form" data-intro="true" data-bank="${bank}" data-index="${key.id}" data-revision="${scene.revision}"><div class="compact-field-grid">${['x','y','z'].map(a=>input(a,'World '+a.toUpperCase(),key[a])).join('')}</div><details><summary>Timing</summary>${key.mode?'<p>Terminal transition frame '+key.frame+' is preserved.</p>':input('frame','Transition frame',key.frame,-32768,32767)}${input('duration','Duration',key.duration,key.mode?0:1,32767)}<p class="muted">Used by: ${scene.intro.map((list,i)=>list?.some(x=>x.id===key.id)?titles[i]:null).filter(Boolean).join(', ')}.</p></details><button type="submit" class="primary">Save starting camera to project</button></form></details>`).join('')}</details>`).join('')+'</details>';
}
export function surfaceDialog(data){
 const car=data.kind==='car',models=s=>s.models.map(m=>car?escape(m):'Model '+(m+1)).join(', ');
 return `<section data-track-surfaces="${escape(data.id)}" data-surface-kind="${car?'car':'track'}" data-part="${data.part}" data-revision="${data.revision}"><h2>${car?'Car surfaces':'Model textures'}</h2><p class="muted">Replace a surface with a PNG up to 4096 × 4096 pixels (32 MiB). Textures and materials are shared by this bank. Changes affect the modern renderer. Reset reveals the selected package or disc values.${car?' Used by includes current model replacements and shared LODs.':''}</p>${data.usageWarning?`<p class="muted">${escape(data.usageWarning)}</p>`:''}<div class="save-list">${data.surfaces.map(s=>`<article class="card"><h3>${escape(s.label)}</h3>${s.png?`<img width="160" height="120" style="object-fit:contain;image-rendering:pixelated" alt="${escape(s.label)}" src="data:image/png;base64,${s.png}"><p>${s.width} × ${s.height} · ${escape(s.origin)}</p>`:'<p>Material without a replaceable image</p>'}<p class="muted">Used by: ${models(s)}.</p><div class="row wrap"><button data-project-material="${escape(s.materialId||s.id)}" data-material-inherited="${s.inherited?'true':'false'}" data-material-value="${escape(s.properties||'inherit auto 1 0 1 1 1 1 0 0 0')}">Color & finish</button><button data-reset-track-material="${escape(s.id)}" ${s.materialOverridden?'':'disabled'}>Reset material</button>${s.png?`<button data-export-track-texture="${escape(s.source)}">Export PNG</button><button data-import-track-texture="${escape(s.source)}">Replace PNG</button><button data-reset-track-texture="${escape(s.source)}" ${s.overridden?'':'disabled'}>Reset project texture</button>`:''}</div><details><summary>Technical identity</summary><p>${escape(s.id)}</p></details></article>`).join('')||'<p>No editable surfaces are used by this model.</p>'}</div><footer><button data-action="close">Close</button></footer></section>`;
}
