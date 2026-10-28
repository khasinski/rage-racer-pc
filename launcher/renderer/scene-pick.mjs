// Project triangles with the same orthographic view as the inspection shader.
// Test terrain too: an object behind the road must not win the selection.
export function pickScene(data,frame,view,x,y){
 const {yaw,pitch,zoom,aspect,target=[0,0,0]}=view,c=Math.cos(yaw),s=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);
 const projected=new Map();
 const cx=frame.center[0],cy=frame.center[1],cz=frame.center[2],sc=frame.scale;
 const tx=target[0]||0,ty=target[1]||0,tz=target[2]||0;
 const project=index=>{
  let point=projected.get(index);
  if(point!==undefined)return point;
  const at=index*9;
  const px=(data.vertices[at]-cx)*sc-tx,py=(data.vertices[at+1]-cy)*sc-ty,pz=(data.vertices[at+2]-cz)*sc-tz;
  const rx=c*px+s*pz,rz=-s*px+c*pz;
  point=[rx*zoom/aspect,(cp*py-sp*rz)*zoom,(sp*py+cp*rz)*.2];
  projected.set(index,point);
  return point;
 };
 let nearest=Infinity,hit=null,nearestRoad=Infinity,hitRoad=null;
 for(const entity of data.entities||[])for(let i=entity.firstIndex;i<entity.firstIndex+entity.indexCount;i+=3){
  const a=project(data.indices[i]),b=project(data.indices[i+1]),d=project(data.indices[i+2]);
  if((x<a[0]&&x<b[0]&&x<d[0])||(x>a[0]&&x>b[0]&&x>d[0])||(y<a[1]&&y<b[1]&&y<d[1])||(y>a[1]&&y>b[1]&&y>d[1]))continue;
  const det=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1]);
  if(Math.abs(det)<1e-12)continue;
  const u=((b[1]-d[1])*(x-d[0])+(d[0]-b[0])*(y-d[1]))/det;
  const v=((d[1]-a[1])*(x-d[0])+(a[0]-d[0])*(y-d[1]))/det,w=1-u-v;
  if(u<-1e-5||v<-1e-5||w<-1e-5)continue;
  const depth=u*a[2]+v*b[2]+w*d[2];
  if(depth>=-1&&depth<=1){
   if(depth<nearest){nearest=depth;hit=entity;}
   if(entity.bank==='road'&&depth<nearestRoad){nearestRoad=depth;hitRoad=entity;}
  }
 }
 if(hitRoad&&(!hit||hit.bank==='terrain'&&nearestRoad<=nearest+0.04))return hitRoad;
 return hit;
}

// Invert the inspection projection on the horizontal world plane. A level
// side view cannot determine ground depth; reject it instead of guessing.
export function groundDrag(frame,view,dx,dy){
 const sp=Math.sin(view.pitch);
 if(Math.abs(sp)<.05)throw Error('Use the top view or tilt the view before moving an object');
 const rx=dx*view.aspect/(view.zoom*frame.scale),rz=-dy/(sp*view.zoom*frame.scale);
 const c=Math.cos(view.yaw),s=Math.sin(view.yaw);
 return {x:Math.round(c*rx-s*rz),z:Math.round(s*rx+c*rz)};
}
