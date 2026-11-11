#ifndef RUSTBOUND_HULL_GEOMETRY_H
#define RUSTBOUND_HULL_GEOMETRY_H
typedef struct {const char *name;SPVec3 center,size;} RVHullBox;
static const RVHullBox rv_hull_boxes[]={
    {"Hull",{0,.4f,0},{2.35f,.65f,4.05f}},
    {"Cab",{0,1.1f,.3f},{1.7f,1.7f,1.55f}},
    {"Hood",{0,.87f,1.08f},{1.85f,.55f,1.8f}}
};
#define RV_HULL_BOXES 3
#define RV_HULL_SUPPORTS (RV_HULL_BOXES*5)
/* The centre and four underside corners of each actual collision box. Keeping
 * these with the collider dimensions prevents clearance and hull drift apart. */
static inline SPVec3 rv_hull_support(unsigned index){
    const RVHullBox *box=&rv_hull_boxes[index/5];unsigned corner=index%5;
    SPVec3 p=box->center;p.y-=box->size.y*.5f;
    if(corner){p.x+=((corner-1)&1?1.f:-1.f)*box->size.x*.5f;p.z+=((corner-1)&2?1.f:-1.f)*box->size.z*.5f;}
    return p;
}
static inline float rv_hull_reference(void){return rv_hull_boxes[0].center.y-rv_hull_boxes[0].size.y*.5f;}
#endif
