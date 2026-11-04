/* SPDX-License-Identifier: MIT
 * Pixel-based movement helpers for resource previews and tests. */
#ifndef RETRODRIVEN_KEEN_MOTION_H
#define RETRODRIVEN_KEEN_MOTION_H
#include <stdint.h>
#define KEEN_PLAYER_W 10
#define KEEN_PLAYER_H 16
/* Tileinfo plane indices per Keen 1-3 Tileinfo format:
 * animation=0, misc=1, top=2, right=3, bottom=4, left=5. */
#define KEEN_FACE_TOP 2
#define KEEN_FACE_RIGHT 3
#define KEEN_FACE_BOTTOM 4
#define KEEN_FACE_LEFT 5

typedef int (*KeenSolid)(int tx,int ty,int face,void *user);
typedef struct {int x,y,vx,vy,grounded;} KeenMotion;
static void keen_motion_reset(KeenMotion *p,int x,int y){
    p->x=x;p->y=y;p->vx=0;p->vy=0;p->grounded=0;
}
static int keen_motion_edge(KeenSolid solid,void *user,int x,int y,int face){
    if(x<0 || y<0)return 1;
    return solid(x/16,y/16,face,user);
}
/* Step one pixel at a time to avoid tunnelling; test the tile face being entered. */
static void keen_motion_step_mode(KeenMotion *p,int dir,int jump,int pogo,KeenSolid solid,void *user){
    if(!p||!solid)return;
    p->vx=(dir>0)?2:(dir<0?-2:0);
    /* Preview pogo bounces on ground contact. */
    if(pogo && p->grounded){p->vy=-7;p->grounded=0;}
    else if(jump && p->grounded){p->vy=-7;p->grounded=0;}
    if(p->vy<5)p->vy++;
    int dx=p->vx,dy=p->vy;
    while(dx){
        int step=dx>0?1:-1, edge=p->x+(step>0?KEEN_PLAYER_W: -1);
        int face=step>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT;
        if(keen_motion_edge(solid,user,edge,p->y+1,face) ||
           keen_motion_edge(solid,user,edge,p->y+KEEN_PLAYER_H-2,face))break;
        p->x+=step;dx-=step;
    }
    if(dy>0)p->grounded=0;
    while(dy){
        int step=dy>0?1:-1, edge=p->y+(step>0?KEEN_PLAYER_H:-1);
        int face=step>0?KEEN_FACE_TOP:KEEN_FACE_BOTTOM;
        if(keen_motion_edge(solid,user,p->x+1,edge,face) ||
           keen_motion_edge(solid,user,p->x+KEEN_PLAYER_W-2,edge,face)){
            if(step>0)p->grounded=1;
            p->vy=0;break;
        }
        p->y+=step;dy-=step;
    }
    /* Keep a grounded player stationary between gravity updates. */
    if(p->vy==0 &&
        (keen_motion_edge(solid,user,p->x+1,p->y+KEEN_PLAYER_H,KEEN_FACE_TOP) ||
         keen_motion_edge(solid,user,p->x+KEEN_PLAYER_W-2,p->y+KEEN_PLAYER_H,KEEN_FACE_TOP)))
        p->grounded=1;
}
/* Compatibility wrapper for preview callers. */
static inline void keen_motion_step(KeenMotion *p,int dir,int jump,KeenSolid solid,void *user){
    keen_motion_step_mode(p,dir,jump,0,solid,user);
}
#endif
