// The native helper decodes disc textures; WebGL provides inspection lighting.
const activePreviews=new WeakMap();
export async function mountModelPreview(dialog,data,options={}){
 const stale=()=>options.previousPreview&&(!dialog.open||activePreviews.get(dialog)!==options.previousPreview);
 const images=new Map();
 for(const [source,texture]of Object.entries(data.textures||{}))if(texture.png){const img=new Image();img.src='data:image/png;base64,'+texture.png;try{await img.decode();}catch{if(stale())return;throw Error('Could not decode the mod texture PNG');}images.set(Number(source),img);}
 if(stale())return;
 activePreviews.get(dialog)?.();
 dialog.innerHTML='<h2>Model preview</h2><p class="muted">Drag to rotate · Scroll to zoom · Arrow keys to rotate</p><div class="row wrap" aria-label="Model views"><button type="button" data-model-view="front">Front</button><button type="button" data-model-view="side">Side</button><button type="button" data-model-view="rear">Rear</button><button type="button" data-model-view="top">Top</button></div><canvas id="model-preview" tabindex="0" aria-label="Interactive textured model preview" style="width:100%;height:420px;touch-action:none;background:#10151e"></canvas><p class="muted">Textures come from your game image and the selected mod. Lighting is simplified for inspection.</p><footer><label>Background <select id="model-background"><option value="dark">Dark</option><option value="light">Light</option></select></label><label><input type="checkbox" id="model-wire"> Wireframe</label><button type="button" id="model-reset">Reset view</button><button type="button" data-action="close">Close</button></footer>';
 if(data.wholeCar)dialog.querySelector('h2').textContent='Car preview';
 if(data.modName){const note=document.createElement('p');note.className='muted';note.textContent=data.modName+' · Geometry, materials and supported texture overrides from this mod are included. Other assets use the base disc.';dialog.querySelector('h2').after(note);}
 const canvas=dialog.querySelector('canvas'),gl=canvas.getContext('webgl');
 if(!gl)throw Error('WebGL is unavailable. Model preview requires graphics acceleration.');
 const shaders=[];
 function shader(type,source){const s=gl.createShader(type);shaders.push(s);gl.shaderSource(s,source);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s;}
 const program=gl.createProgram();
 gl.attachShader(program,shader(gl.VERTEX_SHADER,`attribute vec3 position;attribute vec3 normal;attribute vec3 color;attribute vec2 uv;uniform vec4 view;uniform float unlit;varying vec3 normalDir;varying vec3 shade;varying vec2 texUV;
 void main(){float a=view.x,b=view.y;mat3 yaw=mat3(cos(a),0.,-sin(a),0.,1.,0.,sin(a),0.,cos(a));mat3 pitch=mat3(1.,0.,0.,0.,cos(b),sin(b),0.,-sin(b),cos(b));vec3 p=pitch*yaw*position;vec3 n=pitch*yaw*normal;gl_Position=vec4(p.x*view.z/view.w,p.y*view.z,p.z*.2,1.);float light=.35+.65*abs(dot(normalize(n),normalize(vec3(.4,-.7,1.))));normalDir=n;shade=color*mix(light,1.,unlit);texUV=uv;}`));
 gl.attachShader(program,shader(gl.FRAGMENT_SHADER,'precision mediump float;varying vec3 normalDir;varying vec3 shade;varying vec2 texUV;uniform sampler2D atlas;uniform bool textured;uniform vec4 factor;uniform vec3 emission;uniform vec2 finish;uniform int alphaMode;void main(){vec4 t=textured?texture2D(atlas,texUV):vec4(1.);float alpha=t.a*factor.a;if(t.a<.01||(alphaMode==1&&alpha<.5))discard;float highlight=pow(abs(dot(normalize(normalDir),normalize(vec3(.2,.3,1.)))),mix(128.,2.,finish.x))*finish.y*.35;gl_FragColor=vec4(t.rgb*shade*factor.rgb+emission+highlight,alphaMode==2?alpha:1.);}'));
 gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));gl.useProgram(program);
 const min=[Infinity,Infinity,Infinity],max=[-Infinity,-Infinity,-Infinity],v=data.vertices;
 for(let i=0;i<v.length;i+=9)for(let a=0;a<3;a++){min[a]=Math.min(min[a],v[i+a]);max[a]=Math.max(max[a],v[i+a]);}
 if(options.comparison&&!options.frame){const original=options.comparison.original.vertices;min.fill(Infinity);max.fill(-Infinity);for(let i=0;i<original.length;i+=9)for(let a=0;a<3;a++){min[a]=Math.min(min[a],original[i+a]);max[a]=Math.max(max[a],original[i+a]);}}
 const frame=options.frame||{scale:2/Math.max(...max.map((x,a)=>x-min[a]),1e-6),center:max.map((x,a)=>(x+min[a])/2)}, {scale,center}=frame;
 function vertex(index,out){const at=index*9,material=v[at+6]>>>0,col=(material&0xffff)===65535?[.15,.16,.18]:((material&0xffff)>>>6)===1?[.10,.16,.2]:[1,1,1];if(data.colors){const untextured=(material&0xffff)===65535;for(let c=0;c<3;c++)col[c]=untextured?data.colors[index*4+c]/255:col[c]*Math.min(data.colors[index*4+c]*2/255,1);}
 for(let a=0;a<3;a++)out.push((v[at+a]-center[a])*scale);out.push(v[at+3],v[at+4],v[at+5],...col,v[at+7],v[at+8]);}
 const triangles=[],lines=[],groups=[],byMaterial=new Map();
 for(let i=0;i<data.indices.length;i+=3){const material=v[data.indices[i]*9+6]>>>0;const list=byMaterial.get(material)||[];list.push(...data.indices.slice(i,i+3));byMaterial.set(material,list);}
 for(const [material,ids]of byMaterial){const start=triangles.length/11;ids.forEach(id=>vertex(id,triangles));groups.push({material,start,count:ids.length});}
 for(let i=0;i<data.indices.length;i+=3){const ids=data.indices.slice(i,i+3);for(let e=0;e<3;e++){vertex(ids[e],lines);vertex(ids[(e+1)%3],lines);}}
 const buffers=[triangles,lines].map(values=>{const b=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,b);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(values),gl.STATIC_DRAW);return b;});
 const attributes=['position','normal','color','uv'].map(name=>gl.getAttribLocation(program,name)),view=gl.getUniformLocation(program,'view');
 const textureHandles=[];const textures=new Map();
 for(const [source,encoded]of Object.entries(data.textures||{})){const img=images.get(Number(source));const bytes=img?null:Uint8Array.from(atob(encoded),c=>c.charCodeAt(0));const t=gl.createTexture();textureHandles.push(t);gl.bindTexture(gl.TEXTURE_2D,t);if(img)gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,img);else gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,256,256,0,gl.RGBA,gl.UNSIGNED_BYTE,bytes);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,img&&((img.width&(img.width-1))||(img.height&(img.height-1)))?gl.CLAMP_TO_EDGE:gl.REPEAT);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,img&&((img.width&(img.width-1))||(img.height&(img.height-1)))?gl.CLAMP_TO_EDGE:gl.REPEAT);textures.set(Number(source),t);}
 const textured=gl.getUniformLocation(program,'textured');gl.uniform1i(gl.getUniformLocation(program,'atlas'),0);
 const materialUniforms=Object.fromEntries(['factor','emission','finish','unlit','alphaMode'].map(name=>[name,gl.getUniformLocation(program,name)]));
 function appearance(source){const p=data.materials?.[source]?.properties?.trim().split(/\s+/);gl.uniform4f(materialUniforms.factor,...(p?p.slice(4,8).map(Number):[1,1,1,1]));gl.uniform3f(materialUniforms.emission,...(p?p.slice(8,11).map(Number):[0,0,0]));gl.uniform2f(materialUniforms.finish,p?Number(p[2]):1,p&&p[0]!=='unlit'?.04+.96*Number(p[3]):0);gl.uniform1f(materialUniforms.unlit,p?.[0]==='unlit'?1:0);const blend=p?.[1]==='blend';gl.uniform1i(materialUniforms.alphaMode,blend?2:p?.[1]==='mask'?1:0);if(blend){gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);}else gl.disable(gl.BLEND);}
 let {yaw=-.65,pitch=-.35,zoom=.75,wire=false,background=[.063,.082,.118]}=options.view||{},drag=null;
 dialog.querySelector('#model-background').value=background[0]>.5?'light':'dark';dialog.querySelector('#model-wire').checked=wire;
 if(options.comparison){
  dialog.querySelector('h2').textContent='Car preview · '+((options.mode||'modified')==='original'?'Original':'Modified');
  const controls=document.createElement('div');controls.className='row model-comparison';controls.setAttribute('role','group');controls.setAttribute('aria-label','Compare car');
  for(const mode of ['original','modified']){const button=document.createElement('button');button.type='button';button.dataset.compare=mode;button.textContent=mode==='original'?'Original':'Modified';button.setAttribute('aria-pressed',String((options.mode||'modified')===mode));if((options.mode||'modified')===mode)button.className='primary';
   button.onclick=async()=>{if((options.mode||'modified')===mode)return;for(const b of controls.querySelectorAll('button'))b.disabled=true;try{await mountModelPreview(dialog,options.comparison[mode],{...options,mode,frame,previousPreview:activePreviews.get(dialog),view:{yaw,pitch,zoom,wire,background}});}catch(error){for(const b of controls.querySelectorAll('button'))b.disabled=false;const message=document.createElement('p');message.setAttribute('role','alert');message.textContent=error.message;controls.after(message);}};controls.append(button);
  }dialog.querySelector('canvas').before(controls);
 }
 function draw(){canvas.width=Math.round(canvas.clientWidth*Math.min(devicePixelRatio,2));canvas.height=Math.round(canvas.clientHeight*Math.min(devicePixelRatio,2));gl.viewport(0,0,canvas.width,canvas.height);gl.clearColor(...background,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);gl.enable(gl.DEPTH_TEST);gl.bindBuffer(gl.ARRAY_BUFFER,buffers[wire?1:0]);attributes.forEach((a,i)=>{gl.enableVertexAttribArray(a);gl.vertexAttribPointer(a,i===3?2:3,gl.FLOAT,false,44,i*12);});gl.uniform4f(view,yaw,pitch,zoom,canvas.width/canvas.height);if(wire){appearance(-1);gl.uniform1i(textured,0);gl.drawArrays(gl.LINES,0,lines.length/11);}else for(const group of groups){appearance((group.material&0xffff)===65535?-1:(group.material&0xffff)%64);const texture=((group.material&0xffff)>>>6)===1?null:textures.get((group.material&0xffff)%64);gl.uniform1i(textured,texture?1:0);gl.bindTexture(gl.TEXTURE_2D,texture||null);gl.drawArrays(gl.TRIANGLES,group.start,group.count);}}
 canvas.onpointerdown=e=>{drag=[e.clientX,e.clientY];canvas.setPointerCapture(e.pointerId);canvas.focus();};
 canvas.onpointermove=e=>{if(!drag)return;yaw+=(e.clientX-drag[0])*.01;pitch+=(e.clientY-drag[1])*.01;drag=[e.clientX,e.clientY];draw();};
 canvas.onpointerup=canvas.onpointercancel=()=>{drag=null;};
 canvas.addEventListener('wheel',e=>{e.preventDefault();zoom=Math.max(.15,Math.min(5,zoom*Math.exp(-e.deltaY*.001)));draw();},{passive:false});
 canvas.onkeydown=e=>{if(!['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','+','-'].includes(e.key))return;e.preventDefault();yaw+=e.key==='ArrowLeft'?-.1:e.key==='ArrowRight'?.1:0;pitch+=e.key==='ArrowUp'?-.1:e.key==='ArrowDown'?.1:0;zoom=Math.max(.15,Math.min(5,zoom*(e.key==='+'?1.1:e.key==='-'?.9:1)));draw();};
 for(const button of dialog.querySelectorAll('[data-model-view]'))button.onclick=()=>{const preset={front:[Math.PI,0],side:[Math.PI/2,0],rear:[0,0],top:[0,-Math.PI/2]}[button.dataset.modelView];[yaw,pitch]=preset;draw();};
 dialog.querySelector('#model-background').onchange=e=>{background=e.target.value==='light'?[.78,.8,.83]:[.063,.082,.118];draw();};
 dialog.querySelector('#model-wire').onchange=e=>{wire=e.target.checked;draw();};dialog.querySelector('#model-reset').onclick=()=>{yaw=-.65;pitch=-.35;zoom=.75;draw();};
 const observer=new ResizeObserver(draw);observer.observe(canvas);
 const dispose=()=>{observer.disconnect();textureHandles.forEach(t=>gl.deleteTexture(t));buffers.forEach(b=>gl.deleteBuffer(b));shaders.forEach(s=>gl.deleteShader(s));gl.deleteProgram(program);gl.getExtension('WEBGL_lose_context')?.loseContext();dialog.removeEventListener('close',dispose);activePreviews.delete(dialog);};
 activePreviews.set(dialog,dispose);dialog.addEventListener('close',dispose,{once:true});
 dialog.showModal();draw();canvas.focus();
}
