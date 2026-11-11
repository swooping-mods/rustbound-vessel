#ifndef RUSTBOUND_VISUAL_H
#define RUSTBOUND_VISUAL_H
#include "assets/generated/vessel_data.h"
typedef struct {SURoot mesh,material,textures[3],glow;} RVResources;
static RVResources resources;
static PTR texture_ctor,texture_load,set_texture,mesh_tangents,box_center,box_size,get_children,scene_get_layer,scene_set_layer;
static inline PTR rv_object(PTR component){int ok=1;return call(method("UnityEngine","Component","get_gameObject",0),component,0,&ok);}
static inline PTR rv_transform(PTR component){int ok=1;return call(method("UnityEngine","Component","get_transform",0),component,0,&ok);}
#include "scene_adapter.h"
static inline int rv_float_property(PTR material,const char *name,float value){int ok=1;void *a[]={su.string(name),&value};call(su.material_float,material,a,&ok);return ok;}
static inline int rv_texture_property(PTR material,const char *name,PTR texture){int ok=1;void *a[]={su.string(name),texture};call(set_texture,material,a,&ok);return ok;}
static inline PTR rv_texture(const unsigned char *bytes,int length,int linear){
    int ok=1,size=2,format=4;unsigned char mip=1,lin=(unsigned char)linear;PTR texture=su_new("UnityEngine","Texture2D");
    void *ctor[]={&size,&size,&format,&mip,&lin};call(texture_ctor,texture,ctor,&ok);if(!ok)return 0;
    PTR data=su_array(su.byte_class,(U64)length,bytes,1);void *load[]={texture,data};PTR loaded=call(texture_load,0,load,&ok);
    if(!ok||!loaded||!*(unsigned char *)unbox(loaded)){su_destroy(texture);return 0;}return texture;
}
static inline PTR rv_material(PTR shader){int ok=1;PTR m=su_new("UnityEngine","Material");void *a[]={shader};call(su.material_ctor,m,a,&ok);return ok?m:0;}
static inline int rv_resources(void){
    if(resources.mesh)return 1;
    texture_ctor=su_method("UnityEngine","Texture2D",".ctor","System.Int32;System.Int32;UnityEngine.TextureFormat;System.Boolean;System.Boolean");
    texture_load=su_method("UnityEngine","ImageConversion","LoadImage","UnityEngine.Texture2D;System.Byte[]");
    set_texture=su_method("UnityEngine","Material","SetTexture","System.String;UnityEngine.Texture");
    mesh_tangents=su_method("UnityEngine","Mesh","RecalculateTangents","");
    box_center=su_method("UnityEngine","BoxCollider","set_center","UnityEngine.Vector3");box_size=su_method("UnityEngine","BoxCollider","set_size","UnityEngine.Vector3");
    // The non-generic public children overload is stripped in this build.
    // Generic wrappers retain their shared typed native enumeration boundary.
    get_children=su_method("UnityEngine","GameObject","GetComponentsInternal","System.Type;System.Boolean;System.Boolean;System.Boolean;System.Boolean;System.Object");
    scene_get_layer=su_method("UnityEngine","GameObject","get_layer","");scene_set_layer=su_method("UnityEngine","GameObject","set_layer","System.Int32");
    if(!texture_ctor||!texture_load||!set_texture||!mesh_tangents||!box_center||!box_size||!get_children||!scene_get_layer||!scene_set_layer)return 0;
    PTR shader=0;const char *names[]={"Universal Render Pipeline/Lit","Standard"};int ok=1;
    for(unsigned i=0;i<2&&!shader;i++){void *a[]={su.string(names[i])};shader=call(su.shader_find,0,a,&ok);}
    if(!shader)return 0;PTR m=rv_material(shader);if(!m)return 0;resources.material=su_hold(m);
    const unsigned char *payloads[]={rv_data_albedo,rv_data_normal,rv_data_metallic};const int sizes[]={sizeof(rv_data_albedo)-1,sizeof(rv_data_normal)-1,sizeof(rv_data_metallic)-1};
    for(int i=0;i<3;i++){PTR t=rv_texture(payloads[i],sizes[i],i!=0);if(!t)return 0;resources.textures[i]=su_hold(t);}
    if(!rv_texture_property(m,"_BaseMap",su_get(resources.textures[0]))||!rv_texture_property(m,"_MainTex",su_get(resources.textures[0]))
       ||!rv_texture_property(m,"_BumpMap",su_get(resources.textures[1]))||!rv_texture_property(m,"_MetallicGlossMap",su_get(resources.textures[2])))return 0;
    SUColor white={1,1,1,1};void *color[]={&white};call(su.material_color,m,color,&ok);
    // Shipped shader variants can omit the runtime-enabled metallic map.
    // Keep that scalar fallback rough instead of turning rusted paint to chrome.
    rv_float_property(m,"_Metallic",.5f);rv_float_property(m,"_Smoothness",.28f);rv_float_property(m,"_GlossMapScale",.28f);rv_float_property(m,"_BumpScale",.7f);rv_float_property(m,"_Cull",2);
    const char *keywords[]={"_NORMALMAP","_METALLICSPECGLOSSMAP","_METALLICGLOSSMAP"};
    for(int i=0;i<3;i++){void *a[]={su.string(keywords[i])};call(su.material_keyword,m,a,&ok);}
    PTR mesh=su_new("UnityEngine","Mesh");call(su.mesh_ctor,mesh,0,&ok);if(!ok)return 0;resources.mesh=su_hold(mesh);
    PTR vertices=su_array(su.vector_class,RV_VERTEX_COUNT,rv_data_positions,12);void *va[]={vertices};call(su.mesh_vertices,mesh,va,&ok);
    PTR normals=su_array(su.vector_class,RV_VERTEX_COUNT,rv_data_normals,12);void *na[]={normals};call(su.mesh_normals,mesh,na,&ok);
    PTR uvs=su_array(su.uv_class,RV_VERTEX_COUNT,rv_data_uv,8);void *ua[]={uvs};call(su.mesh_uv,mesh,ua,&ok);
    PTR indices=su_array(su.int_class,RV_INDEX_COUNT,rv_data_indices,4);int sub=0;unsigned char bounds=1;void *ia[]={indices,&sub,&bounds};call(su.mesh_indices,mesh,ia,&ok);
    call(mesh_tangents,mesh,0,&ok);call(su.mesh_bounds,mesh,0,&ok);return ok;
}
static inline PTR rv_attach_mesh(PTR parent,int layer){
    int ok=1;PTR go=rv_child("Rustbound Thunder Vessel",parent);if(!go)return 0;if(!rv_layer(go,layer)){su_destroy(go);return 0;}
    PTR filter=su_add_component(go,"MeshFilter"),renderer=su_add_component(go,"MeshRenderer");if(!filter||!renderer){su_destroy(go);return 0;}
    void *ma[]={su_get(resources.mesh)};call(su.meshfilter_set,filter,ma,&ok);
    PTR material=su_get(resources.material),materials=su_array(su.material_class,1,&material,8);void *ra[]={materials};call(su.renderer_materials,renderer,ra,&ok);
    if(!ok){su_destroy(go);return 0;}return go;
}
static inline PTR rv_children(PTR component,const char *name){
    int ok=1;unsigned char typed=1,recursive=1,inactive=1,reverse=0;PTR type=su_type("UnityEngine",name);
    void *a[]={type,&typed,&recursive,&inactive,&reverse,0};return call(get_children,rv_object(component),a,&ok);
}
static inline PTR rv_box(PTR parent,const char *name,SPVec3 center,SPVec3 size,int layer){
    PTR go=rv_child(name,parent);if(!go)return 0;if(!rv_layer(go,layer)){su_destroy(go);return 0;}
    PTR box=su_add_component(go,"BoxCollider");if(!box){su_destroy(go);return 0;}
    su_set_vector(box_center,box,center);su_set_vector(box_size,box,size);return go;
}
#endif
