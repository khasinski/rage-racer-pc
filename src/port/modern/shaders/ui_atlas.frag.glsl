#version 450
// New RGBA artwork supplies colour/coverage. Original VRAM supplies only the
// per-texel PS1 STP bit, so replacing art retains mixed opaque/semi behaviour.
layout(set=2,binding=0) uniform sampler2D atlas;
layout(set=2,binding=1) uniform sampler2D vram;
layout(set=3,binding=0,std140) uniform UiSource { uvec4 source; vec4 region; } original;
layout(location=0) in vec2 uv;
layout(location=1) in vec4 tint;
layout(location=0) out vec4 fragColor;
uint word16(vec4 c){uvec4 v=uvec4(c*vec4(31,31,31,1)+0.5);return v.r|(v.g<<5u)|(v.b<<10u)|(v.a<<15u);}
bool semiTexel(uint page,uint palette){
 uvec2 texel=uvec2(clamp(floor(original.region.xy+uv*original.region.zw),vec2(0),vec2(255)));
 uvec2 base=uvec2((page&15u)*64u,((page>>4u)&1u)*256u);
 uint depth=(page>>7u)&3u;
 uint packed=word16(texelFetch(vram,ivec2(base+uvec2(texel.x>>(depth==1u?1u:2u),texel.y)),0));
 uint index=depth==1u?((packed>>((texel.x&1u)*8u))&255u):((packed>>((texel.x&3u)*4u))&15u);
 uvec2 clut=uvec2((palette&63u)*16u,(palette>>6u)&511u);
 return texelFetch(vram,ivec2(clut+uvec2(index,0)),0).a>0.5;
}
void main(){
 vec4 c=texture(atlas,uv)*tint;
 if(c.a<=0.0)discard;
 if(original.source.z!=0u&&semiTexel(original.source.x,original.source.y)){
  uint abr=(original.source.x>>5u)&3u;
  if(abr==0u)c*=0.5;
  else if(abr==1u)c.a=0.0;
  else if(abr==3u){c.rgb*=0.25;c.a=0.0;}
 }
 fragColor=c;
}
