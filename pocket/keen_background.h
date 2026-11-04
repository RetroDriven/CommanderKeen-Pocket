/* SPDX-License-Identifier: MIT
 * Vorticons pickup replacement tiles. Reference: Chocolate Keen physics.c. */
#ifndef RETRODRIVEN_KEEN_BACKGROUND_H
#define RETRODRIVEN_KEEN_BACKGROUND_H
#include <stdint.h>
#include "keen_map.h"
#define KEEN_BG_TILES 1024u
/* Chocolate Keen physics.c: Keen 3 retains each pickup group background. */
static inline unsigned keen_background_after_pickup(unsigned episode,unsigned tile,unsigned fallback){
    return episode==2u ? (tile/13u)*13u : fallback;
}
/* Chocolate Keen physics.c distinguishes gray and purple pickup variants;
 * Keen 1's pogo/ammo, ship parts and keys always restore tile 143. */
static inline unsigned keen_background_for_pickup(unsigned episode,unsigned tile,int behavior,unsigned fallback){
    if(episode==2u)return (tile/13u)*13u;
    if(episode>1u||behavior<6||behavior>21||behavior==17)return fallback;
    if((behavior>=6&&behavior<=10)||(episode==1u&&(behavior==15||behavior==16)))
        return tile<0x131u?0x8fu:0x114u;
    return 0x8fu;
}
static unsigned keen_background_choose(const uint8_t *map, const KeenMapInfo *info,
                                       unsigned tiles, const int16_t *behaviors,
                                       int have_behaviors){
    unsigned counts[KEEN_BG_TILES]={0};
    unsigned fallback[KEEN_BG_TILES]={0};
    if(!map || !info || !tiles)return 0;
    if(tiles>KEEN_BG_TILES)tiles=KEEN_BG_TILES;
    for(unsigned y=0;y<info->height;y++)for(unsigned x=0;x<info->width;x++){
        unsigned t=keen_map_tile(map,info,x,y,0);
        if(t>=tiles)continue;
        fallback[t]++;
        if(!have_behaviors || !behaviors || behaviors[t]==0)counts[t]++;
    }
    unsigned best=0, best_count=0;
    for(unsigned t=0;t<tiles;t++)if(counts[t]>best_count){
        best=t;best_count=counts[t];
    }
    if(best_count)return best;
    for(unsigned t=0;t<tiles;t++)if(fallback[t]>best_count){
        best=t;best_count=fallback[t];
    }
    return best;
}
#endif
