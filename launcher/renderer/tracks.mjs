const escape=value=>String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

export function trackWorkspace(tracks,bank,scene,selected=0) {
  const track=tracks.find(t=>t.bank===bank);
  const points=scene?.road?.points||[];
  let map='<p class="muted">Choose a track to inspect its original road.</p>';
  if(points.length) {
    let minX=Infinity,maxX=-Infinity,minZ=Infinity,maxZ=-Infinity;
    for(const p of points){minX=Math.min(minX,p.x);maxX=Math.max(maxX,p.x);minZ=Math.min(minZ,p.z);maxZ=Math.max(maxZ,p.z);}
    const scale=640/Math.max(1,maxX-minX,maxZ-minZ);
    const position=p=>[(p.x-minX)*scale+30,(p.z-minZ)*scale+30];
    const line=points.map(p=>position(p).join(',')).join(' ');
    map=`<svg class="track-map" viewBox="0 0 700 700" role="group" aria-label="Original road points — top view"><polyline points="${line} ${position(points[0]).join(',')}" fill="none" stroke="#90c7d3" stroke-width="3"/>${points.map((p,i)=>{const [x,y]=position(p);return `<circle cx="${x}" cy="${y}" r="${i===selected?7:3}" fill="${i===selected?'#ffcf70':'#90c7d3'}" data-track-point="${i}" tabindex="0" role="button" aria-label="Road point ${i+1}"/>`;}).join('')}</svg>`;
  }
  const p=points[selected];
  return `<div class="row between wrap"><div><div class="eyebrow">Tracks</div><h1>${escape(track?.name||'Track workspace')}</h1><p class="muted">Original disc data · Road inspection</p></div><label>Track and class <select data-track-select>${tracks.map(t=>`<option value="${t.bank}" ${t.bank===bank?'selected':''}>${escape(t.name)} · Class ${t.grade}${t.available?'':' · unavailable for racing'}</option>`).join('')}</select></label></div><div class="settings-layout"><section class="card">${map}<p class="muted">Top view of centerline control points; curved segments connect to the stored arc centers. This is not a terrain or rendered road preview.</p></section><section class="card side-info"><h2>Road point ${p?selected+1:'—'}</h2>${p?`<dl>${Object.entries({X:p.x,Y:p.y,Z:p.z,'Heading (4096 / turn)':p.angle,'Pitch (4096 / turn)':p.pitch,'Cross-slope (1/128)':p.crossSlope,'Left half-width':p.leftWidth,'Right half-width':p.rightWidth,'Segment length':p.length,'Curve mode':p.arc&3,'Arc center index':(p.arc<<16>>16)>>4}).map(([k,v])=>`<dt>${escape(k)}</dt><dd>${v}</dd>`).join('')}</dl><p class="muted">${points.length} points · ${scene.road.arcs.length} arc centers. Editing and route rebuilding are not connected yet.</p>`:''}<details><summary>Source components</summary><ul>${(scene?.blocks||track?.blocks||[]).map(b=>`<li>${escape(b.role)} · ${b.size.toLocaleString('en-US')} bytes</li>`).join('')}</ul></details></section></div>`;
}
