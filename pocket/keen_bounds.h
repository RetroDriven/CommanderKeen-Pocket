/* SPDX-License-Identifier: MIT
 * Vorticons level and camera bounds in pixel coordinates. Reference: Chocolate Keen
 * game/gameplay.c and CVort_check_ceiling in game/physics.c. */
#ifndef RETRODRIVEN_KEEN_BOUNDS_H
#define RETRODRIVEN_KEEN_BOUNDS_H
#include "keen_motion.h"
/* TILEINFO does not make border graphics solid. Clamp side/top bounds separately; crossing
 * the open bottom causes a fall death. */
static inline int keen_bounds_apply(KeenMotion *p,unsigned width,unsigned height){
    int mapw=(int)width*16,maph=(int)height*16;
    int minx=width>=5u?32:0;
    int maxx=width>=5u?((int)width-3)*16:mapw-KEEN_PLAYER_W;
    int miny=height>=5u?32:0;
    if(maxx<minx)maxx=minx;
    if(p->x<minx){p->x=minx;p->vx=0;}
    else if(p->x>maxx){p->x=maxx;p->vx=0;}
    if(p->y<miny){p->y=miny;p->vy=0;}
    return p->y>maph;
}
/* Camera bounds leave a two-tile border. Small maps clamp to a nonnegative full-map view. */
static inline int keen_bounds_camera_axis(int wanted,unsigned tiles,int viewport){
    int pixels=(int)tiles*16;
    int min=pixels>=viewport+64?32:0;
    int max=pixels-min-viewport;
    if(max<min)max=min;
    return wanted<min?min:wanted>max?max:wanted;
}
#endif
