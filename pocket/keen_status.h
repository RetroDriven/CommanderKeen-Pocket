/* SPDX-License-Identifier: MIT
 * Vorticons status panel and 8x8 EGA font. Reference: Chocolate Keen episode1/2/3.c
 * show_pause_menu and ui.c boxes. */
#ifndef KEEN_STATUS_H
#define KEEN_STATUS_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
typedef struct {uint32_t score,extra_at;unsigned lives,ammo,keys,parts,pogo,targets,ankh;} KeenStatusValues;
typedef struct {
 void *user;
 void (*glyph)(void*,int,int,unsigned);
 void (*tile)(void*,int,int,unsigned);
 void (*sprite)(void*,int,int,unsigned);
} KeenStatusCanvas;
static inline unsigned keen_status_font_pixel(const uint8_t *latch,size_t len,
 uint32_t plane,uint32_t offset,unsigned count,unsigned ch,unsigned x,unsigned y){
 if(!latch||x>=8||y>=8||ch>=count||(uint64_t)plane*4u>len)return 0;
 uint64_t pos=(uint64_t)offset+ch*8u+y;if(pos>=plane)return 0;
 unsigned mask=128u>>x,color=0;
 for(unsigned p=0;p<4;p++)if(latch[(size_t)pos+(size_t)p*plane]&mask)color|=1u<<p;
 return color;
}
static inline void ks_text(KeenStatusCanvas *c,int x,int y,const char *s,int alternate){
 for(unsigned i=0;s[i];i++)c->glyph(c->user,x+(int)i*8,y,(unsigned char)s[i]+(alternate?128u:0u));
}
static inline void ks_number(KeenStatusCanvas *c,int x,int y,uint32_t n,int right){
 char s[16];snprintf(s,sizeof(s),"%lu",(unsigned long)n);unsigned len=0;while(s[len])len++;
 ks_text(c,x-(right?(int)len*8:0),y,s,0);
}
static inline unsigned keen_status_height(unsigned ep){return ep==0?13u:ep==1?12u:11u;}
static inline void keen_text_box(KeenStatusCanvas*c,int width,int height){
 int x=(320-(width+2)*8)/2,y=(240-(height+2)*8)/2;
 for(int row=0;row<height+2;row++)for(int col=0;col<width+2;col++){
  unsigned ch=32;
  if(!row)ch=!col?1u:col==width+1?3u:2u;
  else if(row==height+1)ch=!col?6u:col==width+1?8u:7u;
  else if(!col)ch=4;else if(col==width+1)ch=5;
  c->glyph(c->user,x+col*8,y+row*8,ch);
 }
}
static inline void keen_status_box(KeenStatusCanvas *c,unsigned ep,unsigned step){
 if(step>14)step=14;
 if(step<1)step=1;
 int width=(int)step*2,height=(int)keen_status_height(ep)-(28-width);
 int min_height=ep==1?2:1;if(height<min_height)height=min_height;
 int x=(320-(width+2)*8)/2,y=(240-(height+2)*8)/2;
 for(int row=0;row<height+2;row++)for(int col=0;col<width+2;col++){
  unsigned ch=32;
  if(!row)ch=!col?1u:col==width+1?3u:2u;
  else if(row==height+1)ch=!col?6u:col==width+1?8u:7u;
  else if(!col)ch=4;else if(col==width+1)ch=5;
  c->glyph(c->user,x+col*8,y+row*8,ch);
 }
}
static inline void keen_status_draw(KeenStatusCanvas *c,unsigned ep,const KeenStatusValues *v,unsigned step){
 keen_status_box(c,ep,step);if(step<14)return;
 int x=48,y=(240-((int)keen_status_height(ep)+2)*8)/2+8;
 ks_text(c,x,y,"    SCORE     EXTRA KEEN AT ",1);
 ks_number(c,x+80,y+8,v->score,1);ks_number(c,x+208,y+8,v->extra_at,1);
 ks_text(c,x,y+16,ep==0?"    KEENS       SHIP PARTS  ":"    KEENS            PISTOL ",1);
 unsigned max=ep==0?6u:9u;
 for(unsigned n=0;n<v->lives&&n<max;n++)c->sprite(c->user,x+8+(int)n*16,y+24,0);
 if(ep==0){
  for(unsigned n=0;n<4;n++)c->tile(c->user,x+128+(int)n*24,y+28,(v->parts&(1u<<n))?448u+n:321u+n);
  ks_text(c,x,y+48," RAYGUN   POGO    KEYCARDS  ",1);
  c->tile(c->user,x+24,y+60,414);ks_number(c,x+24,y+88,v->ammo,0);
  if(v->pogo)c->tile(c->user,x+88,y+68,415);
  for(unsigned n=0;n<4;n++)if(v->keys&(1u<<n))c->tile(c->user,x+152+(int)(n&1u)*32,y+59+(int)(n>>1)*18,424u+n);
  ks_text(c,x,y+80," CHARGE  ",1);
 }else{
  c->tile(c->user,x+168,y+28,ep==1?414u:216u);ks_number(c,x+192,y+32,v->ammo,0);
  ks_text(c,x,y+48,ep==1?"   TARGETS SAVED      KEYS  ":" ANKH TIME      KEY CARDS   ",1);
  if(ep==1){
   static const char *const cities[]={"London","Cairo","Sydney","New York","Paris","Rome","Moscow","Wash D.C."};
   for(unsigned n=0;n<8;n++)if(v->targets&(1u<<n))ks_text(c,x+(n>=4?80:0),y+56+(int)(n&3u)*8,cities[n],0);
   for(unsigned n=0;n<4;n++)if(v->keys&(1u<<n))c->tile(c->user,x+168+(int)(n&1u)*32,y+56+(int)(n>>1)*16,424u+n);
  }else{
   c->tile(c->user,x+24,y+60,214);ks_number(c,x+56,y+64,v->ankh,0);
   for(unsigned n=0;n<4;n++)if(v->keys&(1u<<n))c->tile(c->user,x+104+(int)n*32,y+60,217u+n);
  }
 }
 ks_text(c,x,y+(int)(keen_status_height(ep)-1u)*8,"     PLEASE PRESS A KEY     ",1);
}
#endif
