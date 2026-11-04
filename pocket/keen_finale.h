/* SPDX-License-Identifier: MIT
 * Keen 1 finale sequence. Reference: Chocolate Keen CVort1_draw_win. */
#ifndef KEEN_FINALE_H
#define KEEN_FINALE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "keen_timing.h"
#define KF_SCENE 1u
#define KF_CLICK 2u
#define KF_FINISH 4u
#define KF_BAD 8u
typedef struct {
 const uint8_t *data;unsigned active,finished,phase,age,count,pending,interval,scene,frame,overlay,window,shown;
 int dialog;int32_t x,y,camera_x,camera_y,saved_x,saved_y;
} KeenFinale;
static inline int kf_i16(const uint8_t*p){unsigned n=p[0]|((unsigned)p[1]<<8);return n<32768?(int)n:(int)n-65536;}
static inline const char*kf_message(const KeenFinale*f,unsigned id){return id<10?(const char*)f->data+396+id*512:0;}
static inline unsigned kf_length(const KeenFinale*f){
 if(f->dialog<0)return 0;
 const char*p=kf_message(f,(unsigned)f->dialog);unsigned n=0;while(n<512&&p[n])n++;return n;
}
static inline unsigned kf_enter(KeenFinale*f,unsigned phase){
 unsigned previous=f->scene,events=0;f->phase=phase;f->age=f->count=f->interval=f->shown=0;f->dialog=-1;f->overlay=0;
 switch(phase){
 case 0:f->scene=80;f->frame=40;f->x=208*256;f->y=624*256;f->camera_x=48*256;f->camera_y=560*256;break;
 case 1:f->dialog=0;break;
 case 2:f->scene=81;f->camera_x=f->camera_y=0;f->x=100*256;f->y=80*256;f->frame=115;break;
 case 7:f->overlay=118;break;
 case 8:f->frame=116;break;
 case 9:f->overlay=117;break;
 case 10:f->frame=115;break;
 case 12:f->saved_x=f->camera_x;f->saved_y=f->camera_y;f->scene=0;f->window=0;f->dialog=1;break;
 case 14:f->window=12;events|=KF_CLICK;break;
 case 15:f->window=13;events|=KF_CLICK;break;
 case 16:f->dialog=2;break;
 case 18:f->window=12;f->dialog=3;events|=KF_CLICK;break;
 case 19:f->dialog=4;break;
 case 20:f->dialog=5;break;
 case 21:f->dialog=6;break;
 case 22:f->dialog=7;break;
 case 24:f->window=13;events|=KF_CLICK;break;
 case 25:f->window=12;f->dialog=8;events|=KF_CLICK;break;
 case 26:f->scene=81;f->frame=116;f->camera_x=f->saved_x;f->camera_y=f->saved_y;break;
 case 28:f->frame=115;break;
 case 29:f->dialog=9;break;
 case 30:f->active=0;f->finished=1;events|=KF_FINISH;break;
 default:break;
 }
 if(previous!=f->scene)events|=KF_SCENE;
 return events;
}
static inline int keen_finale_init(KeenFinale*f,const uint8_t*data,size_t size){
 memset(f,0,sizeof(*f));if(!data||size!=5516||memcmp(data,"KF01",4))return -1;
 for(unsigned i=0;i<10;i++)if(!memchr(data+396+i*512,0,512))return -1;
 const unsigned offsets[]={4,64,236},pairs[]={15,43,40};
 for(unsigned t=0;t<3;t++){
  int ended=0;for(unsigned i=0;i<pairs[t];i++)if(kf_i16(data+offsets[t]+i*4)+kf_i16(data+offsets[t]+i*4+2)==0){ended=1;break;}
  if(!ended)return -1;
 }
 f->data=data;f->active=1;(void)kf_enter(f,0);return 0;
}
static inline unsigned kf_dialog(KeenFinale*f,unsigned hold){
 unsigned length=kf_length(f),typed=length;
 if(f->dialog==3){const char*p=kf_message(f,3);typed=0;while(typed<length&&p[typed++]!=10){}}
 f->shown=(unsigned)(kt_refresh_units(f->age)/(6u*KT_REFRESH_DENOMINATOR));if(f->shown>=typed)f->shown=length;
 return kt_delay_elapsed(f->age,typed*6+hold)?kf_enter(f,f->phase+1):0;
}
static inline unsigned keen_finale_step(KeenFinale*f,unsigned ticks){
 if(!f->active)return 0;
 if(ticks>64)ticks=64;
 f->pending+=ticks;unsigned events=0;
 while(f->active&&f->pending){
  f->pending--;f->age++;f->interval++;
  if(f->phase==3||f->phase==4||f->phase==5||f->phase==7||f->phase==8||f->phase==9){
   if(f->interval<6)continue;
   f->interval-=6;
  }
  switch(f->phase){
  case 0:if(kt_delay_elapsed(f->age,300))events|=kf_enter(f,1);break;
  case 1:events|=kf_dialog(f,240);break;
  case 2:case 10:case 27:{
   unsigned offset=f->phase==2?4u:f->phase==10?236u:64u;
   unsigned bound=f->phase==2?15u:f->phase==10?40u:43u;
   if(f->phase==2){if(!kt_delay_elapsed(f->age,(f->count+1u)*4u))break;}
   else{if(f->interval<6)break;f->interval-=6;}
   if(f->count>=bound){f->active=0;events|=KF_BAD;break;}
   int dx=kf_i16(f->data+offset+f->count*4),dy=kf_i16(f->data+offset+f->count*4+2);f->count++;
   f->x+=dx*256;f->y+=dy*256;if(!(dx+dy))events|=kf_enter(f,f->phase+1);break;
  }
  case 3:f->x+=768;f->y+=256;if(++f->count>=21)events|=kf_enter(f,4);break;
  case 4:f->x+=768;f->y+=256;f->camera_x+=768;f->camera_y+=256;if(++f->count>=201)events|=kf_enter(f,5);break;
  case 5:f->y+=256;if(f->count<20)f->camera_y+=768;if(++f->count>=61)events|=kf_enter(f,6);break;
  case 6:if(kt_delay_elapsed(f->age,20))events|=kf_enter(f,7);break;
  case 7:if(++f->count>=31)events|=kf_enter(f,8);break;
  case 8:f->x-=512;if(++f->count>=51)events|=kf_enter(f,9);break;
  case 9:if(++f->count>=31)events|=kf_enter(f,10);break;
  case 11:if(kt_delay_elapsed(f->age,60))events|=kf_enter(f,12);break;
  case 12:events|=kf_dialog(f,120);break;
  case 13:case 17:case 23:if(kt_delay_elapsed(f->age,60))events|=kf_enter(f,f->phase+1);break;
  case 14:if(kt_delay_elapsed(f->age,120))events|=kf_enter(f,15);break;
  case 15:case 24:if(kt_delay_elapsed(f->age,300))events|=kf_enter(f,f->phase+1);break;
  case 16:case 19:case 20:case 21:case 22:events|=kf_dialog(f,180);break;
  case 18:events|=kf_dialog(f,240);break;
  case 25:events|=kf_dialog(f,300);break;
  case 26:if(kt_delay_elapsed(f->age,30))events|=kf_enter(f,27);break;
  case 28:if(kt_delay_elapsed(f->age,60))events|=kf_enter(f,29);break;
  case 29:events|=kf_dialog(f,400);break;
  default:f->active=0;events|=KF_BAD;break;
  }
 }
 return events;
}
#endif
