const escape=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
export function materialForm(key,properties,revision){
 const values=properties.split(/\s+/),labels=['Shading','Transparency','Roughness','Metallic','Red','Green','Blue','Opacity','Emission red','Emission green','Emission blue'];
 const control=i=>`<label for="property-${i}">${labels[i]}${i<2?`<select id="property-${i}" data-property="${i}">${(i===0?['inherit','lit','unlit']:['auto','opaque','mask','blend']).map(v=>`<option ${values[i]===v?'selected':''}>${v}</option>`).join('')}</select>`:`<input id="property-${i}" data-property="${i}" type="number" min="0" max="1" step="any" value="${escape(values[i])}" required>`}</label>`;
 return `<h2>Color & finish</h2><form id="material-form" data-key="${escape(key)}" data-revision="${revision}"><p>Roughness: 0 = smooth, 1 = rough. Metallic: 0 = non-metal, 1 = metal. Color tints the texture. Opacity: 0 = invisible, 1 = solid.</p>${[2,3,4,5,6,7].map(control).join('')}<details><summary>Advanced lighting & transparency</summary><p>Emission brightens the surface; it does not create a headlight beam.</p>${[0,1,8,9,10].map(control).join('')}</details><details><summary>Technical identity</summary><input id="material-key" readonly value="${escape(key)}"></details><footer><button type="button" data-action="close">Cancel</button><button type="submit">Save material to project</button></footer></form>`;
}
export function installMaterialEditor({request,onChanged,onError,onBegin=async()=>{},onEnd=()=>{}}){
 const dialog=()=>document.querySelector('#dialog');
 document.addEventListener('click',e=>{
  const b=e.target.closest('[data-project-material],[data-reset-track-material]');if(!b)return;e.preventDefault();e.stopImmediatePropagation();
  const host=b.closest('[data-revision]'),revision=Number(host.dataset.revision);
  if(b.dataset.projectMaterial){dialog().innerHTML=materialForm(b.dataset.projectMaterial,b.dataset.materialValue,revision);return;}
  const id=b.dataset.resetTrackMaterial;
  Promise.resolve().then(onBegin).then(()=>request('editProject',['car','track'].flatMap(kind=>[id,id+'.variant.0'].map(id=>({kind,id,field:'material',value:null}))),revision)).then(async p=>{dialog().close();await onChanged(p);}).catch(onError).finally(onEnd);
 },true);
 document.addEventListener('submit',e=>{
  if(e.target.id!=='material-form')return;e.preventDefault();e.stopImmediatePropagation();const form=e.target;if(!form.reportValidity())return;
  const properties=[...form.querySelectorAll('[data-property]')].sort((a,b)=>Number(a.dataset.property)-Number(b.dataset.property)).map(i=>i.value).join(' ');
  Promise.resolve().then(onBegin).then(()=>request('saveMaterial','',form.dataset.key,properties,Number(form.dataset.revision))).then(async p=>{dialog().close();await onChanged(p);}).catch(onError).finally(onEnd);
 },true);
}
