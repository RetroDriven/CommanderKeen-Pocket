/* SPDX-License-Identifier: MIT
 * Vorticons sprite animation. Frame layout: Chocolate Keen episode1/2/3.h and
 * game/enemies.c. */
#ifndef RETRODRIVEN_KEEN_ANIM_H
#define RETRODRIVEN_KEEN_ANIM_H
#include <stdint.h>
#include <string.h>
#include "keen_motion.h"

typedef enum { KEEN_POSE_IDLE=0, KEEN_POSE_WALK, KEEN_POSE_JUMP,
               KEEN_POSE_FALL, KEEN_POSE_POGO } KeenPose;
typedef struct {
    uint32_t walking_ticks;
    int facing; /* 1 right, -1 left; persist facing while idle */
    KeenPose pose;
    unsigned frame;
} KeenAnimation;
typedef struct {
    unsigned idle_right, idle_left;
    unsigned walk_right, walk_left;
    unsigned jump_right, jump_left;
    unsigned fall_right, fall_left;
    unsigned pogo_right, pogo_left;
} KeenAnimationProfile;
/* Check frame names as well as dimensions when identifying walking sprites. */
static inline int keen_animation_walk_name(unsigned frame,const char *name){
    static const char *const names[8]={
        "KEENWALKR1","KEENWALKR2","KEENWALKR3","KEENWALKR4",
        "KEENWALKL1","KEENWALKL2","KEENWALKL3","KEENWALKL4"
    };
    return frame<8u && name && strcmp(names[frame],name)==0;
}
/* Episode indices are 0-2. Walking uses 0-3 right and 4-7 left; jump uses 8/14, fall 13/19
 * and pogo 24/26. These indices apply to Vorticons only. */
static inline const KeenAnimationProfile *keen_animation_profile(unsigned episode){
    static const KeenAnimationProfile profiles[3] = {
        { 0,4, 0,4, 8,14, 13,19, 24,26 },
        { 0,4, 0,4, 8,14, 13,19, 24,26 },
        { 0,4, 0,4, 8,14, 13,19, 24,26 }
    };
    return episode<3u ? &profiles[episode] : 0;
}
static inline void keen_animation_reset(KeenAnimation *a){
    if(!a)return;
    a->walking_ticks=0;a->facing=1;a->pose=KEEN_POSE_IDLE;a->frame=0;
}
/* All animation frames must exist in the EGAHEAD sprite table. */
static inline unsigned keen_animation_required_frames(unsigned episode,unsigned out[18]){
    const KeenAnimationProfile *p=keen_animation_profile(episode);
    if(!p||!out)return 0;
    unsigned j=0;
    out[j++]=p->idle_right;out[j++]=p->idle_left;
    for(unsigned n=0;n<4;n++)out[j++]=p->walk_right+n;
    for(unsigned n=0;n<4;n++)out[j++]=p->walk_left+n;
    out[j++]=p->jump_right;out[j++]=p->jump_left;
    out[j++]=p->fall_right;out[j++]=p->fall_left;
    out[j++]=p->pogo_right;out[j++]=p->pogo_right+1;
    out[j++]=p->pogo_left;out[j++]=p->pogo_left+1;
    return j;
}
static inline void keen_animation_step_for_episode(KeenAnimation *a,const KeenMotion *m,int pogo,unsigned episode){
    const KeenAnimationProfile *p=keen_animation_profile(episode);
    if(!a||!m||!p)return;
    /* Restart the walk cycle when facing changes; left and right use separate frames. */
    int new_facing=a->facing;
    if(m->vx<0)new_facing=-1;
    else if(m->vx>0)new_facing=1;
    if(new_facing!=a->facing)a->walking_ticks=0;
    a->facing=new_facing;
    if(pogo){
        a->pose=KEEN_POSE_POGO;
        a->frame=(a->facing<0?p->pogo_left:p->pogo_right)+(m->grounded?1u:0u);
    }else if(!m->grounded){
        a->pose=m->vy<0?KEEN_POSE_JUMP:KEEN_POSE_FALL;
        a->frame=(a->pose==KEEN_POSE_JUMP)?(a->facing<0?p->jump_left:p->jump_right)
                                              :(a->facing<0?p->fall_left:p->fall_right);
    }else if(m->vx!=0){
        a->pose=KEEN_POSE_WALK;
        a->frame=(a->facing<0?p->walk_left:p->walk_right)+(unsigned)((a->walking_ticks/6u)%4u);
        a->walking_ticks++;
    }else{
        a->pose=KEEN_POSE_IDLE;
        a->frame=a->facing<0?p->idle_left:p->idle_right;
        a->walking_ticks=0;
    }
}
/* Keen 1 compatibility wrapper for previews and tests. */
static inline void keen_animation_step(KeenAnimation *a,const KeenMotion *m,int pogo){
    keen_animation_step_for_episode(a,m,pogo,0u);
}
static inline const char *keen_animation_pose_name(KeenPose pose){
    switch(pose){
    case KEEN_POSE_WALK:return "WALK";
    case KEEN_POSE_JUMP:return "JUMP";
    case KEEN_POSE_FALL:return "FALL";
    case KEEN_POSE_POGO:return "POGO";
    default:return "IDLE";
    }
}
#endif
