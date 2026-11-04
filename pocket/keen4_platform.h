/* SPDX-License-Identifier: MIT
 * Axis/falling platforms and riding. Behavior reference: Omnispeak ck_obj.c,
 * CK_KeenRidePlatform and CK_PhysPushY; original sprite bounds and map stops.
 */
#ifndef RETRODRIVEN_KEEN4_PLATFORM_H
#define RETRODRIVEN_KEEN4_PLATFORM_H
#define K4_PLATFORMS 128
typedef struct {K4Body body;int xd,yd,home;unsigned kind,state;} K4Platform;
typedef struct {K4Platform objects[K4_PLATFORMS];unsigned count;int rider;} Keen4Platforms;
static unsigned k4_platform_info(const K4Terrain*w,int x,int y){return x<0||y<0||x>=w->map->width||y>=w->map->height?0:w->map->planes[2][(size_t)y*w->map->width+x];}
static int k4_platform_init(Keen4Platforms*s,const K4Terrain*w){
    memset(s,0,sizeof(*s));s->rider=-1;
    for(unsigned y=0;y<w->map->height;y++)for(unsigned x=0;x<w->map->width;x++){
        unsigned v=k4_platform_info(w,(int)x,(int)y);if(v<27||v>32||v==31)continue;
        if(s->count==K4_PLATFORMS)return 0;
        K4Platform*a=&s->objects[s->count++];a->body.x=(int)x*256;a->body.y=a->home=(int)y*256;a->body.frame=484;a->kind=v;
        a->xd=v==28?1:v==30?-1:0;a->yd=v==27?-1:v==29||v==32?1:0;k4_rect(w,&a->body);
    }return 1;
}
static void k4_platform_axis(const K4Terrain*w,K4Platform*a){
    K4Body*b=&a->body;int n,t;b->dx=a->xd*24;b->dy=a->yd*24;
    if(a->xd==1){n=b->box.x2+b->dx;t=n>>8;if(t!=b->box.tx2&&k4_platform_info(w,t,b->box.ty1)==31){a->xd=-1;b->dx-=n&255;}}
    else if(a->xd==-1){n=b->box.x1+b->dx;t=n>>8;if(t!=b->box.tx1&&k4_platform_info(w,t,b->box.ty1)==31){a->xd=1;b->dx+=(256-n)&255;}}
    else if(a->yd==1){n=b->box.y2+b->dy;t=n>>8;if(t!=b->box.ty2&&k4_platform_info(w,b->box.tx1,t)==31){if(k4_platform_info(w,b->box.tx1,t-2)==31)b->dy=0;else{a->yd=-1;b->dy-=n&255;}}}
    else if(a->yd==-1){n=b->box.y1+b->dy;t=n>>8;if(t!=b->box.ty1&&k4_platform_info(w,b->box.tx1,t)==31){if(k4_platform_info(w,b->box.tx1,t+2)==31)b->dy=0;else{a->yd=1;b->dy+=256-(n&255);}}}
}
static void k4_platform_fall(const K4Terrain*w,K4Platform*a,int ridden,unsigned ticks){
    K4Body*b=&a->body;b->dx=b->dy=0;
    if(a->state==0){if(ridden){b->dy=32;b->vy=0;if((unsigned)(b->y+b->dy-a->home)>=128)a->state=1;}}
    else if(a->state==1){
        b->dy=k4_gravity(b,ticks,2,4);int n=b->box.y2+b->dy,t=n>>8;
        if(t!=b->box.ty2&&k4_platform_info(w,b->box.tx1,t)==31){b->dy=255-(b->box.y2&255);if(!ridden)a->state=2;}
    }else{
        b->dy=-64;
        if(ridden){b->vy=0;a->state=1;}
        else if(b->y<=a->home){b->dy=a->home-b->y;a->state=0;}
    }
}
/* Platform carrying preserves tile-contact flags and accumulates displacement through the
 * simple-object clipping pass. */
static void k4_platform_carry(const K4Terrain*w,K4Body*b,int dx,int dy){
    int ox=b->x,oy=b->y,old_dx=b->dx,old_dy=b->dy;K4Rect old=b->box;
    b->dx=dx;b->dy=dy;b->x+=dx;b->y+=dy;k4_rect(w,b);
    if(b->clipped){k4_clip_vertical(w,b,&old,b->box.xm-old.xm,b->box.y1-old.y1,b->box.y2-old.y2);k4_clip_horizontal(w,b);}
    b->dx=old_dx+b->x-ox;b->dy=old_dy+b->y-oy;
}
static int k4_platform_can_ride(const Keen4Player*p){
    return p->body.clipped&&p->state!=K4_DEAD&&p->state!=K4_HANG&&p->state!=K4_PULL&&p->state!=K4_POLE&&p->state!=K4_DOOR_ENTER&&p->state!=K4_DOOR_ALIGN&&!(p->state==K4_SHOOT&&p->shoot_kind==2);
}
/* Runs after all objects move, before landing/drawing, as in CK_PlayLoop. */
static void k4_platform_contact(const K4Terrain*w,Keen4Player*p,void*context){
    Keen4Platforms*s=(Keen4Platforms*)context;K4Body*b=&p->body;
    for(unsigned i=0;i<s->count;i++){
        K4Platform*a=&s->objects[i];if(a->kind==32)k4_platform_fall(w,a,s->rider==(int)i,p->ticks);else k4_platform_axis(w,a);
        a->body.x+=a->body.dx;a->body.y+=a->body.dy;k4_rect(w,&a->body);
    }
    if(!k4_platform_can_ride(p)||(p->events&K4_DOOR_EVENT)){s->rider=-1;return;}
    if(s->rider>=0){
        K4Body*a=&s->objects[s->rider].body;
        if(b->box.x2<a->box.x1||b->box.x1>a->box.x2)s->rider=-1;
        else if(b->dy<0){s->rider=-1;if(a->dy<0)k4_platform_carry(w,b,0,a->dy);}
        else{
            k4_platform_carry(w,b,a->dx,a->box.y1-b->box.y2-16);b->x|=a->x&31;
            if(b->bottom)s->rider=-1;else b->top=0x19;
        }
    }
    for(unsigned i=0;s->rider<0&&i<s->count;i++){
        K4Body*a=&s->objects[i].body;
        if(b->box.x2<=a->box.x1||b->box.x1>=a->box.x2||b->box.y1>=a->box.y2||b->box.y2<=a->box.y1)continue;
        int overlap=b->box.y2-a->box.y1,relative=b->dy-a->dy;
        if(overlap>=0&&overlap<=relative){
            s->rider=(int)i;int dx=b->dx,dy=b->dy,stick=b->stick;b->dx=0;b->dy=-overlap;b->stick=0;
            k4_physics(w,b);b->stick=stick;b->dx+=dx;b->dy+=dy;if(!b->bottom)b->top=0x19;
        }
    }
}
static void k4_platform_step(const K4Terrain*w,Keen4Platforms*s,Keen4Player*p,K4Input in,int easy){k4_step_objects(w,p,in,easy,k4_platform_contact,s);}
#endif
