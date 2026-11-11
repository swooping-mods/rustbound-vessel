/* Vehicle-specific hull and tuning; physics lives in Vehicle Framework. */
#include "../vehicle-framework/sdk/hover.h"
#include "hull_geometry.h"
static MoonHoverBox rv_hover_boxes[RV_HULL_BOXES];
static MoonHoverProfile rv_hover_profile={
    .abi=MV_HOVER_ABI,.size=sizeof(MoonHoverProfile),.tuning=RV_CONFIG_DEFAULT,
    .boxes=rv_hover_boxes,.box_count=RV_HULL_BOXES,.reference=.075f,.flight_enabled=1
};
static inline void rv_hover_profile_init(void){
    for(unsigned i=0;i<RV_HULL_BOXES;i++){
        const RVHullBox *b=rv_hull_boxes+i;
        rv_hover_boxes[i]=(MoonHoverBox){b->name,{b->center.x,b->center.y,b->center.z},{b->size.x,b->size.y,b->size.z}};
    }
}
