/* SPDX-License-Identifier: MIT
 * Keen 1 raygun. Reference: Chocolate Keen game/enemies.c. */
#ifndef KEEN_WEAPON_H
#define KEEN_WEAPON_H
#include <string.h>
#include "keen_motion.h"
#define KW_SHOTS 32
#define KW_FIRE 1u
#define KW_EMPTY 2u
#define KW_HIT 4u
typedef int (*KeenShotContact)(int x,int y,void *user);
typedef struct {int32_t x,y;int direction;unsigned state,age;} KeenShot;
typedef struct {KeenShot shots[KW_SHOTS];unsigned shooting,age,fired;int facing;} KeenWeapon;
static inline void keen_weapon_reset(KeenWeapon *w){memset(w,0,sizeof(*w));}
/* Positions: 256 units/pixel. Shot speed is original 400 units/PIT tick.
 * Imported KEENSHOT collision extents are 0..15 by 0..7 pixels. */
static inline unsigned keen_weapon_step_contacts(KeenWeapon *w,unsigned ticks,int trigger,int held,
        int facing,int origin_x,int origin_y,unsigned *ammo,KeenSolid solid,void *user,
        KeenShotContact contact,void *contact_user){
    unsigned events=0;
    if(trigger && !w->shooting){w->shooting=1;w->age=0;w->fired=0;w->facing=facing<0?-1:1;}
    if(ticks>64)ticks=64; /* bounded recovery after a stalled frame */
    for(unsigned t=0;t<ticks;t++){
        for(unsigned i=0;i<KW_SHOTS;i++){
            KeenShot *s=&w->shots[i];
            if(s->state==2){if(++s->age>20)s->state=0;continue;}
            if(s->state!=1)continue;
            int32_t next=s->x+s->direction*400;
            int old=s->x/256,target=next/256,step=s->direction;
            for(int x=old;;x+=step){
                int edge=x+(step>0?15:0),y=s->y/256;
                int face=step>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT;
                if(edge<0 || y<0 || solid(edge/16,y/16,face,user) || solid(edge/16,(y+7)/16,face,user)){
                    s->state=2;s->age=0;s->x=x*256;events|=KW_HIT;break;
                }
                if(contact && contact(x,y,contact_user)){
                    s->state=2;s->age=0;s->x=x*256;events|=KW_HIT;break;
                }
                if(x==target){s->x=next;break;}
            }
        }
        if(w->shooting){
            w->age++;
            if(!w->fired && w->age>1){
                w->fired=1;
                if(!*ammo)events|=KW_EMPTY;
                else for(unsigned i=0;i<KW_SHOTS;i++)if(!w->shots[i].state){
                    w->shots[i]=(KeenShot){origin_x*256,(origin_y+9)*256,w->facing,1,0};
                    --*ammo;events|=KW_FIRE;break;
                }
            }
            if(w->age>30 && !held)w->shooting=0;
        }
    }
    return events;
}
/* Preserve the standalone weapon API for callers without actors. */
static inline unsigned keen_weapon_step(KeenWeapon *w,unsigned ticks,int trigger,int held,
 int facing,int x,int y,unsigned *ammo,KeenSolid solid,void *user){
 return keen_weapon_step_contacts(w,ticks,trigger,held,facing,x,y,ammo,solid,user,0,0);
}
#endif
