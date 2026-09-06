// Pixel editing stays in UI coordinates. The C save library owns the packed
// nibble layout and PlayStation palette conversion.
export function mountLogo(container,source,onApply,onCancel,onDirty=()=>{}){
 const pixels=source.pixels.split(''),colors=source.colors.map(c=>({...c}));
 let selected=1,last=null,stroke=null;const undo=[];
 container.innerHTML=`<h2>Team logo</h2><p class="muted">64 × 64 pixels · Drag to paint. Right-click to pick a color.</p><canvas id="logo-canvas" width="64" height="64" tabindex="0" aria-label="Team logo. Arrow keys move the cursor; Space paints the selected color."></canvas><div id="logo-swatches" class="toolbar"></div><div class="row wrap"><label>Color <input id="logo-color" type="color"></label><label class="row"><input id="logo-transparent" type="checkbox"> Semi-transparent</label></div><p id="logo-position" class="muted" role="status">Pixel 0, 0</p><footer><button type="button" id="logo-undo">Undo stroke</button><button type="button" id="logo-clear">Clear</button><button type="button" id="logo-cancel">Cancel</button><button type="button" id="logo-apply" class="primary">Apply to save</button></footer>`;
 const canvas=container.querySelector('canvas'),ctx=canvas.getContext('2d');let cursor={x:0,y:0};
 const el=id=>container.querySelector('#'+id);
 function history(){undo.push({pixels:pixels.join(''),colors:colors.map(c=>({...c}))});if(undo.length>64)undo.shift();}
 function draw(){
  for(let y=0;y<64;y++)for(let x=0;x<64;x++){
   const c=colors[parseInt(pixels[y*64+x],16)];ctx.globalAlpha=1;ctx.fillStyle=(x+y)%2?'#545b64':'#242c36';ctx.fillRect(x,y,1,1);ctx.globalAlpha=c.transparent?.5:1;ctx.fillStyle=c.rgb;ctx.fillRect(x,y,1,1);
  }ctx.globalAlpha=1;el('logo-undo').disabled=!undo.length;
  onDirty(pixels.join('')!==source.pixels||colors.some((c,i)=>c.rgb!==source.colors[i].rgb||c.transparent!==source.colors[i].transparent));
 }
 function palette(){el('logo-swatches').replaceChildren();colors.forEach((c,i)=>{const button=document.createElement('button');button.type='button';button.className='logo-swatch'+(selected===i?' selected':'');button.style.background=c.rgb;button.textContent=String(i);button.setAttribute('aria-label','Paint with palette color '+i);button.setAttribute('aria-pressed',String(selected===i));button.onclick=()=>{selected=i;palette();};el('logo-swatches').append(button);});el('logo-color').value=colors[selected].rgb;el('logo-transparent').checked=colors[selected].transparent;}
 function point(e){const r=canvas.getBoundingClientRect();return{x:Math.max(0,Math.min(63,Math.floor((e.clientX-r.left)*64/r.width))),y:Math.max(0,Math.min(63,Math.floor((e.clientY-r.top)*64/r.height)))};}
 function paint(to){const from=last||to,steps=Math.max(Math.abs(to.x-from.x),Math.abs(to.y-from.y));for(let i=0;i<=steps;i++){const t=steps?i/steps:0;pixels[Math.round(from.y+(to.y-from.y)*t)*64+Math.round(from.x+(to.x-from.x)*t)]=selected.toString(16);}last=to;cursor=to;el('logo-position').textContent=`Pixel ${to.x}, ${to.y}`;draw();}
 canvas.addEventListener('contextmenu',e=>e.preventDefault());
 canvas.addEventListener('pointerdown',e=>{e.preventDefault();canvas.focus();const p=point(e);if(e.button===2){selected=parseInt(pixels[p.y*64+p.x],16);palette();return;}if(e.button!==0)return;history();stroke=e.pointerId;last=null;canvas.setPointerCapture(e.pointerId);paint(p);});
 canvas.addEventListener('pointermove',e=>{if(stroke===e.pointerId)paint(point(e));});
 for(const name of ['pointerup','pointercancel','lostpointercapture'])canvas.addEventListener(name,()=>{stroke=null;last=null;});
 canvas.addEventListener('keydown',e=>{const delta={ArrowLeft:[-1,0],ArrowRight:[1,0],ArrowUp:[0,-1],ArrowDown:[0,1]}[e.key];if(delta){e.preventDefault();cursor={x:Math.max(0,Math.min(63,cursor.x+delta[0])),y:Math.max(0,Math.min(63,cursor.y+delta[1]))};el('logo-position').textContent=`Pixel ${cursor.x}, ${cursor.y}`;}else if(e.key===' '){e.preventDefault();history();last=null;paint(cursor);last=null;}});
 el('logo-color').addEventListener('change',e=>{history();colors[selected].rgb=e.target.value;palette();draw();});
 el('logo-transparent').addEventListener('change',e=>{history();colors[selected].transparent=e.target.checked;draw();});
 el('logo-undo').onclick=()=>{const prev=undo.pop();if(!prev)return;pixels.splice(0,pixels.length,...prev.pixels);colors.splice(0,colors.length,...prev.colors);palette();draw();};
 el('logo-clear').onclick=()=>{history();pixels.fill('0');draw();};
 el('logo-cancel').onclick=onCancel;el('logo-apply').onclick=()=>onApply({pixels:pixels.join(''),colors});
 palette();draw();
}
