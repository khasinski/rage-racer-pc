const escape=value=>String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

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

export function trackWorkspace(tracks,bank,scene,selected=0,objectIndex=0) {
  const track=tracks.find(t=>t.bank===bank);
  const points=scene?.road?.points||[];
  let map='<p class="muted">Choose a track to inspect its original road.</p>';
  if(points.length) {
    const samples=roadPolyline(scene.road);
    let minX=Infinity,maxX=-Infinity,minZ=Infinity,maxZ=-Infinity;
    for(const p of samples){minX=Math.min(minX,p.x);maxX=Math.max(maxX,p.x);minZ=Math.min(minZ,p.z);maxZ=Math.max(maxZ,p.z);}
    const scale=640/Math.max(1,maxX-minX,maxZ-minZ);
    const position=p=>[(p.x-minX)*scale+30,(p.z-minZ)*scale+30];
    const line=samples.map(p=>position(p).join(',')).join(' ');
    map=`<svg class="track-map" viewBox="0 0 700 700" role="group" aria-label="Current road — top view"><polyline points="${line}" fill="none" stroke="#90c7d3" stroke-width="3"/>${points.map((p,i)=>{const [x,y]=position(p);return `<circle cx="${x}" cy="${y}" r="${i===selected?7:3}" fill="${i===selected?'#ffcf70':'#90c7d3'}" data-track-point="${i}" tabindex="0" role="button" aria-label="Road point ${i+1}"/>`;}).join('')}</svg>`;
  }
  const p=points[selected];
  const roadInput=(field,label,min=-32768,max=32767)=>{
    const value=p[{left_width:'leftWidth',right_width:'rightWidth',cross_slope:'crossSlope'}[field]||field];
    return `<div class="setting"><label for="road-${field}">${label}</label><input id="road-${field}" data-road-field="${field}" type="number" step="1" min="${min}" max="${max}" value="${value}" data-original="${value}" required></div>`;
  };
  const roadPanel=p?`<form id="road-point-form" data-bank="${bank}" data-index="${selected}" data-revision="${scene.revision}"><p class="muted">Changes affect the driveable route, not the visible asphalt mesh. Keep both aligned before testing in game.</p>${roadInput('x','Position X',-2147483648,2147483647)}${roadInput('y','Height')}${roadInput('z','Position Z',-2147483648,2147483647)}${roadInput('left_width','Left half-width')}${roadInput('right_width','Right half-width')}<details><summary>Segment direction & curvature</summary><p class="muted">Heading and pitch use 4096 units per turn. Cross-slope uses 1/128. Length is stored separately from point coordinates; it is not recalculated. Arc is the original packed reference to an existing curve center.</p>${roadInput('angle','Heading')}${roadInput('pitch','Pitch')}${roadInput('cross_slope','Cross-slope')}${roadInput('length','Segment length',1,32767)}${roadInput('arc','Packed curve reference',0,32767)}</details><footer><button type="submit" class="primary">Save point to project</button><button type="button" data-action="publish-mods">Save & apply project</button></footer></form>`:'';
  const arcIndex=p&&(p.arc&3)?(p.arc<<16>>16)>>4:-1,arc=scene?.road?.arcs?.[arcIndex];
  const arcPanel=arc?`<section class="card"><h2>Curve center ${arcIndex+1}</h2><p class="muted">Shared by every segment referencing this center. Changing it affects the driveable curve, not scenery or asphalt geometry.</p><form id="road-arc-form" data-bank="${bank}" data-index="${arcIndex}" data-revision="${scene.revision}">${['x','z'].map(field=>`<div class="setting"><label for="arc-${field}">Center ${field.toUpperCase()}</label><input id="arc-${field}" data-arc-field="${field}" type="number" step="1" min="-2147483648" max="2147483647" value="${arc[field]}" data-original="${arc[field]}" required></div>`).join('')}<button type="submit" class="primary">Save curve center to project</button></form></section>`:'';
  const objects=scene?.objects||[],object=objects[objectIndex];
  const input=(field,label,min=-2147483648,max=2147483647)=>{
    const value=field==='yaw'?(((object.yaw+2048)%4096+4096)%4096-2048)*360/4096:object[field];
    return `<div class="setting"><label for="object-${field}">${label}</label><input id="object-${field}" data-object-field="${field}" type="number" step="${field==='yaw'?'any':'1'}" min="${min}" max="${max}" value="${value}" data-original="${value}" required></div>`;
  };
  const objectPanel=objects.length?`<section class="card"><h2>Placed objects</h2><p class="muted">Edit a separate scenery object. Coordinates use the game's world units. These entries do not include all terrain and buildings.</p><label>Object <select data-track-object>${objects.map((o,i)=>`<option value="${i}" ${i===objectIndex?'selected':''}>Object ${i+1}${o.model===-1?' · disabled':''}</option>`).join('')}</select></label>${object?`<form id="track-object-form" data-bank="${bank}" data-index="${objectIndex}" data-revision="${scene.revision}">${input('x','Position X')}${input('y','Position Y (height)')}${input('z','Position Z')}${input('yaw','Rotation (degrees)',-180,180)}<details class="car-advanced"><summary>Model & rendering flags</summary><p class="muted">Model -1 hides the object. Model numbers refer to this course's model list. Flags: 1 = alternate normals, 2 = alternate environment material, 4 = environment material, 8 = blinking environment material. Combine flags by adding their values.</p>${input('model','Model',-1,32767)}${input('flags','Flags',0,15)}</details><footer><button type="submit" class="primary">Save object to project</button><button type="button" data-action="publish-mods">Save & apply project</button></footer></form>`:''}</section>`:'';
  const trackId='track.'+['big','mid','hi','oval'][((bank-88)/2)%4]+(Math.floor((bank-88)/8)+1);
  const geometryPanel=`<details class="card"><summary>Advanced · Native model banks</summary><p class="muted">Export the current course or terrain geometry, including project changes. Native RMESH imports must retain model order/count and original material slots. These replacements affect the modern renderer; classic rendering and the driveable road are separate. OBJ/GLB editing is not connected yet.</p>${[['course','Course objects'],['terrain','Terrain cells']].map(([name,label])=>`<div class="row wrap"><span>${label}</span><button data-export-track-mesh="${trackId}.${name}">Export native bank</button><button data-import-track-mesh="${trackId}.${name}" data-revision="${scene?.revision}">Import native bank</button></div>`).join('')}</details>`;
  return `<div class="row between wrap"><div><div class="eyebrow">Tracks</div><h1>${escape(track?.name||'Track workspace')}</h1><p class="muted">Road & object editing · Current project</p></div><div class="row wrap"><button data-track-preview="${bank}">View track in 3D</button><label>Track and class <select data-track-select>${tracks.map(t=>`<option value="${t.bank}" ${t.bank===bank?'selected':''}>${escape(t.name)} · Class ${t.grade}${t.available?'':' · unavailable for racing'}</option>`).join('')}</select></label></div></div><div class="settings-layout"><section class="card">${map}<p class="muted">Geometric top view of the current route, using stored curve centers and interpolated radii. Fixed-point physics may differ slightly. This is not a terrain or rendered road preview.</p></section><section class="card side-info"><h2>Road point ${p?selected+1:'—'}</h2>${roadPanel}<details><summary>Source components</summary><ul>${(scene?.blocks||track?.blocks||[]).map(b=>`<li>${escape(b.role)} · ${b.size.toLocaleString('en-US')} bytes</li>`).join('')}</ul></details></section></div>${arcPanel}${objectPanel}${geometryPanel}`;
}
