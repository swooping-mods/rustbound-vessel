#ifndef RUSTBOUND_HULL_ADAPTER_H
#define RUSTBOUND_HULL_ADAPTER_H
#include "hull_geometry.h"
static inline void rv_hull_restore(RVInstance *v){
    for(unsigned i=0;i<v->collider_count;i++){
        PTR collider=su_get(v->colliders[i]);if(su_alive(collider))su_bool(su.collider_enabled,collider,v->collider_enabled[i]);su_drop(&v->colliders[i]);
    }
    v->collider_count=0;
}
static inline int rv_hull_make(RVInstance *v,PTR vehicle,PTR body,PTR parent){
    if(!su_alive(body))return 1;
    int ok=1;PTR colliders=rv_children(vehicle,"Collider");if(!colliders)return 0;
    U64 count=su.array_length(colliders);if(count>64)return 0;
    int layer=integer(scene_get_layer,rv_object(vehicle),&ok);if(!ok)return 0;
    for(U64 i=0;i<count;i++){
        PTR collider=*(PTR *)su.array_address(colliders,8,i);if(!su_alive(collider))continue;
        int enabled=boolean(collider_get_enabled,collider,&ok),trigger=boolean(collider_trigger,collider,&ok);
        PTR attached=call(collider_body,collider,0,&ok);if(!ok)return 0;
        // Trigger zones and other bodies retain their own collision ownership.
        if(!body||!enabled||trigger||attached!=body)continue;
        if(!v->collider_count)layer=integer(scene_get_layer,rv_object(collider),&ok);
        if(!ok)return 0;
        v->collider_enabled[v->collider_count]=1;v->colliders[v->collider_count++]=su_hold(collider);su_bool(su.collider_enabled,collider,0);
    }
    for(unsigned i=0;i<RV_HULL_BOXES;i++)if(!rv_box(parent,rv_hull_boxes[i].name,rv_hull_boxes[i].center,rv_hull_boxes[i].size,layer))return 0;
    return 1;
}
#endif
