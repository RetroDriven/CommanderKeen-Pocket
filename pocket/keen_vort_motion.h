/* SPDX-License-Identifier: MIT
 * Vorticons fixed-point movement and tile collision. Reference: Chocolate Keen enemies.c
 * and physics.c. */
#ifndef KEEN_VORT_MOTION_H
#define KEEN_VORT_MOTION_H
#include <string.h>
#include "keen_motion.h"
enum {KV_GROUND,KV_CHARGE,KV_AIR,KV_POGO_GROUND,KV_POGO_AIR};
#define KV_JUMP 1u
#define KV_LAND 2u
#define KV_HEAD 4u
#define KV_WALL 8u
typedef struct {int32_t fx,fy;int vx,vy,saved;unsigned mode,time,charge,pogo,pending;} KeenVortMotion;
static inline void keen_vort_reset(KeenVortMotion *v,const KeenMotion *p){
 memset(v,0,sizeof(*v));v->fx=p->x*256;v->fy=p->y*256;v->mode=p->grounded?KV_GROUND:KV_AIR;
}
static inline int kv_floor(int32_t v){return v>=0?v/256:-((-v+255)/256);}
static inline int kv_limit(int v){return v>120?120:v< -120?-120:v;}
static inline int kv_slow(int v,int n){return v>0?(v>n?v-n:0):(v< -n?v+n:0);}
static inline int kv_edge(KeenSolid f,void *u,int x,int y,int face){return x<0||y<0||f(x/16,y/16,face,u);}
static inline int kv_column(KeenSolid f,void*u,int x,int top,int bottom,int face){
 if(x<0||top<0)return 1;
 for(int y=top/16;y<=bottom/16;y++)if(f(x/16,y,face,u))return 1;
 return 0;
}
/* Original normal bounds x=4..11,y=1..23; pogo's top is y=0. */
static inline unsigned kv_move(KeenVortMotion *v,KeenMotion *p,int dx,int dy,KeenSolid f,void *u){
 unsigned e=0;int old_ground=p->grounded;
 int extra=v->mode==KV_POGO_GROUND||v->mode==KV_POGO_AIR;
 int32_t nx=v->fx+dx;int target=kv_floor(nx),step=dx<0?-1:1;
 while(p->x!=target){int edge=p->x+(step>0?8:-1),face=step>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT;
  if(kv_column(f,u,edge,p->y-extra,p->y+22,face)){
   e|=KV_WALL;nx=p->x*256;break;
  }p->x+=step;
 }v->fx=nx;
 int32_t ny=v->fy+dy;target=kv_floor(ny);step=dy<0?-1:1;p->grounded=0;
 while(p->y!=target){int edge=p->y+(step>0?23:-1-extra),face=step>0?KEEN_FACE_TOP:KEEN_FACE_BOTTOM;
  if(kv_edge(f,u,p->x,edge,face)||kv_edge(f,u,p->x+7,edge,face)){
   if(step>0)p->grounded=1;else e|=KV_HEAD;
   v->vy=0;ny=p->y*256;break;
  }p->y+=step;
 }v->fy=ny;
 if(v->vy>=0 && (kv_edge(f,u,p->x,p->y+23,KEEN_FACE_TOP)||kv_edge(f,u,p->x+7,p->y+23,KEEN_FACE_TOP))){
  p->grounded=1;v->vy=0;v->fy=p->y*256;
 }
 if(!old_ground && p->grounded)e|=KV_LAND;
 p->vx=v->vx;p->vy=v->vy;return e;
}
/* Add intermediate velocities to del_x/del_y before the final velocity * sprite_sync term.
 * A single-step integrator produces different travel distances. */
static inline void kv_accelerate(KeenVortMotion *v,int a,int *dx){
 for(unsigned i=0;i<6;i++){v->vx=kv_limit(v->vx+a);if(i<5)*dx+=v->vx;}
}
static inline void kv_friction(KeenVortMotion *v,int amount,int *dx){
 int sign=v->vx>0?1:v->vx<0?-1:0;
 if(sign){kv_accelerate(v,-sign*amount,dx);if(v->vx*sign<0)v->vx=0;}
}
static inline unsigned keen_vort_step(KeenVortMotion *v,KeenMotion *p,unsigned ticks,
 int dir,int held_jump,int pogo,int shooting,KeenSolid f,void *u){
 unsigned events=0;if(ticks>64)ticks=64;v->pending+=ticks;
 /* Simulate six game ticks per batch, matching Chocolate Keen drawScreen, independently of
  * Pocket vsync. */
 while(v->pending>=6){
  v->pending-=6;int dx=0,dy=0,skip_gravity=0;
  if(shooting){v->mode=KV_AIR;kv_friction(v,1,&dx);}
  else{
   if(pogo && !v->pogo){v->mode=p->grounded?KV_POGO_GROUND:KV_POGO_AIR;v->time=0;v->saved=v->vx;}
   if(!pogo && v->pogo)v->mode=p->grounded?KV_GROUND:KV_AIR;
   if(v->mode==KV_GROUND && held_jump){v->mode=KV_CHARGE;v->saved=v->vx;v->time=v->charge=0;}
   switch(v->mode){
   case KV_CHARGE:
    if(held_jump)v->charge+=36;else if(v->time<12)v->time=24-v->time;
    v->saved=kv_limit(v->saved+dir*12);v->vx=0;v->time+=6;
    if(v->time>=36){v->vy=-(int)v->charge;v->vx=v->saved;v->mode=KV_AIR;p->grounded=0;events|=KV_JUMP;}
    break;
   case KV_POGO_GROUND:
    v->vx=0;v->time+=6;skip_gravity=1;
    if(v->time>22){v->vy-=200;v->vx=v->saved;v->mode=KV_POGO_AIR;p->grounded=0;events|=KV_JUMP;}
    break;
   default:{
    int effective=dir;
    if(dir){kv_accelerate(v,dir*(v->mode==KV_POGO_AIR?1:2),&dx);if(v->vx*dir<0)effective=0;}
    if(!effective && v->mode!=KV_POGO_AIR)kv_friction(v,v->mode==KV_GROUND?3:1,&dx);
    if(v->mode==KV_POGO_AIR && held_jump && v->vy<0){
     for(unsigned i=0;i<6;i++){v->vy--;if(v->vy< -200)v->vy=-200;if(i<5)dy+=v->vy;}
    }
    break;}
   }
  }
  v->pogo=(unsigned)pogo;
  if(!skip_gravity){
   for(unsigned i=0;i<6;i++){
    v->vy+=3;if(v->vy>200)v->vy=200;if(v->vy< -400)v->vy=-400;
    if(i<5)dy+=v->vy;
   }
  }
  dx+=v->vx*6;dy+=v->vy*6;
  if(dx>3840)dx=3840;
  if(dx< -3840)dx=-3840;
  if(dy>3840)dy=3840;
  if(dy< -3840)dy=-3840;
  unsigned contact=kv_move(v,p,dx,dy,f,u);
  events|=contact&(KV_HEAD|KV_WALL);
  if(p->grounded && (v->mode==KV_AIR||v->mode==KV_POGO_AIR)){
   if(pogo && !shooting){v->mode=KV_POGO_GROUND;v->time=0;v->saved=v->vx;}
   else{v->mode=KV_GROUND;if(contact&KV_LAND)events|=KV_LAND;}
  }else if(!p->grounded && v->mode==KV_GROUND)v->mode=KV_AIR;
 }
 return events;
}
#endif
