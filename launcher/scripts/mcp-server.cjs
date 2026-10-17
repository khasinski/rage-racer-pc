#!/usr/bin/env node
const http = require('node:http');
const path = require('node:path');
const os = require('node:os');
const {LauncherService} = require('../main/service.cjs');

const PREVIEW_PORT = Number(process.env.RAGE_STUDIO_PORT || process.env.PORT || 4173);
const PREVIEW_HOST = process.env.RAGE_STUDIO_HOST || '127.0.0.1';

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

function recalculateRoadGeometry(basePoints, draftPoints) {
  const n = basePoints?.length || 0;
  if (!n) return [];
  const out = draftPoints.map(p => ({...p}));
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
    const origPitch = Math.atan2((b0.y || 0) - (b1.y || 0), Math.max(1, origChord));
    const newPitch = Math.atan2((d0.y || 0) - (d1.y || 0), Math.max(1, newChord));
    const dPitch = Math.round((normalizeAngleDeltaRad(newPitch - origPitch) * 4096) / (2 * Math.PI));
    d0.pitch = clampInt16((b0.pitch || 0) + dPitch);
  }
  return out;
}

function deformRoadPoints(basePoints, sourcePoints, centerIndex, delta, radius = 6) {
  const n = sourcePoints?.length || 0;
  if (!n) return [];
  const next = sourcePoints.map(p => ({...p}));
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

function shapeRoadSection(basePoints, currentPoints, centerIndex, radius = 6, operation = 'smooth', amount = 0) {
  const n = currentPoints?.length || 0;
  if (!n) return [];
  const next = currentPoints.map(p => ({...p}));
  const r = Math.max(1, Math.min(Math.floor((n - 1) / 2), Number(radius) || 6));

  if (operation === 'smooth' || operation === 'smooth-xz') {
    for (let pass = 0; pass < 2; pass++) {
      const snap = next.map(p => ({...p}));
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
  } else if (operation === 'level' || operation === 'smooth-y') {
    for (let pass = 0; pass < 2; pass++) {
      const snap = next.map(p => ({...p}));
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
  } else if (operation === 'widen' || operation === 'width') {
    for (let offset = -r; offset <= r; offset++) {
      const idx = (centerIndex + offset + n) % n;
      const w = 0.5 * (1 + Math.cos((Math.PI * Math.abs(offset)) / (r + 1)));
      const deltaW = Math.round((amount || 40) * w);
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

function diffRoadPoints(basePoints, draftPoints) {
  const entries = [];
  const fieldPairs = [
    ['x', 'x'], ['y', 'y'], ['z', 'z'],
    ['angle', 'angle'], ['pitch', 'pitch'],
    ['crossSlope', 'cross_slope'],
    ['leftWidth', 'left_width'], ['rightWidth', 'right_width'],
    ['length', 'length'], ['arc', 'arc']
  ];
  for (let i = 0; i < Math.min(basePoints.length, draftPoints.length); i++) {
    const b = basePoints[i], d = draftPoints[i];
    const values = {};
    for (const [prop, field] of fieldPairs) {
      if (d[prop] !== undefined && d[prop] !== b[prop]) values[field] = d[prop];
    }
    if (Object.keys(values).length) entries.push({type: 'road', index: i, values});
  }
  return entries;
}

function getTerrainCellModel(cells, idx) {
  const raw = cells?.[idx];
  if (raw === undefined || raw === null) return 1023;
  if (typeof raw === 'number') return raw & 1023;
  return Number(raw.model ?? 1023) & 1023;
}

const TOOLS = [
  {
    name: 'studio_status',
    description: 'Get Rage Car Studio & Rage Track Studio status, disc readiness, project revision, dirty state, and undo/redo availability.',
    inputSchema: {
      type: 'object',
      properties: {},
      additionalProperties: false
    }
  },
  {
    name: 'car_list',
    description: 'List all editable cars in Rage Car Studio, including base player slots (0-31, banks 10-72), custom expandable slots (32-63, banks 200-262), and rival cars.',
    inputSchema: {
      type: 'object',
      properties: {
        includeRivals: {type: 'boolean', description: 'Include non-player rival cars (default false).'}
      },
      additionalProperties: false
    }
  },
  {
    name: 'car_get_spec',
    description: 'Read full identity, engine, transmission, and handling specifications for a player car bank (e.g. 10..72 or custom 200..262).',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Player car bank (even integer 10..72 for slots 0..31, or 200..262 for custom slots 32..63).'}
      },
      required: ['bank'],
      additionalProperties: false
    }
  },
  {
    name: 'car_update_spec',
    description: 'Update car identity, engine, gear ratios, or handling fields in Rage Car Studio (e.g. name, manufacturer, className, unlockClass, price, upgradePrice, revLimit, redline, topGear, baseSteeringGrip, steerResponse, speedDragDivisor, gearRatio1..gearRatio6).',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Player car bank (10..72 or 200..262).'},
        values: {
          type: 'object',
          description: 'Key-value map of car spec fields to update.'
        },
        apply: {type: 'boolean', description: 'Immediately compile and apply changes to the game (default true).'}
      },
      required: ['bank', 'values'],
      additionalProperties: false
    }
  },
  {
    name: 'car_create',
    description: 'Create a new car in an expandable slot (32-63, banks 200-262) or overwrite an existing slot (0-31, banks 10-72) by cloning physics and 3D meshes from a donor car bank.',
    inputSchema: {
      type: 'object',
      properties: {
        targetBank: {
          oneOf: [{type: 'string', enum: ['new']}, {type: 'integer'}],
          description: 'Use "new" to allocate the next free expandable car slot (bank >= 200), or specify an even bank (10..72 or 200..262).'
        },
        sourceBank: {type: 'integer', description: 'Donor car bank (10..72) to clone base 3D meshes and physics from (default 10).'},
        name: {type: 'string', description: 'Display name of the new car (max 31 chars).'},
        manufacturer: {type: 'string', description: 'Manufacturer name (max 31 chars).'},
        className: {type: 'string', description: 'Class badge label (max 31 chars).'},
        unlockClass: {type: 'integer', description: 'Unlock grade 0..5 (0 = available from start).'},
        price: {type: 'integer', description: 'Showroom price in credits.'},
        upgradePrice: {type: 'integer', description: 'Upgrade price in credits.'},
        apply: {type: 'boolean', description: 'Immediately apply changes to the game (default true).'}
      },
      required: ['name'],
      additionalProperties: false
    }
  },
  {
    name: 'car_import_export_3d',
    description: 'Import or export a 3D car part mesh (.glb or .obj) for a car in Rage Car Studio.',
    inputSchema: {
      type: 'object',
      properties: {
        action: {type: 'string', enum: ['export_part', 'import_part'], description: 'Whether to export or import a 3D car part mesh.'},
        bank: {type: 'integer', description: 'Car bank (e.g. 10..72 or 200..262).'},
        part: {type: 'integer', description: 'Part index: 0=Body Shell, 1=Cabin/Interior, 2=Front Wheels, 3=Rear Wheels, 4=Aero/Shadow (default 0).'},
        rival: {type: 'boolean', description: 'True if rival car bank (default false).'},
        format: {type: 'string', enum: ['glb', 'obj'], description: 'Export format when action is export_part (default "glb").'},
        filePath: {type: 'string', description: 'Source file path (.glb/.obj) when action is import_part, or destination folder path when action is export_part.'}
      },
      required: ['action', 'bank', 'filePath'],
      additionalProperties: false
    }
  },
  {
    name: 'track_list',
    description: 'List all tracks and class layouts available in Rage Track Studio (banks 88..134).',
    inputSchema: {
      type: 'object',
      properties: {},
      additionalProperties: false
    }
  },
  {
    name: 'track_get_summary',
    description: 'Get a concise summary of a track in Rage Track Studio: road segment count, world bounds, elevation range, populated 32x32 terrain units, and placed decorations.',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Track bank (even integer 88..134, e.g. 88 = Mythical Coast Class 1).'}
      },
      required: ['bank'],
      additionalProperties: false
    }
  },
  {
    name: 'track_get_road_points',
    description: 'Read road route segments for a track with world coordinates (x, z), heightUp (-y), width (leftWidth, rightWidth), and banking (crossSlope).',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Track bank (88..134).'},
        start: {type: 'integer', description: 'Start segment index (0-based, default 0).'},
        count: {type: 'integer', description: 'Number of segments to return (default 50, max 500).'}
      },
      required: ['bank'],
      additionalProperties: false
    }
  },
  {
    name: 'track_deform_road',
    description: 'Shape the track road route in Rage Track Studio (same as dragging road points or clicking Smooth/Straighten/Level/Widen/Bank). Automatically recalculates segment lengths, headings, and pitches.',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Track bank (88..134).'},
        pointIndex: {type: 'integer', description: 'Center road segment index (0-based).'},
        radius: {type: 'integer', description: 'Falloff neighborhood radius in segments (default 6).'},
        dx: {type: 'number', description: 'World X displacement at pointIndex.'},
        dz: {type: 'number', description: 'World Z displacement at pointIndex.'},
        heightDelta: {type: 'number', description: 'Height change at pointIndex (positive = raise road UP, negative = lower road DOWN).'},
        shapeOp: {
          type: 'string',
          enum: ['smooth', 'straighten', 'level', 'widen', 'bank'],
          description: 'Optional shaping operation to apply across ±radius around pointIndex.'
        },
        shapeAmount: {type: 'number', description: 'Amount for widen (e.g. 40) or bank (e.g. -24 or 24).'},
        apply: {type: 'boolean', description: 'Immediately compile and apply to the game (default false).'}
      },
      required: ['bank', 'pointIndex'],
      additionalProperties: false
    }
  },
  {
    name: 'track_set_terrain_unit',
    description: 'Replace one or more terrain units in the 32x32 terrain grid of a track (assign a 3D terrain model ID 0..terrainModels-1, or 1023 for empty).',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Track bank (88..134).'},
        col: {type: 'integer', description: 'Grid column X (0..31). Used when setting a single tile.'},
        row: {type: 'integer', description: 'Grid row Y (0..31). Used when setting a single tile.'},
        cell: {type: 'integer', description: 'Direct cell index (0..1023, where cell = row * 32 + col).'},
        model: {type: 'integer', description: '3D Terrain Unit model ID (0..terrainModels-1, or 1023 for empty).'},
        cells: {
          type: 'array',
          description: 'Optional batch of terrain cells to update: [{col, row, cell, model}].',
          items: {
            type: 'object',
            properties: {
              col: {type: 'integer'},
              row: {type: 'integer'},
              cell: {type: 'integer'},
              model: {type: 'integer'}
            },
            required: ['model']
          }
        },
        apply: {type: 'boolean', description: 'Immediately compile and apply to the game (default false).'}
      },
      required: ['bank'],
      additionalProperties: false
    }
  },
  {
    name: 'track_manage_decorations',
    description: 'Place, move, rotate, change model, duplicate, or delete track decorations (3D scenery objects) in Rage Track Studio.',
    inputSchema: {
      type: 'object',
      properties: {
        bank: {type: 'integer', description: 'Track bank (88..134).'},
        action: {
          type: 'string',
          enum: ['list', 'add', 'update', 'duplicate', 'remove'],
          description: 'Decoration operation to perform.'
        },
        objectIndex: {type: 'integer', description: 'Decoration index (0-based) for update, duplicate, or remove.'},
        model: {type: 'integer', description: '3D Course Model ID (0..courseModels-1) for add or update.'},
        roadPointIndex: {type: 'integer', description: 'Road segment index (0-based) where the new decoration should be placed (default 0).'},
        x: {type: 'number', description: 'Optional world X position override.'},
        y: {type: 'number', description: 'Optional world Y position override.'},
        z: {type: 'number', description: 'Optional world Z position override.'},
        yawDeg: {type: 'number', description: 'Optional rotation angle in degrees (-360..360).'},
        apply: {type: 'boolean', description: 'Immediately compile and apply to the game (default false).'}
      },
      required: ['bank', 'action'],
      additionalProperties: false
    }
  },
  {
    name: 'project_save_and_apply',
    description: 'Apply/compile all pending Car Studio and Track Studio edits into the game, or undo/redo project history.',
    inputSchema: {
      type: 'object',
      properties: {
        action: {type: 'string', enum: ['apply', 'undo', 'redo'], description: 'Project action (default "apply").'}
      },
      additionalProperties: false
    }
  },
  {
    name: 'launch_race',
    description: 'Save/apply pending edits and boot Rage Racer directly into a race (skipping intro and menus) with the chosen car and track.',
    inputSchema: {
      type: 'object',
      properties: {
        carBank: {type: 'integer', description: 'Player car bank (e.g. 10..72 or custom 200..262, default 10).'},
        trackBank: {type: 'integer', description: 'Track bank (even integer 88..134, default 88).'},
        trackPoint: {type: 'integer', description: 'Optional road segment index to spawn at.'}
      },
      additionalProperties: false
    }
  }
];

function createHttpOrLocalCaller() {
  let localService = null;

  async function callPreviewServer(method, args) {
    return new Promise((resolve, reject) => {
      const payload = JSON.stringify({args});
      const req = http.request(
        {
          hostname: PREVIEW_HOST,
          port: PREVIEW_PORT,
          path: `/api/${encodeURIComponent(method)}`,
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(payload)
          },
          timeout: 120000
        },
        res => {
          const chunks = [];
          res.on('data', d => chunks.push(d));
          res.on('end', () => {
            try {
              const parsed = JSON.parse(Buffer.concat(chunks).toString('utf8'));
              resolve(parsed);
            } catch (e) {
              reject(e);
            }
          });
        }
      );
      req.on('error', reject);
      req.on('timeout', () => {
        req.destroy(new Error('Preview server request timed out'));
      });
      req.write(payload);
      req.end();
    });
  }

  async function ensureLocalService() {
    if (localService) return localService;
    const resourcesDir = path.resolve(__dirname, '../resources');
    const userDataDir = process.env.RAGE_STUDIO_USER_DATA || path.join(os.homedir(), 'Library', 'Application Support', 'Rage Racer Launcher');
    localService = new LauncherService({
      root: userDataDir,
      bin: path.join(resourcesDir, 'bin'),
      config: path.join(resourcesDir, 'rage-port.ini')
    });
    await localService.init();
    return localService;
  }

  return async function callApi(method, ...args) {
    try {
      const res = await callPreviewServer(method, args);
      if (res && typeof res === 'object' && 'ok' in res) {
        if (!res.ok) throw new Error(res.error || `Studio API error in ${method}`);
        return res.value;
      }
    } catch (err) {
      if (!err || (err.code !== 'ECONNREFUSED' && err.code !== 'EPERM' && err.code !== 'EACCES')) {
        if (err.message && !/ECONNREFUSED|connect|socket|timed out/i.test(err.message)) {
          throw err;
        }
      }
    }
    const svc = await ensureLocalService();
    if (typeof svc[method] !== 'function') {
      throw new Error(`Unsupported studio method: ${method}`);
    }
    return await svc[method](...args);
  };
}

function createMcpServer({callApi = createHttpOrLocalCaller()} = {}) {
  async function executeTool(name, args = {}) {
    switch (name) {
      case 'studio_status': {
        const snap = await callApi('snapshot');
        const proj = snap.project || (snap.ready ? await callApi('project').catch(() => null) : null);
        return {
          ready: Boolean(snap.ready),
          toolsReady: Boolean(snap.toolsReady),
          running: Boolean(snap.running),
          disc: snap.disc ? {path: snap.disc.path, region: snap.disc.region} : null,
          project: proj
            ? {
                name: proj.name,
                revision: proj.revision,
                dirty: Boolean(proj.dirty),
                canUndo: Boolean(proj.canUndo),
                canRedo: Boolean(proj.canRedo),
                changeCount: Array.isArray(proj.changes) ? proj.changes.length : 0
              }
            : null
        };
      }

      case 'car_list': {
        const res = await callApi('cars');
        const parts = res?.parts || [];
        const byCar = new Map();
        for (const p of parts) {
          if (!args.includeRivals && p.rival) continue;
          const key = `${p.rival ? 'rival' : 'player'}:${p.bank}`;
          if (!byCar.has(key)) {
            const slot = p.rival
              ? null
              : p.bank >= 200
              ? 32 + Math.floor((p.bank - 200) / 2)
              : Math.floor((p.bank - 10) / 2);
            byCar.set(key, {
              bank: p.bank,
              slot,
              name: p.name,
              rival: Boolean(p.rival),
              customSlot: Boolean(p.customSlot || p.bank >= 200),
              donorBank: p.donorBank || null,
              partsCount: 0
            });
          }
          byCar.get(key).partsCount++;
        }
        return {
          totalCars: byCar.size,
          maxPlayerSlots: 64,
          cars: [...byCar.values()].sort((a, b) => Number(a.rival) - Number(b.rival) || a.bank - b.bank)
        };
      }

      case 'car_get_spec': {
        const bank = Number(args.bank);
        const spec = await callApi('carSpec', bank);
        const fields = {};
        for (const f of spec?.fields || []) {
          fields[f.key] = {
            value: f.value,
            baseValue: f.baseValue,
            origin: f.origin,
            ...(f.min !== undefined ? {min: f.min, max: f.max} : {}),
            ...(f.maxLength !== undefined ? {maxLength: f.maxLength} : {})
          };
        }
        return {
          bank,
          revision: spec?.revision ?? 0,
          customSlot: Boolean(spec?.customSlot || bank >= 200),
          donorBank: spec?.donorBank || null,
          fields
        };
      }

      case 'car_update_spec': {
        const bank = Number(args.bank);
        const values = args.values || {};
        const spec = await callApi('carSpec', bank);
        let project = await callApi('saveCarSpec', bank, values, spec.revision);
        if (args.apply !== false) {
          project = await callApi('applyProject', project.revision);
        }
        return {
          bank,
          updatedFields: Object.keys(values),
          applied: args.apply !== false,
          project: {revision: project.revision, dirty: Boolean(project.dirty)}
        };
      }

      case 'car_create': {
        const proj = await callApi('project');
        const opts = {
          targetBank: args.targetBank ?? 'new',
          sourceBank: Number(args.sourceBank) || 10,
          name: String(args.name || 'NEW CAR'),
          manufacturer: String(args.manufacturer || 'AGE'),
          className: String(args.className || 'PROTO'),
          unlockClass: Number(args.unlockClass ?? 0),
          price: Number(args.price ?? 15000),
          upgradePrice: Number(args.upgradePrice ?? 30000)
        };
        let project = await callApi('createNewCar', opts, proj.revision);
        const createdBank = project.createdBank || (typeof opts.targetBank === 'number' ? opts.targetBank : null);
        if (args.apply !== false) {
          const applied = await callApi('applyProject', project.revision);
          project = {...applied, createdBank};
        }
        const slot = createdBank >= 200 ? 32 + Math.floor((createdBank - 200) / 2) : Math.floor(((createdBank || 10) - 10) / 2);
        return {
          createdBank,
          slot,
          name: opts.name,
          donorBank: opts.sourceBank,
          applied: args.apply !== false,
          project: {revision: project.revision, dirty: Boolean(project.dirty)}
        };
      }

      case 'car_import_export_3d': {
        const bank = Number(args.bank);
        const part = Number(args.part ?? 0);
        const rival = Boolean(args.rival);
        const key = `car.${rival ? 'rival' : 'player'}.${bank}.part.${part}`;
        if (args.action === 'export_part') {
          const folder = await callApi('exportCar', key, args.filePath, args.format || 'glb');
          return {action: 'export_part', key, exportedTo: folder};
        }
        if (args.action === 'import_part') {
          const res = await callApi('importCar', key, args.filePath);
          return {action: 'import_part', key, result: res};
        }
        throw new Error(`Unsupported car_import_export_3d action: ${args.action}`);
      }

      case 'track_list': {
        const res = await callApi('tracks');
        return {tracks: res?.tracks || []};
      }

      case 'track_get_summary': {
        const bank = Number(args.bank);
        const scene = await callApi('trackScene', bank);
        const pts = scene?.road?.points || [];
        let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity, minZ = Infinity, maxZ = -Infinity;
        for (const p of pts) {
          minX = Math.min(minX, p.x); maxX = Math.max(maxX, p.x);
          minY = Math.min(minY, p.y); maxY = Math.max(maxY, p.y);
          minZ = Math.min(minZ, p.z); maxZ = Math.max(maxZ, p.z);
        }
        const cells = scene?.terrain?.cells || [];
        const populatedTerrainUnits = [];
        for (let i = 0; i < cells.length; i++) {
          const m = getTerrainCellModel(cells, i);
          if (m !== 1023) {
            populatedTerrainUnits.push({cell: i, col: i & 31, row: i >> 5, model: m});
          }
        }
        const decorations = (scene?.objects || [])
          .map((o, i) => ({
            objectIndex: i,
            model: o.model,
            x: o.x,
            y: o.y,
            z: o.z,
            yawDeg: Number(((o.yaw || 0) * 360 / 4096).toFixed(2))
          }))
          .filter(o => o.model >= 0);

        return {
          bank,
          revision: scene?.revision ?? 0,
          roadSegmentsCount: pts.length,
          curveArcsCount: (scene?.road?.arcs || []).length,
          bounds: pts.length
            ? {
                minX, maxX,
                minZ, maxZ,
                minHeightUp: -maxY,
                maxHeightUp: -minY
              }
            : null,
          terrainModelsAvailable: Number(scene?.terrain?.models ?? scene?.terrainModels ?? 0),
          populatedTerrainUnitsCount: populatedTerrainUnits.length,
          populatedTerrainUnits,
          decorationModelsAvailable: Number(scene?.courseModels ?? 0),
          placedDecorationsCount: decorations.length,
          placedDecorations: decorations
        };
      }

      case 'track_get_road_points': {
        const bank = Number(args.bank);
        const scene = await callApi('trackScene', bank);
        const pts = scene?.road?.points || [];
        const start = Math.max(0, Math.min(pts.length, Number(args.start) || 0));
        const count = Math.max(1, Math.min(500, Number(args.count) || 50));
        const slice = pts.slice(start, start + count).map((p, idx) => ({
          index: start + idx,
          x: p.x,
          y: p.y,
          heightUp: -p.y,
          z: p.z,
          leftWidth: p.leftWidth,
          rightWidth: p.rightWidth,
          crossSlope: p.crossSlope
        }));
        return {
          bank,
          totalSegments: pts.length,
          start,
          returned: slice.length,
          points: slice
        };
      }

      case 'track_deform_road': {
        const bank = Number(args.bank);
        const scene = await callApi('trackScene', bank);
        const basePoints = scene?.road?.points || [];
        if (!basePoints.length) throw new Error(`Track ${bank} has no road points`);
        const pointIndex = ((Number(args.pointIndex) || 0) % basePoints.length + basePoints.length) % basePoints.length;
        const radius = Number(args.radius ?? 6);
        let current = basePoints.map(p => ({...p}));

        const dx = Number(args.dx) || 0;
        const dz = Number(args.dz) || 0;
        const dy = args.heightDelta !== undefined ? -Number(args.heightDelta) : 0;
        if (dx || dy || dz) {
          current = deformRoadPoints(basePoints, current, pointIndex, {dx, dy, dz}, radius);
        }
        if (args.shapeOp) {
          current = shapeRoadSection(basePoints, current, pointIndex, radius, args.shapeOp, Number(args.shapeAmount) || 0);
        }

        const entries = diffRoadPoints(basePoints, current);
        let project = await callApi('project');
        if (entries.length) {
          project = await callApi('saveRoadBatch', bank, entries, scene.revision);
          if (args.apply) {
            project = await callApi('applyProject', project.revision);
          }
        }
        return {
          bank,
          pointIndex,
          radius,
          modifiedSegmentsCount: entries.length,
          applied: Boolean(args.apply),
          project: {revision: project.revision, dirty: Boolean(project.dirty)}
        };
      }

      case 'track_set_terrain_unit': {
        const bank = Number(args.bank);
        let scene = await callApi('trackScene', bank);
        let revision = scene.revision;
        let project = null;
        const list = Array.isArray(args.cells) && args.cells.length
          ? args.cells
          : [{col: args.col, row: args.row, cell: args.cell, model: args.model}];

        const updated = [];
        for (const item of list) {
          const cellIdx = Number.isInteger(item.cell)
            ? item.cell
            : ((Number(item.row) & 31) * 32 + (Number(item.col) & 31));
          const model = Number(item.model);
          project = await callApi('saveTerrainCell', bank, cellIdx, {model}, revision);
          revision = project.revision;
          updated.push({cell: cellIdx, col: cellIdx & 31, row: cellIdx >> 5, model});
        }
        if (args.apply && project) {
          project = await callApi('applyProject', project.revision);
        }
        return {
          bank,
          updatedTerrainUnits: updated,
          applied: Boolean(args.apply),
          project: project ? {revision: project.revision, dirty: Boolean(project.dirty)} : null
        };
      }

      case 'track_manage_decorations': {
        const bank = Number(args.bank);
        const scene = await callApi('trackScene', bank);
        const objs = scene?.objects || [];
        if (args.action === 'list') {
          return {
            bank,
            decorationModelsAvailable: Number(scene?.courseModels ?? 0),
            decorations: objs.map((o, i) => ({
              objectIndex: i,
              model: o.model,
              hidden: o.model < 0,
              x: o.x,
              y: o.y,
              z: o.z,
              yawDeg: Number(((o.yaw || 0) * 360 / 4096).toFixed(2))
            }))
          };
        }

        let project = null;
        if (args.action === 'add') {
          const model = Number(args.model ?? 0);
          const roadPt = Number(args.roadPointIndex ?? 0);
          project = await callApi('addTrackObject', bank, model, roadPt, scene.revision);
          if (args.x !== undefined || args.y !== undefined || args.z !== undefined || args.yawDeg !== undefined) {
            const updatedScene = await callApi('trackScene', bank);
            const newIdx = updatedScene.objects.length - 1;
            const patch = {};
            if (args.x !== undefined) patch.x = Math.round(Number(args.x));
            if (args.y !== undefined) patch.y = Math.round(Number(args.y));
            if (args.z !== undefined) patch.z = Math.round(Number(args.z));
            if (args.yawDeg !== undefined) patch.yaw = Math.round(Number(args.yawDeg) * 4096 / 360);
            project = await callApi('saveTrackObject', bank, newIdx, patch, updatedScene.revision);
          }
        } else if (args.action === 'update') {
          const idx = Number(args.objectIndex);
          const patch = {};
          if (args.model !== undefined) patch.model = Number(args.model);
          if (args.x !== undefined) patch.x = Math.round(Number(args.x));
          if (args.y !== undefined) patch.y = Math.round(Number(args.y));
          if (args.z !== undefined) patch.z = Math.round(Number(args.z));
          if (args.yawDeg !== undefined) patch.yaw = Math.round(Number(args.yawDeg) * 4096 / 360);
          project = await callApi('saveTrackObject', bank, idx, patch, scene.revision);
        } else if (args.action === 'duplicate') {
          project = await callApi('duplicateTrackObject', bank, Number(args.objectIndex), scene.revision);
        } else if (args.action === 'remove') {
          project = await callApi('removeTrackObject', bank, Number(args.objectIndex), scene.revision);
        } else {
          throw new Error(`Unsupported decoration action: ${args.action}`);
        }

        if (args.apply && project) {
          project = await callApi('applyProject', project.revision);
        }
        return {
          bank,
          action: args.action,
          applied: Boolean(args.apply),
          project: project ? {revision: project.revision, dirty: Boolean(project.dirty)} : null
        };
      }

      case 'project_save_and_apply': {
        const proj = await callApi('project');
        const act = args.action || 'apply';
        const res = act === 'apply'
          ? await callApi('applyProject', proj.revision)
          : await callApi('projectHistory', act, proj.revision);
        return {
          action: act,
          project: {
            revision: res.revision,
            dirty: Boolean(res.dirty),
            canUndo: Boolean(res.canUndo),
            canRedo: Boolean(res.canRedo)
          }
        };
      }

      case 'launch_race': {
        const proj = await callApi('project').catch(() => null);
        if (proj?.dirty) {
          await callApi('applyProject', proj.revision);
        }
        const carBank = Number(args.carBank ?? 10);
        const trackBank = Number(args.trackBank ?? 88);
        await callApi('play', {
          carBank,
          trackBank,
          mode: 'time-attack',
          ...(Number.isInteger(args.trackPoint) && args.trackPoint > 0 ? {trackPoint: args.trackPoint} : {})
        });
        return {
          launched: true,
          directBoot: true,
          carBank,
          trackBank,
          trackPoint: args.trackPoint ?? 0
        };
      }

      default:
        throw new Error(`Unknown MCP tool: ${name}`);
    }
  }

  async function handleRpc(message) {
    if (!message || typeof message !== 'object') return null;
    const {id, method, params} = message;

    // Notifications (no id) do not receive a response
    if (id === undefined || id === null) {
      return null;
    }

    try {
      if (method === 'initialize') {
        return {
          jsonrpc: '2.0',
          id,
          result: {
            protocolVersion: params?.protocolVersion || '2024-11-05',
            capabilities: {
              tools: {}
            },
            serverInfo: {
              name: 'rage-studio-mcp',
              version: '1.0.0'
            }
          }
        };
      }

      if (method === 'ping') {
        return {jsonrpc: '2.0', id, result: {}};
      }

      if (method === 'tools/list') {
        return {
          jsonrpc: '2.0',
          id,
          result: {
            tools: TOOLS
          }
        };
      }

      if (method === 'tools/call') {
        const toolName = params?.name;
        const toolArgs = params?.arguments || {};
        try {
          const data = await executeTool(toolName, toolArgs);
          return {
            jsonrpc: '2.0',
            id,
            result: {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(data, null, 2)
                }
              ],
              isError: false
            }
          };
        } catch (toolErr) {
          return {
            jsonrpc: '2.0',
            id,
            result: {
              content: [
                {
                  type: 'text',
                  text: `Error: ${toolErr?.message || String(toolErr)}`
                }
              ],
              isError: true
            }
          };
        }
      }

      return {
        jsonrpc: '2.0',
        id,
        error: {
          code: -32601,
          message: `Method not found: ${method}`
        }
      };
    } catch (err) {
      return {
        jsonrpc: '2.0',
        id,
        error: {
          code: -32603,
          message: err?.message || String(err)
        }
      };
    }
  }

  return {TOOLS, executeTool, handleRpc};
}

function startStdioServer() {
  const {handleRpc} = createMcpServer();
  let buffer = Buffer.alloc(0);
  let useContentLength = false;

  function sendResponse(resObj) {
    if (!resObj) return;
    const json = JSON.stringify(resObj);
    if (useContentLength) {
      const byteLen = Buffer.byteLength(json, 'utf8');
      process.stdout.write(`Content-Length: ${byteLen}\r\n\r\n${json}`);
    } else {
      process.stdout.write(`${json}\n`);
    }
  }

  async function processBuffer() {
    while (buffer.length > 0) {
      const headerMatch = /^Content-Length:\s*(\d+)\r?\n\r?\n/i.exec(buffer.toString('utf8', 0, Math.min(buffer.length, 128)));
      if (headerMatch) {
        useContentLength = true;
        const headerByteLen = Buffer.byteLength(headerMatch[0], 'utf8');
        const bodyByteLen = Number(headerMatch[1]);
        if (buffer.length < headerByteLen + bodyByteLen) return;
        const rawMsg = buffer.subarray(headerByteLen, headerByteLen + bodyByteLen).toString('utf8');
        buffer = buffer.subarray(headerByteLen + bodyByteLen);
        try {
          const msg = JSON.parse(rawMsg);
          const reply = await handleRpc(msg);
          sendResponse(reply);
        } catch {}
        continue;
      }

      const nlIdx = buffer.indexOf(0x0a);
      if (nlIdx === -1) return;
      const line = buffer.subarray(0, nlIdx).toString('utf8').trim();
      buffer = buffer.subarray(nlIdx + 1);
      if (!line) continue;
      try {
        const msg = JSON.parse(line);
        const reply = await handleRpc(msg);
        sendResponse(reply);
      } catch {}
    }
  }

  process.stdin.on('data', chunk => {
    buffer = Buffer.concat([buffer, chunk]);
    processBuffer();
  });
}

if (require.main === module) {
  startStdioServer();
}

module.exports = {createMcpServer, TOOLS};
