/* Compiled, bounded geometry interchange. GLB is self-contained; the car
 * editor retains its existing texture bindings. Whole-car role assembly and
 * texture authoring are separate contracts, not silently flattened here. */
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

int GlbExportTextures(const char *input,unsigned part,const char *output,const char *textureDirectory){
 size_t size=0,jsonSize=0;unsigned char *source=Load(input,&size),*binary=NULL,*file=NULL;char *json=NULL;yyjson_mut_doc *doc=NULL;int ok=0;
 RageRuntimeMesh mesh;uint32_t first,count;
 if(!source||!RuntimeMeshOpen(&mesh,source,size)||!RuntimeMeshRange(&mesh,part,&first,&count)||!count||count>MAX_VERTICES||count%3)goto done;
 binary=malloc((size_t)count*48);doc=yyjson_mut_doc_new(NULL);if(!binary||!doc)goto done;
 yyjson_mut_val *root=yyjson_mut_obj(doc);yyjson_mut_doc_set_root(doc,root);
 yyjson_mut_val *asset=Object(doc,root,"asset");Text(doc,asset,"version","2.0");Text(doc,asset,"generator","Rage Racer Editor");Text(doc,Object(doc,asset,"extras"),"rr_units","native Y-up");
 yyjson_mut_val *buffers=Array(doc,root,"buffers"),*views=Array(doc,root,"bufferViews"),*accessors=Array(doc,root,"accessors"),*materials=Array(doc,root,"materials"),*meshes=Array(doc,root,"meshes"),*nodes=Array(doc,root,"nodes"),*scenes=Array(doc,root,"scenes");
 Number(doc,Element(doc,buffers),"byteLength",count*48);yyjson_mut_val *view=Element(doc,views);Number(doc,view,"buffer",0);Number(doc,view,"byteLength",count*48);Number(doc,view,"byteStride",48);Number(doc,view,"target",34962);
 yyjson_mut_val *model=Element(doc,meshes),*primitives=Array(doc,model,"primitives");Text(doc,model,"name","part");yyjson_mut_val *node=Element(doc,nodes);Number(doc,node,"mesh",0);Text(doc,node,"name","part");yyjson_mut_arr_add_uint(doc,Array(doc,Element(doc,scenes),"nodes"),0);Number(doc,root,"scene",0);
 size_t binarySize=(size_t)count*48;uint32_t textureSlots[64];for(unsigned i=0;i<64;i++)textureSlots[i]=UINT32_MAX;
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
    if(j&&v.material!=triangleWord)goto done;triangleWord=v.material;
    if(!RuntimeVertexEncode(encoded,sizeof(encoded),&v))goto done;
    memcpy(binary+(size_t)(runEnd+j)*48,encoded,36);
    memcpy(binary+(size_t)(runEnd+j)*48+36,encoded+12,12);
    RageRuntimeVertex standard=v;double length=sqrt((double)v.normal[0]*v.normal[0]+(double)v.normal[1]*v.normal[1]+(double)v.normal[2]*v.normal[2]);
    for(unsigned n=0;n<3;n++)standard.normal[n]=length>1e-20f?v.normal[n]/length:(n==1?1.0f:0.0f);
    if(!RuntimeVertexEncode(encoded,sizeof(encoded),&standard))goto done;memcpy(binary+(size_t)(runEnd+j)*48+12,encoded+12,12);
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
   unsigned id=word&65535,sourceId=id%64;
   if(textureDirectory&&id!=65535&&id/64!=1){
    if(textureSlots[sourceId]==UINT32_MAX){
     char filename[4096];int length=snprintf(filename,sizeof(filename),"%s/texture-%u.png",textureDirectory,sourceId);size_t imageSize=0;unsigned char *image=NULL;
     if(length<0||length>=(int)sizeof(filename)||(image=Load(filename,&imageSize))==NULL)goto done;
     if(imageSize<24||imageSize>32u*1024u*1024u||memcmp(image,"\x89PNG\r\n\x1a\n",8)||memcmp(image+12,"IHDR",4)){free(image);goto done;}
     uint32_t w=(uint32_t)image[16]<<24|(uint32_t)image[17]<<16|(uint32_t)image[18]<<8|image[19],h=(uint32_t)image[20]<<24|(uint32_t)image[21]<<16|(uint32_t)image[22]<<8|image[23];if(!w||!h||w>4096||h>4096){free(image);goto done;}
     size_t padded=(imageSize+3)&~(size_t)3;if(padded>LIMIT-binarySize){free(image);goto done;}unsigned char *next=realloc(binary,binarySize+padded);if(!next){free(image);goto done;}binary=next;memcpy(binary+binarySize,image,imageSize);memset(binary+binarySize+imageSize,0,padded-imageSize);free(image);
     uint32_t viewIndex=(uint32_t)yyjson_mut_arr_size(views),imageIndex=(uint32_t)yyjson_mut_arr_size(images);yyjson_mut_val *v=Element(doc,views);Number(doc,v,"buffer",0);Number(doc,v,"byteOffset",(uint32_t)binarySize);Number(doc,v,"byteLength",(uint32_t)imageSize);
     yyjson_mut_val *img=Element(doc,images);Number(doc,img,"bufferView",viewIndex);Text(doc,img,"mimeType","image/png");textureSlots[sourceId]=(uint32_t)yyjson_mut_arr_size(textures);yyjson_mut_val *tex=Element(doc,textures);Number(doc,tex,"source",imageIndex);Number(doc,tex,"sampler",0);binarySize+=padded;
    }
    Number(doc,Object(doc,pbr,"baseColorTexture"),"index",textureSlots[sourceId]);
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
 for(unsigned i=0;i<count;i++)if(!isfinite(value[i]))return 0;return 1;
}
static int Active(const cgltf_scene *scene,const cgltf_node *node){
 for(unsigned depth=0;node&&depth<64;depth++,node=node->parent)for(size_t i=0;i<scene->nodes_count;i++)if(scene->nodes[i]==node)return 1;
 return 0;
}
int GlbImportTextures(const char *input,const char *output,const char *textureDirectory){
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
   if(!RuntimeVertexEncode(wire+layout.verticesOffset+cursor*40,40,&v))goto done;Put32(wire+layout.indicesOffset+cursor*4,(uint32_t)cursor);cursor++;
  }
 }
 RageRuntimeMesh checked;if(!RuntimeMeshOpen(&checked,wire,layout.totalSize))goto done;
 const unsigned char *png[64]={0};size_t pngSize[64]={0};
 if(textureDirectory)for(size_t i=0;i<node->mesh->primitives_count;i++){
  const cgltf_material *m=node->mesh->primitives[i].material;const cgltf_texture_view *view=&m->pbr_metallic_roughness.base_color_texture;
  if(m->normal_texture.texture||m->occlusion_texture.texture||m->emissive_texture.texture||m->pbr_metallic_roughness.metallic_roughness_texture.texture){fputs("Only base-color PNG textures are supported by this part import\n",stderr);goto done;}
  if(!view->texture)continue;
  uint32_t word;if(!Material(m,&word)||(word&65535)==65535||(word&65535)/64==1||view->texcoord!=0||view->has_transform){fputs("PNG mapping requires a textured native surface with unchanged UV mapping\n",stderr);goto done;}
  const cgltf_image *image=view->texture->image;if(!image||image->uri||!image->buffer_view||!image->mime_type||strcmp(image->mime_type,"image/png"))goto done;
  const unsigned char *bytes=cgltf_buffer_view_data(image->buffer_view);size_t n=image->buffer_view->size;
  if(!bytes||n<24||n>32u*1024u*1024u||memcmp(bytes,"\x89PNG\r\n\x1a\n",8)||memcmp(bytes+12,"IHDR",4))goto done;
  uint32_t w=(uint32_t)bytes[16]<<24|(uint32_t)bytes[17]<<16|(uint32_t)bytes[18]<<8|bytes[19],h=(uint32_t)bytes[20]<<24|(uint32_t)bytes[21]<<16|(uint32_t)bytes[22]<<8|bytes[23];if(!w||!h||w>4096||h>4096)goto done;
  unsigned sourceId=(word&65535)%64;if(png[sourceId]&&(pngSize[sourceId]!=n||memcmp(png[sourceId],bytes,n))){fputs("Different PNGs claim the same native texture source; resolve material mapping first\n",stderr);goto done;}
  png[sourceId]=bytes;pngSize[sourceId]=n;
 }
 char filenames[64][4096];unsigned written=0,slots[64];
 if(textureDirectory)for(unsigned i=0;i<64;i++)if(png[i]){int n=snprintf(filenames[written],sizeof(filenames[written]),"%s/texture-%u.png",textureDirectory,i);if(n<0||n>=(int)sizeof(filenames[written]))goto done;FILE *exists=fopen(filenames[written],"rb");if(exists){fclose(exists);goto done;}slots[written++]=i;}
 unsigned saved=0;for(;saved<written;saved++)if(!Write(filenames[saved],png[slots[saved]],pngSize[slots[saved]]))break;
 if(saved!=written){while(saved)remove(filenames[--saved]);goto done;}
 ok=Write(output,wire,layout.totalSize);if(!ok){while(saved)remove(filenames[--saved]);}
 if(ok&&textureDirectory){printf("{\"textures\":[");for(unsigned i=0;i<written;i++)printf("%s%u",i?",":"",slots[i]);puts("]}");}
 done:cgltf_free(data);free(source);free(wire);if(!ok)fputs("Cannot import GLB geometry; input must be valid, bounded and mapped to one static part\n",stderr);return ok;
}

int GlbExport(const char *input,unsigned part,const char *output){return GlbExportTextures(input,part,output,NULL);}
int GlbImport(const char *input,const char *output){return GlbImportTextures(input,output,NULL);}
