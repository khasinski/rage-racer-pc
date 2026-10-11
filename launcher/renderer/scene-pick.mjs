// Project triangles with the same orthographic view as the inspection shader.
// Test terrain too: an object behind the road must not win the selection.
export function pickScene(data,frame,view,x,y){
 const {yaw,pitch,zoom,aspect}=view,c=Math.cos(yaw),s=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);
 const projected=new Map();
 const project=index=>{
  if(projected.has(index))return projected.get(index);
  const at=index*9,p=[0,1,2].map(a=>(data.vertices[at+a]-frame.center[a])*frame.scale);
  const rx=c*p[0]+s*p[2],rz=-s*p[0]+c*p[2];
  const point=[rx*zoom/aspect,(cp*p[1]-sp*rz)*zoom,(sp*p[1]+cp*rz)*.2];projected.set(index,point);return point;
 };
 let nearest=Infinity,hit=null;
 for(const entity of data.entities||[])for(let i=entity.firstIndex;i<entity.firstIndex+entity.indexCount;i+=3){
  const [a,b,d]=data.indices.slice(i,i+3).map(project);
  const det=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1]);
  if(Math.abs(det)<1e-12)continue;
  const u=((b[1]-d[1])*(x-d[0])+(d[0]-b[0])*(y-d[1]))/det;
  const v=((d[1]-a[1])*(x-d[0])+(a[0]-d[0])*(y-d[1]))/det,w=1-u-v;
  if(u<0||v<0||w<0)continue;
  const depth=u*a[2]+v*b[2]+w*d[2];
  if(depth>=-1&&depth<=1&&depth<nearest){nearest=depth;hit=entity;}
 }
 return hit?.bank==='course'&&Number.isInteger(hit.object)?hit.object:null;
}
