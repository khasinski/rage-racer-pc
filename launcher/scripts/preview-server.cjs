const http = require('node:http');
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const {LauncherService} = require('../main/service.cjs');

const PORT = Number(process.env.PORT || 4173);
const rendererDir = path.resolve(__dirname, '../renderer');
const resourcesDir = path.resolve(__dirname, '../resources');
const userDataDir = path.join(os.homedir(), 'Library', 'Application Support', 'Rage Racer Launcher');

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
};

// Build a 3D car mesh (9 floats per vertex: x, y, z, nx, ny, nz, material, u, v) for browser demo mode if no disc is mounted
function buildDemoCarMesh(bank = 10, partFilter = 'whole') {
  const vertices = [];
  const indices = [];
  const pushQuad = (p0, p1, p2, p3, mat) => {
    const ux = p1[0] - p0[0], uy = p1[1] - p0[1], uz = p1[2] - p0[2];
    const vx = p2[0] - p0[0], vy = p2[1] - p0[1], vz = p2[2] - p0[2];
    let nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
    const len = Math.hypot(nx, ny, nz) || 1;
    nx /= len; ny /= len; nz /= len;
    const base = vertices.length / 9;
    for (const [pt, u, v] of [[p0, 0, 0], [p1, 1, 0], [p2, 1, 1], [p3, 0, 1]]) {
      vertices.push(pt[0], pt[1], pt[2], nx, ny, nz, mat, u, v);
    }
    indices.push(base, base + 1, base + 2, base, base + 2, base + 3);
  };
  const pushBox = (cx, cy, cz, sx, sy, sz, mat) => {
    const x0 = cx - sx, x1 = cx + sx, y0 = cy - sy, y1 = cy + sy, z0 = cz - sz, z1 = cz + sz;
    pushQuad([x0, y1, z0], [x1, y1, z0], [x1, y1, z1], [x0, y1, z1], mat);
    pushQuad([x0, y0, z1], [x1, y0, z1], [x1, y0, z0], [x0, y0, z0], mat);
    pushQuad([x0, y0, z0], [x0, y1, z0], [x0, y1, z1], [x0, y0, z1], mat);
    pushQuad([x1, y0, z1], [x1, y1, z1], [x1, y1, z0], [x1, y0, z0], mat);
    pushQuad([x0, y1, z1], [x1, y1, z1], [x1, y0, z1], [x0, y0, z1], mat);
    pushQuad([x1, y1, z0], [x0, y1, z0], [x0, y0, z0], [x1, y0, z0], mat);
  };

  const wantBody = partFilter === 'whole' || partFilter.endsWith('.0') || partFilter.endsWith('.1');
  const wantFrontWheels = partFilter === 'whole' || partFilter.endsWith('.2') || partFilter.endsWith('.4');
  const wantRearWheels = partFilter === 'whole' || partFilter.endsWith('.3') || partFilter.endsWith('.4');

  if (wantBody) {
    pushBox(0, 48, 0, 210, 34, 440, 1);
    pushQuad([-175, 82, 210], [175, 82, 210], [195, 62, 430], [-195, 62, 430], 1);
    pushQuad([-150, 138, 30], [150, 138, 30], [175, 82, 210], [-175, 82, 210], 64);
    pushQuad([-145, 140, -160], [145, 140, -160], [150, 138, 30], [-150, 138, 30], 1);
    pushQuad([-165, 88, -320], [165, 88, -320], [145, 140, -160], [-145, 140, -160], 64);
    pushBox(-150, 108, -405, 8, 26, 20, 2);
    pushBox(150, 108, -405, 8, 26, 20, 2);
    pushBox(0, 132, -410, 185, 6, 36, 1);
  }
  if (wantFrontWheels) {
    pushBox(-208, 42, 265, 28, 42, 58, 65535);
    pushBox(208, 42, 265, 28, 42, 58, 65535);
  }
  if (wantRearWheels) {
    pushBox(-212, 46, -265, 32, 46, 62, 65535);
    pushBox(212, 46, -265, 32, 46, 62, 65535);
  }
  return {
    vertices,
    indices,
    materials: {},
    textures: []
  };
}

function createDemoState() {
  const carNames = [
    ['ERRISO', 'AGE', 'AGE'], ['VULCANO', 'AGE', 'AGE'], ['ALOUETTE', 'AGE', 'AGE'], ['PEGASO', 'AGE', 'AGE'],
    ['ABEILLE', 'GNADE', 'GNADE'], ['ESPERANZA', 'GNADE', 'GNADE'], ['STALLION', 'GNADE', 'GNADE'],
    ['GECKO', 'LIZARD', 'LIZARD'], ['ACCEL', 'LIZARD', 'LIZARD'], ['BAYONET', 'LIZARD', 'LIZARD'],
    ['HIJACK', 'LIZARD', 'LIZARD'], ['BULSHADE', 'LIZARD', 'LIZARD'], ['SQUALDON', 'ASSOLUTO', 'ASSOLUTE']
  ];
  const parts = [];
  for (let slot = 0; slot < 32; slot++) {
    const bank = 10 + slot * 2;
    const base = carNames[slot % carNames.length];
    const g = Math.min(5, Math.floor(slot / 6) + 1);
    const name = `${base[0]} G${g}`;
    for (let p = 0; p < 5; p++) {
      parts.push({key: `car.player.${bank}.part.${p}`, bank, part: p, rival: false, name});
    }
  }
  const specs = new Map();
  for (let slot = 0; slot < 32; slot++) {
    const bank = 10 + slot * 2;
    const base = carNames[slot % carNames.length];
    const g = Math.min(5, Math.floor(slot / 6) + 1);
    const fields = [
      {key: 'name', type: 'text', value: `${base[0]} G${g}`, baseValue: `${base[0]} G${g}`, maxLength: 31, origin: 'built-in profile'},
      {key: 'manufacturer', type: 'text', value: base[1], baseValue: base[1], maxLength: 31, origin: 'built-in profile'},
      {key: 'className', type: 'text', value: base[2], baseValue: base[2], maxLength: 31, origin: 'built-in profile'},
      {key: 'manualOnly', value: 0, baseValue: 0, min: 0, max: 1, origin: 'built-in profile'},
      {key: 'unlockClass', value: Math.min(5, Math.floor(slot / 6)), baseValue: Math.min(5, Math.floor(slot / 6)), min: 0, max: 5, origin: 'built-in profile'},
      {key: 'price', value: 5000 + slot * 12500, baseValue: 5000 + slot * 12500, min: 0, max: 2147483647, origin: 'built-in profile'},
      {key: 'upgradePrice', value: 12000 + slot * 18000, baseValue: 12000 + slot * 18000, min: 0, max: 2147483647, origin: 'built-in profile'},
      {key: 'revLimit', value: 8200 + (slot % 5) * 350, baseValue: 8200 + (slot % 5) * 350, min: 4000, max: 14000, origin: 'built-in profile'},
      {key: 'redline', value: 7200 + (slot % 5) * 300, baseValue: 7200 + (slot % 5) * 300, min: 3000, max: 13000, origin: 'built-in profile'},
      {key: 'topGear', value: g >= 3 ? 6 : 5, baseValue: g >= 3 ? 6 : 5, min: 4, max: 6, origin: 'built-in profile'},
      {key: 'automaticAccelerationScale', value: 990, baseValue: 990, min: 500, max: 1500, origin: 'built-in profile'},
      {key: 'baseSteeringGrip', value: 320 + slot * 4, baseValue: 320 + slot * 4, min: 10, max: 2000, origin: 'built-in profile'},
      {key: 'steeringGripResponse', value: 1080, baseValue: 1080, min: 100, max: 4000, origin: 'built-in profile'},
      {key: 'referenceTurnRadius', value: 850, baseValue: 850, min: 1, max: 5000, origin: 'built-in profile'},
      {key: 'speedDragDivisor', value: 2200 + slot * 40, baseValue: 2200 + slot * 40, min: 100, max: 10000, origin: 'built-in profile'},
      {key: 'steerResponse', value: 85, baseValue: 85, min: 10, max: 500, origin: 'built-in profile'},
      {key: 'negconSteeringAssistScale', value: 300, baseValue: 300, min: 0, max: 1000, origin: 'built-in profile'},
      {key: 'gearRatio0', value: 2818, baseValue: 2818, min: 100, max: 10000, origin: 'built-in profile'}
    ];
    for (let i = 1; i <= 6; i++) {
      fields.push({key: `downshift${i}`, value: i * 260, baseValue: i * 260, min: -500, max: 4000, origin: 'built-in profile'});
      fields.push({key: `upshift${i}`, value: i * 340 + 180, baseValue: i * 340 + 180, min: -500, max: 4000, origin: 'built-in profile'});
      fields.push({key: `gearRatio${i}`, value: 90 + i * 48, baseValue: 90 + i * 48, min: 20, max: 2000, origin: 'built-in profile'});
      fields.push({key: `torqueScale${i - 1}`, value: 100, baseValue: 100, min: 10, max: 300, origin: 'built-in profile'});
      fields.push({key: `gearLoad${i - 1}`, value: 500 + i * 250, baseValue: 500 + i * 250, min: 50, max: 12000, origin: 'built-in profile'});
    }
    const bandSamples = [0, 2800, 3600, 4200, 4800, 5400, 6000, 6500, 7000, 7400, 7800, 8200, 8600, 9000, 9500, 10000];
    const curveSamples = [46000, 58000, 74000, 92000, 108000, 122000, 134000, 138000, 135000, 126000, 112000, 92000, 64000, 32000, 0, 0];
    for (let i = 0; i < 16; i++) {
      fields.push({key: `torqueBand${i}`, value: bandSamples[i], baseValue: bandSamples[i], min: 0, max: 16000, origin: 'built-in profile'});
      fields.push({key: `torqueCurve${i}`, value: curveSamples[i] + slot * 1500, baseValue: curveSamples[i] + slot * 1500, min: 0, max: 500000, origin: 'built-in profile'});
      if (i < 10) fields.push({key: `torqueLossValue${i}`, value: 25, baseValue: 25, min: 0, max: 500, origin: 'built-in profile'});
      if (i < 9) fields.push({key: `torqueLossRpm${i}`, value: i * 1000, baseValue: i * 1000, min: 0, max: 16000, origin: 'built-in profile'});
    }
    specs.set(bank, fields);
  }

  // Build a 160-segment track scene for Track Studio
  const trackNames = ['Mythical Coast', 'Over Pass City', 'Lakeside Gate', 'The Extreme Oval'];
  const tracks = [];
  for (let grade = 1; grade <= 6; grade++) {
    for (let course = 0; course < 4; course++) {
      const bank = 88 + (grade - 1) * 8 + course * 2;
      tracks.push({
        id: `track/${['mythical-coast', 'over-pass-city', 'lakeside-gate', 'extreme-oval'][course]}/class/${grade}`,
        name: trackNames[course],
        grade,
        bank,
        origin: 'disc',
        available: course !== 3 || grade >= 3
      });
    }
  }

  const roadCount = 160;
  const roadPoints = [];
  for (let i = 0; i < roadCount; i++) {
    const t = (i / roadCount) * Math.PI * 2;
    const r = 14000 + Math.sin(t * 3) * 3600 + Math.cos(t * 2) * 2400;
    const x = Math.round(32768 + Math.cos(t) * r);
    const z = Math.round(32768 + Math.sin(t) * r * 0.78);
    const y = Math.round(600 + Math.sin(t * 2) * 520 + Math.cos(t * 4) * 180);
    const crossSlope = Math.round(Math.sin(t * 3) * 90);
    roadPoints.push({
      x, y, z,
      length: 540,
      heading: Math.round(((t + Math.PI / 2) / (Math.PI * 2)) * 4096) & 4095,
      pitch: 0,
      crossSlope,
      leftWidth: 160,
      rightWidth: 160,
      flags: 0,
      style: 0
    });
  }
  const roadArcs = Array.from({length: 8}, (_, i) => {
    const a = (i / 8) * Math.PI * 2;
    return {x: Math.round(32768 + Math.cos(a) * 11000), z: Math.round(32768 + Math.sin(a) * 8500)};
  });
  const cells = Array.from({length: 1024}, (_, idx) => {
    const r = idx >> 5, c = idx & 31;
    const dist = Math.hypot(c - 15.5, r - 15.5);
    const onRing = dist > 5 && dist < 14.5;
    return {model: onRing ? (idx % 20) : 1023, rotation: 0, flags: 0};
  });
  const objects = Array.from({length: 28}, (_, i) => {
    const pt = roadPoints[(i * 5) % roadPoints.length];
    return {
      model: i % 12,
      x: pt.x + (i % 2 === 0 ? 950 : -950),
      y: pt.y,
      z: pt.z + 300,
      yaw: (pt.heading + 1024) & 4095,
      flags: 0
    };
  });
  const makeAiHints = () => Array.from({length: 30}, (_, i) => ({
    start: i * 5,
    end: i * 5 + 4,
    min_offset: -40,
    max_offset: 40,
    adjustment: (i % 3 === 0 ? 12 : i % 3 === 1 ? -12 : 0)
  }));
  const makeAiSpeeds = () => Array.from({length: 48}, (_, i) => ({
    progress: Math.round((i / 47) * roadCount),
    pitch: 0,
    targets: [185, 200, 215, 230]
  }));
  const makeAiStarts = () => Array.from({length: 12}, (_, i) => {
    const ptIdx = (roadCount - 2 - i * 2 + roadCount) % roadCount;
    const pt = roadPoints[ptIdx];
    const side = (i % 2 === 0 ? -1 : 1) * 180;
    return {x: pt.x + side, z: pt.z, point: ptIdx, seed: i * 17};
  });
  const makeCameras = tableIdx => Array.from({length: 8}, (_, i) => ({
    id: tableIdx * 100 + i,
    mode: (i % 3) + 1,
    section: Math.round((i / 8) * roadCount),
    duration: 90,
    data: [0, 0, 0, 0],
    offset: [0, 320, -450]
  }));

  const makeIntro = sIdx => Array.from({length: 4}, (_, i) => {
    const pt = roadPoints[(i * 4) % roadCount] || {x: 0, y: 0, z: 0};
    return {
      id: sIdx * 10 + i,
      mode: i === 3 ? 1 : 0,
      x: pt.x + (i % 2 === 0 ? -400 : 400),
      y: (pt.y || 0) - 220,
      z: pt.z - 600,
      frame: i * 45,
      duration: 45
    };
  });

  const trackSceneObj = {
    revision: 0,
    courseModels: 16,
    terrainModels: 20,
    road: {points: roadPoints, arcs: roadArcs},
    terrain: {models: 20, cells},
    objects,
    environment: {
      length: 2400,
      skyRow: 0,
      cues: [
        {time: 0, duration: 600, mode: 0, colors: [0x94a3b8, 0x1e3a8a, 0x3b82f6, 0x93c5fd, 0xdbeafe, 0x334155, 0x1e293b, 0x475569, 0x0f172a]},
        {time: 800, duration: 600, mode: 1, colors: [0xfdba74, 0x7c2d12, 0xea580c, 0xfb923c, 0xfed7aa, 0x431407, 0x27272a, 0x52525b, 0x18181b]},
        {time: 1600, duration: 600, mode: 2, colors: [0x1e293b, 0x090d16, 0x1e1b4b, 0x312e81, 0x4338ca, 0x0f172a, 0x020617, 0x1e293b, 0x090d16]}
      ],
      palettes: Array.from({length: 5}, (_, p) => Array.from({length: 16}, (__, c) => ((p * 32 + c * 12) << 16) | ((100 + c * 8) << 8) | (180 + p * 12)))
    },
    ai: {
      hints: [makeAiHints(), makeAiHints()],
      speeds: [makeAiSpeeds(), makeAiSpeeds()],
      starts: [makeAiStarts(), makeAiStarts()]
    },
    cameras: [makeCameras(0), makeCameras(1), makeCameras(2)],
    intro: [makeIntro(0), makeIntro(1)]
  };

  return {parts, specs, tracks, trackSceneObj, revision: 0, changes: []};
}

const demo = createDemoState();

async function main() {
  const service = new LauncherService({
    root: userDataDir,
    bin: path.join(resourcesDir, 'bin'),
    config: path.join(resourcesDir, 'rage-port.ini')
  });
  await service.init().catch(() => {});

  // Serialize service invocations so concurrent browser requests never collide on LauncherService.busy
  let apiQueue = Promise.resolve();
  const enqueueServiceCall = fn => {
    const next = apiQueue.then(fn, fn);
    apiQueue = next.catch(() => {});
    return next;
  };

  let savePaths = new Map();
  let currentSave = null;

  const handleApi = (method, args) => enqueueServiceCall(async () => {
    if (method === 'saves') {
      const list = await service.saves();
      savePaths = new Map(list.map((s, i) => [String(i), s.path]));
      return list.map((s, i) => ({id: String(i), team: s.team, money: s.money}));
    }
    if (method === 'newSave') {
      const region = args[0] || 'PAL';
      const prefixes = {PAL: 'BESCES-00650', 'NTSC-U': 'BASLUS-00403', 'NTSC-J': 'BISLPS-00600'};
      const prefix = prefixes[region] || 'BESCES-00650';
      const savesDir = path.join(userDataDir, 'saves');
      await fs.mkdir(savesDir, {recursive: true});
      const file = path.join(savesDir, `${prefix} RAGE${String(Date.now()).slice(-3)}`);
      const data = await service.newSave(file, region);
      currentSave = {file};
      return {name: path.basename(file), ...data};
    }
    if (method === 'openSave') {
      const id = args[0];
      let file = id !== undefined ? savePaths.get(String(id)) : null;
      if (!file) {
        const list = await service.saves();
        savePaths = new Map(list.map((s, i) => [String(i), s.path]));
        if (id !== undefined) file = savePaths.get(String(id));
        else if (list.length) file = list[0].path;
      }
      if (!file) {
        const savesDir = path.join(userDataDir, 'saves');
        await fs.mkdir(savesDir, {recursive: true});
        file = path.join(savesDir, 'BESCES-00650 RAGE000');
        const created = await service.newSave(file, 'PAL');
        currentSave = {file};
        return {name: path.basename(file), ...created};
      }
      const data = await service.readSave(file);
      currentSave = {file};
      return {name: path.basename(file), ...data};
    }
    if (method === 'cardEntry') {
      const index = Number(args[0]);
      if (!currentSave) throw Error('Open a save or memory card first');
      const data = await service.readSave(currentSave.file, index);
      currentSave.cardIndex = index;
      return {name: path.basename(currentSave.file), ...data};
    }
    if (method === 'saveCopy') {
      const edits = args[0] || {};
      if (!currentSave) throw Error('Open a save first');
      const savesDir = path.join(userDataDir, 'saves');
      await fs.mkdir(savesDir, {recursive: true});
      const outPath = path.join(savesDir, path.basename(currentSave.file).replace(/(\.copy)?$/, '.copy'));
      const data = await service.writeSave(currentSave.file, outPath, currentSave.cardIndex, edits);
      currentSave = {file: outPath};
      return {name: path.basename(outPath), ...data};
    }

    const snap = await service.snapshot().catch(() => ({ready: false}));
    if (snap.ready) {
      if (method === 'trackPreview') {
        return await service.trackScene(args[0], true);
      }
      if (method === 'settings') {
        return await service.saveSettings(args[0]);
      }
      if (method === 'play') {
        return await service.launch(args[0]);
      }
      if (typeof service[method] === 'function') {
        return await service[method](...args);
      }
    }

    // Interactive demo mode when no disc image is mounted in userData
    switch (method) {
      case 'snapshot':
        return {
          ready: true,
          toolsReady: true,
          busy: null,
          running: false,
          disc: {region: 'NTSC-U (Browser Studio Preview)', path: 'Rage Racer Studio Live Preview', archive: 'demo'},
          mods: [],
          conflicts: [],
          settings: {},
          project: {name: 'Studio Project', revision: demo.revision, dirty: demo.changes.length > 0, canUndo: false, canRedo: false, changes: demo.changes}
        };
      case 'project':
        return {name: 'Studio Project', revision: demo.revision, dirty: demo.changes.length > 0, canUndo: false, canRedo: false, changes: demo.changes, selection: {packages: [], resolutions: {}}};
      case 'cars':
        return {parts: demo.parts};
      case 'carSpec': {
        const bank = Number(args[0]) || 10;
        return {id: `car_${(bank - 10) / 2}`, revision: demo.revision, fields: demo.specs.get(bank) || demo.specs.get(10)};
      }
      case 'saveCarSpec': {
        const [bank, edits] = args;
        const fields = demo.specs.get(Number(bank));
        if (fields && edits) {
          for (const [k, v] of Object.entries(edits)) {
            const f = fields.find(x => x.key === k);
            if (f) { f.value = v; f.origin = 'project'; }
            if (k === 'name') {
              for (const p of demo.parts) if (p.bank === Number(bank) && !p.rival) p.name = String(v);
            }
          }
        }
        demo.revision++;
        demo.changes.push({kind: 'car', id: String(bank), field: 'spec', value: 1});
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'createNewCar': {
        const [opts] = args;
        let targetBank = opts.targetBank === 'new' || opts.targetBank === undefined
          ? Math.max(198, ...demo.parts.filter(p => !p.rival && p.bank >= 200).map(p => p.bank)) + 2
          : (Number(opts.targetBank) || 10);
        const sourceBank = Number(opts.sourceBank) || 10;
        const srcFields = demo.specs.get(sourceBank) || demo.specs.get(10);
        const cloned = srcFields.map(f => ({...f}));
        for (const k of ['name', 'manufacturer', 'className', 'unlockClass', 'price', 'upgradePrice']) {
          if (opts[k] !== undefined) {
            const f = cloned.find(x => x.key === k);
            if (f) { f.value = opts[k]; f.origin = 'project'; }
          }
        }
        demo.specs.set(targetBank, cloned);
        const carName = String(opts.name || cloned.find(x => x.key === 'name')?.value || 'NEW CAR');
        if (targetBank >= 200 && !demo.parts.some(p => p.bank === targetBank && !p.rival)) {
          for (let p = 0; p < 5; p++) {
            demo.parts.push({key: `car.player.${targetBank}.part.${p}`, bank: targetBank, part: p, rival: false, customSlot: true, donorBank: sourceBank, name: carName});
          }
        } else if (opts.name) {
          for (const p of demo.parts) if (p.bank === targetBank && !p.rival) p.name = carName;
        }
        demo.revision++;
        demo.changes.push({kind: 'car', id: String(targetBank), field: 'new', value: 1});
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes, createdBank: targetBank};
      }
      case 'previewWholeCar':
        return buildDemoCarMesh(Number(args[0]) || 10, 'whole');
      case 'previewCar':
        return buildDemoCarMesh(10, String(args[0] || 'whole'));
      case 'tracks':
        return {tracks: demo.tracks};
      case 'trackScene':
        return {...demo.trackSceneObj, revision: demo.revision};
      case 'trackPreview':
        return {vertices: [], indices: [], colors: [], entities: [], materials: {}, textures: []};
      case 'previewTrackPart':
        return buildDemoCarMesh(10, 'whole');
      case 'saveRoadBatch': {
        const [, entries] = args;
        for (const e of entries || []) {
          if (e.type === 'arc' && demo.trackSceneObj.road.arcs[e.index]) {
            Object.assign(demo.trackSceneObj.road.arcs[e.index], e.values);
          } else if (demo.trackSceneObj.road.points[e.index]) {
            const pt = demo.trackSceneObj.road.points[e.index];
            for (const [k, v] of Object.entries(e.values || {})) {
              pt[{left_width: 'leftWidth', right_width: 'rightWidth', cross_slope: 'crossSlope'}[k] || k] = v;
            }
          }
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        demo.changes.push({kind: 'track', id: 'road', field: 'batch', value: 1});
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveRoadPoint': {
        const [, idx, values] = args;
        const pt = demo.trackSceneObj.road.points[idx];
        if (pt && values) {
          for (const [k, v] of Object.entries(values)) {
            pt[{left_width: 'leftWidth', right_width: 'rightWidth', cross_slope: 'crossSlope'}[k] || k] = v;
          }
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveTerrainCell': {
        const [, idx, values] = args;
        if (values && values.model !== undefined) {
          demo.trackSceneObj.terrain.cells[idx] = Number(values.model) & 1023;
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'addTrackObject': {
        const [, model, ptIdx] = args;
        const p = demo.trackSceneObj.road.points[ptIdx] || {x: 32768, y: 0, z: 32768};
        demo.trackSceneObj.objects.push({model: Number(model), x: p.x, y: p.y, z: p.z, yaw: 0, flags: 0});
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveTrackObject': {
        const [, idx, values] = args;
        if (demo.trackSceneObj.objects[idx] && values) {
          Object.assign(demo.trackSceneObj.objects[idx], values);
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'duplicateTrackObject': {
        const [, idx] = args;
        const orig = demo.trackSceneObj.objects[idx];
        if (orig) demo.trackSceneObj.objects.push({...orig, x: orig.x + 250, z: orig.z + 250});
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'removeTrackObject': {
        const [, idx] = args;
        if (demo.trackSceneObj.objects[idx]) demo.trackSceneObj.objects[idx].model = -1;
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'deleteCustomCar': {
        const [bank] = args;
        const b = Number(bank);
        if (b >= 200) {
          demo.parts = demo.parts.filter(p => p.bank !== b);
          demo.specs.delete(b);
          demo.changes = demo.changes.filter(c => !(c.kind === 'car' && (c.id === String(b) || c.id === `custom_car_${b}` || c.id.includes(`.${b}.`))));
          demo.revision++;
        }
        return {name: 'Studio Project', revision: demo.revision, dirty: true, applied: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'restoreOriginalCar': {
        const [bank, , rival] = args;
        const b = Number(bank) || 10;
        if (!rival && demo.specs.has(b)) {
          for (const f of demo.specs.get(b)) {
            f.value = f.baseValue;
            f.origin = 'built-in profile';
            if (f.key === 'name') {
              for (const p of demo.parts) if (p.bank === b && !p.rival) p.name = String(f.baseValue);
            }
          }
        }
        demo.changes = demo.changes.filter(c => !(c.kind === 'car' && (c.id === String(b) || c.id.includes(`.${b}.`))));
        demo.revision++;
        return {name: 'Studio Project', revision: demo.revision, dirty: false, applied: true, canUndo: false, canRedo: false, changes: demo.changes};
      }
      case 'restoreOriginalTrack': {
        const fresh = createDemoState();
        demo.trackSceneObj = fresh.trackSceneObj;
        demo.changes = demo.changes.filter(c => c.kind !== 'track');
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: false, applied: true, canUndo: false, canRedo: false, changes: demo.changes};
      }
      case 'saveAiHint': {
        const [, idx, values] = args;
        const series = Math.floor(Number(idx) / 30), itemIdx = Number(idx) % 30;
        if (demo.trackSceneObj.ai?.hints?.[series]?.[itemIdx]) {
          Object.assign(demo.trackSceneObj.ai.hints[series][itemIdx], values);
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveAiSpeed': {
        const [, idx, values] = args;
        const series = Math.floor(Number(idx) / 48), itemIdx = Number(idx) % 48;
        const row = demo.trackSceneObj.ai?.speeds?.[series]?.[itemIdx];
        if (row) {
          if (values.progress !== undefined) row.progress = values.progress;
          if (values.pitch !== undefined) row.pitch = values.pitch;
          for (let t = 0; t < 4; t++) if (values[`target_${t}`] !== undefined && Array.isArray(row.targets)) row.targets[t] = values[`target_${t}`];
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveStart':
      case 'saveStartSlot': {
        const [, idx, values] = args;
        const series = Math.floor(Number(idx) / 12), itemIdx = Number(idx) % 12;
        if (demo.trackSceneObj.ai?.starts?.[series]?.[itemIdx]) {
          Object.assign(demo.trackSceneObj.ai.starts[series][itemIdx], values);
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveIntro': {
        const [, id, values] = args;
        for (const list of demo.trackSceneObj.intro || []) {
          const item = (list || []).find(k => k.id === Number(id));
          if (item) Object.assign(item, values);
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveEnvironment': {
        const [, entries] = args;
        const env = demo.trackSceneObj.environment;
        if (env) {
          for (const e of entries || []) {
            if (e.type === 'environment') {
              if (e.values.length !== undefined) env.length = e.values.length;
              if (e.values.sky_row !== undefined) env.skyRow = e.values.sky_row;
            } else if (e.type === 'palette') {
              const pal = env.palettes?.[e.index];
              if (pal) {
                for (let c = 0; c < 16; c++) if (e.values[`color_${c}`] !== undefined) pal[c] = e.values[`color_${c}`];
              }
            } else {
              const cue = env.cues?.[e.index];
              if (cue) {
                if (e.values.time !== undefined) cue.time = e.values.time;
                if (e.values.duration !== undefined) cue.duration = e.values.duration;
                if (e.values.mode !== undefined) cue.mode = e.values.mode;
                for (let c = 0; c < 9; c++) if (e.values[`color_${c}`] !== undefined && Array.isArray(cue.colors)) cue.colors[c] = e.values[`color_${c}`];
              }
            }
          }
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveCamera': {
        const [, id, values] = args;
        for (const table of demo.trackSceneObj.cameras || []) {
          const cam = table.find(c => c.id === Number(id));
          if (cam) {
            if (values.mode !== undefined) cam.mode = values.mode;
            if (values.section !== undefined) cam.section = values.section;
            if (values.duration !== undefined) cam.duration = values.duration;
            for (let i = 0; i < 4; i++) if (values[`data_${i}`] !== undefined && Array.isArray(cam.data)) cam.data[i] = values[`data_${i}`];
            for (let i = 0; i < 3; i++) if (values[`offset_${i}`] !== undefined && Array.isArray(cam.offset)) cam.offset[i] = values[`offset_${i}`];
          }
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveTrackBatch': {
        const [, entries] = args;
        for (const e of entries || []) {
          if (e.type === 'cell' && demo.trackSceneObj.terrain.cells[e.index]) {
            Object.assign(demo.trackSceneObj.terrain.cells[e.index], e.values);
          } else if (e.type === 'ai' || e.type === 'ai-hint') {
            const s = Math.floor(e.index / 30), i = e.index % 30;
            if (demo.trackSceneObj.ai?.hints?.[s]?.[i]) Object.assign(demo.trackSceneObj.ai.hints[s][i], e.values);
          } else if (e.type === 'start') {
            const s = Math.floor(e.index / 12), i = e.index % 12;
            if (demo.trackSceneObj.ai?.starts?.[s]?.[i]) Object.assign(demo.trackSceneObj.ai.starts[s][i], e.values);
          } else if (e.type === 'intro') {
            for (const list of demo.trackSceneObj.intro || []) {
              const item = (list || []).find(k => k.id === Number(e.index));
              if (item) Object.assign(item, e.values);
            }
          }
        }
        demo.revision++;
        demo.trackSceneObj.revision = demo.revision;
        return {name: 'Studio Project', revision: demo.revision, dirty: true, canUndo: true, canRedo: false, changes: demo.changes};
      }
      case 'saveProject':
      case 'applyProject':
        return {name: 'Studio Project', revision: demo.revision, dirty: false, applied: true, canUndo: false, canRedo: false, changes: demo.changes};
      case 'cancel':
        return null;
      default:
        return null;
    }
  });

  const server = http.createServer(async (req, res) => {
    try {
      const url = new URL(req.url, `http://127.0.0.1:${PORT}`);
      if (req.method === 'POST' && url.pathname.startsWith('/api/')) {
        const method = decodeURIComponent(url.pathname.slice(5));
        if (method === 'cancel') {
          await service.cancel().catch(() => {});
          res.writeHead(200, {'Content-Type': 'application/json; charset=utf-8'});
          res.end(JSON.stringify({ok: true, value: null}));
          return;
        }
        const chunks = [];
        for await (const chunk of req) chunks.push(chunk);
        const body = chunks.length ? JSON.parse(Buffer.concat(chunks).toString('utf8')) : {};
        try {
          const value = await handleApi(method, Array.isArray(body.args) ? body.args : []);
          res.writeHead(200, {'Content-Type': 'application/json; charset=utf-8'});
          res.end(JSON.stringify({ok: true, value}));
        } catch (err) {
          res.writeHead(200, {'Content-Type': 'application/json; charset=utf-8'});
          res.end(JSON.stringify({ok: false, error: err.message || String(err)}));
        }
        return;
      }

      const rel = url.pathname === '/' ? '/car-studio.html' : url.pathname;
      const safePath = path.normalize(path.join(rendererDir, rel));
      if (!safePath.startsWith(rendererDir)) {
        res.writeHead(403);
        res.end('Forbidden');
        return;
      }
      const bytes = await fs.readFile(safePath);
      const ext = path.extname(safePath);
      res.writeHead(200, {'Content-Type': MIME[ext] || 'application/octet-stream', 'Cache-Control': 'no-store'});
      res.end(bytes);
    } catch (e) {
      res.writeHead(404);
      res.end('Not found');
    }
  });

  server.listen(PORT, '127.0.0.1', () => {
    console.log(`Rage Studio Browser Server listening on:`);
    console.log(`  Car Studio:   http://127.0.0.1:${PORT}/car-studio.html`);
    console.log(`  Track Studio: http://127.0.0.1:${PORT}/track-studio.html`);
  });
}

main();
