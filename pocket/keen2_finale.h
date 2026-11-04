/* SPDX-License-Identifier: MIT
 * Keen 2 victory and Earth-destruction sequences. Reference: Chocolate Keen episode2.c.
 * Dialogue and explosion tables come from FINALE2.CK2. */
#ifndef KEEN2_FINALE_H
#define KEEN2_FINALE_H
#include "keen_finale.h"
#define K2F_TANK 32u
#define K2F_EARTH 64u
typedef struct {
 const uint8_t*data;unsigned active,failed,phase,age,interval,count,shown;
 int scene,dialog;unsigned frame;int32_t x,y,camera_x,camera_y,saved_x,saved_y;
} KeenFinale2;
static inline const char*k2f_message(const KeenFinale2*f,unsigned n){return n<8?(const char*)f->data+216+n*512:"";}
static inline unsigned k2f_enter(KeenFinale2*f,unsigned phase){
 int old=f->scene;f->phase=phase;f->age=f->interval=f->count=f->shown=0;f->dialog=-1;unsigned events=0;
 switch(phase){
  case 0:f->scene=81;f->x=0x7000;f->y=0x3000;f->camera_x=f->camera_y=0;f->frame=132;f->dialog=0;break;
  case 3:f->scene=81;f->camera_x=0;f->camera_y=0xc000;f->x=0xa000;f->y=0x11000;f->frame=72;f->dialog=1;break;
  case 5:f->scene=0;f->dialog=2;break;
  case 6:case 7:case 8:case 9:f->dialog=(int)phase-3;break;
  case 10:case 25:f->active=0;events|=KF_FINISH;break;
  case 21:f->dialog=7;f->shown=(unsigned)strlen(k2f_message(f,7));break;
  case 22:f->scene=81;f->camera_x=f->camera_y=0;f->x=0x6000;f->y=0x5000;f->frame=0;break;
  case 23:events|=K2F_TANK;f->frame=58;break;
  case 24:events|=K2F_EARTH;f->frame=0;break;
  default:break;
 }
 if(old!=f->scene)events|=KF_SCENE;
 return events;
}
static inline int keen2_finale_init(KeenFinale2*f,const uint8_t*data,size_t size,int failed,int shake){
 memset(f,0,sizeof(*f));if(!data||size!=4312||memcmp(data,"KF02",4))return -1;
 for(unsigned i=0;i<8;i++)if(!memchr(data+216+i*512,0,512))return -1;
 f->data=data;f->active=1;f->failed=!!failed;f->scene=-1;
 (void)k2f_enter(f,failed?(shake?20u:22u):0u);return 0;
}
static inline unsigned keen2_finale_step(KeenFinale2*f,unsigned ticks,KeenActors*random){
 if(ticks>64)ticks=64;
 unsigned events=0;
 while(f->active&&ticks--){
  f->age++;f->interval++;
  if(f->dialog>=0&&f->phase!=21){
   unsigned length=(unsigned)strlen(k2f_message(f,(unsigned)f->dialog));
   f->shown=(unsigned)(kt_refresh_units(f->age)/(6u*KT_REFRESH_DENOMINATOR));if(f->shown>length)f->shown=length;
   unsigned hold=f->phase==0?300:f->phase==9?400:120;
   if(kt_delay_elapsed(f->age,length*6+hold))events|=k2f_enter(f,f->phase+1);
   continue;
  }
  switch(f->phase){
   case 1:
    if(f->interval<6)break;
    f->interval-=6;f->x=0x7000+(int32_t)(f->count*0x28800u/120u);f->y=0x3000+(int32_t)(f->count*0x13800u/120u);
    f->camera_x=f->x-0x7000;f->camera_y=f->y-0x3000;if(++f->count>=120)events|=k2f_enter(f,2);break;
   case 2:if(kt_delay_elapsed(f->age,120))events|=k2f_enter(f,3);break;
   case 4:if(f->interval>=6){f->interval-=6;f->x-=256;f->y-=256;if(++f->count>=120)events|=k2f_enter(f,5);}break;
   case 20:
    if(f->interval>=6){f->interval-=6;f->camera_x=f->saved_x-((int)ka_jump(random,64)-32)*256;
     f->camera_y=f->saved_y-((int)ka_jump(random,64)-32)*256;if(++f->count>=79)events|=k2f_enter(f,21);}break;
   case 21:if(kt_delay_elapsed(f->age,100))events|=k2f_enter(f,22);break;
   case 22:if(kt_delay_elapsed(f->age,120))events|=k2f_enter(f,23);break;
   case 23:
    if(f->interval<6)break;
    f->interval-=6;f->x=0x6000+(int32_t)(0x29000u*f->count/20u);f->y=0x5000+(int32_t)(0x11000u*f->count/20u);
    f->frame=58+f->count%2;f->camera_x=f->x-0x6000;f->camera_y=f->y-0x5000;
    if(++f->count>=21)events|=k2f_enter(f,24);
    break;
   case 24:
    if(f->interval>6&&kt_delay_elapsed(f->interval-6,3)){f->interval=0;if(++f->count>=60)events|=k2f_enter(f,25);}break;
   default:f->active=0;events|=KF_BAD;break;
  }
 }
 return events;
}
#endif
