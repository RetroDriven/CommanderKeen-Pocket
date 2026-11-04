/* SPDX-License-Identifier: MIT
 * Galaxy stunner: ACTION.CK4 and Omnispeak CK_SpawnShot/CK_ShotDrawFunc.
 */
#ifndef RETRODRIVEN_KEEN4_WEAPON_H
#define RETRODRIVEN_KEEN4_WEAPON_H
#include "keen4_motion.h"
#define K4_SHOTS 32u
typedef struct{K4Body body;unsigned active,timer;int sx,sy;} K4Shot;
typedef struct{K4Shot shots[K4_SHOTS];unsigned ammo,fired,hits,empty;} Keen4Weapons;
static void k4_weapon_init(Keen4Weapons*w){memset(w,0,sizeof(*w));w->ammo=5;}
static void k4_shot_hit(const K4Terrain*t,K4Shot*s){s->active=2;s->timer=0;s->body.frame=178;s->body.dx=s->body.dy=0;k4_physics_base(t,&s->body,0);}
static int k4_weapon_fire(Keen4Weapons*w,const K4Terrain*t,const Keen4Player*p){
    if(!w->ammo){w->empty++;return 0;}
    for(unsigned n=0;n<K4_SHOTS;n++)if(!w->shots[n].active){
        K4Shot*s=&w->shots[n];memset(s,0,sizeof(*s));s->active=1;s->sx=p->fire_dx;s->sy=p->fire_dy;s->body.x=p->fire_x;s->body.y=p->fire_y;s->body.frame=174;s->body.dir=s->sx;s->body.clipped=1;k4_rect(t,&s->body);k4_physics_base(t,&s->body,0);w->ammo--;w->fired++;return 1;
    }
    return 0;
}
static void k4_weapons_tick(Keen4Weapons*w,const K4Terrain*t,int camera_x,int camera_y){
    for(unsigned n=0;n<K4_SHOTS;n++){
        K4Shot*s=&w->shots[n];K4Body*b=&s->body;if(!s->active)continue;
        s->timer+=2;
        if(s->active==2){if(s->timer>=24){s->active=0;continue;}b->frame=s->timer<12?178:179;k4_rect(t,b);continue;}
        b->frame=174+(int)((s->timer/6)%4);b->dx=s->sx*128;b->dy=s->sy*128;b->stick=0;k4_physics_base(t,b,0);
        if(b->top==1&&b->box.tx1!=b->box.tx2&&k4_at(t,b->box.tx2,b->box.ty1-1,0)==17){b->top=17;b->x+=256-(b->x&255);}
        else if(b->top==17&&b->box.tx1!=b->box.tx2)b->x&=~255;
        if(b->bottom==1&&b->box.tx1!=b->box.tx2&&k4_at(t,b->box.tx2,b->box.ty2+1,2)==17){b->bottom=17;b->x+=256-(b->x&255);}
        else if(b->bottom==17&&b->box.tx1!=b->box.tx2)b->x&=~255;
        if(b->top!=17&&b->bottom!=17){if(b->top||b->bottom||b->left||b->right){k4_shot_hit(t,s);w->hits++;}}
        else{b->y+=s->sy*128;k4_rect(t,b);}
        int cx=camera_x>>4,cy=camera_y>>4;
        if(b->box.tx2+10<cx||b->box.tx1-10>cx+20||b->box.ty2+6<cy||b->box.ty1-6>cy+13)s->active=0;
    }
}
#endif
