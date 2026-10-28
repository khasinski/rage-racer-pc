// The native helper decodes disc textures; WebGL provides inspection lighting.
import {pickScene,groundDrag} from './scene-pick.mjs';
const activePreviews=new WeakMap();
export async function mountModelPreview(dialog,data,options={}){
 const owner=options.embedded?dialog.closest('dialog'):dialog.tagName==='DIALOG'?dialog:null;
 const isOpen=()=>owner?owner.open:dialog.isConnected;
 const stale=()=>options.embedded&&(!dialog.isConnected||(owner&&!owner.open))||options.previousPreview&&(!isOpen()||activePreviews.get(dialog)!==options.previousPreview);
 const images=new Map();
 for(const [source,texture]of Object.entries(data.textures||{}))if(texture.png){const img=new Image();img.src='data:image/png;base64,'+texture.png;try{await img.decode();}catch{if(stale())return;throw Error('Could not decode the mod texture PNG');}images.set(Number(source),img);}
 if(stale())return;
 activePreviews.get(dialog)?.();
 const inline=Boolean(options.inline);
 const cssHeight=typeof options.height==='number'?`${options.height}px`:(options.height||(options.embedded?'260px':'420px'));
 const trackCamHtml=data.worldScene?`<button type="button" data-model-focus title="Center orbit camera on selected road segment or object and zoom in (Hotkey: C)">🎯 Focus Seg</button><button type="button" data-model-drive title="First-person driver perspective camera on selected road segment — step segments with [ and ] to drive along the track (Hotkey: V)">🏎️ Drive View</button>`:'';
 const hasGhost=Boolean(options.ghostVertices&&options.ghostVertices.length===data.vertices?.length);
 dialog.innerHTML=`<div class="preview-header row between wrap"><div class="preview-title-block"><h2>Model preview</h2><p class="muted preview-hint">Drag to rotate · Scroll to zoom · Keys: 1/3/7/9 Views, W Wire, G Grid, B Box, T Turn, L Light, F Reset</p></div><div class="row wrap action-row" style="gap:6px;" aria-label="Model views">${trackCamHtml}<button type="button" data-model-view="front" title="Front View (Hotkey: 1)">Front</button><button type="button" data-model-view="side" title="Side View (Hotkey: 3)">Side</button><button type="button" data-model-view="rear" title="Rear View (Hotkey: 9)">Rear</button><button type="button" data-model-view="top" title="Top View (Hotkey: 7)">Top</button>${options.expandAction||''}</div></div><canvas ${inline?'':'id="model-preview"'} tabindex="0" aria-label="Interactive textured model preview" style="display:block;width:100%;height:${cssHeight};touch-action:none;background:#10151e;border-radius:8px"></canvas><div class="preview-footer row between wrap"><p class="muted preview-caption">Textures come from your game image and the selected mod.</p><footer class="row wrap" style="gap:8px;"><label class="row" title="Show ground plane at tire contact level (Hotkey: G)"><input type="checkbox" data-model-grid> Grid</label><label class="row" title="Show 3D bounding box (Hotkey: B)"><input type="checkbox" data-model-bbox> BBox</label>${hasGhost?`<label class="row" title="Overlay un-fitted base mesh wireframe for scale/stance reference (Hotkey: O)"><input type="checkbox" data-model-ghost> 👻 Base Cage</label>`:''}<label class="row" title="Toggle wireframe rendering (Hotkey: W)"><input type="checkbox" data-model-wire ${inline?'':'id="model-wire"'}> Wireframe</label><label class="row" title="Auto-rotate showroom turntable (Hotkey: T)"><input type="checkbox" data-model-turntable> 🔄 Turn</label><button type="button" data-model-sun title="Rotate inspection sun light angle +45° (Hotkey: L)">☀️ Light</button><button type="button" data-model-reset ${inline?'':'id="model-reset"'} title="Reset camera view (Hotkey: F)">Reset</button>${options.embedded||inline?'':'<button type="button" data-action="close">Close</button>'}</footer></div>`;
 if(data.worldScene){dialog.querySelector('h2').textContent='Track 3D scene';dialog.querySelector('.preview-caption').textContent='Click road, terrain, or scenery in 3D to select · Shift+Drag to move · V for Drive View';}
 if(data.previewName){dialog.querySelector('h2').textContent=data.previewName;dialog.querySelector('.preview-caption').textContent='Current project geometry and textures.';}
 if(data.previewNote){dialog.querySelector('.preview-caption').textContent=data.previewNote;}
 if(data.wholeCar)dialog.querySelector('h2').textContent=data.carTitle||'3D car preview';
 if(data.modName){const note=document.createElement('p');note.className='muted';note.textContent=data.modName+' · Geometry, materials and supported texture overrides from this mod are included. Other assets use the base disc.';dialog.querySelector('.preview-header').after(note);}
 const canvas=dialog.querySelector('canvas'),gl=canvas.getContext('webgl');
 if(!gl)throw Error('WebGL is unavailable. Model preview requires graphics acceleration.');
 const shaders=[];
 function shader(type,source){const s=gl.createShader(type);shaders.push(s);gl.shaderSource(s,source);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s;}
 const program=gl.createProgram();
 gl.attachShader(program,shader(gl.VERTEX_SHADER,`precision mediump float;attribute vec3 position;attribute vec3 normal;attribute vec3 color;attribute vec2 uv;uniform vec4 view;uniform vec3 camTarget;uniform float projMode;uniform float unlit;uniform vec3 sunDir;varying vec3 normalDir;varying vec3 shade;varying vec2 texUV;
 void main(){float a=view.x,b=view.y;mat3 yaw=mat3(cos(a),0.,-sin(a),0.,1.,0.,sin(a),0.,cos(a));mat3 pitch=mat3(1.,0.,0.,0.,cos(b),sin(b),0.,-sin(b),cos(b));vec3 p=pitch*yaw*(position-camTarget);vec3 n=pitch*yaw*normal;if(projMode>.5){float depth=-p.z,near=.0004,far=2.5;gl_Position=vec4(p.x*view.z/view.w,p.y*view.z,depth*((far+near)/(far-near))-(2.*far*near)/(far-near),depth);}else{gl_Position=vec4(p.x*view.z/view.w,p.y*view.z,p.z*.2,1.);}float light=.35+.65*abs(dot(normalize(n),normalize(sunDir)));normalDir=n;shade=color*mix(light,1.,unlit);texUV=uv;}`));
 gl.attachShader(program,shader(gl.FRAGMENT_SHADER,'precision mediump float;varying vec3 normalDir;varying vec3 shade;varying vec2 texUV;uniform sampler2D atlas;uniform bool textured;uniform vec4 factor;uniform vec3 emission;uniform vec2 finish;uniform int alphaMode;uniform vec3 sunDir;void main(){vec4 t=textured?texture2D(atlas,texUV):vec4(1.);float alpha=t.a*factor.a;if(t.a<.01||(alphaMode==1&&alpha<.5))discard;vec3 halfVec=normalize(sunDir+vec3(0.,0.,1.));float highlight=pow(abs(dot(normalize(normalDir),halfVec)),mix(128.,2.,finish.x))*finish.y*.38;gl_FragColor=vec4(t.rgb*shade*factor.rgb+emission+highlight,alphaMode==2?alpha:1.);}'));
 gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getShaderInfoLog(program));gl.useProgram(program);
 const min=[Infinity,Infinity,Infinity],max=[-Infinity,-Infinity,-Infinity];let v=data.vertices;
 const refVerts=hasGhost?options.ghostVertices:v;
 for(let i=0;i<refVerts.length;i+=9)for(let a=0;a<3;a++){min[a]=Math.min(min[a],refVerts[i+a]);max[a]=Math.max(max[a],refVerts[i+a]);}
 if(options.comparison&&!options.frame){const original=options.comparison.original.vertices;min.fill(Infinity);max.fill(-Infinity);for(let i=0;i<original.length;i+=9)for(let a=0;a<3;a++){min[a]=Math.min(min[a],original[i+a]);max[a]=Math.max(max[a],original[i+a]);}}
 const frame=options.frame||{scale:2/Math.max(...max.map((x,a)=>x-min[a]),1e-6),center:max.map((x,a)=>(x+min[a])/2)}, {scale,center}=frame;
 function vertex(index,out){const at=index*9,material=v[at+6]>>>0,col=(material&0xffff)===65535?[.15,.16,.18]:!data.worldScene&&((material&0xffff)>>>6)===1?[.10,.16,.2]:[1,1,1];if(data.colors){const untextured=(material&0xffff)===65535;for(let c=0;c<3;c++)col[c]=untextured?data.colors[index*4+c]/255:col[c]*Math.min(data.colors[index*4+c]*2/255,1);}
 for(let a=0;a<3;a++)out.push((v[at+a]-center[a])*scale);out.push(v[at+3],v[at+4],v[at+5],...col,v[at+7],v[at+8]);}
 const triangles=[],lines=[],groups=[],byMaterial=new Map();
 const groupBias=m=>data.worldScene&&(m&0xffff)===65535?-128:((m<<8)>>24);
 for(let i=0;i<data.indices.length;i+=3){const material=v[data.indices[i]*9+6]>>>0;let list=byMaterial.get(material);if(!list){list=[];byMaterial.set(material,list);}list.push(data.indices[i],data.indices[i+1],data.indices[i+2]);}
 const sortedMaterials=[...byMaterial.entries()].sort((a,b)=>groupBias(b[0])-groupBias(a[0]));
 for(const [material,ids]of sortedMaterials){const start=triangles.length/11;for(let j=0;j<ids.length;j++)vertex(ids[j],triangles);groups.push({material,start,count:ids.length});}
 for(let i=0;i<data.indices.length;i+=3){const i0=data.indices[i],i1=data.indices[i+1],i2=data.indices[i+2];vertex(i0,lines);vertex(i1,lines);vertex(i1,lines);vertex(i2,lines);vertex(i2,lines);vertex(i0,lines);}
 let linesDirty=false;
 const buffers=[triangles,lines].map(values=>{const b=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,b);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(values),gl.STATIC_DRAW);return b;});
 const guideBuffer=gl.createBuffer(),ghostBuffer=gl.createBuffer();
 buffers.push(guideBuffer,ghostBuffer);
 let guideCount=0,ghostCount=0;
 const savedState=options.view||options.viewState||dialog._viewState||dialog._savedView||{};
 let {yaw=-.65,pitch=-.35,zoom=.75,wire=false,showGrid=Boolean(data.wholeCar),showBox=false,showGhost=false,turntable=false,sunAngle=.55,driveMode=false}=savedState,target=Array.isArray(savedState.target)?[...savedState.target]:[0,0,0],drag=null,panning=false,rafId=0;
 let selectedCenter=[0,0,0],selectedRoadIdx=Number.isInteger(data.selectedRoadPoint)&&data.selectedRoadPoint>=0?data.selectedRoadPoint:0;
 const maxZoom=data.worldScene?80:18;
 function pushGuideVertex(x,y,z,col,out){out.push(x,y,z,0,1,0,col[0],col[1],col[2],0,0);}
 if(hasGhost){
  const gv=options.ghostVertices,gLines=[],gCol=[.22,.82,1];
  for(let i=0;i<data.indices.length;i+=3){
   const ids=data.indices.slice(i,i+3);
   for(let e=0;e<3;e++){
    const a=ids[e]*9,b=ids[(e+1)%3]*9;
    pushGuideVertex((gv[a]-center[0])*scale,(gv[a+1]-center[1])*scale,(gv[a+2]-center[2])*scale,gCol,gLines);
    pushGuideVertex((gv[b]-center[0])*scale,(gv[b+1]-center[1])*scale,(gv[b+2]-center[2])*scale,gCol,gLines);
   }
  }
  gl.bindBuffer(gl.ARRAY_BUFFER,ghostBuffer);
  gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(gLines),gl.STATIC_DRAW);
  ghostCount=gLines.length/11;
 }
 const captionEl=dialog.querySelector('.preview-caption');
 const titleEl=dialog.querySelector('h2');
 const baseTitle=titleEl?titleEl.textContent:'Model preview';
 const baseCaption=captionEl?captionEl.textContent:'';
 let baseGroundY=Infinity;
 for(let i=0;i<refVerts.length;i+=9)baseGroundY=Math.min(baseGroundY,refVerts[i+1]);
 function updateStatsAndGuides(){
  const curMin=[Infinity,Infinity,Infinity],curMax=[-Infinity,-Infinity,-Infinity];
  for(let i=0;i<v.length;i+=9)for(let a=0;a<3;a++){curMin[a]=Math.min(curMin[a],v[i+a]);curMax[a]=Math.max(curMax[a],v[i+a]);}
  const roadY=curMin[1];
  if(captionEl&&Number.isFinite(curMin[0])){
   const tris=Math.floor(data.indices.length/3),verts=Math.floor(v.length/9);
   const w=Math.round(curMax[0]-curMin[0]),h=Math.round(curMax[1]-curMin[1]),l=Math.round(curMax[2]-curMin[2]);
   const groundDelta=Math.round(curMin[1]-roadY);
   const groundTag=!data.worldScene?` · Ground ΔY: ${groundDelta>0?'+'+groundDelta:groundDelta}`:'';
   const modeTag=driveMode?` · 🏎️ Driver Camera (Seg #${selectedRoadIdx+1} · Drag to look · [ / ] to drive)`:'';
   captionEl.textContent=`${tris.toLocaleString()} tris · ${verts.toLocaleString()} verts · ${w}×${h}×${l}${groundTag}${modeTag}${inline?'':' — '+baseCaption}`;
  }
  const guides=[];
  if(showGrid&&Number.isFinite(curMin[0])){
   const gy=(roadY-center[1])*scale;
   const span=1.35,steps=10;
   for(let i=-steps;i<=steps;i++){
    const t=(i/steps)*span;
    const isAxis=i===0;
    const colX=isAxis?[.22,.68,.95]:[.18,.24,.34];
    const colZ=isAxis?[.95,.62,.22]:[.18,.24,.34];
    pushGuideVertex(-span,gy,t,colX,guides);pushGuideVertex(span,gy,t,colX,guides);
    pushGuideVertex(t,gy,-span,colZ,guides);pushGuideVertex(t,gy,span,colZ,guides);
   }
  }
  if(showBox&&Number.isFinite(curMin[0])){
   const x0=(curMin[0]-center[0])*scale,y0=(curMin[1]-center[1])*scale,z0=(curMin[2]-center[2])*scale;
   const x1=(curMax[0]-center[0])*scale,y1=(curMax[1]-center[1])*scale,z1=(curMax[2]-center[2])*scale;
   const corners=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0],[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]];
   const bCol=[.38,.86,1];
   for(const [a,b]of [[0,1],[1,2],[2,3],[3,0],[4,5],[5,6],[6,7],[7,4],[0,4],[1,5],[2,6],[3,7]]){
    pushGuideVertex(...corners[a],bCol,guides);
    pushGuideVertex(...corners[b],bCol,guides);
   }
  }
  gl.bindBuffer(gl.ARRAY_BUFFER,guideBuffer);
  gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(guides),gl.DYNAMIC_DRAW);
  guideCount=guides.length/11;
 }
 updateStatsAndGuides();
 function ensureWireLines(){
  if(!linesDirty)return;
  lines.length=0;
  for(let i=0;i<data.indices.length;i+=3){const i0=data.indices[i],i1=data.indices[i+1],i2=data.indices[i+2];vertex(i0,lines);vertex(i1,lines);vertex(i1,lines);vertex(i2,lines);vertex(i2,lines);vertex(i0,lines);}
  gl.bindBuffer(gl.ARRAY_BUFFER,buffers[1]);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(lines),gl.DYNAMIC_DRAW);
  linesDirty=false;
 }
 dialog.updateVertices=nextVertices=>{
  v=nextVertices;
  triangles.length=0;groups.length=0;
  for(const [material,ids]of sortedMaterials){const start=triangles.length/11;for(let j=0;j<ids.length;j++)vertex(ids[j],triangles);groups.push({material,start,count:ids.length});}
  gl.bindBuffer(gl.ARRAY_BUFFER,buffers[0]);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(triangles),gl.DYNAMIC_DRAW);
  linesDirty=true;
  if(wire)ensureWireLines();
  updateStatsAndGuides();
  draw();
 };
 const selTriBuffer=gl.createBuffer(),selLineBuffer=gl.createBuffer();
 buffers.push(selTriBuffer,selLineBuffer);
 let selTriCount=0,selLineCount=0;
 function pushSelVertex(x,y,z,col,out){out.push(x,y,z,0,1,0,col[0],col[1],col[2],0,0);}
 const driveBtn=dialog.querySelector('[data-model-drive]'),focusBtn=dialog.querySelector('[data-model-focus]');
 function applyDriveCamera(segIdx=selectedRoadIdx,keepFov=false){
  const cams=data.roadCameras||[];
  if(!cams.length)return false;
  const idx=((segIdx%cams.length)+cams.length)%cams.length;
  selectedRoadIdx=idx;
  const cam=cams[idx];
  target=[(cam.x-center[0])*scale,(cam.y-center[1])*scale,(cam.z-center[2])*scale];
  yaw=cam.yaw;
  pitch=cam.pitch;
  if(!keepFov||!driveMode)zoom=1.55;
  driveMode=true;
  if(driveBtn)driveBtn.classList.add('active');
  if(titleEl&&data.worldScene)titleEl.textContent=`Track 3D · 🏎️ Drive View (Seg #${idx+1})`;
  updateStatsAndGuides();
  draw();
  return true;
 }
 function exitDriveCamera(){
  if(!driveMode)return;
  driveMode=false;
  if(driveBtn)driveBtn.classList.remove('active');
  if(titleEl)titleEl.textContent=baseTitle;
  updateStatsAndGuides();
 }
 function focusSelectedEntity(){
  exitDriveCamera();
  target=[...selectedCenter];
  zoom=Math.max(zoom<4?10:zoom,6);
  if(pitch>-0.18)pitch=-0.38;
  draw();
 }
 dialog.focusSelection=focusSelectedEntity;
 dialog.setDriveView=enable=>{
  if(enable===undefined)enable=!driveMode;
  if(enable)applyDriveCamera(selectedRoadIdx,false);
  else{exitDriveCamera();target=[...selectedCenter];zoom=Math.min(maxZoom,Math.max(6,zoom));pitch=-.38;draw();}
  return driveMode;
 };
 function setSelection(sel){
  selTriCount=0;selLineCount=0;
  if(!sel||!Array.isArray(data.entities)){draw();return;}
  const list=Array.isArray(sel)?sel:[sel];
  const roadSel=list.find(s=>s?.bank==='road'&&Number.isInteger(s.point));
  if(roadSel&&roadSel.point>=0)selectedRoadIdx=roadSel.point;
  const roadCount=data.entities.reduce((m,e)=>e.bank==='road'&&Number.isInteger(e.point)?Math.max(m,e.point+1):m,0);
  const matched=[];
  for(const ent of data.entities){
   let role=null;
   for(const s of list){
    if(!s||s.bank!==ent.bank)continue;
    if(s.bank==='terrain'&&Number.isInteger(s.cell)&&ent.cell===s.cell){role={col:[.16,.95,.65],box:true};break;}
    if(s.bank==='course'&&Number.isInteger(s.object)&&ent.object===s.object){role={col:[.28,.78,1],box:true};break;}
    if(s.bank==='road'&&Number.isInteger(s.point)&&Number.isInteger(ent.point)){
     if(ent.point===s.point){role={col:[1,.82,.18],box:true};break;}
     const rad=Number.isInteger(s.radius)?s.radius:0;
     if(rad>0&&roadCount>0){
      const d=Math.min(Math.abs(ent.point-s.point),roadCount-Math.abs(ent.point-s.point));
      if(d<=rad){role={col:d===1?[.38,.86,1]:[.24,.68,.98],box:false};}
     }
    }
   }
   if(role)matched.push({ent,...role});
  }
  if(!matched.length){
   if(driveMode&&roadSel&&data.roadCameras?.length)applyDriveCamera(roadSel.point,true);
   else draw();
   return;
  }
  const selTris=[],selLines=[];
  let bMin=[Infinity,Infinity,Infinity],bMax=[-Infinity,-Infinity,-Infinity],boxCol=matched[0].col;
  for(const {ent,col,box} of matched){
   if(box)boxCol=col;
   const end=Math.min(data.indices.length,ent.firstIndex+ent.indexCount);
   for(let i=ent.firstIndex;i<end;i+=3){
    const ids=data.indices.slice(i,i+3);
    if(ids.length<3)continue;
    const pts=ids.map(id=>{
     const at=id*9;
     const px=(v[at]-center[0])*scale,py=(v[at+1]-center[1])*scale,pz=(v[at+2]-center[2])*scale;
     if(box){
      bMin[0]=Math.min(bMin[0],px);bMin[1]=Math.min(bMin[1],py);bMin[2]=Math.min(bMin[2],pz);
      bMax[0]=Math.max(bMax[0],px);bMax[1]=Math.max(bMax[1],py);bMax[2]=Math.max(bMax[2],pz);
     }
     return [px,py,pz];
    });
    for(const p of pts)pushSelVertex(p[0],p[1],p[2],col,selTris);
    for(let e=0;e<3;e++){
     const p0=pts[e],p1=pts[(e+1)%3];
     pushSelVertex(p0[0],p0[1],p0[2],col,selLines);
     pushSelVertex(p1[0],p1[1],p1[2],col,selLines);
    }
   }
  }
  if(Number.isFinite(bMin[0])){
   selectedCenter=[(bMin[0]+bMax[0])/2,(bMin[1]+bMax[1])/2,(bMin[2]+bMax[2])/2];
   const pad=0.0025;
   const x0=bMin[0]-pad,y0=bMin[1]-pad,z0=bMin[2]-pad,x1=bMax[0]+pad,y1=bMax[1]+pad,z1=bMax[2]+pad;
   const corners=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0],[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]];
   for(const [a,b]of [[0,1],[1,2],[2,3],[3,0],[4,5],[5,6],[6,7],[7,4],[0,4],[1,5],[2,6],[3,7]]){
    pushSelVertex(...corners[a],boxCol,selLines);
    pushSelVertex(...corners[b],boxCol,selLines);
   }
  }
  gl.bindBuffer(gl.ARRAY_BUFFER,selTriBuffer);
  gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(selTris),gl.DYNAMIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER,selLineBuffer);
  gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(selLines),gl.DYNAMIC_DRAW);
  selTriCount=selTris.length/11;
  selLineCount=selLines.length/11;
  if(driveMode&&roadSel&&data.roadCameras?.length){
   applyDriveCamera(roadSel.point,true);
   return;
  }
  draw();
 }
 dialog.setSelectedEntity=setSelection;
 const attributes=['position','normal','color','uv'].map(name=>gl.getAttribLocation(program,name)),view=gl.getUniformLocation(program,'view'),camTargetLoc=gl.getUniformLocation(program,'camTarget'),projModeLoc=gl.getUniformLocation(program,'projMode'),sunDirLoc=gl.getUniformLocation(program,'sunDir');
 const textureHandles=[];const textures=new Map();
 for(const [source,encoded]of Object.entries(data.textures||{})){
  const srcNum=Number(source);
  const img=images.get(srcNum);
  const bytes=img?null:Uint8Array.from(atob(encoded),c=>c.charCodeAt(0));
  const t=gl.createTexture();textureHandles.push(t);gl.bindTexture(gl.TEXTURE_2D,t);
  if(img)gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,img);
  else gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,256,256,0,gl.RGBA,gl.UNSIGNED_BYTE,bytes);
  const wrap=data.worldScene||(img&&((img.width&(img.width-1))||(img.height&(img.height-1))))?gl.CLAMP_TO_EDGE:gl.REPEAT;
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,wrap);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,wrap);textures.set(srcNum,t);
 }
 const textured=gl.getUniformLocation(program,'textured');gl.uniform1i(gl.getUniformLocation(program,'atlas'),0);
 const materialUniforms=Object.fromEntries(['factor','emission','finish','unlit','alphaMode'].map(name=>[name,gl.getUniformLocation(program,name)]));
 function appearance(source){const p=data.materials?.[source]?.properties?.trim().split(/\s+/);gl.uniform4f(materialUniforms.factor,...(p?p.slice(4,8).map(Number):[1,1,1,1]));gl.uniform3f(materialUniforms.emission,...(p?p.slice(8,11).map(Number):[0,0,0]));gl.uniform2f(materialUniforms.finish,p?Number(p[2]):1,p&&p[0]!=='unlit'?.04+.96*Number(p[3]):0);gl.uniform1f(materialUniforms.unlit,p?.[0]==='unlit'?1:0);const blend=p?.[1]==='blend';gl.uniform1i(materialUniforms.alphaMode,blend?2:p?.[1]==='mask'?1:0);if(blend){gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);}else gl.disable(gl.BLEND);}
 const background=[.063,.082,.118];
 const wireInput=dialog.querySelector('[data-model-wire]'),gridInput=dialog.querySelector('[data-model-grid]'),bboxInput=dialog.querySelector('[data-model-bbox]'),ghostInput=dialog.querySelector('[data-model-ghost]'),turnInput=dialog.querySelector('[data-model-turntable]'),sunBtn=dialog.querySelector('[data-model-sun]'),resetBtn=dialog.querySelector('[data-model-reset]');
 wireInput.checked=wire;
 if(gridInput)gridInput.checked=showGrid;
 if(bboxInput)bboxInput.checked=showBox;
 if(ghostInput)ghostInput.checked=showGhost;
 if(turnInput)turnInput.checked=turntable;
 if(driveMode&&driveBtn)driveBtn.classList.add('active');
 if(options.comparison){
  dialog.querySelector('h2').textContent='Car preview · '+((options.mode||'modified')==='original'?'Original':'Modified');
  const controls=document.createElement('div');controls.className='row model-comparison';controls.setAttribute('role','group');controls.setAttribute('aria-label','Compare car');
  for(const mode of ['original','modified']){const button=document.createElement('button');button.type='button';button.dataset.compare=mode;button.textContent=mode==='original'?'Original':'Modified';button.setAttribute('aria-pressed',String((options.mode||'modified')===mode));if((options.mode||'modified')===mode)button.className='primary';
   button.onclick=async()=>{if((options.mode||'modified')===mode)return;for(const b of controls.querySelectorAll('button'))b.disabled=true;try{await mountModelPreview(dialog,options.comparison[mode],{...options,mode,frame,previousPreview:activePreviews.get(dialog),view:{yaw,pitch,zoom,wire,showGrid,showBox,showGhost,turntable,sunAngle,driveMode,target:[...target],background}});}catch(error){for(const b of controls.querySelectorAll('button'))b.disabled=false;const message=document.createElement('p');message.setAttribute('role','alert');message.textContent=error.message;controls.after(message);}};controls.append(button);
  }dialog.querySelector('canvas').before(controls);
 }
 function bindAttribs(){attributes.forEach((a,i)=>{gl.enableVertexAttribArray(a);gl.vertexAttribPointer(a,i===3?2:3,gl.FLOAT,false,44,i*12);});}
 function draw(){dialog._viewState={yaw,pitch,zoom,wire,showGrid,showBox,showGhost,turntable,sunAngle,driveMode,target:[...target]};if(!canvas.clientWidth||!canvas.clientHeight)return;const w=Math.round(canvas.clientWidth*Math.min(devicePixelRatio,2)),h=Math.round(canvas.clientHeight*Math.min(devicePixelRatio,2));if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;}gl.viewport(0,0,canvas.width,canvas.height);gl.clearColor(...background,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);gl.enable(gl.DEPTH_TEST);gl.depthFunc(data.worldScene?gl.LEQUAL:gl.LESS);gl.depthMask(true);gl.frontFace(gl.CCW);gl.cullFace(gl.BACK);gl.bindBuffer(gl.ARRAY_BUFFER,buffers[wire?1:0]);bindAttribs();gl.uniform4f(view,yaw,pitch,zoom,canvas.width/canvas.height);gl.uniform3f(camTargetLoc,target[0],target[1],target[2]);gl.uniform1f(projModeLoc,driveMode?1:0);gl.uniform3f(sunDirLoc,Math.sin(sunAngle)*.85,-.65,Math.cos(sunAngle)*.85);if(wire){gl.disable(gl.CULL_FACE);appearance(-1);gl.uniform1i(textured,0);gl.drawArrays(gl.LINES,0,lines.length/11);}else for(const group of groups){const id=group.material&0xffff;const isRoadRibbon=Boolean(data.worldScene&&id===65535);const bias=groupBias(group.material);const source=data.worldScene?id:id%64,isGlass=!data.worldScene&&(id>>>6)===1;if(driveMode&&!isRoadRibbon)gl.enable(gl.CULL_FACE);else gl.disable(gl.CULL_FACE);appearance(id===65535?-1:source);if(isRoadRibbon){gl.uniform1f(materialUniforms.unlit,1);gl.uniform1i(materialUniforms.alphaMode,2);gl.uniform4f(materialUniforms.factor,1,1,1,driveMode?0.18:0.86);gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);gl.depthMask(!driveMode);gl.enable(gl.POLYGON_OFFSET_FILL);gl.polygonOffset(-6.0,-6.0);}else if(data.worldScene&&bias!==0){gl.enable(gl.POLYGON_OFFSET_FILL);gl.polygonOffset(bias*0.15,bias*0.5);}const texture=isGlass?null:textures.get(source);gl.uniform1i(textured,texture?1:0);gl.bindTexture(gl.TEXTURE_2D,texture||null);gl.drawArrays(gl.TRIANGLES,group.start,group.count);if(isRoadRibbon||data.worldScene){gl.disable(gl.POLYGON_OFFSET_FILL);gl.depthMask(true);if(isRoadRibbon)gl.disable(gl.BLEND);}}gl.disable(gl.CULL_FACE);gl.depthFunc(gl.LESS);if(showGhost&&ghostCount>0){gl.bindTexture(gl.TEXTURE_2D,null);gl.uniform1i(textured,0);gl.uniform1f(materialUniforms.unlit,1);gl.uniform2f(materialUniforms.finish,1,0);gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);gl.depthMask(false);gl.uniform1i(materialUniforms.alphaMode,2);gl.uniform4f(materialUniforms.factor,1,1,1,.38);gl.uniform3f(materialUniforms.emission,0,0,0);gl.bindBuffer(gl.ARRAY_BUFFER,ghostBuffer);bindAttribs();gl.drawArrays(gl.LINES,0,ghostCount);gl.depthMask(true);gl.disable(gl.BLEND);}if(guideCount>0&&!driveMode){gl.bindTexture(gl.TEXTURE_2D,null);gl.uniform1i(textured,0);gl.uniform1f(materialUniforms.unlit,1);gl.uniform2f(materialUniforms.finish,1,0);gl.disable(gl.BLEND);gl.depthMask(true);gl.uniform1i(materialUniforms.alphaMode,0);gl.uniform4f(materialUniforms.factor,1,1,1,1);gl.uniform3f(materialUniforms.emission,0,0,0);gl.bindBuffer(gl.ARRAY_BUFFER,guideBuffer);bindAttribs();gl.drawArrays(gl.LINES,0,guideCount);}if((selTriCount>0||selLineCount>0)&&!driveMode){gl.bindTexture(gl.TEXTURE_2D,null);gl.uniform1i(textured,0);gl.uniform1f(materialUniforms.unlit,1);gl.uniform2f(materialUniforms.finish,1,0);gl.depthFunc(gl.LEQUAL);if(selTriCount>0){gl.enable(gl.POLYGON_OFFSET_FILL);gl.polygonOffset(-7.0,-7.0);gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);gl.depthMask(false);gl.uniform1i(materialUniforms.alphaMode,2);gl.uniform4f(materialUniforms.factor,1,1,1,.65);gl.uniform3f(materialUniforms.emission,0,0,0);gl.bindBuffer(gl.ARRAY_BUFFER,selTriBuffer);bindAttribs();gl.drawArrays(gl.TRIANGLES,0,selTriCount);gl.disable(gl.POLYGON_OFFSET_FILL);}if(selLineCount>0){gl.disable(gl.BLEND);gl.depthMask(true);gl.uniform1i(materialUniforms.alphaMode,0);gl.uniform4f(materialUniforms.factor,1,1,1,1);gl.uniform3f(materialUniforms.emission,.15,.15,.15);gl.bindBuffer(gl.ARRAY_BUFFER,selLineBuffer);bindAttribs();gl.drawArrays(gl.LINES,0,selLineCount);}gl.depthFunc(gl.LESS);gl.depthMask(true);gl.disable(gl.BLEND);}}
 function syncTurntable(){
  cancelAnimationFrame(rafId);rafId=0;
  if(!turntable)return;
  let last=performance.now();
  const step=now=>{
   if(!turntable||!isOpen())return;
   const dt=Math.min(0.05,(now-last)/1000);last=now;
   if(!drag&&!moving){yaw+=dt*0.55;draw();}
   rafId=requestAnimationFrame(step);
  };
  rafId=requestAnimationFrame(step);
 }
 let press=null,moving=null;
 const point=e=>{const r=canvas.getBoundingClientRect();return [(e.clientX-r.left)/r.width*2-1,1-(e.clientY-r.top)/r.height*2];};
 if(options.onMoveEntity)dialog.querySelector('.preview-hint').textContent+=' · Shift-drag an object or road segment to move it on the ground';
 canvas.oncontextmenu=e=>e.preventDefault();
 canvas.onpointerdown=e=>{
  press=[e.clientX,e.clientY];
  panning=!driveMode&&(e.button===1||e.button===2||e.altKey);
  if(!driveMode&&!panning&&e.shiftKey&&options.onMoveEntity){
   const hit=pickScene(data,frame,{yaw,pitch,zoom,aspect:canvas.clientWidth/canvas.clientHeight,target},...point(e));
   if((hit?.bank==='course'&&Number.isInteger(hit.object))||(hit?.bank==='road'&&Number.isInteger(hit.point)))moving={entity:hit,start:point(e)};
   else panning=true;
  }
  drag=[e.clientX,e.clientY];
  canvas.setPointerCapture(e.pointerId);
  canvas.focus();
 };
 canvas.onpointermove=e=>{
  if(!drag||moving)return;
  const dx=e.clientX-drag[0],dy=e.clientY-drag[1];
  drag=[e.clientX,e.clientY];
  if(panning&&!driveMode){
   const step=2/(Math.max(120,canvas.clientHeight)*zoom);
   const c=Math.cos(yaw),s=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);
   const rx=-dx*step,ry=dy*step;
   target[0]+=c*rx+s*sp*ry;
   target[1]+=cp*ry;
   target[2]+=s*rx-c*sp*ry;
   draw();
   return;
  }
  const sens=driveMode?.006:.01;
  yaw+=dx*sens;
  pitch=Math.max(-1.52,Math.min(1.52,pitch+dy*sens));
  draw();
 };
 canvas.onpointerup=async e=>{drag=null;panning=false;if(moving){const move=moving;moving=null;press=null;const end=point(e);try{const delta=groundDrag(frame,{yaw,pitch,zoom,aspect:canvas.clientWidth/canvas.clientHeight},end[0]-move.start[0],end[1]-move.start[1]);if(delta.x||delta.z){canvas.style.pointerEvents='none';const next=await options.onMoveEntity(move.entity,delta);if(isOpen()&&activePreviews.get(dialog)===dispose)await mountModelPreview(dialog,next,{...options,frame,view:{yaw,pitch,zoom,wire,showGrid,showBox,showGhost,turntable,sunAngle,driveMode,target:[...target],background}});}}catch(error){const note=document.createElement('p');note.setAttribute('role','alert');note.textContent=error.message;canvas.after(note);}finally{canvas.style.pointerEvents='';}return;}if(!driveMode&&press&&Math.hypot(e.clientX-press[0],e.clientY-press[1])<7&&options.onSelectEntity){const rect=canvas.getBoundingClientRect();const entity=pickScene(data,frame,{yaw,pitch,zoom,aspect:rect.width/rect.height,target},(e.clientX-rect.left)/rect.width*2-1,1-(e.clientY-rect.top)/rect.height*2);if(entity){setSelection(entity);options.onSelectEntity(entity);}}press=null;};
 canvas.onpointercancel=()=>{drag=null;press=null;moving=null;panning=false;};
 canvas.addEventListener('wheel',e=>{
  e.preventDefault();
  const minZ=driveMode?.45:.12,maxZ=driveMode?6:maxZoom;
  zoom=Math.max(minZ,Math.min(maxZ,zoom*Math.exp(-e.deltaY*.0012)));
  draw();
 },{passive:false});
 canvas.onkeydown=e=>{
  if(driveMode&&(e.key==='ArrowUp'||e.key==='ArrowDown')&&data.roadCameras?.length){
   e.preventDefault();
   const nextSeg=((selectedRoadIdx+(e.key==='ArrowUp'?1:-1))%data.roadCameras.length+data.roadCameras.length)%data.roadCameras.length;
   applyDriveCamera(nextSeg,true);
   options.onSelectEntity?.({bank:'road',point:nextSeg});
   return;
  }
  if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','+','-'].includes(e.key)){
   e.preventDefault();yaw+=e.key==='ArrowLeft'?-.1:e.key==='ArrowRight'?.1:0;pitch+=e.key==='ArrowUp'?-.1:e.key==='ArrowDown'?.1:0;zoom=Math.max(.12,Math.min(maxZoom,zoom*(e.key==='+'?1.15:e.key==='-'?.87:1)));draw();return;
  }
  if(e.metaKey||e.ctrlKey||e.altKey)return;
  const k=e.key.toLowerCase();
  if(k==='1'){e.preventDefault();exitDriveCamera();yaw=Math.PI;pitch=0;draw();}
  else if(k==='3'){e.preventDefault();exitDriveCamera();yaw=Math.PI/2;pitch=0;draw();}
  else if(k==='9'){e.preventDefault();exitDriveCamera();yaw=0;pitch=0;draw();}
  else if(k==='7'){e.preventDefault();exitDriveCamera();yaw=0;pitch=-Math.PI/2;draw();}
  else if(k==='w'){e.preventDefault();wire=!wire;if(wire)ensureWireLines();wireInput.checked=wire;draw();}
  else if(k==='g'&&gridInput){e.preventDefault();showGrid=!showGrid;gridInput.checked=showGrid;updateStatsAndGuides();draw();}
  else if(k==='b'&&bboxInput){e.preventDefault();showBox=!showBox;bboxInput.checked=showBox;updateStatsAndGuides();draw();}
  else if(k==='o'&&ghostInput){e.preventDefault();showGhost=!showGhost;ghostInput.checked=showGhost;draw();}
  else if(k==='t'&&turnInput){e.preventDefault();turntable=!turntable;turnInput.checked=turntable;syncTurntable();}
  else if(k==='l'){e.preventDefault();sunAngle=(sunAngle+Math.PI/4)%(Math.PI*2);draw();}
  else if(k==='c'&&data.worldScene){e.preventDefault();focusSelectedEntity();}
  else if(k==='v'&&data.worldScene){e.preventDefault();dialog.setDriveView();}
  else if(k==='f'||k==='r'){e.preventDefault();exitDriveCamera();target=[0,0,0];yaw=-.65;pitch=-.35;zoom=.75;draw();}
 };
 for(const button of dialog.querySelectorAll('[data-model-view]'))button.onclick=()=>{exitDriveCamera();const preset={front:[Math.PI,0],side:[Math.PI/2,0],rear:[0,0],top:[0,-Math.PI/2]}[button.dataset.modelView];[yaw,pitch]=preset;draw();};
 if(focusBtn)focusBtn.onclick=()=>focusSelectedEntity();
 if(driveBtn)driveBtn.onclick=()=>dialog.setDriveView();
 wireInput.onchange=e=>{wire=e.target.checked;if(wire)ensureWireLines();draw();};
 if(gridInput)gridInput.onchange=e=>{showGrid=e.target.checked;updateStatsAndGuides();draw();};
 if(bboxInput)bboxInput.onchange=e=>{showBox=e.target.checked;updateStatsAndGuides();draw();};
 if(ghostInput)ghostInput.onchange=e=>{showGhost=e.target.checked;draw();};
 if(turnInput)turnInput.onchange=e=>{turntable=e.target.checked;syncTurntable();};
 if(sunBtn)sunBtn.onclick=()=>{sunAngle=(sunAngle+Math.PI/4)%(Math.PI*2);draw();};
 resetBtn.onclick=()=>{exitDriveCamera();target=[0,0,0];yaw=-.65;pitch=-.35;zoom=.75;draw();};
 const observer=new ResizeObserver(draw);observer.observe(canvas);
 const dispose=()=>{cancelAnimationFrame(rafId);observer.disconnect();delete dialog.setSelectedEntity;delete dialog.focusSelection;delete dialog.setDriveView;textureHandles.forEach(t=>gl.deleteTexture(t));buffers.forEach(b=>gl.deleteBuffer(b));shaders.forEach(s=>gl.deleteShader(s));gl.deleteProgram(program);gl.getExtension('WEBGL_lose_context')?.loseContext();owner?.removeEventListener('close',dispose);activePreviews.delete(dialog);};
 activePreviews.set(dialog,dispose);owner?.addEventListener('close',dispose,{once:true});
 if(options.selectedEntity)setSelection(options.selectedEntity);
 if(driveMode&&data.roadCameras?.length)applyDriveCamera(selectedRoadIdx,true);
 if(turntable)syncTurntable();
 if(!options.embedded&&!inline){dialog.showModal();canvas.focus();}draw();
}
