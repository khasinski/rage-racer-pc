#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif
/* Compiled, bounded static-part interchange for car and track models.
 * Car texture banks use modulo-64 sources; tracks retain full material IDs.
 * Whole-car assembly preserves separate wheel instances and pivots. */
#define CGLTF_IMPLEMENTATION
#include "../external/cgltf/cgltf.h"
#include "gltf.h"
#include "render/rmesh.h"
#include "yyjson.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL3/SDL.h>

enum { LIMIT=128*1024*1024, MAX_VERTICES=2000000, MAX_PRIMITIVES=10000 };
static void *Load(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");void *p=NULL;long n;
 if(!f)return NULL;
 if(!fseek(f,0,SEEK_END)&&(n=ftell(f))>0&&n<=LIMIT&&!fseek(f,0,SEEK_SET)){
  p=malloc((size_t)n);if(p&&fread(p,1,(size_t)n,f)!=(size_t)n){free(p);p=NULL;}if(p)*size=(size_t)n;
 }
 fclose(f);return p;
}
static void Put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));}
static int Write(const char *path,const void *data,size_t size){
 FILE *f=fopen(path,"wbx");if(!f)return 0;int ok=fwrite(data,1,size,f)==size;if(fclose(f))ok=0;if(!ok)remove(path);return ok;
}
static void *Allocate(void *user,cgltf_size size){size_t *remaining=user;if(size>*remaining)return NULL;*remaining-=size;return malloc(size);}
static void Release(void *user,void *p){(void)user;free(p);}
static yyjson_mut_val *Object(yyjson_mut_doc *d,yyjson_mut_val *parent,const char *key){yyjson_mut_val *v=yyjson_mut_obj(d);yyjson_mut_obj_add_val(d,parent,key,v);return v;}
static yyjson_mut_val *Array(yyjson_mut_doc *d,yyjson_mut_val *parent,const char *key){yyjson_mut_val *v=yyjson_mut_arr(d);yyjson_mut_obj_add_val(d,parent,key,v);return v;}
static void Number(yyjson_mut_doc *d,yyjson_mut_val *obj,const char *key,uint32_t n){yyjson_mut_obj_add_uint(d,obj,key,n);}
static void Text(yyjson_mut_doc *d,yyjson_mut_val *obj,const char *key,const char *s){yyjson_mut_obj_add_strcpy(d,obj,key,s);}
static yyjson_mut_val *Element(yyjson_mut_doc *d,yyjson_mut_val *array){yyjson_mut_val *v=yyjson_mut_obj(d);yyjson_mut_arr_add_val(array,v);return v;}

static int ExportGlb(const char *input,unsigned part,const char *output,const char *textureDirectory,int track){
 size_t size=0,jsonSize=0;unsigned char *source=Load(input,&size),*binary=NULL,*file=NULL;char *json=NULL;yyjson_mut_doc *doc=NULL;int ok=0;
 RageRuntimeMesh mesh;uint32_t first,count;
 if(!source||!RuntimeMeshOpen(&mesh,source,size)||!RuntimeMeshRange(&mesh,part,&first,&count)||!count||count>MAX_VERTICES||count%3)goto done;
 binary=malloc((size_t)count*48);doc=yyjson_mut_doc_new(NULL);if(!binary||!doc)goto done;
 yyjson_mut_val *root=yyjson_mut_obj(doc);yyjson_mut_doc_set_root(doc,root);
 yyjson_mut_val *asset=Object(doc,root,"asset");Text(doc,asset,"version","2.0");Text(doc,asset,"generator","Rage Racer Editor");Text(doc,Object(doc,asset,"extras"),"rr_units","native Y-up");
 yyjson_mut_val *buffers=Array(doc,root,"buffers"),*views=Array(doc,root,"bufferViews"),*accessors=Array(doc,root,"accessors"),*materials=Array(doc,root,"materials"),*meshes=Array(doc,root,"meshes"),*nodes=Array(doc,root,"nodes"),*scenes=Array(doc,root,"scenes");
 Number(doc,Element(doc,buffers),"byteLength",count*48);yyjson_mut_val *view=Element(doc,views);Number(doc,view,"buffer",0);Number(doc,view,"byteLength",count*48);Number(doc,view,"byteStride",48);Number(doc,view,"target",34962);
 yyjson_mut_val *model=Element(doc,meshes),*primitives=Array(doc,model,"primitives");Text(doc,model,"name","part");yyjson_mut_val *node=Element(doc,nodes);Number(doc,node,"mesh",0);Text(doc,node,"name","part");yyjson_mut_arr_add_uint(doc,Array(doc,Element(doc,scenes),"nodes"),0);Number(doc,root,"scene",0);
 size_t binarySize=(size_t)count*48;uint32_t textureSlots[64],textureIds[64],textureCount=0;for(unsigned i=0;i<64;i++)textureSlots[i]=UINT32_MAX;
 yyjson_mut_val *images=Array(doc,root,"images"),*textures=Array(doc,root,"textures"),*samplers=Array(doc,root,"samplers");
 yyjson_mut_val *sampler=Element(doc,samplers);Number(doc,sampler,"magFilter",9728);Number(doc,sampler,"minFilter",9728);Number(doc,sampler,"wrapS",10497);Number(doc,sampler,"wrapT",10497);
 uint32_t words[MAX_PRIMITIVES],runs=0,wordCount=0;
 for(uint32_t i=0;i<count;){
  uint32_t word=0,runEnd=i;
  float low[3]={INFINITY,INFINITY,INFINITY},high[3]={-INFINITY,-INFINITY,-INFINITY};
  do{
   uint32_t triangleWord=0;
   for(unsigned j=0;j<3;j++){
    uint32_t index;RageRuntimeVertex v;unsigned char encoded[40];
    if(!RuntimeMeshIndex(&mesh,first+runEnd+j,&index)||!RuntimeMeshVertex(&mesh,index,&v))goto done;
    if(j&&v.material!=triangleWord)goto done;
    triangleWord=v.material;
    if(!RuntimeVertexEncode(encoded,sizeof(encoded),&v))goto done;
    memcpy(binary+(size_t)(runEnd+j)*48,encoded,36);
    memcpy(binary+(size_t)(runEnd+j)*48+36,encoded+12,12);
    RageRuntimeVertex standard=v;double length=sqrt((double)v.normal[0]*v.normal[0]+(double)v.normal[1]*v.normal[1]+(double)v.normal[2]*v.normal[2]);
    for(unsigned n=0;n<3;n++)standard.normal[n]=length>1e-20f?v.normal[n]/length:(n==1?1.0f:0.0f);
    if(!RuntimeVertexEncode(encoded,sizeof(encoded),&standard))goto done;
    memcpy(binary+(size_t)(runEnd+j)*48+12,encoded+12,12);
    for(unsigned c=0;c<3;c++){if(v.position[c]<low[c])low[c]=v.position[c];if(v.position[c]>high[c])high[c]=v.position[c];}
   }
   if(runEnd==i)word=triangleWord;
   else if(triangleWord!=word)break;
   runEnd+=3;
  }while(runEnd<count);
  // Bounds are recomputed for the accepted run: the lookahead can differ.
  for(unsigned c=0;c<3;c++){low[c]=INFINITY;high[c]=-INFINITY;}
  for(uint32_t k=i;k<runEnd;k++){uint32_t index;RageRuntimeVertex v;if(!RuntimeMeshIndex(&mesh,first+k,&index)||!RuntimeMeshVertex(&mesh,index,&v))goto done;for(unsigned c=0;c<3;c++){if(v.position[c]<low[c])low[c]=v.position[c];if(v.position[c]>high[c])high[c]=v.position[c];}}
  if(++runs>MAX_PRIMITIVES)goto done;
  uint32_t material=0;while(material<wordCount&&words[material]!=word)material++;
  if(material==wordCount){if(wordCount==MAX_PRIMITIVES)goto done;words[wordCount++]=word;yyjson_mut_val *m=Element(doc,materials);char name[48];snprintf(name,sizeof(name),"rage_%u",word);Text(doc,m,"name",name);Number(doc,Object(doc,m,"extras"),"rr_material",word);yyjson_mut_val *pbr=Object(doc,m,"pbrMetallicRoughness");yyjson_mut_obj_add_real(doc,pbr,"metallicFactor",0);yyjson_mut_obj_add_real(doc,pbr,"roughnessFactor",1);
   unsigned id=word&65535,sourceId=track?id:id%64;
   if(textureDirectory&&id!=65535&&(track||id/64!=1)){
    unsigned slot=0;while(slot<textureCount&&textureIds[slot]!=sourceId)slot++;if(slot==textureCount){if(textureCount==64)goto done;textureIds[textureCount++]=sourceId;}
    if(textureSlots[slot]==UINT32_MAX){
     char filename[4096];int length=snprintf(filename,sizeof(filename),"%s/texture-%u.png",textureDirectory,sourceId);size_t imageSize=0;unsigned char *image=NULL;
     if(length<0||length>=(int)sizeof(filename)||(image=Load(filename,&imageSize))==NULL)goto done;
     if(imageSize<24||imageSize>32u*1024u*1024u||memcmp(image,"\x89PNG\r\n\x1a\n",8)||memcmp(image+12,"IHDR",4)){free(image);goto done;}
     uint32_t w=(uint32_t)image[16]<<24|(uint32_t)image[17]<<16|(uint32_t)image[18]<<8|image[19],h=(uint32_t)image[20]<<24|(uint32_t)image[21]<<16|(uint32_t)image[22]<<8|image[23];if(!w||!h||w>4096||h>4096){free(image);goto done;}
     size_t padded=(imageSize+3)&~(size_t)3;if(padded>LIMIT-binarySize){free(image);goto done;}unsigned char *next=realloc(binary,binarySize+padded);if(!next){free(image);goto done;}binary=next;memcpy(binary+binarySize,image,imageSize);memset(binary+binarySize+imageSize,0,padded-imageSize);free(image);
     uint32_t viewIndex=(uint32_t)yyjson_mut_arr_size(views),imageIndex=(uint32_t)yyjson_mut_arr_size(images);yyjson_mut_val *v=Element(doc,views);Number(doc,v,"buffer",0);Number(doc,v,"byteOffset",(uint32_t)binarySize);Number(doc,v,"byteLength",(uint32_t)imageSize);
     yyjson_mut_val *img=Element(doc,images);Number(doc,img,"bufferView",viewIndex);Text(doc,img,"mimeType","image/png");textureSlots[slot]=(uint32_t)yyjson_mut_arr_size(textures);yyjson_mut_val *tex=Element(doc,textures);Number(doc,tex,"source",imageIndex);Number(doc,tex,"sampler",0);binarySize+=padded;
    }
    Number(doc,Object(doc,pbr,"baseColorTexture"),"index",textureSlots[slot]);
   }
  }
  yyjson_mut_val *primitive=Element(doc,primitives),*attributes=Object(doc,primitive,"attributes");Number(doc,primitive,"mode",4);Number(doc,primitive,"material",material);
  const char *names[]={"POSITION","NORMAL","COLOR_0","TEXCOORD_0","_RR_NORMAL"},*types[]={"VEC3","VEC3","VEC4","VEC2","VEC3"};unsigned offsets[]={0,12,24,28,36};
  for(unsigned c=0;c<5;c++){
   Number(doc,attributes,names[c],(runs-1)*5+c);yyjson_mut_val *a=Element(doc,accessors);Number(doc,a,"bufferView",0);Number(doc,a,"byteOffset",i*48+offsets[c]);Number(doc,a,"componentType",c==2?5121:5126);Number(doc,a,"count",runEnd-i);Text(doc,a,"type",types[c]);
   if(c==2)yyjson_mut_obj_add_bool(doc,a,"normalized",true);
   if(c==0){yyjson_mut_val *min=Array(doc,a,"min"),*max=Array(doc,a,"max");for(unsigned n=0;n<3;n++){yyjson_mut_arr_add_real(doc,min,low[n]);yyjson_mut_arr_add_real(doc,max,high[n]);}}
  }
  i=runEnd;
 }
 if(!yyjson_mut_arr_size(images)){yyjson_mut_obj_remove_str(root,"images");yyjson_mut_obj_remove_str(root,"textures");yyjson_mut_obj_remove_str(root,"samplers");}
 yyjson_mut_obj_remove_str(yyjson_mut_arr_get(buffers,0),"byteLength");Number(doc,yyjson_mut_arr_get(buffers,0),"byteLength",(uint32_t)binarySize);
 json=yyjson_mut_write(doc,0,&jsonSize);if(!json||jsonSize>8*1024*1024)goto done;
 size_t padded=(jsonSize+3)&~(size_t)3,total=12+8+padded+8+binarySize;if(total>LIMIT)goto done;
 file=malloc(total);if(!file)goto done;Put32(file,0x46546c67);Put32(file+4,2);Put32(file+8,(uint32_t)total);Put32(file+12,(uint32_t)padded);Put32(file+16,0x4e4f534a);memcpy(file+20,json,jsonSize);memset(file+20+jsonSize,' ',padded-jsonSize);Put32(file+20+padded,(uint32_t)binarySize);Put32(file+24+padded,0x004e4942);memcpy(file+28+padded,binary,binarySize);ok=Write(output,file,total);
 done:free(source);free(binary);free(file);free(json);yyjson_mut_doc_free(doc);if(!ok)fputs("Cannot export GLB geometry (invalid mesh, excessive primitives or existing output)\n",stderr);return ok;
}

static int Material(const cgltf_material *m,uint32_t *word){
 if(!m)return 0;
 if(m->extras.data){yyjson_doc *d=yyjson_read(m->extras.data,strlen(m->extras.data),0);yyjson_val *v=d?yyjson_obj_get(yyjson_doc_get_root(d),"rr_material"):NULL;
  double numeric=yyjson_get_num(v);int valid=yyjson_is_num(v)&&isfinite(numeric)&&numeric>=0&&numeric<=UINT32_MAX&&floor(numeric)==numeric;if(valid)*word=(uint32_t)numeric;yyjson_doc_free(d);if(valid)return 1;
 }
 if(m->name&&strncmp(m->name,"rage_",5)==0){const char *p=m->name+5;uint32_t n=0;if(!*p)return 0;for(;*p;p++){if(*p<'0'||*p>'9'||n>UINT32_MAX/10||(n==UINT32_MAX/10&&(unsigned)(*p-'0')>UINT32_MAX%10))return 0;n=n*10+(unsigned)(*p-'0');}*word=n;return 1;}
 return 0;
}
static const cgltf_accessor *Attribute(const cgltf_primitive *p,cgltf_attribute_type type,int index,const char *name){
 const cgltf_accessor *out=NULL;
 for(size_t i=0;i<p->attributes_count;i++){const cgltf_attribute *a=&p->attributes[i];if(name?(!a->name||strcmp(a->name,name)):(a->type!=type||a->index!=index))continue;if(out)return NULL;out=a->data;}
 return out;
}
static int Read(const cgltf_accessor *a,size_t at,float *value,unsigned count){
 if(!a||a->is_sparse||a->count<=at||cgltf_num_components(a->type)!=count||!cgltf_accessor_read_float(a,at,value,count))return 0;
 for(unsigned i=0;i<count;i++)if(!isfinite(value[i]))return 0;
 return 1;
}
static int Active(const cgltf_scene *scene,const cgltf_node *node){
 for(unsigned depth=0;node&&depth<64;depth++,node=node->parent)for(size_t i=0;i<scene->nodes_count;i++)if(scene->nodes[i]==node)return 1;
 return 0;
}
static int ImportGlb(const char *input,const char *output,const char *textureDirectory,int track){
 size_t size=0,budget=LIMIT;unsigned char *source=Load(input,&size),*wire=NULL;cgltf_data *data=NULL;int ok=0;
 cgltf_options options={0};options.memory.alloc_func=Allocate;options.memory.free_func=Release;options.memory.user_data=&budget;
 if(!source||cgltf_parse(&options,source,size,&data)!=cgltf_result_success)goto done;
 if(data->file_type!=cgltf_file_type_glb||data->buffers_count!=1||data->buffers[0].uri||data->buffers[0].size>LIMIT||data->extensions_used_count||data->extensions_required_count||(!textureDirectory&&(data->textures_count||data->images_count))||data->textures_count>64||data->images_count>64||data->skins_count||data->animations_count||data->nodes_count>10000||data->cameras_count||data->lights_count){fputs("Use a self-contained static GLB without textures, extensions, animation, skin, camera or lights\n",stderr);goto done;}
 if(cgltf_load_buffers(&options,data,NULL)!=cgltf_result_success||cgltf_validate(data)!=cgltf_result_success)goto done;
 if(textureDirectory)for(size_t i=0;i<data->images_count;i++)if(data->images[i].uri){fputs("GLB images must be embedded PNG buffer views, not external/data URIs\n",stderr);goto done;}
 const cgltf_scene *scene=data->scene?data->scene:data->scenes_count==1?data->scenes:NULL;const cgltf_node *node=NULL;
 if(!scene)goto done;
 for(size_t i=0;i<data->nodes_count;i++){
  const cgltf_node *candidate=&data->nodes[i];unsigned depth=0;for(const cgltf_node *parent=candidate;parent;parent=parent->parent)if(++depth>64)goto done;
  if(candidate->mesh&&Active(scene,candidate)){if(node){fputs("GLB has multiple mesh nodes; import a single part instead of flattening a whole car\n",stderr);goto done;}node=candidate;}
 }
 if(!node||node->skin||node->mesh->weights_count||node->mesh->primitives_count>MAX_PRIMITIVES)goto done;
 float matrix[16];cgltf_node_transform_world(node,matrix);for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))goto done;
 if(matrix[3]!=0||matrix[7]!=0||matrix[11]!=0||matrix[15]!=1)goto done;
 double co[9]={matrix[5]*(double)matrix[10]-matrix[9]*(double)matrix[6],matrix[9]*(double)matrix[2]-matrix[1]*(double)matrix[10],matrix[1]*(double)matrix[6]-matrix[5]*(double)matrix[2],matrix[8]*(double)matrix[6]-matrix[4]*(double)matrix[10],matrix[0]*(double)matrix[10]-matrix[8]*(double)matrix[2],matrix[4]*(double)matrix[2]-matrix[0]*(double)matrix[6],matrix[4]*(double)matrix[9]-matrix[8]*(double)matrix[5],matrix[8]*(double)matrix[1]-matrix[0]*(double)matrix[9],matrix[0]*(double)matrix[5]-matrix[4]*(double)matrix[1]};
 double det=matrix[0]*co[0]+matrix[4]*co[1]+matrix[8]*co[2];if(!isfinite(det)||fabs(det)<1e-20)goto done;
 size_t count=0;
 for(size_t i=0;i<node->mesh->primitives_count;i++){
  const cgltf_primitive *p=&node->mesh->primitives[i];const cgltf_accessor *positions=Attribute(p,cgltf_attribute_type_position,0,NULL);
  if(!positions||p->type!=cgltf_primitive_type_triangles||p->targets_count||p->has_draco_mesh_compression||p->extensions_count||p->mappings_count||positions->count>MAX_VERTICES)goto done;
  if(p->indices&&(p->indices->is_sparse||p->indices->type!=cgltf_type_scalar||(p->indices->component_type!=cgltf_component_type_r_8u&&p->indices->component_type!=cgltf_component_type_r_16u&&p->indices->component_type!=cgltf_component_type_r_32u)))goto done;
  size_t n=p->indices?p->indices->count:positions->count;if(!n||n%3||n>MAX_VERTICES-count)goto done;count+=n;
 }
 RageRuntimeMeshLayout layout;if(!count||!RuntimeMeshLayout(1,(uint32_t)count,(uint32_t)count,&layout)||layout.totalSize>LIMIT)goto done;
 wire=calloc(1,layout.totalSize);if(!wire||!RuntimeMeshEncodeHeader(wire,layout.totalSize,1,(uint32_t)count,(uint32_t)count))goto done;
 Put32(wire+layout.offsetsOffset,0);Put32(wire+layout.offsetsOffset+4,(uint32_t)count);size_t cursor=0;
 for(size_t i=0;i<node->mesh->primitives_count;i++){
  const cgltf_primitive *p=&node->mesh->primitives[i];uint32_t word;
  if(!Material(p->material,&word)){fputs("Missing material mapping: retain rr_material extras or rage_N material names\n",stderr);goto done;}
  const cgltf_accessor *positions=Attribute(p,cgltf_attribute_type_position,0,NULL),*normal=Attribute(p,cgltf_attribute_type_normal,0,NULL),*raw=Attribute(p,cgltf_attribute_type_custom,0,"_RR_NORMAL"),*color=Attribute(p,cgltf_attribute_type_color,0,NULL),*uv=Attribute(p,cgltf_attribute_type_texcoord,0,NULL);
  if(!uv&&(word&0xffff)!=65535){fputs("Textured car geometry needs TEXCOORD_0 UVs\n",stderr);goto done;}
  if(!normal&&!raw){fputs("GLB needs exported vertex normals\n",stderr);goto done;}
  for(size_t a=0;a<p->attributes_count;a++){
   const cgltf_attribute *attr=&p->attributes[a];
   int known=(attr->type==cgltf_attribute_type_position||attr->type==cgltf_attribute_type_normal||attr->type==cgltf_attribute_type_color||attr->type==cgltf_attribute_type_texcoord)&&attr->index==0;
   if(attr->name&&!strcmp(attr->name,"_RR_NORMAL"))known=1;
   if(attr->data->is_sparse||!known){fputs("Unsupported or sparse GLB vertex attribute\n",stderr);goto done;}
  }
  const cgltf_material *m=p->material;if(m->double_sided||m->alpha_mode!=cgltf_alpha_mode_opaque||m->emissive_factor[0]||m->emissive_factor[1]||m->emissive_factor[2]){fputs("GLB material appearance edits are not imported by this geometry operation\n",stderr);goto done;}
  for(unsigned c=0;c<4;c++)if(m->pbr_metallic_roughness.base_color_factor[c]!=1){fputs("GLB base-color edits are not imported by this geometry operation\n",stderr);goto done;}
  if(m->pbr_metallic_roughness.metallic_factor!=0||m->pbr_metallic_roughness.roughness_factor!=1){fputs("GLB metallic/roughness edits are not imported by this geometry operation\n",stderr);goto done;}
  size_t n=p->indices?p->indices->count:positions->count;
  for(size_t j=0;j<n;j++){
   size_t corner=det<0?(j/3*3+(j%3==1?2:j%3==2?1:0)):j,index=p->indices?cgltf_accessor_read_index(p->indices,corner):corner;
   RageRuntimeVertex v={.color={255,255,255,255},.material=word};float xyz[3],norm[3],rgba[4]={1,1,1,1};
   if(index>=positions->count||!Read(positions,index,xyz,3)||!Read(raw?raw:normal,index,norm,3))goto done;
   for(unsigned c=0;c<3;c++){v.position[c]=matrix[c]*xyz[0]+matrix[c+4]*xyz[1]+matrix[c+8]*xyz[2]+matrix[c+12];v.normal[c]=(float)((co[c*3]*norm[0]+co[c*3+1]*norm[1]+co[c*3+2]*norm[2])/det);}
   if(uv&&!Read(uv,index,v.uv,2))goto done;
   if(color){unsigned components=(unsigned)cgltf_num_components(color->type);if((components!=3&&components!=4)||!Read(color,index,rgba,components))goto done;}
   for(unsigned c=0;c<4;c++){if(rgba[c]<0||rgba[c]>1)goto done;v.color[c]=(uint8_t)lroundf(rgba[c]*255);}
   if(!RuntimeVertexEncode(wire+layout.verticesOffset+cursor*40,40,&v))goto done;
   Put32(wire+layout.indicesOffset+cursor*4,(uint32_t)cursor);
   cursor++;
  }
 }
 RageRuntimeMesh checked;if(!RuntimeMeshOpen(&checked,wire,layout.totalSize))goto done;
 const unsigned char *png[64]={0};size_t pngSize[64]={0};unsigned textureIds[64],textureCount=0;
 if(textureDirectory)for(size_t i=0;i<node->mesh->primitives_count;i++){
  const cgltf_material *m=node->mesh->primitives[i].material;const cgltf_texture_view *view=&m->pbr_metallic_roughness.base_color_texture;
  if(m->normal_texture.texture||m->occlusion_texture.texture||m->emissive_texture.texture||m->pbr_metallic_roughness.metallic_roughness_texture.texture){fputs("Only base-color PNG textures are supported by this part import\n",stderr);goto done;}
  if(!view->texture)continue;
  uint32_t word;if(!Material(m,&word)||(word&65535)==65535||(!track&&(word&65535)/64==1)||view->texcoord!=0||view->has_transform){fputs("PNG mapping requires a textured native surface with unchanged UV mapping\n",stderr);goto done;}
  const cgltf_image *image=view->texture->image;if(!image||image->uri||!image->buffer_view||!image->mime_type||strcmp(image->mime_type,"image/png"))goto done;
  const unsigned char *bytes=cgltf_buffer_view_data(image->buffer_view);size_t n=image->buffer_view->size;
  if(!bytes||n<24||n>32u*1024u*1024u||memcmp(bytes,"\x89PNG\r\n\x1a\n",8)||memcmp(bytes+12,"IHDR",4))goto done;
  uint32_t w=(uint32_t)bytes[16]<<24|(uint32_t)bytes[17]<<16|(uint32_t)bytes[18]<<8|bytes[19],h=(uint32_t)bytes[20]<<24|(uint32_t)bytes[21]<<16|(uint32_t)bytes[22]<<8|bytes[23];if(!w||!h||w>4096||h>4096)goto done;
  unsigned sourceId=track?(word&65535):(word&65535)%64,slot=0;while(slot<textureCount&&textureIds[slot]!=sourceId)slot++;if(slot==textureCount){if(textureCount==64)goto done;textureIds[textureCount++]=sourceId;}if(png[slot]&&(pngSize[slot]!=n||memcmp(png[slot],bytes,n))){fputs("Different PNGs claim the same native texture source; resolve material mapping first\n",stderr);goto done;}
  png[slot]=bytes;pngSize[slot]=n;
 }
 char filenames[64][4096];unsigned written=0,slots[64];
 if(textureDirectory)for(unsigned i=0;i<64;i++)if(png[i]){int n=snprintf(filenames[written],sizeof(filenames[written]),"%s/texture-%u.png",textureDirectory,textureIds[i]);if(n<0||n>=(int)sizeof(filenames[written]))goto done;FILE *exists=fopen(filenames[written],"rb");if(exists){fclose(exists);goto done;}slots[written++]=i;}
 unsigned saved=0;for(;saved<written;saved++)if(!Write(filenames[saved],png[slots[saved]],pngSize[slots[saved]]))break;
 if(saved!=written){while(saved)remove(filenames[--saved]);goto done;}
 ok=Write(output,wire,layout.totalSize);if(!ok){while(saved)remove(filenames[--saved]);}
 if(ok&&textureDirectory){printf("{\"textures\":[");for(unsigned i=0;i<written;i++)printf("%s%u",i?",":"",textureIds[slots[i]]);puts("]}");}
 done:cgltf_free(data);free(source);free(wire);if(!ok)fputs("Cannot import GLB geometry; input must be valid, bounded and mapped to one static part\n",stderr);return ok;
}

int GlbExportTextures(const char *input,unsigned part,const char *output,const char *textures){return ExportGlb(input,part,output,textures,0);}
int GlbImportTextures(const char *input,const char *output,const char *textures){return ImportGlb(input,output,textures,0);}
int GlbExportTrack(const char *input,unsigned part,const char *output,const char *textures){return ExportGlb(input,part,output,textures,1);}
int GlbImportTrack(const char *input,const char *output,const char *textures){return ImportGlb(input,output,textures,1);}
int GlbExport(const char *input,unsigned part,const char *output){return GlbExportTextures(input,part,output,NULL);}
int GlbImport(const char *input,const char *output){return GlbImportTextures(input,output,NULL);}

/* Assemble validated part GLBs without baking away wheel instances/pivots. */
static int Shift(yyjson_mut_doc *doc,yyjson_mut_val *obj,const char *key,uint32_t offset){
 yyjson_mut_val *v=yyjson_mut_obj_get(obj,key);if(!v)return 1;if(!yyjson_mut_is_uint(v)||yyjson_mut_get_uint(v)>UINT32_MAX-offset)return 0;
 uint32_t n=(uint32_t)yyjson_mut_get_uint(v)+offset;yyjson_mut_obj_remove_str(obj,key);Number(doc,obj,key,n);return 1;
}
int GlbAssemble(const char *output){
 enum { INPUT_LIMIT=1024*1024 };char *input=malloc(INPUT_LIMIT+1u),*json=NULL;unsigned char *bin=NULL,*file=NULL;yyjson_doc *manifest=NULL,*partDoc=NULL;yyjson_mut_doc *doc=NULL;int ok=0;size_t binSize=0,jsonSize=0;
 if(!input)goto done;
 size_t n=fread(input,1,INPUT_LIMIT+1u,stdin);
 if(ferror(stdin)||n>INPUT_LIMIT)goto done;
 manifest=yyjson_read(input,n,0);
 if(!manifest)goto done;
 yyjson_val *spec=yyjson_doc_get_root(manifest),*parts=yyjson_obj_get(spec,"parts"),*instances=yyjson_obj_get(spec,"instances");
 if(!yyjson_is_obj(yyjson_obj_get(spec,"source"))||!yyjson_is_arr(parts)||yyjson_arr_size(parts)!=3||!yyjson_is_arr(instances)||yyjson_arr_size(instances)<3||yyjson_arr_size(instances)>8)goto done;
 doc=yyjson_mut_doc_new(NULL);if(!doc)goto done;yyjson_mut_val *root=yyjson_mut_obj(doc);yyjson_mut_doc_set_root(doc,root);
 yyjson_mut_val *asset=Object(doc,root,"asset");Text(doc,asset,"version","2.0");Text(doc,asset,"generator","Rage Racer Editor");Text(doc,Object(doc,asset,"extras"),"rr_units","native Y-up");
 yyjson_mut_val *source=yyjson_val_mut_copy(doc,yyjson_obj_get(spec,"source"));if(!source)goto done;yyjson_mut_obj_add_val(doc,Object(doc,root,"extras"),"rr_source",source);
 const char *names[]={"bufferViews","accessors","materials","images","textures","samplers","meshes"};yyjson_mut_val *arrays[7];for(unsigned i=0;i<7;i++)arrays[i]=Array(doc,root,names[i]);
 for(unsigned part=0;part<3;part++){
  yyjson_val *description=yyjson_arr_get(parts,part),*path=yyjson_obj_get(description,"file"),*id=yyjson_obj_get(description,"id");
  if(!yyjson_is_str(path)||strlen(yyjson_get_str(path))!=yyjson_get_len(path)||!yyjson_is_str(id)||!yyjson_get_len(id)||yyjson_get_len(id)>159||strlen(yyjson_get_str(id))!=yyjson_get_len(id))goto done;
  size_t size=0;unsigned char *bytes=Load(yyjson_get_str(path),&size);if(!bytes)goto done;
  cgltf_data *parsed=NULL;size_t budget=LIMIT;cgltf_options opts={0};opts.memory.alloc_func=Allocate;opts.memory.free_func=Release;opts.memory.user_data=&budget;
  if(cgltf_parse(&opts,bytes,size,&parsed)!=cgltf_result_success||parsed->file_type!=cgltf_file_type_glb||parsed->buffers_count!=1||parsed->buffers[0].uri||parsed->meshes_count!=1||parsed->nodes_count!=1||parsed->extensions_used_count||cgltf_load_buffers(&opts,parsed,NULL)!=cgltf_result_success||cgltf_validate(parsed)!=cgltf_result_success){cgltf_free(parsed);free(bytes);goto done;}
  partDoc=yyjson_read(parsed->json,parsed->json_size,0);if(!partDoc){cgltf_free(parsed);free(bytes);goto done;}yyjson_val *partRoot=yyjson_doc_get_root(partDoc);
  if(parsed->buffers[0].size>LIMIT-binSize){cgltf_free(parsed);free(bytes);goto done;}size_t partSize=(parsed->buffers[0].size+3)&~(size_t)3;if(partSize>LIMIT-binSize){cgltf_free(parsed);free(bytes);goto done;}
  unsigned char *next=realloc(bin,binSize+partSize);if(!next){cgltf_free(parsed);free(bytes);goto done;}bin=next;memcpy(bin+binSize,parsed->buffers[0].data,parsed->buffers[0].size);memset(bin+binSize+parsed->buffers[0].size,0,partSize-parsed->buffers[0].size);
  uint32_t offsets[7];for(unsigned i=0;i<7;i++)offsets[i]=(uint32_t)yyjson_mut_arr_size(arrays[i]);
  for(unsigned group=0;group<7;group++){
   yyjson_val *table=yyjson_obj_get(partRoot,names[group]),*entry;size_t i,count;
   if(!table)continue;
   if(!yyjson_is_arr(table)){cgltf_free(parsed);
   free(bytes);
   goto done;
   }
   yyjson_arr_foreach(table,i,count,entry){
    yyjson_mut_val *copy=yyjson_val_mut_copy(doc,entry);if(!copy||!yyjson_mut_is_obj(copy)){cgltf_free(parsed);free(bytes);goto done;}
    int valid=1;
    if(group==0){if(!yyjson_mut_obj_get(copy,"byteOffset"))Number(doc,copy,"byteOffset",0);valid=Shift(doc,copy,"byteOffset",(uint32_t)binSize);}
    else if(group==1)valid=Shift(doc,copy,"bufferView",offsets[0]);
    else if(group==2){yyjson_mut_val *pbr=yyjson_mut_obj_get(copy,"pbrMetallicRoughness"),*texture=pbr?yyjson_mut_obj_get(pbr,"baseColorTexture"):NULL;if(texture)valid=Shift(doc,texture,"index",offsets[4]);}
    else if(group==3)valid=Shift(doc,copy,"bufferView",offsets[0]);
    else if(group==4)valid=Shift(doc,copy,"source",offsets[3])&&Shift(doc,copy,"sampler",offsets[5]);
    else if(group==6){yyjson_mut_val *primitives=yyjson_mut_obj_get(copy,"primitives"),*primitive;size_t j,number;
     if(!yyjson_mut_is_arr(primitives))valid=0;
     yyjson_mut_arr_foreach(primitives,j,number,primitive){valid=valid&&Shift(doc,primitive,"indices",offsets[1])&&Shift(doc,primitive,"material",offsets[2]);yyjson_mut_val *attributes=yyjson_mut_obj_get(primitive,"attributes"),*k,*v;size_t a,total;yyjson_mut_obj_foreach(attributes,a,total,k,v){if(!yyjson_mut_is_uint(v)||yyjson_mut_get_uint(v)>UINT32_MAX-offsets[1])valid=0;else yyjson_mut_set_uint(v,yyjson_mut_get_uint(v)+offsets[1]);}}
     yyjson_mut_obj_remove_str(copy,"name");Text(doc,copy,"name",yyjson_get_str(id));Text(doc,Object(doc,copy,"extras"),"rr_part",yyjson_get_str(id));
    }
    if(!valid){cgltf_free(parsed);free(bytes);goto done;}yyjson_mut_arr_add_val(arrays[group],copy);
   }
  }
  binSize+=partSize;cgltf_free(parsed);free(bytes);yyjson_doc_free(partDoc);partDoc=NULL;
 }
 yyjson_mut_val *nodes=Array(doc,root,"nodes"),*car=Element(doc,nodes),*children=Array(doc,car,"children");Text(doc,car,"name","Car");
 yyjson_mut_obj_add_val(doc,Object(doc,car,"extras"),"rr_source",yyjson_mut_val_mut_copy(doc,source));
 yyjson_val *instance;size_t i,count;yyjson_arr_foreach(instances,i,count,instance){
  yyjson_val *index=yyjson_obj_get(instance,"mesh"),*name=yyjson_obj_get(instance,"name"),*position=yyjson_obj_get(instance,"position"),*scale=yyjson_obj_get(instance,"scale");
  if(!yyjson_is_uint(index)||yyjson_get_uint(index)>=3||!yyjson_is_str(name)||!yyjson_get_len(name)||yyjson_get_len(name)>159||strlen(yyjson_get_str(name))!=yyjson_get_len(name)||!yyjson_is_arr(position)||yyjson_arr_size(position)!=3||!yyjson_is_arr(scale)||yyjson_arr_size(scale)!=3)goto done;
  for(unsigned c=0;c<3;c++){yyjson_val *p=yyjson_arr_get(position,c),*s=yyjson_arr_get(scale,c);if(!yyjson_is_num(p)||!yyjson_is_num(s)||!isfinite(yyjson_get_num(p))||!isfinite(yyjson_get_num(s))||yyjson_get_num(s)==0)goto done;}
  yyjson_mut_arr_add_uint(doc,children,i+1);yyjson_mut_val *node=Element(doc,nodes);Number(doc,Object(doc,node,"extras"),"rr_instance",(uint32_t)i);Number(doc,node,"mesh",(uint32_t)yyjson_get_uint(index));Text(doc,node,"name",yyjson_get_str(name));yyjson_mut_obj_add_val(doc,node,"translation",yyjson_val_mut_copy(doc,position));yyjson_mut_obj_add_val(doc,node,"scale",yyjson_val_mut_copy(doc,scale));Text(doc,yyjson_mut_obj_get(node,"extras"),"rr_part",yyjson_get_str(yyjson_obj_get(yyjson_arr_get(parts,(size_t)yyjson_get_uint(index)),"id")));
 }
 Number(doc,Element(doc,Array(doc,root,"buffers")),"byteLength",(uint32_t)binSize);yyjson_mut_arr_add_uint(doc,Array(doc,Element(doc,Array(doc,root,"scenes")),"nodes"),0);Number(doc,root,"scene",0);
 for(unsigned group=0;group<7;group++)if(!yyjson_mut_arr_size(arrays[group]))yyjson_mut_obj_remove_str(root,names[group]);
 json=yyjson_mut_write(doc,0,&jsonSize);if(!json||jsonSize>8*1024*1024)goto done;size_t padded=(jsonSize+3)&~(size_t)3,total=28+padded+binSize;if(total>LIMIT)goto done;file=malloc(total);if(!file)goto done;Put32(file,0x46546c67);Put32(file+4,2);Put32(file+8,(uint32_t)total);Put32(file+12,(uint32_t)padded);Put32(file+16,0x4e4f534a);memcpy(file+20,json,jsonSize);memset(file+20+jsonSize,' ',padded-jsonSize);Put32(file+20+padded,(uint32_t)binSize);Put32(file+24+padded,0x004e4942);memcpy(file+28+padded,bin,binSize);
 cgltf_data *check=NULL;cgltf_options validation={0};if(cgltf_parse(&validation,file,total,&check)==cgltf_result_success&&cgltf_load_buffers(&validation,check,NULL)==cgltf_result_success&&cgltf_validate(check)==cgltf_result_success)ok=Write(output,file,total);cgltf_free(check);
 done:free(input);free(json);free(bin);free(file);yyjson_doc_free(partDoc);yyjson_doc_free(manifest);yyjson_mut_doc_free(doc);if(!ok)fputs("Cannot assemble complete car GLB; invalid part/layout or output limit\n",stderr);return ok;
}

/* Split a complete car into local meshes, preserving the shared wheel role.
 * The caller supplies the original disc layout, never a guessed wheel pivot. */
int GlbSeparate(const char *input,const char *directory){
 size_t size=0,budget=LIMIT;unsigned char *bytes=Load(input,&size);cgltf_data *data=NULL;yyjson_doc *specDoc=NULL,*jsonDoc=NULL;int ok=0;char request[65537],paths[3][4096]={{0}};unsigned written=0;
 size_t n=fread(request,1,sizeof(request),stdin);if(ferror(stdin)||n==sizeof(request))goto done;
 specDoc=yyjson_read(request,n,0);if(!specDoc)goto done;yyjson_val *spec=yyjson_doc_get_root(specDoc),*parts=yyjson_obj_get(spec,"parts"),*instances=yyjson_obj_get(spec,"instances"),*expected=yyjson_obj_get(spec,"source");
 if(!yyjson_is_obj(expected)||!yyjson_is_arr(parts)||yyjson_arr_size(parts)!=3||!yyjson_is_arr(instances)||yyjson_arr_size(instances)<3||yyjson_arr_size(instances)>8)goto done;
 cgltf_options opts={0};opts.memory.alloc_func=Allocate;opts.memory.free_func=Release;opts.memory.user_data=&budget;
 if(!bytes||cgltf_parse(&opts,bytes,size,&data)!=cgltf_result_success||data->file_type!=cgltf_file_type_glb||data->buffers_count!=1||data->buffers[0].uri||data->extensions_used_count||data->extensions_required_count||data->animations_count||data->skins_count||data->cameras_count||data->lights_count||data->nodes_count>10000||cgltf_load_buffers(&opts,data,NULL)!=cgltf_result_success||cgltf_validate(data)!=cgltf_result_success)goto done;
 jsonDoc=yyjson_read(data->json,data->json_size,0);if(!jsonDoc)goto done;yyjson_val *root=yyjson_doc_get_root(jsonDoc),*actual=yyjson_obj_get(yyjson_obj_get(root,"extras"),"rr_source");
 const cgltf_scene *scene=data->scene?data->scene:data->scenes_count==1?data->scenes:NULL;if(!scene)goto done;
 /* 3D editors retain node custom properties, but can drop document extras. */
 if(!actual)for(size_t i=0;i<data->nodes_count;i++)if(Active(scene,&data->nodes[i])){yyjson_val *node=yyjson_arr_get(yyjson_obj_get(root,"nodes"),i),*candidate=yyjson_obj_get(yyjson_obj_get(node,"extras"),"rr_source");if(candidate){if(actual)goto done;actual=candidate;}}
 for(unsigned i=0;i<3;i++){const char *key=i==2?"fingerprint":i?"region":"key";yyjson_val *a=yyjson_obj_get(actual,key),*e=yyjson_obj_get(expected,key);if(i==2&&yyjson_is_null(a)&&yyjson_is_null(e))continue;if(!yyjson_is_str(a)||!yyjson_is_str(e)||yyjson_get_len(a)!=yyjson_get_len(e)||memcmp(yyjson_get_str(a),yyjson_get_str(e),yyjson_get_len(a))){fputs("Choose a complete GLB exported for this car slot and game version\n",stderr);goto done;}}
 const cgltf_mesh *meshes[3]={0};float matrices[3][16];unsigned seen=0,roles=0;
 for(size_t i=0;i<data->nodes_count;i++){
  const cgltf_node *node=&data->nodes[i];unsigned depth=0;for(const cgltf_node *p=node;p;p=p->parent)if(++depth>64)goto done;
  if(!node->mesh||!Active(scene,node))continue;
  yyjson_val *description=yyjson_arr_get(yyjson_obj_get(root,"nodes"),i),*extras=yyjson_obj_get(description,"extras"),*instanceId=yyjson_obj_get(extras,"rr_instance"),*roleId=yyjson_obj_get(extras,"rr_part");
  if(!yyjson_is_uint(instanceId)||yyjson_get_uint(instanceId)>=yyjson_arr_size(instances)||!yyjson_is_str(roleId)){fputs("Missing car part/instance mapping; retain exported custom properties\n",stderr);goto done;}
  unsigned id=(unsigned)yyjson_get_uint(instanceId);if(seen&(1u<<id))goto done;seen|=1u<<id;
  yyjson_val *instance=yyjson_arr_get(instances,id),*role=yyjson_obj_get(instance,"mesh"),*position=yyjson_obj_get(instance,"position"),*scale=yyjson_obj_get(instance,"scale");
  if(!yyjson_is_uint(role)||yyjson_get_uint(role)>=3||!yyjson_is_arr(position)||yyjson_arr_size(position)!=3||!yyjson_is_arr(scale)||yyjson_arr_size(scale)!=3)goto done;
  unsigned r=(unsigned)yyjson_get_uint(role);yyjson_val *part=yyjson_obj_get(yyjson_arr_get(parts,r),"id");if(!yyjson_is_str(part)||yyjson_get_len(part)!=yyjson_get_len(roleId)||memcmp(yyjson_get_str(part),yyjson_get_str(roleId),yyjson_get_len(part)))goto done;
  float world[16],local[16];cgltf_node_transform_world(node,world);
  for(unsigned c=0;c<3;c++){yyjson_val *p=yyjson_arr_get(position,c),*s=yyjson_arr_get(scale,c);double pivot=yyjson_get_num(p),axis=yyjson_get_num(s);if(!yyjson_is_num(p)||!yyjson_is_num(s)||!isfinite(pivot)||!isfinite(axis)||!axis)goto done;for(unsigned col=0;col<4;col++){local[col*4+c]=(float)((world[col*4+c]-(col==3?pivot:0))/axis);if(!isfinite(local[col*4+c]))goto done;}}
  for(unsigned col=0;col<4;col++)local[col*4+3]=world[col*4+3];
  if(meshes[r]){if(meshes[r]!=node->mesh){fputs("Shared front wheels must use the same mesh\n",stderr);goto done;}for(unsigned c=0;c<16;c++)if(fabsf(local[c]-matrices[r][c])>1e-5f*fmaxf(1,fabsf(matrices[r][c]))){fputs("Wheel instances have incompatible edits; edit their shared mesh or apply the same local transform\n",stderr);goto done;}}
  else{meshes[r]=node->mesh;memcpy(matrices[r],local,sizeof(local));roles|=1u<<r;}
 }
 if(roles!=7||seen!=(1u<<yyjson_arr_size(instances))-1u)goto done;
 for(unsigned r=0;r<3;r++){
  yyjson_mut_doc *out=yyjson_mut_doc_new(NULL);if(!out)goto done;yyjson_mut_val *copy=yyjson_val_mut_copy(out,root);yyjson_mut_doc_set_root(out,copy);
  yyjson_mut_obj_remove_str(copy,"meshes");yyjson_mut_arr_add_val(Array(out,copy,"meshes"),yyjson_val_mut_copy(out,yyjson_arr_get(yyjson_obj_get(root,"meshes"),(size_t)(meshes[r]-data->meshes))));
  yyjson_mut_obj_remove_str(copy,"nodes");yyjson_mut_val *node=Element(out,Array(out,copy,"nodes")),*matrix=Array(out,node,"matrix");Number(out,node,"mesh",0);for(unsigned c=0;c<16;c++)yyjson_mut_arr_add_real(out,matrix,matrices[r][c]);
  yyjson_mut_obj_remove_str(copy,"scenes");yyjson_mut_arr_add_uint(out,Array(out,Element(out,Array(out,copy,"scenes")),"nodes"),0);yyjson_mut_obj_remove_str(copy,"scene");Number(out,copy,"scene",0);
  size_t jsonSize=0;char *json=yyjson_mut_write(out,0,&jsonSize);size_t padded=(jsonSize+3)&~(size_t)3,binSize=(data->buffers[0].size+3)&~(size_t)3,total=28+padded+binSize;unsigned char *file=json&&jsonSize<=8u*1024u*1024u&&total<=LIMIT?calloc(1,total):NULL;
  int valid=0;if(file){Put32(file,0x46546c67);Put32(file+4,2);Put32(file+8,(uint32_t)total);Put32(file+12,(uint32_t)padded);Put32(file+16,0x4e4f534a);memcpy(file+20,json,jsonSize);memset(file+20+jsonSize,' ',padded-jsonSize);Put32(file+20+padded,(uint32_t)binSize);Put32(file+24+padded,0x004e4942);memcpy(file+28+padded,data->buffers[0].data,data->buffers[0].size);int count=snprintf(paths[r],sizeof(paths[r]),"%s/part-%u.glb",directory,r);valid=count>0&&count<(int)sizeof(paths[r])&&Write(paths[r],file,total);}
  free(file);free(json);yyjson_mut_doc_free(out);if(!valid)goto done;written++;
 }
 ok=1;
 done:if(!ok){while(written)remove(paths[--written]);fputs("Cannot split complete car GLB; project remains unchanged\n",stderr);}yyjson_doc_free(jsonDoc);yyjson_doc_free(specDoc);cgltf_free(data);free(bytes);return ok;
}

/* Metadata inspection/remapping only: existing import validates actual content. */
int GlbMapping(const char *input,const char *output){
 size_t size=0,budget=LIMIT;unsigned char *bytes=Load(input,&size),*file=NULL;cgltf_data *data=NULL;yyjson_doc *original=NULL,*request=NULL;yyjson_mut_doc *doc=NULL;char *json=NULL;int ok=0;
 cgltf_options options={0};options.memory.alloc_func=Allocate;options.memory.free_func=Release;options.memory.user_data=&budget;
 if(!bytes||cgltf_parse(&options,bytes,size,&data)!=cgltf_result_success||data->file_type!=cgltf_file_type_glb||data->buffers_count!=1||data->buffers[0].uri||data->extensions_used_count||data->extensions_required_count||data->animations_count||data->skins_count||data->cameras_count||data->lights_count||data->nodes_count>10000||data->materials_count>MAX_PRIMITIVES||cgltf_load_buffers(&options,data,NULL)!=cgltf_result_success||cgltf_validate(data)!=cgltf_result_success)goto done;
 const cgltf_scene *scene=data->scene?data->scene:data->scenes_count==1?data->scenes:NULL;if(!scene)goto done;
 for(size_t i=0;i<data->nodes_count;i++){unsigned depth=0;for(const cgltf_node *p=&data->nodes[i];p;p=p->parent)if(++depth>64)goto done;}
 original=yyjson_read(data->json,data->json_size,0);doc=yyjson_mut_doc_new(NULL);if(!original||!doc)goto done;
 if(!output){
  yyjson_mut_val *root=yyjson_mut_obj(doc);yyjson_mut_doc_set_root(doc,root);yyjson_mut_val *nodes=Array(doc,root,"nodes"),*materials=Array(doc,root,"materials");
  for(size_t i=0;i<data->nodes_count;i++)if(data->nodes[i].mesh&&Active(scene,&data->nodes[i])){yyjson_mut_val *node=Element(doc,nodes);Number(doc,node,"node",(uint32_t)i);Number(doc,node,"mesh",(uint32_t)(data->nodes[i].mesh-data->meshes));Text(doc,node,"name",data->nodes[i].name?data->nodes[i].name:"Unnamed part");yyjson_val *extras=yyjson_obj_get(yyjson_arr_get(yyjson_obj_get(yyjson_doc_get_root(original),"nodes"),i),"extras");yyjson_val *instance=yyjson_obj_get(extras,"rr_instance");if(yyjson_is_uint(instance))Number(doc,node,"instance",(uint32_t)yyjson_get_uint(instance));}
  unsigned char used[MAX_PRIMITIVES]={0};unsigned active=0;
  for(size_t n=0;n<data->nodes_count;n++)if(data->nodes[n].mesh&&Active(scene,&data->nodes[n])){const cgltf_mesh *mesh=data->nodes[n].mesh;if(++active>8||mesh->primitives_count>MAX_PRIMITIVES)goto done;for(size_t p=0;p<mesh->primitives_count;p++){const cgltf_material *m=mesh->primitives[p].material;if(!m){fputs("Assign a material to every mesh in the 3D editor before import\n",stderr);goto done;}used[m-data->materials]=1;}}
  for(size_t i=0;i<data->materials_count;i++){if(!used[i])continue;yyjson_mut_val *m=Element(doc,materials);Number(doc,m,"material",(uint32_t)i);Text(doc,m,"name",data->materials[i].name?data->materials[i].name:"Unnamed material");uint32_t word;if(Material(&data->materials[i],&word))Number(doc,m,"word",word);}
  json=yyjson_mut_write(doc,0,NULL);if(json){puts(json);ok=1;}goto done;
 }
 char text[65537];size_t length=fread(text,1,sizeof(text),stdin);if(ferror(stdin)||length==sizeof(text))goto done;request=yyjson_read(text,length,0);if(!request)goto done;
 yyjson_val *spec=yyjson_doc_get_root(request),*nodes=yyjson_obj_get(spec,"nodes"),*materials=yyjson_obj_get(spec,"materials"),*source=yyjson_obj_get(spec,"source");if(!yyjson_is_arr(nodes)||yyjson_arr_size(nodes)>8||!yyjson_is_arr(materials)||yyjson_arr_size(materials)>MAX_PRIMITIVES||!yyjson_is_obj(source))goto done;
 yyjson_mut_val *root=yyjson_val_mut_copy(doc,yyjson_doc_get_root(original));yyjson_mut_doc_set_root(doc,root);yyjson_mut_val *extras=yyjson_mut_obj_get(root,"extras");if(!extras)extras=Object(doc,root,"extras");yyjson_mut_obj_remove_str(extras,"rr_source");yyjson_mut_obj_add_val(doc,extras,"rr_source",yyjson_val_mut_copy(doc,source));
 yyjson_val *entry;size_t i,count;yyjson_arr_foreach(nodes,i,count,entry){yyjson_val *id=yyjson_obj_get(entry,"node"),*instance=yyjson_obj_get(entry,"instance"),*part=yyjson_obj_get(entry,"part");if(!yyjson_is_uint(id)||yyjson_get_uint(id)>=data->nodes_count||!data->nodes[yyjson_get_uint(id)].mesh||!Active(scene,&data->nodes[yyjson_get_uint(id)])||!yyjson_is_uint(instance)||yyjson_get_uint(instance)>=8||!yyjson_is_str(part)||!yyjson_get_len(part)||yyjson_get_len(part)>159)goto done;
  for(size_t j=0;j<i;j++)if(yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(nodes,j),"node"))==yyjson_get_uint(id)||yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(nodes,j),"instance"))==yyjson_get_uint(instance))goto done;
  yyjson_mut_val *node=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"nodes"),(size_t)yyjson_get_uint(id)),*e=yyjson_mut_obj_get(node,"extras");if(!e)e=Object(doc,node,"extras");yyjson_mut_obj_remove_str(e,"rr_instance");yyjson_mut_obj_remove_str(e,"rr_part");Number(doc,e,"rr_instance",(uint32_t)yyjson_get_uint(instance));Text(doc,e,"rr_part",yyjson_get_str(part));
 }
 yyjson_arr_foreach(materials,i,count,entry){yyjson_val *id=yyjson_obj_get(entry,"material"),*word=yyjson_obj_get(entry,"word");if(!yyjson_is_uint(id)||yyjson_get_uint(id)>=data->materials_count||!yyjson_is_uint(word)||yyjson_get_uint(word)>UINT32_MAX)goto done;for(size_t j=0;j<i;j++)if(yyjson_get_uint(yyjson_obj_get(yyjson_arr_get(materials,j),"material"))==yyjson_get_uint(id))goto done;
  yyjson_mut_val *material=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"materials"),(size_t)yyjson_get_uint(id)),*e=yyjson_mut_obj_get(material,"extras");if(!e)e=Object(doc,material,"extras");yyjson_mut_obj_remove_str(e,"rr_material");Number(doc,e,"rr_material",(uint32_t)yyjson_get_uint(word));
 }
 yyjson_val *preview=yyjson_obj_get(spec,"preview_node");
 if(preview){
  if(!yyjson_is_uint(preview)||yyjson_get_uint(preview)>=data->nodes_count)goto done;
  const cgltf_node *selected=&data->nodes[yyjson_get_uint(preview)];
  if(!selected->mesh||!Active(scene,selected))goto done;
  float transform[16];cgltf_node_transform_world(selected,transform);for(unsigned c=0;c<16;c++)if(!isfinite(transform[c]))goto done;
  yyjson_mut_obj_remove_str(root,"meshes");yyjson_mut_arr_add_val(Array(doc,root,"meshes"),yyjson_val_mut_copy(doc,yyjson_arr_get(yyjson_obj_get(yyjson_doc_get_root(original),"meshes"),(size_t)(selected->mesh-data->meshes))));
  yyjson_mut_obj_remove_str(root,"nodes");yyjson_mut_val *node=Element(doc,Array(doc,root,"nodes")),*matrix=Array(doc,node,"matrix");Number(doc,node,"mesh",0);for(unsigned c=0;c<16;c++)yyjson_mut_arr_add_real(doc,matrix,transform[c]);
  yyjson_mut_obj_remove_str(root,"scenes");yyjson_mut_arr_add_uint(doc,Array(doc,Element(doc,Array(doc,root,"scenes")),"nodes"),0);yyjson_mut_obj_remove_str(root,"scene");Number(doc,root,"scene",0);
 }
 size_t jsonSize=0;json=yyjson_mut_write(doc,0,&jsonSize);size_t padded=(jsonSize+3)&~(size_t)3,binSize=(data->buffers[0].size+3)&~(size_t)3,total=28+padded+binSize;if(!json||jsonSize>8u*1024u*1024u||total>LIMIT)goto done;file=calloc(1,total);if(!file)goto done;
 Put32(file,0x46546c67);Put32(file+4,2);Put32(file+8,(uint32_t)total);Put32(file+12,(uint32_t)padded);Put32(file+16,0x4e4f534a);memcpy(file+20,json,jsonSize);memset(file+20+jsonSize,' ',padded-jsonSize);Put32(file+20+padded,(uint32_t)binSize);Put32(file+24+padded,0x004e4942);memcpy(file+28+padded,data->buffers[0].data,data->buffers[0].size);ok=Write(output,file,total);
 done:free(json);free(file);free(bytes);cgltf_free(data);yyjson_doc_free(original);yyjson_doc_free(request);yyjson_mut_doc_free(doc);if(!ok)fputs("Cannot inspect/map static car GLB; input or mapping is invalid\n",stderr);return ok;
}

/* External glTF stays inside the selected file's directory. Normalise it to
 * one immutable GLB before any editor import/history operation. */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#endif
static int SidecarKind(const char *path,int directory){
#ifdef _WIN32
 int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);if(count<=0)return 0;wchar_t *wide=malloc((size_t)count*sizeof(*wide));if(!wide)return 0;DWORD flags=INVALID_FILE_ATTRIBUTES;if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,count))flags=GetFileAttributesW(wide);free(wide);return flags!=INVALID_FILE_ATTRIBUTES&&!(flags&FILE_ATTRIBUTE_REPARSE_POINT)&&!!(flags&FILE_ATTRIBUTE_DIRECTORY)==directory;
#else
 struct stat info;return !lstat(path,&info)&&!S_ISLNK(info.st_mode)&&(directory?S_ISDIR(info.st_mode):S_ISREG(info.st_mode));
#endif
}
static int Hex(unsigned char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int Sidecar(const char *input,const char *uri,char path[4096]){
 if(!uri||!*uri||strlen(uri)>=4096)return 0;
 char relative[4096];
 size_t at=0;
 for(size_t i=0;uri[i];i++){unsigned char c=(unsigned char)uri[i];if(c=='?'||c=='#')return 0;if(c=='%'){if(!uri[i+1]||!uri[i+2]||Hex((unsigned char)uri[i+1])<0||Hex((unsigned char)uri[i+2])<0)return 0;c=(unsigned char)(Hex((unsigned char)uri[i+1])*16+Hex((unsigned char)uri[i+2]));i+=2;}if(!c||c=='\\'||c==':'||c<32)return 0;relative[at++]=(char)c;}relative[at]=0;if(relative[0]=='/')return 0;
 const char *slash=strrchr(input,'/'),*back=strrchr(input,'\\');if(back&&(!slash||back>slash))slash=back;size_t root=slash?(size_t)(slash-input)+1:0;if(root>=4096)return 0;memcpy(path,input,root);path[root]=0;
 const char *part=relative;while(*part){const char *end=strchr(part,'/');size_t length=end?(size_t)(end-part):strlen(part);if(!length||(length==2&&!memcmp(part,"..",2)))return 0;if(length!=1||*part!='.'){size_t n=strlen(path);if(n+length+2>4096)return 0;memcpy(path+n,part,length);path[n+length]=0;if(!SidecarKind(path,end!=NULL))return 0;if(end){path[n+length]='/';path[n+length+1]=0;}}if(!end)break;part=end+1;if(!*part)return 0;}
 return strlen(path)>root&&SidecarKind(path,0);
}
static int Base64(unsigned char c){return c>='A'&&c<='Z'?c-'A':c>='a'&&c<='z'?c-'a'+26:c>='0'&&c<='9'?c-'0'+52:c=='+'?62:c=='/'?63:-1;}
static unsigned char *JpegPng(const unsigned char *bytes,size_t size,size_t *pngSize){
 unsigned w=0,h=0;size_t at=2;unsigned char *pixels=NULL,*png=NULL;SDL_Surface *surface=NULL;SDL_IOStream *stream=NULL;
 if(size<4||size>32u*1024u*1024u||bytes[0]!=255||bytes[1]!=216||bytes[size-2]!=255||bytes[size-1]!=217)return NULL;
 while(at<size){if(bytes[at++]!=255)goto done;while(at<size&&bytes[at]==255)at++;if(at>=size)goto done;unsigned marker=bytes[at++];if(marker==218||marker==217||!marker)goto done;if(marker==1||(marker>=208&&marker<=215))continue;
  if(size-at<2)goto done;
  unsigned length=(unsigned)bytes[at]*256+bytes[at+1];
  if(length<2||length>size-at)goto done;
  if(marker==192||marker==193||marker==194){if(length<8||bytes[at+2]!=8)goto done;h=(unsigned)bytes[at+3]*256+bytes[at+4];w=(unsigned)bytes[at+5]*256+bytes[at+6];unsigned components=bytes[at+7];if((components!=1&&components!=3&&components!=4)||length!=8+components*3)goto done;break;}at+=length;
 }
 if(!w||!h||w>4096||h>4096)goto done;
 pixels=malloc((size_t)w*h*4);
 if(!pixels)goto done;
 if(!SDL_ConvertPixels((int)w,(int)h,SDL_PIXELFORMAT_MJPG,bytes,(int)size,SDL_PIXELFORMAT_RGBA32,pixels,(int)w*4)){fprintf(stderr,"Cannot decode JPEG: %s\n",SDL_GetError());goto done;}
 surface=SDL_CreateSurfaceFrom((int)w,(int)h,SDL_PIXELFORMAT_RGBA32,pixels,(int)w*4);stream=SDL_IOFromDynamicMem();if(!surface||!stream||!SDL_SavePNG_IO(surface,stream,false))goto done;
 Sint64 n=SDL_GetIOSize(stream);const void *data=SDL_GetPointerProperty(SDL_GetIOProperties(stream),SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER,NULL);if(n<=0||n>32*1024*1024||!data)goto done;png=malloc((size_t)n);if(png){memcpy(png,data,(size_t)n);*pngSize=(size_t)n;}
 done:if(stream)SDL_CloseIO(stream);SDL_DestroySurface(surface);free(pixels);return png;
}
static unsigned char *Asset(const char *input,const char *uri,size_t *size,size_t limit,int image){
 if(!uri)return NULL;
 if(strncmp(uri,"data:",5)){char path[4096];if(!Sidecar(input,uri,path))return NULL;unsigned char *bytes=Load(path,size);if(bytes&&*size>limit){free(bytes);return NULL;}return bytes;}
 const char *payload=NULL;
 const char *types[]={"data:application/octet-stream;base64,","data:application/gltf-buffer;base64,","data:image/png;base64,","data:image/jpeg;base64,"};
 for(unsigned i=image?2:0;i<(image?4u:2u);i++){size_t n=strlen(types[i]);if(!strncmp(uri,types[i],n)){payload=uri+n;break;}}
 if(!payload)return NULL;
 size_t n=strlen(payload);
 if(n%4||n/4>limit/3+1)return NULL;
 size_t count=n/4*3;
 if(n&&payload[n-1]=='=')count--;
 if(n>1&&payload[n-2]=='=')count--;
 if(count>limit)return NULL;
 unsigned char *bytes=malloc(count?count:1);if(!bytes)return NULL;size_t at=0;
 for(size_t i=0;i<n;i+=4){int a=Base64((unsigned char)payload[i]),b=Base64((unsigned char)payload[i+1]),c=payload[i+2]=='='?-2:Base64((unsigned char)payload[i+2]),d=payload[i+3]=='='?-2:Base64((unsigned char)payload[i+3]);
  if(a<0||b<0||c==-1||d==-1||(c==-2&&(d!=-2||i+4!=n||(b&15)))||(c>=0&&d==-2&&(i+4!=n||(c&3)))){free(bytes);return NULL;}
  if(at>=count){free(bytes);return NULL;}bytes[at++]=(unsigned char)(a*4+b/16);if(c>=0){if(at>=count){free(bytes);return NULL;}bytes[at++]=(unsigned char)(b*16+c/4);if(d>=0){if(at>=count){free(bytes);return NULL;}bytes[at++]=(unsigned char)(c*64+d);}}
 }
 if(at!=count||(image&&((!strncmp(uri,types[2],strlen(types[2]))&&(count<8||memcmp(bytes,"\x89PNG\r\n\x1a\n",8)))||(!strncmp(uri,types[3],strlen(types[3]))&&(count<2||bytes[0]!=255||bytes[1]!=216))))){free(bytes);return NULL;}*size=count;return bytes;
}
static int UriMatches(yyjson_doc *doc,const char *group,size_t index,const char *uri){
 yyjson_val *value=yyjson_obj_get(yyjson_arr_get(yyjson_obj_get(yyjson_doc_get_root(doc),group),index),"uri");return yyjson_is_str(value)&&strlen(uri)==yyjson_get_len(value);
}
static int SaveGlb(yyjson_mut_doc *doc,const unsigned char *bin,size_t binSize,const char *output){
 size_t jsonSize=0;char *json=yyjson_mut_write(doc,0,&jsonSize);size_t padded=(jsonSize+3)&~(size_t)3,aligned=(binSize+3)&~(size_t)3,total=28+padded+aligned;unsigned char *file=NULL;cgltf_data *check=NULL;int ok=0;if(!json||jsonSize>8u*1024u*1024u||total>LIMIT)goto done;
 file=calloc(1,total);if(!file)goto done;Put32(file,0x46546c67);Put32(file+4,2);Put32(file+8,(uint32_t)total);Put32(file+12,(uint32_t)padded);Put32(file+16,0x4e4f534a);memcpy(file+20,json,jsonSize);memset(file+20+jsonSize,' ',padded-jsonSize);Put32(file+20+padded,(uint32_t)aligned);Put32(file+24+padded,0x004e4942);memcpy(file+28+padded,bin,binSize);
 size_t budget=LIMIT;cgltf_options options={0};options.memory.alloc_func=Allocate;options.memory.free_func=Release;options.memory.user_data=&budget;if(cgltf_parse(&options,file,total,&check)==cgltf_result_success&&cgltf_load_buffers(&options,check,NULL)==cgltf_result_success&&cgltf_validate(check)==cgltf_result_success)ok=Write(output,file,total);
 done:cgltf_free(check);free(file);free(json);return ok;
}
int GltfPack(const char *input,const char *output){
 size_t size=0,budget=LIMIT,binSize=0;unsigned char *bytes=Load(input,&size),*bin=NULL;cgltf_data *data=NULL;yyjson_doc *original=NULL;yyjson_mut_doc *doc=NULL;int ok=0;uint32_t offsets[64];
 cgltf_options options={0};options.memory.alloc_func=Allocate;options.memory.free_func=Release;options.memory.user_data=&budget;
 if(!bytes||cgltf_parse(&options,bytes,size,&data)!=cgltf_result_success||(data->file_type!=cgltf_file_type_gltf&&data->file_type!=cgltf_file_type_glb)||data->json_size>8u*1024u*1024u||!data->buffers_count||data->buffers_count>64||data->images_count>64||data->extensions_used_count||data->extensions_required_count||data->animations_count||data->skins_count||data->cameras_count||data->lights_count)goto done;
 if(cgltf_validate(data)!=cgltf_result_success)goto done;
 original=yyjson_read(data->json,data->json_size,0);doc=yyjson_mut_doc_new(NULL);if(!original||!doc)goto done;yyjson_mut_val *root=yyjson_val_mut_copy(doc,yyjson_doc_get_root(original));yyjson_mut_doc_set_root(doc,root);
 for(size_t i=0;i<data->buffers_count;i++){
  const char *uri=data->buffers[i].uri;size_t count=0;unsigned char *buffer=NULL;if(!uri&&i==0&&data->file_type==cgltf_file_type_glb){count=data->bin_size;buffer=malloc(count);if(buffer)memcpy(buffer,data->bin,count);}else{if(!uri||!UriMatches(original,"buffers",i,uri))goto done;buffer=Asset(input,uri,&count,LIMIT-binSize,0);}size_t required=data->buffers[i].size,padded=(required+3)&~(size_t)3;if(!buffer||count<required||required>LIMIT||padded>LIMIT-binSize){free(buffer);fprintf(stderr,"Cannot read glTF buffer %.160s: use a complete local BIN or valid base64 buffer URI\n",uri?uri:"embedded GLB buffer");goto done;}unsigned char *next=realloc(bin,binSize+padded);if(!next){free(buffer);goto done;}bin=next;offsets[i]=(uint32_t)binSize;memcpy(bin+binSize,buffer,required);memset(bin+binSize+required,0,padded-required);binSize+=padded;free(buffer);
 }
 yyjson_mut_val *views=yyjson_mut_obj_get(root,"bufferViews"),*entry;size_t i,count;if(!views)views=Array(doc,root,"bufferViews");if(!yyjson_mut_is_arr(views))goto done;
 yyjson_mut_arr_foreach(views,i,count,entry){yyjson_mut_val *buffer=yyjson_mut_obj_get(entry,"buffer");if(!yyjson_mut_is_uint(buffer)||yyjson_mut_get_uint(buffer)>=data->buffers_count)goto done;uint32_t index=(uint32_t)yyjson_mut_get_uint(buffer);if(!yyjson_mut_obj_get(entry,"byteOffset"))Number(doc,entry,"byteOffset",0);if(!Shift(doc,entry,"byteOffset",offsets[index]))goto done;yyjson_mut_set_uint(buffer,0);}
 yyjson_mut_val *images=yyjson_mut_obj_get(root,"images");
 for(size_t image=0;image<data->images_count;image++){
  const cgltf_image *source=&data->images[image];unsigned char *png=NULL;size_t pngSize=0;
  if(source->uri){if(!UriMatches(original,"images",image,source->uri))goto done;png=Asset(input,source->uri,&pngSize,32u*1024u*1024u,1);}
  else{if(!source->buffer_view||!source->mime_type)goto done;if(!strcmp(source->mime_type,"image/png"))continue;if(strcmp(source->mime_type,"image/jpeg"))goto done;const cgltf_buffer_view *v=source->buffer_view;pngSize=v->size;if(pngSize>32u*1024u*1024u)goto done;png=malloc(pngSize);if(png)memcpy(png,bin+offsets[v->buffer-data->buffers]+v->offset,pngSize);}
  if(png&&pngSize>=2&&png[0]==255&&png[1]==216){size_t convertedSize=0;unsigned char *converted=JpegPng(png,pngSize,&convertedSize);free(png);png=converted;pngSize=convertedSize;}
  size_t padded=(pngSize+3)&~(size_t)3;if(!png||pngSize<24||memcmp(png,"\x89PNG\r\n\x1a\n",8)||memcmp(png+12,"IHDR",4)||padded>LIMIT-binSize){free(png);fputs("Cannot decode model image; use a valid PNG or JPEG up to 4096 x 4096\n",stderr);goto done;}
  unsigned char *next=realloc(bin,binSize+padded);if(!next){free(png);goto done;}bin=next;memcpy(bin+binSize,png,pngSize);memset(bin+binSize+pngSize,0,padded-pngSize);free(png);
  yyjson_mut_val *view=Element(doc,views);Number(doc,view,"buffer",0);Number(doc,view,"byteOffset",(uint32_t)binSize);Number(doc,view,"byteLength",(uint32_t)pngSize);binSize+=padded;
  yyjson_mut_val *target=yyjson_mut_arr_get(images,image);yyjson_mut_obj_remove_str(target,"uri");yyjson_mut_obj_remove_str(target,"bufferView");yyjson_mut_obj_remove_str(target,"mimeType");Number(doc,target,"bufferView",(uint32_t)yyjson_mut_arr_size(views)-1);Text(doc,target,"mimeType","image/png");
 }
 yyjson_mut_obj_remove_str(root,"buffers");Number(doc,Element(doc,Array(doc,root,"buffers")),"byteLength",(uint32_t)binSize);ok=SaveGlb(doc,bin,binSize,output);
 done:free(bytes);free(bin);cgltf_free(data);yyjson_doc_free(original);yyjson_mut_doc_free(doc);if(!ok)fputs("Cannot import glTF; use a bounded static scene with local or base64 BIN/PNG assets\n",stderr);return ok;
}
