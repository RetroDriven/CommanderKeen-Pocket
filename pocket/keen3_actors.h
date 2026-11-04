/* SPDX-License-Identifier: MIT
 * Keen 3 actors. Reference: Chocolate Keen episode3.c and game/enemies.c. */
#ifndef KEEN3_ACTORS_H
#define KEEN3_ACTORS_H
#include "keen2_actors.h"
#define K3_MOM_FIRE 131072u
#define K3_MEEP_FIRE 262144u
#define K3_FOOB 524288u
#define K3_SPARK 1048576u
#define K3_STOMP 2097152u
enum {K3_SCARED=9,K3_RUN};
typedef struct {int x,y,direction,side,period;unsigned kind,time,wait;} KeenBody3;
typedef struct {
 KeenActors core;KeenActorBox boxes[101];int32_t old_x[KA_MAX],old_y[KA_MAX];
 KeenBody3 bodies[32];unsigned body_count,sparks,arm_destruct,arm_row,arm_time,destruct,row,time,finished,body_pending;
 int stun_vx,stun_vy;
} KeenActors3;
static inline KeenActorBox k3_box(const KeenActors3*w,unsigned frame){return frame>=47&&frame<148?w->boxes[frame-47]:(KeenActorBox){0,0,15,15};}
static inline int keen3_actors_init(KeenActors3*w,const uint8_t*map,const KeenMapInfo*mi,const uint8_t*head,size_t hlen,const KeenSpriteInfo*si,const uint8_t*data,size_t len,int keen_x,int keen_y){
 memset(w,0,sizeof(*w));if(!data||len!=294||memcmp(data,"KA03",4)||!map||!mi||!si||si->sprite_count<148)return -1;
 KeenActors*c=&w->core;memcpy(c->random,data+4,256);for(unsigned i=1;i<18;i++)c->jump[i]=keen_le16(data+258+i*2);
 c->j1=17;c->j2=5;c->width=mi->width;c->height=mi->height;
 for(unsigned frame=47;frame<148;frame++){
  KeenSprite sp;if(keen_sprite_get(head,hlen,si,frame,&sp))return -1;
  const uint8_t*m=head+si->table_offset+frame*128u;KeenActorBox*b=&w->boxes[frame-47];
  *b=(KeenActorBox){keen_le16(m+8)/256,keen_le16(m+10)/256,keen_le16(m+12)/256,keen_le16(m+14)/256};
  if(b->l>b->r||b->t>b->b||b->r>=(int)sp.w||b->b>=(int)sp.h)return -1;
 }
 for(unsigned y=0;y<mi->height;y++)for(unsigned x=0;x<mi->width;x++){
  unsigned id=keen_map_tile(map,mi,x,y,1);if(id<1||id>18)continue;
  if(id>=14){if(w->body_count>=32)return -1;w->bodies[w->body_count++]=(KeenBody3){(int)x,(int)y,id==17?-1:1,id==17?-1:1,15,id,0,0};continue;}
  if(c->count>=KA_MAX)return -1;
  KeenActor*a=&c->actors[c->count++];a->kind=id;a->x=(int32_t)x*4096;a->y=(int32_t)y*4096;a->mode=KA_WALK;
  int direction=a->x<keen_x*256?1:-1;a->health=1;
  switch(id){
   case 1:case 11:a->frame=71;a->vx=direction*90;break;
   case 2:a->frame=47;a->vx=direction*250;break;
   case 3:a->frame=85;a->vx=direction*50;a->health=5;break;
   case 4:a->frame=120;a->vx=direction*65;a->y+=2048;break;
   case 5:a->frame=77;a->vx=direction;a->health=3;break;
   case 6:a->frame=93;a->vx=direction*50;break;
   case 7:case 8:a->frame=id==7?109:110;a->vx=direction*400;a->vy=a->y>keen_y*256?-400:400;break;
   case 9:a->frame=107;a->vx=75;a->y-=1024;break;
   case 10:a->frame=107;a->vy=75;break;
   case 12:case 13:a->frame=114;break;
  }
 }
 c->ready=1;return 0;
}
static inline unsigned k3_shot_frame(const KeenEnemyShot*s,unsigned ticks){
 if(s->state!=1)return s->state==2?105:106;
 if(s->kind==2)return (s->vx<0?57u:59u)+((ticks>>3)&1u);
 if(s->kind==3)return (s->vx<0?128u:126u)+((ticks>>3)&1u);
 return s->vy?104u:103u;
}
static inline void k3_shoot(KeenActors3*w,int32_t x,int32_t y,int vx,int vy,unsigned kind,KeenSolid f,void*u){
 for(unsigned i=0;i<KA_BULLETS;i++)if(!w->core.shots[i].state){
  KeenEnemyShot*s=&w->core.shots[i];*s=(KeenEnemyShot){x,y,vx,1,0,kind,vy};
  if(kind==2&&f(kv_floor(x)/16+(vx>0),kv_floor(y)/16,vx>0?KEEN_FACE_RIGHT:KEEN_FACE_LEFT,u)){s->state=2;w->core.events|=KA_SHOT_HIT;}
  w->core.events|=kind==2?K3_MOM_FIRE:kind==3?K3_MEEP_FIRE:KA_TANK_FIRE;return;
 }
}
static inline void k3_damage(KeenActors3*w,KeenActor*a,int enemy){
 KeenActors*c=&w->core;if(a->mode==KA_DYING||a->mode==KA_CORPSE)return;
 if(a->kind==12||a->kind==13){if(enemy)return;
  if(a->kind==12){w->sparks++;if(w->sparks==6){w->arm_destruct=1;w->arm_row=6;w->arm_time=0;for(unsigned i=0;i<w->body_count;i++)if(w->bodies[i].kind==16)w->bodies[i].kind=0;}}
  else{w->destruct=1;w->row=2;w->time=0;for(unsigned i=0;i<w->body_count;i++)if(w->bodies[i].kind>=17)w->bodies[i].kind=0;for(unsigned i=0;i<c->count;i++)c->actors[i].kind=0;}
  a->kind=0;c->shot_killed=1;c->events|=K3_SPARK;return;
 }
 if(a->kind==7||a->kind==8||a->kind==9||a->kind==10)return;
 if(enemy&&(a->kind==3||a->kind==4||a->kind==5))return;
 if(a->kind==5){if(a->health--!=0)return;}else if(a->kind!=6&&a->health&&--a->health)return;
 a->frame=a->kind==1||a->kind==11?75:a->kind==2?55:a->kind==3?91:a->kind==4?124:a->kind==5?83:99;
 a->mode=KA_DYING;a->time=0;a->death_left=a->kind==6?3u:2u;a->vx=0;c->shot_killed=1;c->events|=a->kind==6?K3_FOOB:KA_VORT_DIE;
}
static inline unsigned keen3_actors_shot(KeenActors3*w,int x,int y){
 KeenActors*c=&w->core;c->shot_killed=0;
 for(unsigned i=0;i<c->count;i++){
  KeenActor*a=&c->actors[i];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE)continue;
  KeenActorBox b=k3_box(w,a->frame);int ax=kv_floor(a->x),ay=kv_floor(a->y);
  if(ka_overlap(x,y,x+15,y+7,ax+b.l,ay+b.t,ax+b.r,ay+b.b)){unsigned kind=a->kind;k3_damage(w,a,0);return kind;}
 }
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot*s=&c->shots[i];if(s->state!=1)continue;
  KeenActorBox b=k3_box(w,k3_shot_frame(s,c->ticks));int sx=kv_floor(s->x),sy=kv_floor(s->y);
  if(ka_overlap(x,y,x+15,y+7,sx+b.l,sy+b.t,sx+b.r,sy+b.b)){if(s->kind==2){s->state=2;s->age=0;}return 19;}
 }
 return 0;
}
static inline void k3_frame(KeenActors3*w,KeenActor*a,int32_t kx,int32_t ky,int cx,int cy,KeenSolid f,void*u){
 KeenActors*c=&w->core;unsigned mode=a->mode;int dx=0,dy=0;
 if(mode==KA_DYING){a->time+=6;if(a->time>40){a->time-=40;if(a->death_left>1){a->death_left--;a->frame++;}else a->mode=KA_CORPSE;}dy=k2_gravity(&a->vy);}
 else if(mode==KA_CORPSE)dy=k2_gravity(&a->vy);
 else if(a->kind==12){a->frame=114+((c->ticks>>3)&3u);return;}
 else if(a->kind==13){a->frame=146+(((c->ticks&255u)/24u)&1u);return;}
 else if(a->kind==7||a->kind==8){if(a->kind==8)a->frame=110+((c->ticks>>3)&3u);dx=a->vx*6;dy=a->vy*6;}
 else if(a->kind==9||a->kind==10){
  if(mode==KA_TURN){a->time+=6;if(a->time>75){a->vx=a->saved_vx;a->vy=(int)a->edge_jump;a->mode=KA_WALK;}return;}
  a->frame=107+((c->ticks>>5)&1u);dx=a->vx*6;dy=a->vy*6;
 }else if(a->kind==5){
  if(mode!=KA_JUMP){a->vx=a->x<kx?250:-250;a->frame=(a->vx>0?79u:77u)+((c->ticks>>5)&1u);
   if(ky-a->y>20480||ky-a->y< -20480||ka_random(c)>=18)return;
   a->vy=-(int)ka_jump(c,350);a->mode=KA_JUMP;return;}
  a->frame=a->vx>0?82:81;dx=a->vx*6;dy=k2_gravity(&a->vy);
 }else if(a->kind==3||a->kind==4){
  int mom=a->kind==3;
  if(mode==KA_SHOOT){a->frame=mom?(a->saved_vx<0?89:90):(a->saved_vx<0?123:122);a->time+=6;
   if(a->time>=(mom?30u:60u)&&!a->shot_once){a->shot_once=1;
    k3_shoot(w,a->x+(!mom&&a->saved_vx>0?4096:0),a->y+(mom?3072:1024),a->saved_vx<0?-(mom?150:200):(mom?150:200),0,mom?2:3,f,u);}
   if(a->time>(mom?50u:80u)){a->vx=a->saved_vx;a->mode=KA_WALK;}return;}
  a->frame=(mom?(a->vx>0?87u:85u):(a->vx>0?118u:120u))+((c->ticks>>4)&1u);
  if(ka_random(c)<6){a->saved_vx=mom?-a->vx:a->vx;a->vx=0;a->time=a->shot_once=0;a->mode=KA_SHOOT;}
  dx=a->vx*6;dy=k2_gravity(&a->vy);
 }else if(a->kind==6){
  if(mode==K3_SCARED){a->frame=97+((c->ticks/10)&1u);a->time+=6;if(a->time>24){a->vx=a->x>kx?250:-250;a->mode=K3_RUN;}return;}
  a->frame=(a->vx>0?95u:93u)+((c->ticks>>(mode==K3_RUN?3:5))&1u);
  int x=kv_floor(a->x)/16,y=kv_floor(a->y)/16;
  if(mode==KA_WALK&&x>cx/16&&y>cy/16&&x<cx/16+21&&y<cy/16+13&&ka_random(c)<12){a->mode=K3_SCARED;a->time=0;c->events|=K3_FOOB;}
  dx=a->vx*6;dy=k2_gravity(&a->vy);
 }else if(a->kind==2){
  a->frame=(a->vx>0?51u:47u)+(mode==KA_JUMP?3u:(c->ticks>>4)&3u);
  if(mode==KA_WALK&&ka_random(c)<18){a->vy=-(int)ka_jump(c,400);a->mode=KA_JUMP;}
  dx=a->vx*6;dy=k2_gravity(&a->vy);
 }else{
  if(mode==KA_LOOK){a->vx=0;a->frame=71+((c->ticks>>5)&3u);a->time+=6;if(a->time>=80){a->vx=a->x>kx?-90:90;a->mode=KA_WALK;}}
  else if(mode==KA_JUMP)a->frame=a->vx>0?73:74;
  else{a->frame=(a->vx>0?67u:63u)+((c->ticks>>4)&3u);if(ka_random(c)<12){a->vy=-(int)ka_jump(c,300);a->mode=KA_JUMP;}else if(ka_random(c)<12){a->vx=a->vx>0?(a->vx==90?120:90):(a->vx==-90?-120:-90);}}
  dx=a->vx*6;dy=k2_gravity(&a->vy);
 }
 int32_t old_y=a->y;unsigned hit=ka_move(a,k3_box(w,a->frame),dx,dy,f,u);
 if(mode==KA_DYING||mode==KA_CORPSE)return;
 if(a->kind==7||a->kind==8){if(hit&1)a->vx=400;if(hit&4)a->vx=-400;if(hit&2)a->vy=-400;if(hit&8)a->vy=400;return;}
 if(a->kind==9||a->kind==10){if(hit&5){a->saved_vx=hit&1?75:-75;a->vx=0;a->edge_jump=a->time=0;a->mode=KA_TURN;}
  if(hit&8){a->edge_jump=75;a->vy=a->saved_vx=0;a->time=0;a->mode=KA_TURN;}else if(hit&2)a->vy=-75;return;}
 if(a->kind==5){if(hit&2)a->mode=KA_WALK;return;}
 int speed=a->kind==2?250:a->kind==3?50:a->kind==4?65:a->kind==6?(mode==K3_RUN?-1:50):90;
 if(hit&1)a->vx=speed;
 if(hit&4)a->vx=-speed;
 if(a->kind==3&&!(hit&2)){a->y=old_y;a->vx=-a->vx;a->x+=a->vx*2;}
 if(a->kind==4&&!(hit&2)){a->saved_vx=a->vx;a->vx=0;a->mode=KA_SHOOT;a->time=a->shot_once=0;}
 if(a->kind==1||a->kind==2||a->kind==11){if(mode==KA_JUMP&&(hit&2)){a->mode=a->kind==2?KA_WALK:KA_LOOK;a->time=0;}else if(!(hit&2))a->mode=KA_JUMP;}
}
static inline void keen3_actors_step(KeenActors3*w,unsigned ticks,int32_t kx,int32_t ky,int cx,int cy,KeenSolid f,void*u){
 KeenActors*c=&w->core;if(!c->ready)return;if(ticks>64)ticks=64;
 for(unsigned i=0;i<c->count;i++){w->old_x[i]=c->actors[i].x;w->old_y[i]=c->actors[i].y;}c->pending+=ticks;
 while(c->pending>=6){c->pending-=6;c->ticks+=6;
  for(unsigned i=0;i<KA_BULLETS;i++){
   KeenEnemyShot*s=&c->shots[i];if(!s->state)continue;
   if(s->state!=1){s->age+=6;if(s->age>=20)s->state=0;continue;}
   int x=kv_floor(s->x),y=kv_floor(s->y);if(x<cx-160||x>cx+480||y<cy-128||y>cy+368){s->state=0;continue;}
   if(s->kind==3)s->x+=s->vx*6;
   else{KeenActor shot;memset(&shot,0,sizeof(shot));shot.x=s->x;shot.y=s->y;shot.vx=s->vx;shot.vy=s->vy;
    unsigned hit=ka_move(&shot,k3_box(w,k3_shot_frame(s,c->ticks)),s->vx*6,s->vy*6,f,u);s->x=shot.x;s->y=shot.y;
    if(hit){s->state=ka_random(c)>128?2:3;s->age=0;c->events|=KA_SHOT_HIT;}}
  }
  for(unsigned i=0;i<c->count;i++){
   KeenActor*a=&c->actors[i];if(!a->kind)continue;int x=kv_floor(a->x)/16,y=kv_floor(a->y)/16;
   if(!a->active){if(!(x>cx/16-2&&x<cx/16+23&&y>cy/16-2&&y<cy/16+15))continue;a->active=1;}
   else if(a->x<0||a->x>(int32_t)c->width*4096||a->y>(int32_t)c->height*4096){a->kind=0;continue;}
   else if(x<cx/16-8||x>cx/16+28||y<cy/16-8||y>cy/16+21){a->active=0;if(a->mode==KA_DYING||a->mode==KA_CORPSE)a->kind=0;continue;}
   if(a->y<0)a->y=0;
   k3_frame(w,a,kx,ky,cx,cy,f,u);
  }
  for(unsigned i=0;i<w->body_count;i++){KeenBody3*b=&w->bodies[i];if(b->kind!=14&&b->kind!=15)continue;b->time+=6;
   if(b->time>=400){b->time-=400;k3_shoot(w,b->x*4096+(b->kind==15?1024:0),b->y*4096+(b->kind==14?768:0),b->kind==14?400:0,b->kind==15?400:0,1,f,u);}}
  for(unsigned i=0;i<KA_BULLETS;i++){
   KeenEnemyShot*s=&c->shots[i];if(s->state!=1||s->kind==3)continue;KeenActorBox sb=k3_box(w,k3_shot_frame(s,c->ticks));
   for(unsigned j=0;j<c->count;j++){KeenActor*a=&c->actors[j];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE||(s->kind==2&&a->kind==3))continue;
    KeenActorBox b=k3_box(w,a->frame);int x=kv_floor(a->x),y=kv_floor(a->y),sx=kv_floor(s->x),sy=kv_floor(s->y);
    if(!ka_overlap(sx+sb.l,sy+sb.t,sx+sb.r,sy+sb.b,x+b.l,y+b.t,x+b.r,y+b.b))continue;
    k3_damage(w,a,1);if(s->kind==1)continue;s->state=2;s->age=0;c->events|=KA_SHOT_HIT;break;}
  }
 }
}
static inline unsigned keen3_actors_player(KeenActors3*w,KeenMotion*p,KeenVortMotion*m,int32_t old_fy,int invincible,KeenSolid f,void*u){
 KeenActors*c=&w->core;unsigned events=0;
 for(unsigned i=0;i<c->count;i++){
  KeenActor*a=&c->actors[i];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE)continue;KeenActorBox b=k3_box(w,a->frame);
  if(!ka_overlap(m->fx,m->fy,m->fx+2048,m->fy+5888,a->x+b.l*256,a->y+b.t*256,a->x+b.r*256,a->y+b.b*256))continue;
  if(a->kind==6){k3_damage(w,a,1);continue;}
  if(a->kind==9||a->kind==10||a->kind==7){
   int32_t dx=a->x-w->old_x[i],dy=a->y-w->old_y[i],kx=m->fx-1024,ky=m->fy-256;
   int32_t kl=m->fx,kr=m->fx+2048,kt=m->fy,kb=m->fy+5888,al=a->x+b.l*256,ar=a->x+b.r*256,at=a->y+b.t*256,ab=a->y+b.b*256;
   int16_t ox=(int16_t)(a->x<kx?((ar&65535)-(kl&65535)+1):((kr&65535)-(al&65535)+1));
   int16_t oy=(int16_t)(a->y<ky?((ab&65535)-(kt&65535)+1):((kb&65535)-(at&65535)+1));
   int16_t delta=(int16_t)(a->y<ky?dy-(m->fy-old_fy)+2:(m->fy-old_fy)-dy+2);int mx=0,my=0,top=0;
   if(delta<oy){mx=a->x>kx?-ox:ox;if((a->x>kx&&a->vx<m->vx)||(a->x<=kx&&a->vx>m->vx))m->vx=a->vx;}
   else if(a->y>ky){mx=(int)dx;my=-oy-128;top=1;if(m->mode==KV_AIR)m->mode=KV_GROUND;else if(m->mode==KV_POGO_AIR){m->mode=KV_POGO_GROUND;m->time=0;m->saved=m->vx;m->vx=0;}m->vy=a->vy<0?a->vy/2:a->vy;}
   else{my=oy;if(a->vy>m->vy)m->vy=a->vy;}(void)kv_move(m,p,mx,my,f,u);if(top)p->grounded=1;events|=K2_CARRY;
  }else if(a->kind==3||a->kind==4){int32_t kx=m->fx-1024;int overlap=a->x>kx?(m->fx+2048)-(a->x+b.l*256)+1:(a->x+b.r*256)-m->fx+1;
   (void)kv_move(m,p,a->x>kx?-overlap:overlap,0,f,u);}
  else if(!invincible&&a->kind==2){w->stun_vx=a->vx;w->stun_vy=a->vy;events|=K2_STUN;}
  else if(!invincible&&(a->kind==1||a->kind==5||a->kind==8||a->kind==11))events|=KA_KILL_KEEN;
 }
 if(!invincible)for(unsigned i=0;i<KA_BULLETS;i++){KeenEnemyShot*s=&c->shots[i];if(s->state!=1)continue;KeenActorBox b=k3_box(w,k3_shot_frame(s,c->ticks));int x=kv_floor(s->x),y=kv_floor(s->y);
  if(ka_overlap(p->x,p->y,p->x+7,p->y+22,x+b.l,y+b.t,x+b.r,y+b.b)){if(s->kind!=3){s->state=2;s->age=0;}events|=KA_KILL_KEEN;}}
 return events;
}

typedef unsigned (*K3TileRead)(int,int,void*);
typedef void (*K3TileWrite)(int,int,unsigned,void*);
static inline void k3_destroy(KeenActors3*w,int x,int y,K3TileRead get,K3TileWrite set,void*u){
 if(get(x,y,u)==169)return;
 set(x,y,169,u);w->core.events|=KA_SHOT_HIT;
 for(unsigned i=0;i<KA_BULLETS;i++)if(!w->core.shots[i].state){w->core.shots[i]=(KeenEnemyShot){x*4096,y*4096,0,ka_random(&w->core)>128?2u:3u,0,1,0};break;}
}
static inline void keen3_bodies_step(KeenActors3*w,unsigned ticks,K3TileRead get,K3TileWrite set,void*u){
 if(ticks>64)ticks=64;
 w->body_pending+=ticks;
 while(w->body_pending>=6){w->body_pending-=6;
  if(w->arm_destruct){w->arm_time+=6;if(w->arm_time>=30){w->arm_time-=30;if(++w->arm_row==19)w->arm_destruct=0;
   else for(int x=5;x<=19;x++)if(x<=7||x>=17)k3_destroy(w,x,(int)w->arm_row,get,set,u);}}
  if(w->destruct){w->time+=6;if(w->time>=20){w->time-=20;if(++w->row==24){w->destruct=0;w->finished=1;}
   else if(w->row<19)for(int x=8;x<=16;x++)k3_destroy(w,x,(int)w->row,get,set,u);}}
  for(unsigned i=0;i<w->body_count;i++){
   KeenBody3*b=&w->bodies[i];if(b->kind<16||b->kind>18)continue;
   if(b->wait){b->wait+=6;if(b->wait>200)b->wait=0;continue;}
   b->time+=6;if(b->time<(unsigned)(b->kind==16?b->period:35))continue;b->time-=(unsigned)(b->kind==16?b->period:35);
   int x=b->x,y=b->y;
   if(b->kind==16){
    set(x,y,169,u);for(int side=-1;side<=1;side+=2)for(int row=0;row<3;row++)set(x+side,y+row,169,u);
    if(b->direction==-1){if(get(x,y-1,u)!=597){b->direction=1;b->period=15;}else b->y--;}
    else if(get(x,y+3,u)==471){b->direction=-1;b->period=40;}else{set(x,y,597,u);b->y++;}
    y=b->y;set(x,y,597,u);for(int side=-1;side<=1;side+=2){set(x+side,y,618,u);set(x+side,y+1,620,u);set(x+side,y+2,619,u);}
   }else{
    for(int d=0;d<=3;d++)set(x+b->side*d,y,169,u);
    if(b->direction==-1){if(get(x,y-1,u)!=597)b->direction=1;else b->y--;}
    else if(get(x,y+1,u)==430){b->direction=-1;b->wait=1;w->core.events|=K3_STOMP;}
    else{set(x,y,597,u);b->y++;}
    y=b->y;set(x,y,b->side==-1?622:621,u);for(int d=1;d<=2;d++)set(x+b->side*d,y,623,u);set(x+b->side*3,y,b->side==-1?621:622,u);
   }
  }
 }
}
#endif
