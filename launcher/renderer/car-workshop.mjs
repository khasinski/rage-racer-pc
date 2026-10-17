import {displayedShiftSpeed} from './car-speed.mjs';

const escape = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

export const workshopTabs = {
  appearance: '3D Model & Parts',
  driving: 'Metadata & Physics',
  files: 'Mod Overrides'
};

export const carMetaTabs = {
  identity: 'Identity & Shop',
  drivetrain: 'Engine & Gearbox',
  handling: 'Handling & Grip',
  curves: 'Torque Curve'
};

export function carImportForm(data) {
  const select = (items, value, key) =>
    '<option value="">Choose…</option>' +
    items.map(item => '<option value="' + item[key] + '" ' + (item[key] === value ? 'selected' : '') + '>' + escape(item.label) + '</option>').join('');
  return (
    '<h2>Assign car parts & materials</h2>' +
    '<p>Import into <strong>' + escape(data.name || 'selected car') + '</strong> (' + escape(data.region || 'current source') + ').</p>' +
    '<p class="muted">Map each mesh node in your file to its car slot and material.</p>' +
    '<form id="car-import-form" data-token="' + escape(data.token) + '">' +
    '<h3>Mesh nodes (Parts)</h3>' +
    data.nodes.map(node =>
      '<div class="setting">' +
        '<label for="car-node-' + node.node + '">' + escape(node.name) + '</label>' +
        '<select required id="car-node-' + node.node + '" data-import-node="' + node.node + '">' + select(data.roles, node.instance, 'instance') + '</select>' +
        '<button type="button" data-preview-import="' + node.node + '">View part</button>' +
      '</div>'
    ).join('') +
    '<section data-import-preview aria-label="Imported part preview"></section>' +
    '<h3>Materials & textures</h3>' +
    data.materials.map(material =>
      '<div class="setting">' +
        '<label for="car-map-' + material.material + '">' + escape(material.name) + '</label>' +
        '<select required id="car-map-' + material.material + '" data-import-material="' + material.material + '">' + select(data.choices, material.word, 'word') + '</select>' +
        '<button type="button" data-target-material="' + material.material + '">View target texture</button>' +
        '<span data-target-swatch="' + material.material + '" hidden></span>' +
      '</div>'
    ).join('') +
    '<footer><button type="button" data-action="close">Cancel</button><button type="submit" class="primary">Import mapped car</button></footer>' +
    '</form>'
  );
}

export function carPartName(car, part) {
  const offset = part - car.part;
  return ['Body', 'Body at a distance', 'Front wheels', 'Rear wheels', 'Wheels at a distance'][offset] || 'Extra part';
}

function renderTorqueChart(fields, revLimit, redline) {
  const band = Array.from({length: 16}, (_, i) => Number(fields.find(f => f.key === 'torqueBand' + i)?.value ?? i * 500));
  const curve = Array.from({length: 16}, (_, i) => Number(fields.find(f => f.key === 'torqueCurve' + i)?.value ?? 0));
  const maxRpm = Math.max(...band, Number(revLimit || 9000), 1000);
  const maxTorque = Math.max(...curve, 100);
  const w = 520, h = 150, padL = 38, padR = 14, padT = 14, padB = 24;
  const plotW = w - padL - padR, plotH = h - padT - padB;
  const pts = band.map((rpm, i) => {
    const x = padL + (Math.max(0, rpm) / maxRpm) * plotW;
    const y = padT + plotH - (Math.max(0, curve[i]) / maxTorque) * plotH;
    return `${x.toFixed(1)},${y.toFixed(1)}`;
  });
  const areaPts = `${padL},${padT + plotH} ${pts.join(' ')} ${pts.at(-1)?.split(',')[0] || padL},${padT + plotH}`;
  const redX = redline ? padL + (Math.min(maxRpm, Number(redline)) / maxRpm) * plotW : null;
  return `
    <div class="torque-chart-card">
      <div class="row between"><small class="eyebrow">Engine Torque Curve (RPM vs Torque)</small><small class="muted">Peak: ${Math.max(...curve)} · Max RPM: ${maxRpm}</small></div>
      <svg class="torque-chart" viewBox="0 0 ${w} ${h}" role="img" aria-label="Engine torque curve">
        <line x1="${padL}" y1="${padT + plotH}" x2="${w - padR}" y2="${padT + plotH}" stroke="#273244" stroke-width="1"/>
        <line x1="${padL}" y1="${padT}" x2="${padL}" y2="${padT + plotH}" stroke="#273244" stroke-width="1"/>
        ${redX ? `<line x1="${redX.toFixed(1)}" y1="${padT}" x2="${redX.toFixed(1)}" y2="${padT + plotH}" stroke="#e05d5d" stroke-dasharray="3,3" stroke-width="1.5"/>` : ''}
        <polygon points="${areaPts}" fill="rgba(232,168,56,0.16)"/>
        <polyline points="${pts.join(' ')}" fill="none" stroke="#e8a838" stroke-width="2.2"/>
        ${band.map((rpm, i) => {
          const [x, y] = pts[i].split(',');
          return `<circle cx="${x}" cy="${y}" r="3" fill="#ffcf70"><title>${rpm} RPM: ${curve[i]}</title></circle>`;
        }).join('')}
        <text x="${padL}" y="${h - 6}" fill="#8a96a8" font-size="10">0 RPM</text>
        <text x="${w - padR}" y="${h - 6}" fill="#8a96a8" font-size="10" text-anchor="end">${maxRpm} RPM</text>
        <text x="${padL - 4}" y="${padT + 8}" fill="#8a96a8" font-size="10" text-anchor="end">${maxTorque}</text>
      </svg>
    </div>`;
}

export function renderInlineCarSpec(bank, data, activeMetaTab = 'identity') {
  if (!data?.fields?.length) {
    return `<div class="card"><p class="muted">Loading car metadata…</p></div>`;
  }
  const metaTab = Object.hasOwn(carMetaTabs, activeMetaTab) ? activeMetaTab : 'identity';
  const handling = {
    baseSteeringGrip: ['Base steering grip', 'Primary cornering grip budget'],
    steeringGripResponse: ['Steering response', 'Drivetrain grip response scale'],
    referenceTurnRadius: ['Turn radius ref', 'Baseline curve radius for grip scaling'],
    speedDragDivisor: ['Aero drag divisor', 'Higher values reduce top-speed drag'],
    steerResponse: ['AI steer divisor', 'AI heading correction divisor'],
    negconSteeringAssistScale: ['neGcon assist', 'Steering assist scale (‰)']
  };
  const labels = {
    ...handling,
    name: ['Car name', 'Displayed in menus (max 31 chars)'],
    manufacturer: ['Manufacturer', 'Brand name & shop category'],
    className: ['Class label', 'Grade badge text shown in garage'],
    price: ['Purchase price', 'Cost in credits'],
    upgradePrice: ['Upgrade price', 'Next stage upgrade cost'],
    unlockClass: ['Unlock level', '0 = unlocked from start'],
    revLimit: ['Rev limit (RPM)', 'Engine RPM ceiling'],
    redline: ['Redline (RPM)', 'Tachometer warning RPM'],
    topGear: ['Forward gears', 'Number of forward gears (4–6)'],
    automaticAccelerationScale: ['Auto accel scale (‰)', '1000 = 100% acceleration']
  };
  const originTag = f => f?.origin && f.origin !== 'disc' ? `<span class="origin-badge" data-origin="${escape(f.origin)}">${escape(f.origin)}</span>` : '';
  const resetBtn = f => {
    if (!f || f.baseValue === undefined || f.value === f.baseValue) return '';
    const speed = /^(downshift|upshift)/.test(f.key);
    const base = f.type === 'text' ? f.baseValue : (speed ? displayedShiftSpeed(f.baseValue) : f.baseValue);
    return `<button type="button" class="field-reset" data-reset-field="${f.key}" data-base-value="${escape(base)}" title="Reset to ${escape(base)}">↺</button>`;
  };
  const input = f => {
    if (!f) return '';
    if (f.type === 'text') {
      return `<div class="field-control"><input id="spec-${f.key}" aria-label="${f.key}" data-spec="${f.key}" data-text data-original="${escape(f.value)}" type="text" maxlength="${f.maxLength}" value="${escape(f.value)}" required>${resetBtn(f)}</div>`;
    }
    const speed = /^(downshift|upshift)/.test(f.key);
    const value = speed ? displayedShiftSpeed(f.value) : f.value;
    const min = speed ? displayedShiftSpeed(f.min) : f.min;
    const max = speed ? displayedShiftSpeed(f.max) : f.max;
    return `<div class="field-control"><input id="spec-${f.key}" aria-label="${f.key}" data-spec="${f.key}" data-original="${value}" ${speed ? 'data-speed="true" ' : ''}type="number" min="${min}" max="${max}" step="${speed ? '0.01' : '1'}" value="${value}" required>${resetBtn(f)}</div>`;
  };
  const fieldRow = key => {
    const f = data.fields.find(x => x.key === key);
    if (!f || !labels[key]) return '';
    return `<div class="compact-field"><label for="spec-${f.key}"><span>${labels[key][0]} ${originTag(f)}</span><small>${labels[key][1]}</small></label>${input(f)}</div>`;
  };
  const manual = data.fields.find(f => f.key === 'manualOnly');
  const revLimit = data.fields.find(f => f.key === 'revLimit')?.value;
  const redline = data.fields.find(f => f.key === 'redline')?.value;
  const tabNav = `<nav class="tabs meta-subtabs" aria-label="Car metadata sections">${Object.entries(carMetaTabs).map(([id, label]) => `<button type="button" data-car-meta-tab="${id}" class="${metaTab === id ? 'active' : ''}">${label}</button>`).join('')}</nav>`;
  const hidePane = id => metaTab === id ? '' : ' hidden';

  return `
    <form id="car-spec-form" class="car-inline-spec" data-bank="${bank}" data-revision="${data.revision}">
      ${tabNav}
      <div data-car-meta-pane="identity"${hidePane('identity')}>
        <div class="compact-field-grid">
          ${['name', 'manufacturer', 'className', 'unlockClass', 'price', 'upgradePrice'].map(fieldRow).join('')}
        </div>
      </div>
      <div data-car-meta-pane="drivetrain"${hidePane('drivetrain')}>
        <div class="compact-field-grid">
          ${manual ? `<div class="compact-field"><label for="spec-manualOnly"><span>Transmission ${originTag(manual)}</span><small>AT + MT or manual only</small></label><div class="field-control"><select id="spec-manualOnly" data-spec="manualOnly" data-original="${manual.value}"><option value="0" ${!manual.value ? 'selected' : ''}>AT and MT</option><option value="1" ${manual.value ? 'selected' : ''}>MT only</option></select>${resetBtn(manual)}</div></div>` : ''}
          ${['topGear', 'revLimit', 'redline', 'automaticAccelerationScale'].map(fieldRow).join('')}
        </div>
        <h4 class="sub-eyebrow">Gearbox &amp; Shift Thresholds (km/h)</h4>
        <div class="table-scroll compact-table-wrap">
          <table class="asset-table compact-table">
            <thead><tr><th>Gear</th><th>Downshift</th><th>Upshift</th><th>Ratio</th><th>Torque</th><th>Load</th></tr></thead>
            <tbody>
              ${Array.from({length: 6}, (_, i) => `
                <tr>
                  <th>G${i + 1}</th>
                  <td>${input(data.fields.find(f => f.key === 'downshift' + (i + 1)))}</td>
                  <td>${input(data.fields.find(f => f.key === 'upshift' + (i + 1)))}</td>
                  <td>${input(data.fields.find(f => f.key === 'gearRatio' + (i + 1)))}</td>
                  <td>${input(data.fields.find(f => f.key === 'torqueScale' + i))}</td>
                  <td>${input(data.fields.find(f => f.key === 'gearLoad' + i))}</td>
                </tr>`).join('')}
            </tbody>
          </table>
        </div>
        <details class="car-advanced"><summary>Neutral gear ratio (index 0)</summary>${input(data.fields.find(f => f.key === 'gearRatio0'))}</details>
      </div>
      <div data-car-meta-pane="handling"${hidePane('handling')}>
        <div class="compact-field-grid">
          ${Object.keys(handling).map(fieldRow).join('')}
        </div>
      </div>
      <div data-car-meta-pane="curves"${hidePane('curves')}>
        ${renderTorqueChart(data.fields, revLimit, redline)}
        <div class="table-scroll compact-table-wrap" style="max-height:240px">
          <table class="asset-table compact-table">
            <thead><tr><th>#</th><th>RPM Band</th><th>Torque</th><th>Loss RPM</th><th>Loss Value</th></tr></thead>
            <tbody>
              ${Array.from({length: 16}, (_, i) => `
                <tr>
                  <th>${i}</th>
                  <td>${input(data.fields.find(f => f.key === 'torqueBand' + i))}</td>
                  <td>${input(data.fields.find(f => f.key === 'torqueCurve' + i))}</td>
                  <td>${input(data.fields.find(f => f.key === 'torqueLossRpm' + i)) || '<span class="muted">—</span>'}</td>
                  <td>${input(data.fields.find(f => f.key === 'torqueLossValue' + i)) || '<span class="muted">—</span>'}</td>
                </tr>`).join('')}
            </tbody>
          </table>
        </div>
      </div>
      <div class="sticky-actions">
        <button type="button" data-car-spec="${bank}">Full table view</button>
        <div class="row wrap">
          <button type="submit" data-project-only>Save metadata</button>
          <button class="primary" type="submit">Save &amp; apply</button>
        </div>
      </div>
    </form>`;
}

export function carWorkspace(car, parts, {tab = 'appearance', changes = '', spec = null, metaTab = 'identity', inlinePart = 'whole'} = {}) {
  if (!Object.hasOwn(workshopTabs, tab)) tab = 'appearance';
  const modelIoButtons = tab === 'appearance'
    ? '<button class="primary" data-export-set="' + car.key + '">⬇ Export Car (.glb / .obj)</button>' +
      '<button class="primary" data-import-set="' + car.key + '" data-format="glb">⬆ Import Car (.glb / .gltf)</button>' +
      '<button data-import-set="' + car.key + '">Import OBJ Folder</button>'
    : '';
  const topToolbar =
    '<div class="row wrap">' +
      modelIoButtons +
      '<button data-car-materials="' + escape(car.key) + '">🎨 Surfaces &amp; Textures</button>' +
      '<button data-whole-car="' + car.bank + '" data-whole-body="' + car.part + '" data-whole-rival="' + car.rival + '">⤢ 3D Popup</button>' +
    '</div>';

  const partsTree = tab === 'appearance'
    ? '<div class="car-parts-panel">' +
        '<div class="row between"><span class="eyebrow">Multi-Part Model Hierarchy (5 Slots)</span><small class="muted">Click a slot to inspect in 3D · Import/Export individual meshes</small></div>' +
        '<div class="part-hierarchy-list">' +
          '<div class="part-node-row ' + (inlinePart === 'whole' ? 'active' : '') + '">' +
            '<button type="button" class="part-node-select ' + (inlinePart === 'whole' ? 'active' : '') + '" data-inline-part="whole">● Complete Assembled Car (Body + Wheels)</button>' +
            '<div class="row wrap">' +
              '<button type="button" data-export-set="' + car.key + '">Export All</button>' +
              '<button type="button" data-import-set="' + car.key + '" data-format="glb">Import All (.glb)</button>' +
            '</div>' +
          '</div>' +
          parts.map(p => {
            const isCurrent = inlinePart === p.key;
            return (
              '<div class="part-node-row ' + (isCurrent ? 'active' : '') + '">' +
                '<button type="button" class="part-node-select ' + (isCurrent ? 'active' : '') + '" data-inline-part="' + escape(p.key) + '">├─ ' + carPartName(car, p.part) + '</button>' +
                '<div class="row wrap part-node-actions">' +
                  '<button type="button" data-import-car="' + escape(p.key) + '">Import</button>' +
                  '<button type="button" data-export-car="' + escape(p.key) + '" data-format="glb">GLB</button>' +
                  '<button type="button" data-export-car="' + escape(p.key) + '">OBJ</button>' +
                  '<button type="button" data-car-materials="' + escape(p.key) + '">Materials</button>' +
                  '<button type="button" data-preview-car="' + escape(p.key) + '">View</button>' +
                '</div>' +
              '</div>'
            );
          }).join('') +
        '</div>' +
        '<details class="car-advanced" style="margin-top:8px"><summary>OBJ sets and individual parts</summary><button data-import-set="' + car.key + '">Import OBJ car set folder</button></details>' +
      '</div>'
    : '<div class="part-strip row wrap" aria-label="Quick part inspection"><button type="button" data-inline-part="whole" class="active">Complete car</button>' +
      parts.map(p => '<button type="button" data-inline-part="' + escape(p.key) + '">' + carPartName(car, p.part) + '</button>').join('') +
      '</div>';

  const stage =
    '<div class="car-stage">' +
      '<div id="car-inline-preview" class="inline-stage" data-inline-car="' + escape(car.key) + '" data-inline-bank="' + car.bank + '" data-inline-body="' + car.part + '" data-inline-rival="' + car.rival + '" aria-label="Interactive 3D car preview"></div>' +
      partsTree +
    '</div>';

  let inspectorContent = '';
  if (tab === 'appearance') {
    inspectorContent = car.rival
      ? '<div class="card"><div class="eyebrow">Rival Car</div><h3>Rival 3D Model</h3><p class="muted">Export or import the complete multi-part car (.glb / .obj) or replace individual body and wheel meshes on the left.</p></div>'
      : (spec
          ? renderInlineCarSpec(car.bank, spec, metaTab)
          : '<div class="card"><h3>Multi-Part Model &amp; Surfaces</h3><p class="muted">Export the complete car (.glb / .obj) for Blender, or import a multi-element replacement model.</p></div>');
  } else if (tab === 'driving') {
    inspectorContent = car.rival
      ? '<h3>Rival driving parameters</h3><p class="muted">This entry identifies a rival model, not a separate player specification. Editing its geometry does not change AI difficulty. Choose a player car to edit a player transmission.</p>'
      : (spec
          ? renderInlineCarSpec(car.bank, spec, metaTab)
          : '<h3>Engine & transmission</h3><p class="muted">Choose automatic/manual availability and the number of gears.</p><button data-car-spec="' + car.bank + '">Edit driving parameters</button>');
  } else if (tab === 'files') {
    inspectorContent =
      '<h3>Changes for this car</h3>' +
      changes +
      '<div class="sticky-actions"><button class="primary" data-action="publish-mods">Save enabled mods for game</button></div>' +
      '<details class="car-advanced"><summary>Technical details</summary><dl><dt>Game asset</dt><dd>' + escape(car.key) + '</dd><dt>Model bank</dt><dd>' + car.bank + '</dd><dt>Body part</dt><dd>' + car.part + '</dd></dl></details>';
  }

  return (
    '<section class="card car-project">' +
      '<div class="row between wrap">' +
        '<div><small class="eyebrow">' + (car.rival ? 'RIVAL CAR' : 'PLAYER CAR · MULTI-PART STUDIO') + '</small><h2>' + escape(car.name) + '</h2></div>' +
        topToolbar +
      '</div>' +
      '<div class="car-studio-grid">' +
        stage +
        '<div class="car-inspector">' +
          '<nav class="tabs workshop-tabs" aria-label="Car editor sections">' +
            Object.entries(workshopTabs).map(([id, label]) =>
              '<button data-workshop-tab="' + id + '" aria-current="' + (id === tab ? 'page' : 'false') + '" class="' + (id === tab ? 'active' : '') + '">' + label + '</button>'
            ).join('') +
          '</nav>' +
          '<section aria-label="' + workshopTabs[tab] + '">' + inspectorContent + '</section>' +
        '</div>' +
      '</div>' +
    '</section>'
  );
}
