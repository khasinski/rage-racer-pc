// Imported vertices use renderer axes and GTE scale; placements use world
// units. Expand only referenced submeshes, keeping source banks distinct.
module.exports=function assembleTrack(banks){
 const out={worldScene:true,vertices:[],colors:[],indices:[],textures:{},materials:{},entities:[]},materials=new Map();
 for(const [name,{geometry,instances}]of Object.entries(banks))for(const instance of instances){
  const range=geometry.ranges[instance.mesh];if(!range)throw Error('Unknown track preview model');
  const [first,count]=range,vertices=new Map(),start=out.indices.length;
  const angle=instance.yaw*Math.PI*2/4096,c=Math.cos(angle),s=Math.sin(angle);
  for(const index of geometry.indices.slice(first,first+count)){
   if(!vertices.has(index)){
    if(out.vertices.length/9>=500000)throw Error('Track preview exceeds geometry limit');
    const at=index*9,v=geometry.vertices.slice(at,at+9);if(v.length!==9)throw Error('Invalid track preview index');
    const x=v[0]*0.25,z=v[2]*0.25,nx=v[3],nz=v[5];
    v[0]=c*x-s*z+instance.position[0];v[1]=v[1]*0.25+instance.position[1];v[2]=s*x+c*z+instance.position[2];
    v[3]=c*nx-s*nz;v[5]=s*nx+c*nz;
    const word=v[6]>>>0,id=word&65535,key=name+':'+id;
    if(id!==65535){
     if(!materials.has(key)){
      const target=materials.size;materials.set(key,target);
      if(geometry.textures?.[id])out.textures[target]=geometry.textures[id];
      if(geometry.materials?.[id])out.materials[target]=geometry.materials[id];
     }
     v[6]=((word&0xffff0000)|materials.get(key))>>>0;
    }
    vertices.set(index,out.vertices.length/9);out.vertices.push(...v);
    out.colors.push(...geometry.colors.slice(index*4,index*4+4));
   }
   out.indices.push(vertices.get(index));
  }
  out.entities.push({bank:name,mesh:instance.mesh,object:instance.object,firstIndex:start,indexCount:count});
 }
 return out;
};
