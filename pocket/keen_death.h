/* Keen 1 death presentation: original 200-tick pause, frames 22/23,
 * then straight upward flight at -400 units/tick. Reference: shared
 * enemies.c CVort_think_keen_death. SPDX-License-Identifier: MIT */
#ifndef KEEN_DEATH_H
#define KEEN_DEATH_H
#include <string.h>
#include "keen_vort_motion.h"
typedef struct {unsigned state,pending,time,frame;int32_t x,y;int vx;} KeenDeath;
static inline int keen_death_draw_y(int body_y){return body_y-1;}
typedef unsigned (*KeenDeathRandom)(void *user);
static inline void keen_death_begin(KeenDeath *d,const KeenMotion *p){
 memset(d,0,sizeof(*d));d->state=1;d->frame=22;d->x=p->x*256;d->y=(p->y+8)*256;
}
static inline void keen_death_step(KeenDeath *d,unsigned ticks,unsigned clock,int camera_y,
 KeenDeathRandom random,void *user){
 if(!d->state||d->state==3)return;
 if(ticks>64)ticks=64;
 d->pending+=ticks;
 while(d->pending>=6 && d->state!=3){
  d->pending-=6;d->time+=6;d->frame=22+((clock>>4)&1u);
  if(d->state==1 && d->time>=200){d->state=2;d->vx=(int)random(user)-128;}
  if(d->state==2){d->x+=d->vx*6;d->y-=400*6;if(keen_death_draw_y(kv_floor(d->y))+15<camera_y)d->state=3;}
 }
}
#endif
