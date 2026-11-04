/* SPDX-License-Identifier: MIT
 * Keen 3 dialogue and camera sequences. Reference: Chocolate Keen
 * CVort3_handle_grand_intellect and CVort3_draw_win. */
#ifndef KEEN3_CINEMA_H
#define KEEN3_CINEMA_H
#include <stdint.h>
#include <string.h>
typedef struct {unsigned active,kind,phase,age,shown,wait_release,ship,events;const uint8_t*data;} KeenCinema3;
static inline const char*k3c_text(const KeenCinema3*c){unsigned id=c->kind==2?14+c->ship:c->kind==0?c->phase:c->phase<4?9+c->phase:13;return id<18?(const char*)c->data+4+id*512:"";}
static inline int keen3_cinema_init(KeenCinema3*c,const uint8_t*data,size_t size,unsigned kind,unsigned ship){
 if(!data||size!=9237||memcmp(data,"KC03",4)||kind>2||ship>3)return -1;
 for(unsigned i=0;i<18;i++)if(!memchr(data+4+i*512,0,512))return -1;
 memset(c,0,sizeof(*c));c->data=data;c->kind=kind;c->ship=ship;c->active=c->wait_release=1;if(kind==2)c->shown=(unsigned)strlen(k3c_text(c));return 0;
}
static inline void keen3_cinema_step(KeenCinema3*c,unsigned ticks,int pressed,int held){
 c->events=0;if(!c->active)return;
 if(c->wait_release){if(!held)c->wait_release=0;pressed=0;}
 if(c->kind==0&&pressed){c->active=0;return;}
 if(c->kind==2){if(pressed)c->active=0;return;}
 if(ticks>64)ticks=64;
 for(unsigned t=0;t<ticks&&c->active;t++){
  c->age++;
  if(c->kind==0){static const unsigned delay[]={180,180,240,240,240,180,180,180,240};
   unsigned length=(unsigned)strlen(k3c_text(c));c->shown=c->age/6;if(c->shown>length)c->shown=length;
   if(c->age>=length*6+delay[c->phase]){c->phase++;c->age=c->shown=0;if(c->phase==9)c->active=0;}
  }else{
   unsigned duration=0;
   if(c->phase<4){unsigned length=(unsigned)strlen(k3c_text(c));c->shown=c->age/6;if(c->shown>length)c->shown=length;duration=length*6+120;}
   else if(c->phase<9){static const unsigned delay[]={22,12,7,12,17};duration=delay[c->phase-4];}
   else if(c->phase==9)duration=120;
   else if(c->phase==10)duration=400;
   else if(c->phase==11){unsigned length=(unsigned)strlen(k3c_text(c));c->shown=c->age/6;if(c->shown>length)c->shown=length;if(c->shown==length&&pressed)c->active=0;}
   if(duration&&c->age>=duration){c->phase++;c->age=c->shown=0;c->events|=1u<<c->phase;}
  }
 }
}
#endif
