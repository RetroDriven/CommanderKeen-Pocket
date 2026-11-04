/* Native-pixel presentation for the 320x240 Pocket canvas. SPDX-License-Identifier: MIT */
#ifndef KEEN_VIEW_H
#define KEEN_VIEW_H
#include <stdint.h>
#include <string.h>
/* Flat DOS artwork keeps its pixels. Fill its surrounding canvas using the
 * dominant edge colours, avoiding unrelated black bars on light menu pages. */
static inline uint8_t keen_view_edge_colour(const uint8_t *row){
 unsigned counts[256]={0},best=0;
 for(unsigned x=0;x<320;x++){unsigned c=row[x];if(++counts[c]>counts[best])best=c;}
 return (uint8_t)best;
}
static inline void keen_view_flat_margins(uint8_t *fb,int enhanced,uint8_t black){
 uint8_t top=enhanced?keen_view_edge_colour(fb+320*20):black;
 uint8_t bottom=enhanced?keen_view_edge_colour(fb+320*219):black;
 memset(fb,top,320*20);memset(fb+320*220,bottom,320*20);
}
static inline void keen_view_center_original(uint8_t *fb,uint8_t black){
 memmove(fb+320*20,fb,320*200);
 keen_view_flat_margins(fb,0,black);
}
#endif
