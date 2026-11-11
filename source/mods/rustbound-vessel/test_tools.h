/* Private scenario controls; unavailable unless the owning launcher opts in. */
static int test_was_funds,test_was_turn,test_was_pitch,test_was_stage,test_was_focus,test_was_garage,test_was_build,test_was_material,test_material,test_was_side,test_side,test_was_reset;
static PTR test_money,test_set_funds,test_shop_open,test_look;
static PTR test_camera_transform(PTR walker){PTR camera=0;field_value(walker,get_field(object_class(walker),"viewCamera"),&camera);return rv_transform(camera?camera:walker);}
static int test_garage_menu(PTR walker){
    int ok=1,inactive=0,sort=0;PTR type=su_type("MoonCleaners","MoonGarage");void *args[]={type,&inactive,&sort};PTR garages=call(find_objects,0,args,&ok);
    U64 n=garages?su.array_length(garages):0;if(!ok||n>128)return 0;PTR nearest=0;float best=1e7f;
    SPVec3 player=su_vector(su.tr_get_position,rv_transform(walker));
    for(U64 i=0;i<n;i++){
        PTR garage=*(PTR *)su.array_address(garages,8,i);if(!su_alive(garage))continue;
        float distance=rv_length(rv_from(su_sub(su_vector(su.tr_get_position,rv_transform(garage)),player)));
        if(distance<best){best=distance;nearest=garage;}
    }
    if(!nearest){event("test-no-garage-in-world");return 0;}
    PTR bays=0;field_value(nearest,get_field(object_class(nearest),"bays"),&bays);
    static char report[3072];char *p=cat(report,"{\"event\":\"test-garage-bays\",\"position\":");
    p=vector_text(p,rv_from(su_vector(su.tr_get_position,rv_transform(nearest))));p=cat(p,",\"bays\":[");
    U64 total=bays?su.array_length(bays):0;if(total>16)total=16;
    for(U64 i=0;i<total;i++){if(i)p=cat(p,",");PTR bay=*(PTR *)su.array_address(bays,8,i);p=cat(p,"{\"position\":");p=vector_text(p,rv_from(su_vector(su.tr_get_position,bay)));p=cat(p,",\"forward\":");p=vector_text(p,rv_from(su_vector(su.tr_forward,bay)));p=cat(p,"}");}
    p=cat(p,"]}\n");DWORD written=0;WriteFile(vessel_log,report,(DWORD)(p-report),&written,0);
    PTR menu=game_component(walker,"MoonGarageMenu"),open=su_method("MoonCleaners","MoonGarageMenu","Open","System.Int32");
    if(!menu||!open){event("test-garage-menu-unavailable");return 0;}
    int id=integer(method("MoonCleaners","MoonGarage","get_Id",0),nearest,&ok);if(!ok)return 0;
    void *show[]={&id};PTR opened=call(open,menu,show,&ok);return ok&&opened&&*(unsigned char *)unbox(opened);
}
static PTR test_player_visual(PTR walker){
    int ok=1,inactive=0,sort=0;PTR type=su_type("MoonCleaners","MoonPlayerVisual");void *args[]={type,&inactive,&sort};PTR objects=call(find_objects,0,args,&ok);
    U64 count=objects?su.array_length(objects):0;if(!ok||count>64)return 0;
    for(U64 i=0;i<count;i++){PTR visual=*(PTR *)su.array_address(objects,8,i),owner=0;field_value(visual,get_field(object_class(visual),"walker"),&owner);if(owner==walker)return visual;}
    return 0;
}
static void test_scene_probe(PTR walker,RVInstance *v){
    PTR camera=test_camera_transform(walker),aimed=0,binding=0,label=0,last_notice=0;
    field_value(su_get(equipment_root),get_field(object_class(su_get(equipment_root)),"aimedVehicle"),&aimed);
    PTR interact=get_field(find_class("MoonCleaners","MoonInput"),"Interact");if(interact)su.static_field(interact,&binding);
    if(binding)field_value(binding,get_field(object_class(binding),"<KeyLabel>k__BackingField"),&label);
    char key_label[100]={0},last[200]={0};su_string(label,key_label,100);
    field_value(su_get(equipment_root),get_field(object_class(su_get(equipment_root)),"<LastNotice>k__BackingField"),&last_notice);su_string(last_notice,last,200);
    unsigned char knocked=0,test_controls=0;int tool=-1;
    field_value(walker,get_field(object_class(walker),"<Knocked>k__BackingField"),&knocked);
    field_value(walker,get_field(object_class(walker),"<TestControlsActive>k__BackingField"),&test_controls);
    field_value(su_get(equipment_root),get_field(object_class(su_get(equipment_root)),"<ActiveTool>k__BackingField"),&tool);
    SPVec3 origin=su_vector(su.tr_get_position,camera),direction=su_vector(su.tr_forward,camera);float length=6;int mask=1,triggers=2,ok=1;
    union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));
    void *ray[]={&origin,&direction,hit.bytes,&length,&mask,&triggers};PTR found=call(su.physics_ray,0,ray,&ok),collider=0,vehicle=0;
    int has_hit=ok&&found&&*(unsigned char *)unbox(found),hit_layer=0;
    if(has_hit){
        collider=call(su.hit_collider,hit.bytes,0,&ok);hit_layer=integer(scene_get_layer,rv_object(collider),&ok);
        unsigned char inactive=0;void *parent[]={su_type("MoonCleaners","MoonPrototypeVehicle"),&inactive};
        vehicle=call(su_method("UnityEngine","Component","GetComponentInParent","System.Type;System.Boolean"),collider,parent,&ok);
    }
    static char b[4096]; /* Single game-thread diagnostic; avoid a Windows stack-probe dependency. */
    char *p=cat(b,"{\"event\":\"test-vessel-scene\",\"interact_key\":");p=sp_quote(p,key_label);
    p=cat(p,",\"aimed_matches\":");p=num(p,aimed==su_get(v->vehicle));p=cat(p,",\"ray_hit\":");p=num(p,(U64)has_hit);
    p=cat(p,",\"hit_matches\":");p=num(p,vehicle==su_get(v->vehicle));p=cat(p,",\"hit_layer\":");p=num(p,(U64)hit_layer);
    p=cat(p,",\"driven\":");p=num(p,(U64)boolean(driven_method,su_get(v->vehicle),&ok));
    p=cat(p,",\"input_ready\":");p=num(p,(U64)boolean(method("MoonCleaners","MoonPrototypeEquipment","get_InputReady",0),su_get(equipment_root),&ok));
    p=cat(p,",\"knocked\":");p=num(p,knocked);p=cat(p,",\"test_controls\":");p=num(p,test_controls);p=cat(p,",\"tool_in_hand\":");p=num(p,tool>=0);
    p=cat(p,",\"last_notice\":");p=sp_quote(p,last);
    p=cat(p,",\"source_solid_layers\":[");
    for(unsigned i=0;i<v->collider_count;i++){if(i)p=cat(p,",");p=num(p,(U64)integer(scene_get_layer,rv_object(su_get(v->colliders[i])),&ok));}
    p=cat(p,"],\"camera_position\":");p=vector_text(p,rv_from(origin));p=cat(p,",\"vehicle_position\":");p=vector_text(p,rv_from(su_vector(rb_position,su_get(v->body))));
    p=cat(p,",\"hull_scale\":");p=vector_text(p,rv_from(su_vector(method("UnityEngine","Transform","get_lossyScale",0),su_transform(su_get(v->visual)))));
    p=cat(p,",\"transform_offset\":");p=vector_text(p,rv_from(su_sub(su_vector(su.tr_get_position,su_get(v->transform)),su_vector(rb_position,su_get(v->body)))));
    PTR storage=0;field_value(su_get(v->vehicle),get_field(object_class(su_get(v->vehicle)),"<StorageId>k__BackingField"),&storage);
    void *owned_arg[]={storage};PTR owned=storage?call(su_method("MoonCleaners","MoonWorld","OwnedVehicleAt","System.String"),su_get(world_root),owned_arg,&ok):0;
    p=cat(p,",\"owned_record\":");p=num(p,owned!=0);
    p=cat(p,",\"vehicle_index\":");p=num(p,(U64)integer(index_method,su_get(v->vehicle),&ok));
    PTR authored=0,seat=0,anchor=0;field_value(su_get(v->vehicle),authored_field,&authored);
    field_value(su_get(v->vehicle),get_field(object_class(su_get(v->vehicle)),"seat"),&seat);
    unsigned char closed=0;if(authored){field_value(authored,get_field(object_class(authored),"enclosedCab"),&closed);field_value(authored,get_field(object_class(authored),"driverAnchor"),&anchor);}
    p=cat(p,",\"enclosed_cab\":");p=num(p,closed);
    p=cat(p,",\"seat_local\":");p=vector_text(p,rv_from(su_vector(method("UnityEngine","Transform","get_localPosition",0),seat)));
    p=cat(p,",\"driver_anchor_local\":");p=vector_text(p,rv_from(su_vector(method("UnityEngine","Transform","get_localPosition",0),anchor)));
    PTR visual=test_player_visual(walker),renderers=0;unsigned char in_cab=0,hidden=0;
    if(visual){field_value(visual,get_field(object_class(visual),"inCab"),&in_cab);field_value(visual,get_field(object_class(visual),"hiddenInCab"),&hidden);field_value(visual,get_field(object_class(visual),"renderers"),&renderers);}
    U64 total=renderers?su.array_length(renderers):0,off=0;if(total>192)total=0;
    PTR force_off=method("UnityEngine","Renderer","get_forceRenderingOff",0);
    for(U64 i=0;i<total;i++)off+=(U64)boolean(force_off,*(PTR *)su.array_address(renderers,8,i),&ok);
    p=cat(p,",\"avatar_in_cab\":");p=num(p,in_cab);p=cat(p,",\"avatar_hidden_in_cab\":");p=num(p,hidden);
    p=cat(p,",\"avatar_renderers\":");p=num(p,total);p=cat(p,",\"avatar_forced_off\":");p=num(p,off);

    p=cat(p,",\"body_forward\":");p=vector_text(p,rv_from(su_vector(su.tr_forward,su_get(v->transform))));
    p=cat(p,",\"body_velocity\":");p=vector_text(p,rv_from(su_vector(rb_get_velocity,su_get(v->body))));
    p=cat(p,",\"native_drive\":");p=fixed_text(p,su_number(vehicle_drive,su_get(v->vehicle)));
    p=cat(p,",\"surface_hover\":");p=num(p,v->flight.surface);
    p=cat(p,",\"flight_active\":");p=num(p,v->flight.active);p=cat(p,",\"floor_clearance\":");
    RVVec position=rv_from(su_vector(rb_position,su_get(v->body))),up=rv_normal(rv_sub(position,center),rv_v(0,1,0));float clearance=0;
    int has_floor=ground(v,position,up,&clearance);p=fixed_text(p,has_floor?clearance:-1000);
    PTR tracks=game_component(su_get(v->vehicle),"MoonVehicleTracks"),dust=0;
    field_value(su_get(v->vehicle),get_field(object_class(su_get(v->vehicle)),"wheelDust"),&dust);
    p=cat(p,",\"tracks_enabled\":");p=num(p,tracks?(U64)boolean(method("UnityEngine","Behaviour","get_enabled",0),tracks,&ok):0);
    p=cat(p,",\"track_stretches\":");p=num(p,tracks?(U64)integer(method("MoonCleaners","MoonVehicleTracks","get_Stretches",0),tracks,&ok):0);
    p=cat(p,",\"wheel_dust_emitted\":");p=num(p,dust?(U64)integer(method("MoonCleaners","MoonVehicleDust","get_EmittedCount",0),dust,&ok):0);

    p=cat(p,",\"calls_ok\":");p=num(p,(U64)ok);p=cat(p,"}\n");DWORD n=0;WriteFile(vessel_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(vessel_log);
}
static int test_stage(PTR walker){
    int ok=1;if(boolean(method("MoonCleaners","MoonWalker","get_Mounted",0),walker,&ok)||!ok)return 0;
    SPVec3 origin=su_vector(su.tr_get_position,rv_transform(walker));RVVec up=rv_normal(rv_sub(rv_from(origin),center),rv_v(0,1,0));
    RVVec forward=rv_normal(rv_plane(rv_from(su_vector(su.tr_forward,test_camera_transform(walker))),up),rv_v(0,0,1)),right=rv_cross(up,forward);
    static const float directions[8][2]={{1,0},{0,1},{-1,0},{0,-1},{.707f,.707f},{-.707f,.707f},{.707f,-.707f},{-.707f,-.707f}};
    for(int i=0;i<8;i++){
        RVVec dir=rv_add(rv_mul(forward,directions[i][0]),rv_mul(right,directions[i][1]));RVVec candidate=rv_add(rv_from(origin),rv_mul(dir,35));
        SPVec3 local_up=rv_to(rv_normal(rv_sub(candidate,center),up)),start=su_add(rv_to(candidate),su_mul(local_up,25)),down=su_mul(local_up,-1);
        float length=60;int mask=~(1<<2),triggers=1;union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));
        void *ray[]={&start,&down,hit.bytes,&length,&mask,&triggers};PTR found=call(su.physics_ray,0,ray,&ok);if(!ok||!found||!*(unsigned char *)unbox(found))continue;
        SPVec3 point,normal;su_copy(&point,hit.bytes+su.hit_point,12);su_copy(&normal,hit.bytes+su.hit_normal,12);if(su_dot(normal,local_up)<.8f)continue;
        SPVec3 a=su_add(point,su_mul(local_up,.4f)),b=su_add(point,su_mul(local_up,1.5f));float radius=.3f;
        void *overlap[]={&a,&b,&radius,&mask,&triggers};PTR blocked=call(su.physics_capsule,0,overlap,&ok);if(!ok||!blocked||*(unsigned char *)unbox(blocked))continue;
        PTR body=0;field_value(walker,get_field(object_class(walker),"body"),&body);if(!su_alive(body))return 0;
        su_set_vector(method("UnityEngine","Rigidbody","set_position",1),body,su_add(point,su_mul(local_up,.1f)));su_set_vector(rb_velocity,body,su_v(0,0,0));
        SPVec3 aim=rv_to(dir);void *look[]={&aim};call(test_look,walker,look,&ok);return ok;
    }
    return 0;
}
static int test_focus_vessel(PTR walker){
    PTR camera=test_camera_transform(walker);RVVec origin=rv_from(su_vector(su.tr_get_position,camera));RVInstance *nearest=0;float best=1000;
    for(unsigned i=0;i<32;i++)if(vessels[i].vehicle&&!vessels[i].remote){
        RVVec point=rv_from(su_vector(su.tr_get_position,su_get(vessels[i].transform)));float distance=rv_length(rv_sub(point,origin));if(distance<best){best=distance;nearest=&vessels[i];}
    }
    if(!nearest)return 0;RVVec p=rv_from(su_vector(su.tr_get_position,su_get(nearest->transform))),up=rv_normal(rv_sub(p,center),rv_v(0,1,0));
    SPVec3 direction=rv_to(rv_normal(rv_sub(rv_add(p,rv_mul(up,.8f)),origin),rv_v(0,0,1)));int ok=1;void *a[]={&direction};call(test_look,walker,a,&ok);if(ok)test_scene_probe(walker,nearest);return ok;
}
static int test_side_view(PTR walker){
    int ok=1;if(boolean(method("MoonCleaners","MoonWalker","get_Mounted",0),walker,&ok)||!ok)return 0;
    RVInstance *v=0;for(unsigned i=0;i<32;i++)if(vessels[i].vehicle&&!vessels[i].remote){v=&vessels[i];break;}if(!v)return 0;
    RVVec p=rv_from(su_vector(rb_position,su_get(v->body))),up=rv_normal(rv_sub(p,center),rv_v(0,1,0));
    RVVec right=rv_from(su_vector(su.tr_right,su_get(v->transform))),point=rv_add(p,rv_mul(right,(test_side++%2)?4.5f:-4.5f));
    SPVec3 start=rv_to(rv_add(point,rv_mul(up,8))),down=rv_to(rv_mul(up,-1));float length=20;int mask=~(1<<2),triggers=1;
    union{U64 align;unsigned char bytes[96];}hit;su_zero(&hit,sizeof(hit));void *ray[]={&start,&down,hit.bytes,&length,&mask,&triggers};
    PTR found=call(su.physics_ray,0,ray,&ok);if(!ok||!found||!*(unsigned char *)unbox(found))return 0;
    SPVec3 floor,normal;su_copy(&floor,hit.bytes+su.hit_point,12);su_copy(&normal,hit.bytes+su.hit_normal,12);if(su_dot(normal,rv_to(up))<.65f)return 0;
    SPVec3 a=su_add(floor,rv_to(rv_mul(up,.4f))),b=su_add(floor,rv_to(rv_mul(up,1.5f)));float radius=.3f;
    void *overlap[]={&a,&b,&radius,&mask,&triggers};PTR blocked=call(su.physics_capsule,0,overlap,&ok);if(!ok||!blocked||*(unsigned char *)unbox(blocked))return 0;
    PTR body=0;field_value(walker,walker_body_field,&body);if(!su_alive(body))return 0;
    su_set_vector(method("UnityEngine","Rigidbody","set_position",1),body,su_add(floor,rv_to(rv_mul(up,.1f))));su_set_vector(rb_velocity,body,su_v(0,0,0));return 1;
}
/* Disposable purchase replay only: delete this exact locally owned, empty,
 * unmounted test vessel. Vanilla dictionaries and all other records survive. */
static int test_reset_purchase(PTR walker,PTR world){
    int ok=1;RVInstance *v=0;for(unsigned i=0;i<32;i++)if(vessels[i].vehicle&&!vessels[i].remote){v=&vessels[i];break;}if(!v)return 0;
    PTR vehicle=su_get(v->vehicle);if(call(owner_method,vehicle,0,&ok)!=walker||boolean(driven_method,vehicle,&ok)||!ok)return 0;
    PTR cargo=method("MoonCleaners","MoonPrototypeVehicle","get_CargoCount",0),dust=call(method("MoonCleaners","MoonPrototypeVehicle","get_DustKilograms",0),vehicle,0,&ok);
    if(integer(cargo,vehicle,&ok)||!ok||!dust||*(double *)unbox(dust)>.0001)return 0;
    PTR storage=0;field_value(vehicle,get_field(object_class(vehicle),"<StorageId>k__BackingField"),&storage);if(!storage)return 0;
    SURoot keep=su_hold(storage),body_keep=su_hold(vehicle);PTR dict[3]={0};const char *names[]={"ownedVehicles","garageVehicles","vehicleStorage"};
    for(int i=0;i<3;i++){field_value(world,get_field(object_class(world),names[i]),&dict[i]);if(!dict[i])ok=0;}
    if(ok&&drop_instance(v)){
        for(int i=0;i<3;i++){void *args[]={su_get(keep)};call(get_method(object_class(dict[i]),"Remove",1),dict[i],args,&ok);}
        call(method("MoonCleaners","MoonPrototypeVehicle","PrepareRemoval",0),vehicle,0,&ok);su_destroy(rv_object(vehicle));
        int revision=integer(method("MoonCleaners","MoonWorld","get_OwnedVehiclesRevision",0),world,&ok)+1;void *args[]={&revision};
        call(method("MoonCleaners","MoonWorld","set_OwnedVehiclesRevision",1),world,args,&ok);call(method("MoonCleaners","MoonWorld","CrewVehiclesMoved",0),world,0,&ok);
    }else ok=0;
    su_drop(&keep);su_drop(&body_keep);return ok;
}
static int test_bind(void){
    char mode[8];test_tools=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",mode,8)==1&&mode[0]=='1';if(!test_tools)return 1;
    test_money=su_method("System","Decimal","op_Implicit","System.Int32");
    test_set_funds=su_method("MoonCleaners","MoonWorld","SetTeamDollars","System.Decimal");
    test_shop_open=su_method("MoonCleaners","MoonUpgradeShop","SetOpen","System.Boolean");
    test_look=su_method("MoonCleaners","MoonWalker","TestLook","UnityEngine.Vector3");
    return test_money&&test_set_funds&&test_shop_open&&test_look;
}
static void test_shortcuts(PTR walker,PTR world){
    if(!test_tools)return;int funds=key(0x79),turn=key(0x7a),pitch=key(0x7b),stage=key(0x76),focus=key(0x77),garage=key(0x75),build=key(0x74),material=key(0x73),side=key(0x72),reset=key(0x71),ok=1;
    if(input_allowed(walker)){
        if(side&&!test_was_side)event(test_side_view(walker)?"test-side-staged":"test-side-unavailable");
        if(reset&&!test_was_reset)event(test_reset_purchase(walker,world)?"test-purchase-reset":"test-reset-unavailable");
        if(material&&!test_was_material){
            test_material=(test_material+1)%4;PTR m=su_get(resources.material);
            const char *keywords[]={"_NORMALMAP","_METALLICSPECGLOSSMAP","_METALLICGLOSSMAP"};
            PTR change=method("UnityEngine","Material",test_material==1?"DisableKeyword":"EnableKeyword",1);
            for(int i=0;i<3;i++){void *a[]={su.string(keywords[i])};call(change,m,a,&ok);}
            rv_float_property(m,"_Metallic",test_material==1?0:.5f);rv_float_property(m,"_BumpScale",test_material==1?0:.7f);
            rv_float_property(m,"_Cull",test_material==2?2:test_material==3?1:0);
            event(test_material==1?"test-material-diffuse":test_material==2?"test-material-back-cull":test_material==3?"test-material-front-cull":"test-material-original");
        }
        if(funds&&!test_was_funds){
            int amount=5000;void *a[]={&amount};PTR money=call(test_money,0,a,&ok);
            if(money&&ok){void *set[]={unbox(money)};call(test_set_funds,world,set,&ok);}
            if(ok){PTR shop=game_component(walker,"MoonUpgradeShop");unsigned char yes=1;void *show[]={&yes};if(shop)call(test_shop_open,shop,show,&ok);}
            event(ok?"test-funds-and-shop":"test-setup-failed");
        }
        if((turn&&!test_was_turn)||(pitch&&!test_was_pitch)){
            PTR transform=test_camera_transform(walker);SPVec3 up=su_vector(su.tr_up,rv_transform(walker)),forward=su_normal(su_sub(su_vector(su.tr_forward,transform),su_mul(up,su_dot(su_vector(su.tr_forward,transform),up))));
            SPVec3 direction=turn?su_cross(up,forward):su_sub(forward,su_mul(up,.6f));void *a[]={&direction};call(test_look,walker,a,&ok);event(ok?"test-camera":"test-camera-failed");
        }
        if(stage&&!test_was_stage)event(test_stage(walker)?"test-outdoor-staged":"test-stage-unavailable");
        if(focus&&!test_was_focus)event(test_focus_vessel(walker)?"test-vessel-focused":"test-focus-unavailable");
        if(garage&&!test_was_garage)event(test_garage_menu(walker)?"test-garage-opened":"test-garage-unavailable");
        if(build&&!test_was_build){
            PTR builder=game_component(walker,"MoonDepotBuilder");unsigned char yes=1;void *show[]={&yes};
            PTR begin=su_method("MoonCleaners","MoonDepotBuilder","BeginPlacement",""),open=su_method("MoonCleaners","MoonDepotBuilder","SetOpen","System.Boolean");
            if(builder&&begin&&open){call(begin,builder,0,&ok);call(open,builder,show,&ok);}else ok=0;
            event(ok?"test-build-catalog-opened":"test-build-catalog-unavailable");
        }
    }
    test_was_funds=funds;test_was_turn=turn;test_was_pitch=pitch;test_was_stage=stage;test_was_focus=focus;test_was_garage=garage;test_was_build=build;test_was_material=material;test_was_side=side;test_was_reset=reset;
}
