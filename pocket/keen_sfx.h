/* SPDX-License-Identifier: MIT
 * Vorticons movement sound requests. Reference: Chocolate Keen engine_timing.c and
 * game/enemies.c. */
#ifndef RETRODRIVEN_KEEN_SFX_H
#define RETRODRIVEN_KEEN_SFX_H
#include "keen_motion.h"
#include "keen_sound.h"
typedef struct { uint32_t last_us,ticks; uint64_t remainder; } KeenSfxClock;
static inline void keen_sfx_reset(KeenSfxClock *c,uint32_t now){
    c->last_us=now;c->ticks=0;c->remainder=0;
}
static inline void keen_sfx_clock_step(KeenSfxClock *c,uint32_t now){
    uint32_t elapsed=now-c->last_us; /* handles the microsecond timer wrap */
    uint64_t scaled=c->remainder+(uint64_t)elapsed*KS_PIT_HZ;
    const uint64_t period=(uint64_t)KS_TICK_DIVISOR*1000000u;
    c->ticks+=(uint32_t)(scaled/period);
    c->remainder=scaled%period;c->last_us=now;
}
static inline const char *keen_sfx_movement(const KeenSfxClock *c,
        const KeenMotion *before,const KeenMotion *after,int jump,int pogo){
    /* Normal jump and pogo takeoff use sound 6 (bank index 5). Toggling pogo in midair is
     * silent. */
    if(before->grounded && (jump||pogo) && after->vy<0)
        return "KEENJUMPSND";
    if(!before->grounded && after->grounded && !pogo)
        return "KEENLANDSND";
    if(before->vy<-1 && after->vy==0 && !after->grounded)
        return "BUMPHEADSND";
    /* Footsteps require displacement and alternate in 16-tick windows. Landing and takeoff
     * have priority over a footstep. */
    if(before->grounded && after->grounded && !pogo &&
       before->x!=after->x && ((c->ticks>>4)&1u))
        return ((c->ticks>>5)&1u)?"KEENWLK2SND":"KEENWALKSND";
    return 0;
}
#endif
