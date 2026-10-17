const test = require('node:test');
const assert = require('node:assert/strict');
const {createMcpServer} = require('../scripts/mcp-server.cjs');

test('MCP server exposes Car Studio and Track Studio tools and executes agent workflows', async () => {
  let revision = 1;
  let dirty = false;
  const parts = [
    {key: 'car.player.10.part.0', bank: 10, part: 0, rival: false, name: 'ERRISO G1'},
    {key: 'car.player.10.part.1', bank: 10, part: 1, rival: false, name: 'ERRISO G1'}
  ];
  const specs = new Map([
    [
      10,
      [
        {key: 'name', value: 'ERRISO G1', baseValue: 'ERRISO G1', origin: 'disc'},
        {key: 'revLimit', value: 8500, baseValue: 8500, min: 4000, max: 14000, origin: 'disc'}
      ]
    ]
  ]);
  const trackScene = {
    revision: 1,
    courseModels: 6,
    terrain: {
      models: 12,
      cells: Array.from({length: 1024}, (_, i) => (i === 0 ? 3 : 1023))
    },
    road: {
      points: Array.from({length: 16}, (_, i) => ({
        x: i * 1000,
        y: -100,
        z: i * 500,
        angle: 0,
        pitch: 0,
        crossSlope: 0,
        leftWidth: 160,
        rightWidth: 160,
        length: 64,
        arc: 0
      })),
      arcs: []
    },
    objects: [{model: 2, x: 1000, y: -100, z: 500, yaw: 0, flags: 0}]
  };
  let lastPlayPayload = null;

  const mockCallApi = async (method, ...args) => {
    switch (method) {
      case 'snapshot':
        return {
          ready: true,
          toolsReady: true,
          running: false,
          disc: {path: '/mock/rage.cue', region: 'SLUS-00403'},
          project: {name: 'Test Project', revision, dirty, canUndo: revision > 1, canRedo: false, changes: []}
        };
      case 'project':
        return {name: 'Test Project', revision, dirty, canUndo: revision > 1, canRedo: false, changes: []};
      case 'cars':
        return {parts};
      case 'carSpec':
        return {revision, fields: specs.get(args[0]) || specs.get(10)};
      case 'saveCarSpec': {
        const [bank, values] = args;
        const list = specs.get(bank) || specs.get(10).map(f => ({...f}));
        for (const [k, v] of Object.entries(values)) {
          const item = list.find(f => f.key === k);
          if (item) item.value = v;
        }
        specs.set(bank, list);
        revision++;
        dirty = true;
        return {revision, dirty};
      }
      case 'createNewCar': {
        const [opts] = args;
        const createdBank = opts.targetBank === 'new' ? 200 : Number(opts.targetBank);
        parts.push({key: `car.player.${createdBank}.part.0`, bank: createdBank, part: 0, rival: false, customSlot: createdBank >= 200, donorBank: opts.sourceBank, name: opts.name});
        specs.set(createdBank, [{key: 'name', value: opts.name, baseValue: 'ERRISO G1', origin: 'project'}]);
        revision++;
        dirty = true;
        return {revision, dirty, createdBank};
      }
      case 'tracks':
        return {tracks: [{bank: 88, name: 'Mythical Coast', grade: 1, available: true}]};
      case 'trackScene':
        return {...trackScene, revision};
      case 'saveRoadBatch': {
        const [, entries] = args;
        for (const e of entries) {
          const pt = trackScene.road.points[e.index];
          if (pt) {
            for (const [k, v] of Object.entries(e.values)) {
              pt[{left_width: 'leftWidth', right_width: 'rightWidth', cross_slope: 'crossSlope'}[k] || k] = v;
            }
          }
        }
        revision++;
        dirty = true;
        return {revision, dirty};
      }
      case 'saveTerrainCell': {
        const [, cellIdx, values] = args;
        trackScene.terrain.cells[cellIdx] = Number(values.model) & 1023;
        revision++;
        dirty = true;
        return {revision, dirty};
      }
      case 'addTrackObject': {
        const [, model, ptIdx] = args;
        const p = trackScene.road.points[ptIdx] || {x: 0, y: 0, z: 0};
        trackScene.objects.push({model, x: p.x, y: p.y, z: p.z, yaw: 0, flags: 0});
        revision++;
        dirty = true;
        return {revision, dirty};
      }
      case 'saveTrackObject': {
        const [, idx, patch] = args;
        Object.assign(trackScene.objects[idx], patch);
        revision++;
        dirty = true;
        return {revision, dirty};
      }
      case 'applyProject':
        dirty = false;
        return {revision, dirty};
      case 'play':
        lastPlayPayload = args[0];
        return {running: true};
      default:
        throw new Error(`Unhandled mock API: ${method}`);
    }
  };

  const server = createMcpServer({callApi: mockCallApi});

  // 1. Initialize & list tools
  const initRes = await server.handleRpc({jsonrpc: '2.0', id: 1, method: 'initialize', params: {}});
  assert.equal(initRes.result.serverInfo.name, 'rage-studio-mcp');

  const listRes = await server.handleRpc({jsonrpc: '2.0', id: 2, method: 'tools/list'});
  const toolNames = listRes.result.tools.map(t => t.name);
  assert.ok(toolNames.includes('car_create'));
  assert.ok(toolNames.includes('track_deform_road'));
  assert.ok(toolNames.includes('track_set_terrain_unit'));
  assert.ok(toolNames.includes('track_manage_decorations'));
  assert.ok(toolNames.includes('launch_race'));

  // 2. Create a new car in expandable Slot #32 (Bank 200)
  const createCarRes = await server.executeTool('car_create', {
    targetBank: 'new',
    sourceBank: 10,
    name: 'AGENT GT',
    manufacturer: 'AGE',
    className: 'PROTO'
  });
  assert.equal(createCarRes.createdBank, 200);
  assert.equal(createCarRes.slot, 32);

  // 3. Deform road route with positive heightDelta (raises road UP -> decreases GTE Y)
  const beforeY = trackScene.road.points[4].y;
  const deformRes = await server.executeTool('track_deform_road', {
    bank: 88,
    pointIndex: 4,
    radius: 2,
    dx: 120,
    heightDelta: 50
  });
  assert.ok(deformRes.modifiedSegmentsCount > 0);
  assert.equal(trackScene.road.points[4].y, beforeY - 50);

  // 4. Swap a 32x32 terrain unit
  const terrainRes = await server.executeTool('track_set_terrain_unit', {
    bank: 88,
    col: 5,
    row: 2,
    model: 7
  });
  assert.equal(terrainRes.updatedTerrainUnits[0].cell, 2 * 32 + 5);
  assert.equal(trackScene.terrain.cells[2 * 32 + 5], 7);

  // 5. Place & rotate a decoration
  await server.executeTool('track_manage_decorations', {
    bank: 88,
    action: 'add',
    model: 4,
    roadPointIndex: 3,
    yawDeg: 90
  });
  assert.equal(trackScene.objects.length, 2);
  assert.equal(trackScene.objects[1].model, 4);
  assert.equal(trackScene.objects[1].yaw, 1024);

  // 6. Launch direct race
  const raceRes = await server.executeTool('launch_race', {carBank: 200, trackBank: 88, trackPoint: 4});
  assert.equal(raceRes.launched, true);
  assert.deepEqual(lastPlayPayload, {carBank: 200, trackBank: 88, mode: 'time-attack', trackPoint: 4});
});
