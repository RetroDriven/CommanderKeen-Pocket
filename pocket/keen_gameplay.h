/* SPDX-License-Identifier: MIT
 * Keen 1-3 tile interactions using Vorticons TILEINFO behavior values. */
#ifndef RETRODRIVEN_KEEN_GAMEPLAY_H
#define RETRODRIVEN_KEEN_GAMEPLAY_H
#include <stdint.h>
#include <string.h>
#define KEEN_MAX_MAP_CELLS 65536u
#define KEEN_CONSUMED_BYTES (KEEN_MAX_MAP_CELLS/8u)
typedef struct {
    uint32_t score,extra_life_base;
    unsigned targets_mask,ankh_ticks;
    unsigned collected, deaths, ammo;
    unsigned key_mask, ship_mask;
    int found_pogo, reached_exit;
    uint8_t consumed[KEEN_CONSUMED_BYTES];
    const char *event;
    const char *sound_cue; /* set only by a newly accepted tile event */
} KeenGameplay;
typedef enum {
    KG_NONE=0, KG_PICKUP=1, KG_HAZARD=2, KG_EXIT=3, KG_LOCKED=4, KG_MESSAGE=5
} KeenEvent;
static void keen_gameplay_reset(KeenGameplay *g){
    if(!g)return;
    memset(g,0,sizeof(*g));g->event="MOVE TO COLLECT ORIGINAL TILES";
}
/* Chocolate Keen CVort_add_score: one life per scoring event crossing 20000. */
static int keen_gameplay_extra_life(KeenGameplay *g){
    if(g->score-g->extra_life_base<20000u)return 0;
    g->extra_life_base=(g->score/20000u)*20000u;return 1;
}
static int keen_gameplay_used(const KeenGameplay *g,unsigned index){
    return g && index<KEEN_MAX_MAP_CELLS &&
           ((g->consumed[index>>3u]>>(index&7u))&1u);
}
static void keen_gameplay_consume(KeenGameplay *g,unsigned index){
    if(g && index<KEEN_MAX_MAP_CELLS)g->consumed[index>>3u]|=(uint8_t)(1u<<(index&7u));
}
/* Requires valid TILEINFO and an in-bounds map cell. Consumed tiles remain inactive on
 * later frames. */
static KeenEvent keen_gameplay_touch(KeenGameplay *g,unsigned episode,int16_t behavior,
                                     unsigned cell){
    if(!g)return KG_NONE;
    g->sound_cue=0;
    if(g->reached_exit || cell>=KEEN_MAX_MAP_CELLS || keen_gameplay_used(g,cell))return KG_NONE;
    if(behavior==1){g->deaths++;g->event="HAZARD - RESET TO SPAWN";return KG_HAZARD;}
    if(behavior>=2 && behavior<=5){
        unsigned bit=1u<<(unsigned)(behavior-2);
        if(!(g->key_mask&bit)){g->event="DOOR LOCKED - KEY REQUIRED";return KG_LOCKED;}
        /* Door collision and animated bodies are resolved by the application. */
        g->event="MATCHING DOOR KEY";return KG_NONE;
    }
    if(behavior==17){g->reached_exit=1;g->event="LEVEL EXIT - A FOR MAP";
        g->sound_cue="LVLDONESND";return KG_EXIT;}
    if(behavior>=6 && behavior<=10){
        static const unsigned points[5]={500,100,200,1000,5000};
        g->score+=points[behavior-6];g->collected++;
        g->event="POINT ITEM COLLECTED";g->sound_cue="GOTBONUSSND";
    }else if(behavior>=11 && behavior<=14 && episode==0u){
        g->ship_mask|=1u<<(unsigned)(behavior-11);
        g->score+=10000;g->collected++;g->event="SHIP PART FOUND";g->sound_cue="GOTPARTSND";
    }else if(behavior==15 || (behavior==28 && episode==2u)){
        /* Keen 3 behavior 28 grants one shot (Chocolate Keen physics.c). */
        g->ammo+=(behavior==28)?1u:5u;g->collected++;
        g->event="AMMO COLLECTED";g->sound_cue="GOTITEMSND";
    }else if(behavior==16){
        g->found_pogo=1;g->collected++;g->event="POGO FOUND";g->sound_cue="GOTITEMSND";
    }else if(behavior>=18 && behavior<=21){
        g->key_mask|=1u<<(unsigned)(behavior-18);
        g->collected++;g->event="KEY COLLECTED";g->sound_cue="GETCARDSND";
    }else{
        /* Messages, switches, actors and teleporters are handled by the game controller. */
        return KG_NONE;
    }
    keen_gameplay_consume(g,cell);
    return KG_PICKUP;
}
#endif
