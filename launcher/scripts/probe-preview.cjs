// Serialized into the packaged renderer by probe-package.cjs. The triangle and
// one-pixel PNG are synthetic; no retail game data or unlocked IPC is needed.
module.exports=async function probePreview(){
 const {mountModelPreview}=await import('./model-preview.js');
 const dialog=document.querySelector('#dialog');
 const modified={vertices:[0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1],indices:[0,1,2],textures:{}};
 const original={...modified,textures:{0:{png:'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLttAAAAABJRU5ErkJggg=='}}};
 const check=(condition,message)=>{if(!condition)throw Error(message);};
 const until=async predicate=>{for(let i=0;i<100;i++){if(predicate())return;await new Promise(r=>setTimeout(r,20));}throw Error('Preview state did not settle');};
 const camera=()=>{const gl=dialog.querySelector('canvas').getContext('webgl');check(!gl.isContextLost(),'Active preview context lost');const program=gl.getParameter(gl.CURRENT_PROGRAM);return JSON.stringify(Array.from(gl.getUniform(program,gl.getUniformLocation(program,'view'))));};
 await mountModelPreview(dialog,modified,{comparison:{original,modified},mode:'modified'});
 dialog.querySelector('[data-model-view=side]').click();
 const before=camera();
 for(let i=0;i<20;i++){
  const mode=i%2?'modified':'original';dialog.querySelector('[data-compare='+mode+']').click();
  await until(()=>dialog.querySelector('[data-compare='+mode+']')?.getAttribute('aria-pressed')==='true');
  check(camera()===before,'Comparison changed the camera');
 }
 const decode=Image.prototype.decode;let release;
 try{
  Image.prototype.decode=function(){return new Promise((resolve,reject)=>{release=()=>decode.call(this).then(resolve,reject);});};
  dialog.querySelector('[data-compare=original]').click();
  await until(()=>!!release);
  dialog.close();await release();await new Promise(r=>setTimeout(r,100));
  check(!dialog.open,'Delayed texture decode reopened the closed preview');
 }finally{Image.prototype.decode=decode;if(dialog.open)dialog.close();}
 return true;
};
