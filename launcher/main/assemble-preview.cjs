// Geometry is decoded by the native helper. Keep per-part texture sources
// distinct when joining the neutral body and suspension instances.
module.exports=function assemble(layout,parts){
 const result={vertices:[],colors:[],indices:[],textures:{},materials:{},wholeCar:true};
 const sources=new Map();
 for(const instance of layout.instances){
  const mesh=parts.get(instance.part),base=result.vertices.length/9;
  for(let i=0;i<mesh.vertices.length;i+=9){
   const vertex=mesh.vertices.slice(i,i+9),word=vertex[6]>>>0,id=word&65535;
   for(let axis=0;axis<3;axis++){
    vertex[axis]=vertex[axis]*instance.scale[axis]+instance.position[axis];
    vertex[axis+3]*=instance.scale[axis];
   }
   if(id!==65535){
    const source=id%64,key=instance.part+':'+source;
    if(!sources.has(key)){
     if(sources.size>=64)throw Error('Car preview exceeds the texture source limit');
     const target=sources.size;sources.set(key,target);
     if(mesh.textures[source])result.textures[target]=mesh.textures[source];
     if(mesh.materials?.[source])result.materials[target]=mesh.materials[source];
    }
    vertex[6]=((word&0xffff0000)|(id-source+sources.get(key)))>>>0;
   }
   result.vertices.push(...vertex);
   result.colors.push(...(mesh.colors?.slice(i/9*4,i/9*4+4)||[128,128,128,255]));
  }
  for(const index of mesh.indices)result.indices.push(base+index);
 }
 return result;
};
