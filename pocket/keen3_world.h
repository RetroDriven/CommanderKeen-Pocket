/* SPDX-License-Identifier: MIT
 * Keen 3 world-map transport. Reference: Chocolate Keen worldmap.c and episode3.c. The
 * sprite stays at the departure shore until dismount. */
#ifndef KEEN3_WORLD_H
#define KEEN3_WORLD_H
#include "keen_map.h"
typedef struct {
 int32_t x,y;int dx,dy,previous_x,previous_y;unsigned ready,mounted,cooldown,tics,frame,pending;
 unsigned teleport_x[16],teleport_y[16],teleport_mask;
} KeenWorld3;
static inline unsigned k3_world_object(const uint8_t*map,const KeenMapInfo*mi,int x,int y){return x<0||y<0||x>=(int)mi->width||y>=(int)mi->height?0:keen_map_tile(map,mi,(unsigned)x,(unsigned)y,1);}
static inline void keen3_world_init(KeenWorld3*w,const uint8_t*map,const KeenMapInfo*mi){
 memset(w,0,sizeof(*w));
 for(unsigned y=0;y<mi->height;y++)for(unsigned x=0;x<mi->width;x++){
  unsigned id=keen_map_tile(map,mi,x,y,1);
  if(!w->ready&&x==mi->width/2&&id==0x2000){w->ready=1;w->x=(int32_t)x*4096;w->y=(int32_t)y*4096;w->previous_x=(int)x;w->previous_y=(int)y;w->dx=-512;w->tics=8;w->frame=130;}
  if((id&0xf00)==0xf00){unsigned index=(id>>4)&15u;w->teleport_x[index]=x;w->teleport_y[index]=y;w->teleport_mask|=1u<<index;}
 }
}
static inline void keen3_world_step(KeenWorld3*w,unsigned ticks,const uint8_t*map,const KeenMapInfo*mi,int*keen_x,int*keen_y,int dx,int dy,unsigned sprite_w,unsigned sprite_h){
 if(!w->ready)return;
 if(ticks>64)ticks=64;
 w->pending+=ticks;
 while(w->pending>=6){w->pending-=6;
  if(w->tics<=8){w->x+=w->dx;w->y+=w->dy;}
  unsigned index=(unsigned)((w->dy/512+1)*3+w->dx/512+1);static const unsigned frame[]={136,136,134,130,130,132,130,132,132};
  w->frame=frame[index]+(w->mounted?8u:0u);
  if(!--w->tics){w->tics=8;int x=w->x/4096,y=w->y/4096,found=0;
   for(int xx=x-1;xx<=x+1&&!found;xx++)for(int yy=y-1;yy<=y+1;yy++){
    unsigned id=k3_world_object(map,mi,xx,yy);if((id!=0x2000&&id!=0x2100)||(xx==x&&yy==y)||(xx==w->previous_x&&yy==w->previous_y))continue;
    w->dx=xx<x?-512:xx>x?512:0;w->dy=yy<y?-512:yy>y?512:0;w->previous_x=x;w->previous_y=y;w->tics=id==0x2100?130:8;found=1;break;
   }
  }
  if(w->mounted){if((dx||dy)&&k3_world_object(map,mi,w->x/4096+dx,w->y/4096+dy)==0x2200){w->mounted=0;*keen_x=w->x/256+dx*16;*keen_y=w->y/256+dy*16;w->cooldown=30;}}
  else if(w->cooldown)w->cooldown--;
  else if(*keen_x>=w->x/256&&*keen_x<=w->x/256+(int)sprite_w&&*keen_y>=w->y/256&&*keen_y<=w->y/256+(int)sprite_h){w->mounted=1;w->cooldown=30;}
 }
}
#endif
