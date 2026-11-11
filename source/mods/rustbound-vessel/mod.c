/* Rustbound vessel: Unity scene calls run on the observed game thread. */
#include "../../runtime/native/gameplay.h"
#include "../vehicle-framework/sdk/client.h"
#include "assets/generated/garage_icon.h"
#define RV_VEHICLE_ID 22
static const MoonVehicles *vehicle_framework;
#include "inspect.h"
#include "unity.h"
#include "flight_core.h"
#include "hover_profile.h"
#include "visual.h"
int _fltused=0;
/* Large POD snapshots can lower to this intrinsic in the freestanding DLL. */
void *memcpy(void *to,const void *from,U64 n){su_copy(to,from,n);return to;}
static PTR vessel_log;
static int ticks,thread_id,world_frames,failed;
static SURoot world_root,walker_root,equipment_root;
static PTR world_field,walker_body_field,restoring,ui_blocked,typing,focused,find_objects,component_component,component_parent;
static PTR body_field,owner_method,driven_method,index_method,remote_index,authored_field,vehicle_enter;
static PTR rb_velocity,rb_get_velocity,rb_position,rb_move_rotation,rb_angular,rb_constraints,rb_get_constraints;
static PTR rb_ccd,rb_interpolate,rb_gravity,fixed_delta,hit_body,ray_hits,notify;
static PTR vehicle_grounded,vehicle_airborne,vehicle_wheels,vehicle_drive,vehicle_steer,vehicle_air_control;
static PTR renderer_get_enabled,collider_get_enabled,collider_body,collider_trigger;
static SURoot query_hits;
static short (*key_state)(int);
static PTR (*foreground_window)(void);
static DWORD (*window_pid)(PTR,DWORD *);
typedef struct {
    SURoot vehicle,body,transform,visual,audio,tracks;
    int tracks_enabled;
    SURoot hidden[192],colliders[64];unsigned renderer_count,collider_count;
    unsigned char render_enabled[192],collider_enabled[64];
    /* Read-only framework snapshot for audio and private scene probes. */
    RVFlight flight;RVOutput output;int remote;
    RVVec floor_up,previous_position;RVPreview preview;int has_position;
    int terrain_ticks,preview_ticks,missing_floor_ticks;float terrain_min_clearance,terrain_nose_up;float clearance,observe_landing_until;int has_floor;
} RVInstance;
static RVInstance vessels[32];
#include "hull_adapter.h"
static RVVec center;
static PTR audio_pitch,audio_volume,audio_stop,audio_play,audio_clip,audio_set_clip,audio_loop;
static inline RVVec rv_from(SPVec3 v){return rv_v(v.x,v.y,v.z);}
static inline SPVec3 rv_to(RVVec v){return su_v(v.x,v.y,v.z);}
static void event(const char *name){
    char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"ticks\":");p=num(p,(U64)ticks);p=cat(p,"}\n");
    DWORD n=0;WriteFile(vessel_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(vessel_log);
}
static char *fixed_text(char *p,float value){
    if(!rv_finite(value))return cat(p,"null");if(value<0){*p++='-';value=-value;}
    U64 n=(U64)(value*1000+.5f);p=num(p,n/1000);*p++='.';*p++=(char)('0'+n/100%10);*p++=(char)('0'+n/10%10);*p++=(char)('0'+n%10);*p=0;return p;
}
static char *vector_text(char *p,RVVec v){*p++='[';p=fixed_text(p,v.x);*p++=',';p=fixed_text(p,v.y);*p++=',';p=fixed_text(p,v.z);*p++=']';*p=0;return p;}
static void sample(const char *name,RVInstance *v,const RVOutput *o,RVVec position,RVVec measured){
    char b[1536],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"ticks\":");p=num(p,(U64)ticks);p=cat(p,",\"seconds\":");p=fixed_text(p,su_number(su.time_unscaled,0));p=cat(p,",\"position\":");p=vector_text(p,position);
    p=cat(p,",\"measured_velocity\":");p=vector_text(p,measured);p=cat(p,",\"commanded_velocity\":");p=vector_text(p,o->velocity);p=cat(p,",\"pitch\":");p=fixed_text(p,o->pitch);
    p=cat(p,",\"measured_up\":");p=vector_text(p,rv_from(su_vector(su.tr_up,su_get(v->transform))));p=cat(p,",\"gravity_up\":");p=vector_text(p,v->flight.up);
    p=cat(p,",\"roll\":");p=fixed_text(p,o->roll);p=cat(p,",\"clearance\":");p=fixed_text(p,v->clearance);
    p=cat(p,",\"terrain_ticks\":");p=num(p,(U64)v->terrain_ticks);p=cat(p,",\"preview_ticks\":");p=num(p,(U64)v->preview_ticks);
    p=cat(p,",\"missing_floor_ticks\":");p=num(p,(U64)v->missing_floor_ticks);p=cat(p,",\"terrain_min_clearance\":");p=fixed_text(p,v->terrain_min_clearance);
    p=cat(p,",\"terrain_nose_up\":");p=fixed_text(p,v->terrain_nose_up);
    p=cat(p,",\"forward\":");p=vector_text(p,o->forward);p=cat(p,",\"floor_up\":");p=vector_text(p,v->floor_up);
    p=cat(p,",\"preview\":");p=num(p,(U64)v->preview.valid);p=cat(p,",\"preview_gap\":");p=fixed_text(p,v->preview.clearance);
    p=cat(p,",\"preview_distance\":");p=fixed_text(p,v->preview.distance);p=cat(p,",\"preview_up\":");p=vector_text(p,v->preview.normal);
    p=cat(p,",\"has_floor\":");p=num(p,(U64)v->has_floor);p=cat(p,",\"landing\":");p=num(p,(U64)v->flight.landing);p=cat(p,"}\n");DWORD n=0;WriteFile(vessel_log,b,(DWORD)(p-b),&n,0);
}
static PTR game_component(PTR component,const char *name){int ok=1;PTR type=su_type("MoonCleaners",name);void *a[]={type};return call(component_component,component,a,&ok);}
static int key(int code){return (key_state(code)&0x8000)!=0;}
static int input_allowed(PTR walker){
    int ok=1;DWORD pid=0;window_pid(foreground_window(),&pid);
    return pid==GetCurrentProcessId()&&boolean(focused,0,&ok)&&!boolean(ui_blocked,walker,&ok)&&!boolean(typing,0,&ok)&&ok;
}
static void test_shortcuts(PTR walker,PTR world);
static int test_bind(void);
static int test_tools;
static void find_equipment(PTR walker){
    if(su_alive(su_get(equipment_root)))return;su_drop(&equipment_root);
    PTR equipment=game_component(walker,"MoonPrototypeEquipment");
    if(!equipment){
        int ok=1,inactive=0,sort=0;PTR type=su_type("MoonCleaners","MoonPrototypeEquipment");void *a[]={type,&inactive,&sort};PTR array=call(find_objects,0,a,&ok);
        U64 n=array?su.array_length(array):0;if(n>64)return;
        for(U64 i=0;i<n;i++){PTR candidate=*(PTR *)su.array_address(array,8,i);PTR player=call(method("MoonCleaners","MoonPrototypeEquipment","get_Player",0),candidate,0,&ok);if(ok&&player==walker){equipment=candidate;break;}}
    }
    if(equipment){equipment_root=su_hold(equipment);event("equipment-ready");}
}

static RVInstance *instance(PTR vehicle){for(unsigned i=0;i<32;i++)if(su_get(vessels[i].vehicle)==vehicle)return &vessels[i];return 0;}
static void stop_thrust(RVInstance *v){if(v->audio){int ok=1;su_float(audio_volume,su_get(v->audio),0);call(audio_stop,su_get(v->audio),0,&ok);}}
static int end_flight(RVInstance *v){stop_thrust(v);return 1;}
static int drop_instance(RVInstance *v){
    if(!end_flight(v))return 0;
    if(v->audio){int ok=1;call(audio_stop,su_get(v->audio),0,&ok);su_drop(&v->audio);}
    for(unsigned i=0;i<v->renderer_count;i++){PTR renderer=su_get(v->hidden[i]);if(su_alive(renderer))su_bool(su.renderer_enabled,renderer,v->render_enabled[i]);su_drop(&v->hidden[i]);}
    rv_hull_restore(v);
    if(su_alive(su_get(v->visual)))su_destroy(su_get(v->visual));su_drop(&v->visual);su_drop(&v->body);su_drop(&v->transform);su_drop(&v->vehicle);su_zero(v,sizeof(*v));return 1;
}
static int add_instance(PTR vehicle,int remote){
    if(instance(vehicle))return 1;RVInstance *v=0;for(unsigned i=0;i<32;i++)if(!vessels[i].vehicle){v=&vessels[i];break;}if(!v)return 0;
    int ok=1;if(integer(remote?remote_index:index_method,vehicle,&ok)!=RV_VEHICLE_ID||!ok)return 1;
    PTR authored=0,body=0;field_value(vehicle,remote?get_field(object_class(vehicle),"<AuthoredView>k__BackingField"):authored_field,&authored);
    field_value(vehicle,remote?get_field(object_class(vehicle),"body"):body_field,&body);if(!su_alive(authored)||(!remote&&!su_alive(body)))return 1;
    PTR tr=rv_transform(vehicle);if(!su_alive(tr))return 0;
    int model_layer=integer(scene_get_layer,rv_object(authored),&ok);if(!ok)return 0;
    v->vehicle=su_hold(vehicle);v->transform=su_hold(tr);v->body=su_hold(body);v->remote=remote;
    // AuthoredView may share the carrier root. Snapshot/hide its renderers
    // before adding our child, or recursive enumeration hides our mesh too.
    PTR renderers=rv_children(authored,"Renderer");if(!renderers){drop_instance(v);return 0;}U64 count=su.array_length(renderers);
    if(count>192){drop_instance(v);return 0;}
    for(U64 i=0;i<count;i++){PTR r=*(PTR *)su.array_address(renderers,8,i);if(su_alive(r)){v->render_enabled[v->renderer_count]=(unsigned char)boolean(renderer_get_enabled,r,&ok);v->hidden[v->renderer_count++]=su_hold(r);su_bool(su.renderer_enabled,r,0);}}
    if(!ok){drop_instance(v);return 0;}
    // A private child owns every authored addition. Shared source resources
    // and vanilla audio clips are never edited or destroyed.
    if(!remote&&!vehicle_framework->configure_instance(vehicle,authored,0)){drop_instance(v);return 0;}
    PTR container=rv_child("Rustbound Hover Vessel",tr);if(!container){drop_instance(v);return 0;}
    v->visual=su_hold(container);
    if(!rv_attach_mesh(su_transform(container),model_layer)){drop_instance(v);return 0;}
    {
        // The source's fenders have open undersides. A low platform and cabin
        // are solid; three boxes avoid a convex hull closing the entire arch.
        if(!rv_hull_make(v,vehicle,body,su_transform(container))){drop_instance(v);return 0;}
        if(!remote){su_int(rb_ccd,body,2);su_int(rb_interpolate,body,1);}
    }
    event(remote?"remote-vessel-created":"vessel-created");return 1;
}
static void scan_type(const char *name,int remote){
    int ok=1,inactive=0,sort=0;PTR type=su_type("MoonCleaners",name);void *a[]={type,&inactive,&sort};PTR array=call(find_objects,0,a,&ok);
    if(!ok||!array)return;U64 count=su.array_length(array);if(count>512){event("vehicle-capacity-failed");failed=1;return;}
    for(U64 i=0;i<count;i++){PTR obj=*(PTR *)su.array_address(array,8,i);if(su_alive(obj)&&!add_instance(obj,remote)){event("vessel-create-failed");failed=1;return;}}
}
static int sync_hover(RVInstance *v){
    MoonHoverState state;if(!vehicle_framework->hover_state(su_get(v->vehicle),&state,sizeof(state)))return 0;
    v->flight=state.flight;v->output=state.output;v->floor_up=state.floor_up;v->preview=state.preview;v->clearance=state.clearance;v->has_floor=state.has_floor;
    v->terrain_ticks=(int)state.fixed_ticks;v->preview_ticks=(int)state.preview_ticks;v->missing_floor_ticks=(int)state.missing_floor_ticks;
    v->terrain_min_clearance=state.min_clearance;v->terrain_nose_up=state.max_nose_up;return 1;
}
static int ground(RVInstance *v,RVVec p,RVVec up,float *clearance){
    (void)p;(void)up;if(!sync_hover(v))return 0;*clearance=v->clearance;return v->has_floor;
}
static void thrust_audio(RVInstance *v,float power){
    if(!v->audio){
        PTR donor=game_component(su_get(v->vehicle),"MoonVehicleAudio"),source=0;if(donor)field_value(donor,get_field(object_class(donor),"drive"),&source);
        int ok=1;PTR clip=source?call(audio_clip,source,0,&ok):0;if(!clip)return;
        PTR own=su_add_component(su_get(v->visual),"AudioSource");if(!own)return;void *a[]={clip};call(audio_set_clip,own,a,&ok);su_bool(audio_loop,own,1);
        su_float(method("UnityEngine","AudioSource","set_spatialBlend",1),own,1);su_float(method("UnityEngine","AudioSource","set_minDistance",1),own,3);
        su_float(method("UnityEngine","AudioSource","set_maxDistance",1),own,45);su_float(audio_volume,own,0);call(audio_play,own,0,&ok);if(!ok)return;v->audio=su_hold(own);event("vanilla-drive-audio-reused");
    }
    su_float(audio_pitch,su_get(v->audio),.72f+power*.6f);su_float(audio_volume,su_get(v->audio),v->flight.active?.10f+power*.12f:0);
}
static void world_drop(void){
    for(unsigned i=0;i<32;i++)if(vessels[i].vehicle)drop_instance(&vessels[i]);su_drop(&world_root);su_drop(&walker_root);su_drop(&equipment_root);world_frames=0;event("world-teardown");
}
#include "test_tools.h"
static int resources_ready;
static int prepare_resources(void){if(!resources_ready)resources_ready=rv_resources();return resources_ready;}
static int vehicle_update(PTR walker){
    if(!thread_id){thread_id=(int)GetCurrentThreadId();event("main-thread");}if((DWORD)thread_id!=GetCurrentThreadId())return 0;
    if(failed){for(unsigned i=0;i<32;i++)if(vessels[i].vehicle)drop_instance(&vessels[i]);return 0;}
    ticks++;PTR world=0;field_value(walker,world_field,&world);
    if(world_root&&(!su_alive(su_get(world_root))||su_get(world_root)!=world||su_get(walker_root)!=walker))world_drop();if(!su_alive(world))return 0;
    int ok=1;if(boolean(restoring,world,&ok)||!ok){world_frames=0;return 0;}if(++world_frames<120)return 0;
    if(!world_root){
        world_root=su_hold(world);walker_root=su_hold(walker);find_equipment(walker);center=rv_from(su_vector(su.tr_get_position,rv_transform(world)));
        if(!prepare_resources()){event("resource-create-failed");failed=1;return 0;}event("world-ready");scan_type("MoonPrototypeVehicle",0);scan_type("MoonRemoteVehicle",1);
    }
    if(ticks%60==0){
        find_equipment(walker);
        for(unsigned i=0;i<32;i++)if(vessels[i].vehicle&&!su_alive(su_get(vessels[i].vehicle)))drop_instance(&vessels[i]);
        scan_type("MoonPrototypeVehicle",0);scan_type("MoonRemoteVehicle",1);
    }
    test_shortcuts(walker,world);
    for(unsigned i=0;i<32;i++)if(vessels[i].vehicle&&!vessels[i].remote&&sync_hover(vessels+i)){
        RVInstance *v=vessels+i;thrust_audio(v,v->flight.active?v->output.power:0);
        if(ticks%20==0){MoonHoverState state;if(vehicle_framework->hover_state(su_get(v->vehicle),&state,sizeof(state)))sample("flight-sample",v,&state.output,state.position,state.measured);}
    }
    return 0;
}
static void framework_update(void *walker){vehicle_update(walker);}
static int framework_spawn(void *vehicle,int remote){return !failed&&prepare_resources()&&add_instance(vehicle,remote)&&instance(vehicle)!=0;}
static void framework_destroy(void *vehicle,int remote){(void)remote;RVInstance *v=instance(vehicle);if(v)drop_instance(v);}
static MoonVehicleDefinition rustbound_definition={
    .abi=MV_ABI,.size=sizeof(MoonVehicleDefinition),.key="rustbound-vessel.rustbound",.id=RV_VEHICLE_ID,.template_id=4,
    .name="Rustbound Hover Vessel",.hint="Enclosed fast hover vessel. Shift+F9 lift/land; Space/Ctrl altitude; Shift boost.",
    .summary="RUSTBOUND | Enclosed hover vessel. Smooth banking, altitude hold and 230 km/h boost.",
    .price=1500,.capacity=5,.garage=1,.width=2.55f,.depth=4.2f,.height=1.98f,.speed=12,
    .cockpit=1,.fit_seat=1,.driver={0,.65f,.4f},.seat={0,.25f,.4f},
    .icon_png=rv_icon_png,.icon_size=sizeof(rv_icon_png),.icon_width=384,.icon_height=384,
    .update=framework_update,.spawn=framework_spawn,.destroy=framework_destroy,.hover=&rv_hover_profile
};
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,1000);if(!n||n>=1000)return 0;
    cat(cat(path,folder),"\\rustbound-vessel.jsonl");vessel_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(vessel_log==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)||!sp_inspect(game,folder)){event("bind-failed");return 0;}
    if(!su_bind(game)){event(su.failed?su.failed:"unity-bind-failed");return 0;}
    PTR guard=su.new_array(su.byte_class,1);SURoot guard_root=su_hold(guard);
    if(!guard_root||su_get(guard_root)!=guard){event("gc-handle-failed");return 0;}su_drop(&guard_root);event("gc-handle-roundtrip");
    world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");walker_body_field=get_field(find_class("MoonCleaners","MoonWalker"),"body");body_field=get_field(find_class("MoonCleaners","MoonPrototypeVehicle"),"body");authored_field=get_field(find_class("MoonCleaners","MoonPrototypeVehicle"),"authored");
#define RV_BIND(V,NS,T,N,S) V=su_method(NS,T,N,S);if(!V){event(T "::" N "-bind-failed");return 0;}
    RV_BIND(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");RV_BIND(ui_blocked,"MoonCleaners","MoonWalker","get_UIBlocked","");
    RV_BIND(typing,"MoonCleaners","MoonInput","get_Typing","");RV_BIND(focused,"UnityEngine","Application","get_isFocused","");
    RV_BIND(owner_method,"MoonCleaners","MoonPrototypeVehicle","get_Owner","");RV_BIND(driven_method,"MoonCleaners","MoonPrototypeVehicle","get_IsDriven","");
    RV_BIND(vehicle_enter,"MoonCleaners","MoonPrototypeVehicle","Enter","");
    RV_BIND(index_method,"MoonCleaners","MoonPrototypeVehicle","get_Index","");RV_BIND(remote_index,"MoonCleaners","MoonRemoteVehicle","get_Index","");
    RV_BIND(vehicle_grounded,"MoonCleaners","MoonPrototypeVehicle","set_Grounded","System.Boolean");RV_BIND(vehicle_airborne,"MoonCleaners","MoonPrototypeVehicle","set_InAir","System.Boolean");
    RV_BIND(vehicle_wheels,"MoonCleaners","MoonPrototypeVehicle","set_WheelsOnGround","System.Int32");
    RV_BIND(vehicle_air_control,"MoonCleaners","MoonPrototypeVehicle","set_AirControl","System.Boolean");
    RV_BIND(vehicle_drive,"MoonCleaners","MoonPrototypeVehicle","get_DriveInput","");RV_BIND(vehicle_steer,"MoonCleaners","MoonPrototypeVehicle","get_Steering","");
    RV_BIND(find_objects,"UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode");
    RV_BIND(component_component,"UnityEngine","Component","GetComponent","System.Type");
    RV_BIND(component_parent,"UnityEngine","Component","GetComponentInParent","System.Type;System.Boolean");
    RV_BIND(rb_velocity,"UnityEngine","Rigidbody","set_linearVelocity","UnityEngine.Vector3");RV_BIND(rb_get_velocity,"UnityEngine","Rigidbody","get_linearVelocity","");
    RV_BIND(rb_position,"UnityEngine","Rigidbody","get_position","");RV_BIND(rb_move_rotation,"UnityEngine","Rigidbody","MoveRotation","UnityEngine.Quaternion");
    RV_BIND(rb_angular,"UnityEngine","Rigidbody","set_angularVelocity","UnityEngine.Vector3");RV_BIND(rb_constraints,"UnityEngine","Rigidbody","set_constraints","UnityEngine.RigidbodyConstraints");
    RV_BIND(rb_get_constraints,"UnityEngine","Rigidbody","get_constraints","");RV_BIND(rb_ccd,"UnityEngine","Rigidbody","set_collisionDetectionMode","UnityEngine.CollisionDetectionMode");
    RV_BIND(rb_interpolate,"UnityEngine","Rigidbody","set_interpolation","UnityEngine.RigidbodyInterpolation");RV_BIND(rb_gravity,"UnityEngine","Rigidbody","set_useGravity","System.Boolean");
    RV_BIND(fixed_delta,"UnityEngine","Time","get_fixedDeltaTime","");RV_BIND(hit_body,"UnityEngine","RaycastHit","get_rigidbody","");
    RV_BIND(ray_hits,"UnityEngine","Physics","RaycastNonAlloc","UnityEngine.Vector3;UnityEngine.Vector3;UnityEngine.RaycastHit[];System.Single;System.Int32;UnityEngine.QueryTriggerInteraction");
    RV_BIND(notify,"MoonCleaners","MoonPrototypeEquipment","Notify","System.String");
    RV_BIND(audio_pitch,"UnityEngine","AudioSource","set_pitch","System.Single");RV_BIND(audio_volume,"UnityEngine","AudioSource","set_volume","System.Single");
    RV_BIND(audio_stop,"UnityEngine","AudioSource","Stop","");RV_BIND(audio_play,"UnityEngine","AudioSource","Play","");
    RV_BIND(audio_clip,"UnityEngine","AudioSource","get_clip","");RV_BIND(audio_set_clip,"UnityEngine","AudioSource","set_clip","UnityEngine.AudioClip");RV_BIND(audio_loop,"UnityEngine","AudioSource","set_loop","System.Boolean");
    RV_BIND(renderer_get_enabled,"UnityEngine","Renderer","get_enabled","");RV_BIND(collider_get_enabled,"UnityEngine","Collider","get_enabled","");
    RV_BIND(collider_body,"UnityEngine","Collider","get_attachedRigidbody","");RV_BIND(collider_trigger,"UnityEngine","Collider","get_isTrigger","");
#undef RV_BIND
    int (*value_size)(PTR,DWORD *)=(int(*)(PTR,DWORD *))GetProcAddress(game,"il2cpp_class_value_size");DWORD align=0;
    PTR hit_class=find_class("UnityEngine","RaycastHit"),entity_class=find_class("UnityEngine","EntityId");
    if(!world_field||!walker_body_field||!body_field||!authored_field||!value_size||!hit_class||!entity_class){event("layout-bind-failed");return 0;}
    su.hit_size=value_size(hit_class,&align);
    PTR collider_id_field=get_field(hit_class,"m_Collider");
    if(su.hit_size!=48||value_size(entity_class,&align)!=8||!collider_id_field||su.field_offset(collider_id_field)-16!=40){event("hit-layout-failed");return 0;}
    query_hits=su_hold(su.new_array(find_class("UnityEngine","RaycastHit"),64));if(!query_hits)return 0;
    if(!test_bind()){event("test-bind-failed");return 0;}

    PTR user=LoadLibraryA("user32.dll");if(!user)return 0;key_state=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");foreground_window=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");if(!key_state||!foreground_window||!window_pid)return 0;
    rv_hover_profile_init();
    // Registration, controls and physics belong to the framework.
    (void)prepare_hook;(void)install_hooks;
    vehicle_framework=mv_connect(domain,game);
    if(!vehicle_framework||!vehicle_framework->register_vehicle(&rustbound_definition)){event("framework-registration-failed");return 0;}
    event("vessel-registered-index-22");event("ready");return 1;
}
