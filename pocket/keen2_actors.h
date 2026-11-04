/* SPDX-License-Identifier: MIT
 * Keen 2 actors. Reference: Chocolate Keen episode2.c and game/enemies.c. */
#ifndef KEEN2_ACTORS_H
#define KEEN2_ACTORS_H
#include "keen_actors.h"
#define K2_TARGET 16384u
#define K2_STUN 32768u
#define K2_CARRY 65536u
enum {K2_SCRUB_LEFT=9,K2_SCRUB_DOWN,K2_SCRUB_RIGHT,K2_SCRUB_UP,K2_SCRUB_FALL};
typedef struct {
 KeenActors core;KeenActorBox boxes[86];
 unsigned targets,level,lights;int stun_vx,stun_vy,carry_dx,carry_dy;
 unsigned blast_active,blast_x,blast_y,blast_phase,blast_time;
 int32_t old_x[KA_MAX],old_y[KA_MAX];
} KeenActors2;
static inline unsigned keen2_city_bit(unsigned level){
 static const unsigned levels[]={4,6,7,9,11,13,15,16};
 for(unsigned i=0;i<8;i++)if(level==levels[i])return 1u<<i;
 return 0;
}
static inline KeenActorBox k2_box(const KeenActors2*w,unsigned frame){
 return frame>=48&&frame<134?w->boxes[frame-48]:(KeenActorBox){0,0,15,15};
}
static inline int keen2_actors_init(KeenActors2*w,const uint8_t*map,const KeenMapInfo*mi,
 const uint8_t*head,size_t hlen,const KeenSpriteInfo*si,const uint8_t*data,size_t len,int keen_x,unsigned level){
 memset(w,0,sizeof(*w));
 if(!data||len!=294||memcmp(data,"KA02",4)||!map||!mi||!si||si->sprite_count<134)return -1;
 KeenActors*c=&w->core;memcpy(c->random,data+4,256);
 for(unsigned i=1;i<18;i++)c->jump[i]=keen_le16(data+258+i*2);
 c->j1=17;c->j2=5;c->width=mi->width;c->height=mi->height;w->level=level;w->lights=1;
 for(unsigned frame=48;frame<134;frame++){
  KeenSprite sp;if(keen_sprite_get(head,hlen,si,frame,&sp))return -1;
  const uint8_t*m=head+si->table_offset+frame*128u;KeenActorBox*b=&w->boxes[frame-48];
  *b=(KeenActorBox){keen_le16(m+8)/256,keen_le16(m+10)/256,keen_le16(m+12)/256,keen_le16(m+14)/256};
  if(b->l>b->r||b->t>b->b||b->r>=(int)sp.w||b->b>=(int)sp.h)return -1;
 }
 /* Shared enemy-shot wall probes use no episode-specific frame metadata. */
 for(unsigned y=0;y<mi->height;y++)for(unsigned x=0;x<mi->width;x++){
  unsigned id=keen_map_tile(map,mi,x,y,1);if(id<1||id>7)continue;
  if(c->count>=KA_MAX)return -1;
  KeenActor*a=&c->actors[c->count++];a->kind=id;a->x=(int32_t)x*4096;a->y=(int32_t)y*4096;
  a->mode=KA_WALK;a->vx=a->x<keen_x*256?90:-90;
  switch(id){
   case 1:a->frame=82;a->health=1;break;
   case 2:a->frame=48;a->health=1;a->vx=a->x<keen_x*256?250:-250;break;
   case 3:a->frame=88;a->health=2;a->vx=a->x<keen_x*256?100:-100;break;
   case 4:a->frame=102;a->vx=0;a->mode=K2_SCRUB_LEFT;break;
   case 5:a->frame=112;a->vx=100;a->health=99;break;
   case 6:a->frame=126;a->vx=75;a->y-=1024;break;
   case 7:a->frame=128;a->vx=0;break;
  }
 }
 c->ready=1;return 0;
}
static inline void k2_shoot(KeenActors2*w,KeenActor*a){
 KeenActors*c=&w->core;
 for(unsigned i=0;i<KA_BULLETS;i++)if(!c->shots[i].state){
  int offset=a->kind==3?-256:-1024;
  c->shots[i]=(KeenEnemyShot){a->x,a->y+offset+2304,a->vx>0?350:-350,1,0,1,0};
  c->events|=KA_TANK_FIRE;return;
 }
}
static inline void k2_damage(KeenActors2*w,KeenActor*a,int enemy){
 KeenActors*c=&w->core;if(a->mode==KA_DYING||a->mode==KA_CORPSE)return;
 if(a->kind==7){
  w->targets|=keen2_city_bit(w->level);c->events|=K2_TARGET;
  if(enemy)return;
  w->blast_active=1;w->blast_x=(unsigned)(a->x/4096);w->blast_y=(unsigned)(a->y/4096);
  w->blast_phase=w->blast_time=0;a->kind=0;c->shot_killed=1;return;
 }
 if(a->kind==5||a->kind==6)return;
 if(a->kind==3){if(enemy)return;if(a->health--!=0)return;}
 else if(a->kind==1||a->kind==2){if(a->health&&--a->health)return;}
 else if(a->kind!=4)return;
 a->mode=KA_DYING;a->time=0;a->vx=0;a->death_left=2;
 a->frame=a->kind==1?86:a->kind==2?56:a->kind==3?100:110;c->shot_killed=1;
 if(a->kind!=4)c->events|=KA_VORT_DIE;
}
static inline unsigned keen2_actors_shot(KeenActors2*w,int x,int y){
 KeenActors*c=&w->core;c->shot_killed=0;
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot*s=&c->shots[i];if(s->state!=1)continue;
  if(ka_overlap(x,y,x+15,y+7,kv_floor(s->x),kv_floor(s->y),kv_floor(s->x)+15,kv_floor(s->y)+7)){
   s->state=ka_random(c)>128?2:3;s->age=0;return 8;
  }
 }
 for(unsigned i=0;i<c->count;i++){
  KeenActor*a=&c->actors[i];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE)continue;
  KeenActorBox b=k2_box(w,a->frame);int ax=kv_floor(a->x),ay=kv_floor(a->y);
  if(ka_overlap(x,y,x+15,y+7,ax+b.l,ay+b.t,ax+b.r,ay+b.b)){unsigned kind=a->kind;k2_damage(w,a,0);return kind;}
 }
 return 0;
}
static inline int k2_gravity(int*vy){
 int delta=0;for(unsigned tick=0;tick<6;tick++){*vy+=3;if(*vy>200)*vy=200;if(tick<5)delta+=*vy;}return delta+*vy*6;
}
static inline void k2_scrub(KeenActors2*w,KeenActor*a,KeenSolid f,void*u){
 unsigned mode=a->mode;int dx=0,dy=0;
 if(mode==K2_SCRUB_LEFT){a->vx=-80;a->vy=80;a->frame=102;dx=-480;dy=480+(!a->time?1024:0);}
 else if(mode==K2_SCRUB_DOWN){a->vx=80;a->vy=80;a->frame=108;dx=480+(!a->time?1024:0);dy=480;}
 else if(mode==K2_SCRUB_RIGHT){a->vx=80;a->vy=-80;a->frame=106;dx=480;dy=-480-(!a->time?1024:0);}
 else if(mode==K2_SCRUB_UP){a->vx=-80;a->vy=-80;a->frame=104;dx=-480-(!a->time?1024:0);dy=-480;}
 else{a->vx=0;a->frame=102;dy=k2_gravity(&a->vy);}
 a->frame+=(w->core.ticks>>5)&1u;
 unsigned hit=ka_move(a,k2_box(w,a->frame),dx,dy,f,u),next=mode;
 if(mode==K2_SCRUB_LEFT){if(hit&1)next=K2_SCRUB_UP;else if(!(hit&2)){next=a->time?K2_SCRUB_DOWN:K2_SCRUB_FALL;if(a->time)a->y+=256;}}
 else if(mode==K2_SCRUB_DOWN){if(hit&2)next=K2_SCRUB_LEFT;else if(!(hit&4)){next=a->time?K2_SCRUB_RIGHT:K2_SCRUB_FALL;if(a->time)a->x+=256;}}
 else if(mode==K2_SCRUB_RIGHT){if(hit&4)next=K2_SCRUB_DOWN;else if(!(hit&8)){next=a->time?K2_SCRUB_UP:K2_SCRUB_FALL;if(a->time)a->y-=256;}}
 else if(mode==K2_SCRUB_UP){if(hit&8)next=K2_SCRUB_RIGHT;else if(!(hit&1)){next=a->time?K2_SCRUB_LEFT:K2_SCRUB_FALL;if(a->time)a->x-=256;}}
 else if(hit&2)next=K2_SCRUB_LEFT;
 a->time=next==mode?1:0;a->mode=next;
}
static inline void k2_frame(KeenActors2*w,KeenActor*a,int32_t kx,int32_t ky,KeenSolid f,void*u){
 KeenActors*c=&w->core;unsigned mode=a->mode;
 if(mode==KA_DYING){a->vx=0;a->time+=6;if(a->time>40){a->time-=40;a->frame++;a->mode=KA_CORPSE;}}
 else if(mode==KA_CORPSE)a->vx=0;
 else if(a->kind==4){k2_scrub(w,a,f,u);return;}
 else if(a->kind==7){a->frame=128+((c->ticks>>3)&3u);return;}
 else if(a->kind==6){
  if(mode==KA_TURN){a->time+=6;if(a->time>75){a->vx=a->saved_vx;a->mode=KA_WALK;}return;}
  a->frame=126+((c->ticks>>5)&1u);
  unsigned hit=ka_move(a,k2_box(w,a->frame),a->vx*6,0,f,u);
  if(hit&5){a->saved_vx=hit&1?75:-75;a->vx=0;a->time=0;a->mode=KA_TURN;}return;
 }else if(a->kind==5){
  if(mode==KA_TURN){a->frame=120+((c->ticks>>4)&1u);a->time+=6;if(a->time>50){a->vx=a->saved_vx;a->mode=KA_WALK;}return;}
  a->frame=(a->vx>0?112:116)+((c->ticks>>4)&3u);
  if(mode==KA_SHOOT){a->time+=6;if(a->time>=50){if(a->time>150)a->mode=KA_WALK;a->shot_once+=6;if(a->shot_once>20){a->shot_once=0;k2_shoot(w,a);}}return;}
  if(ka_random(c)<6){a->mode=KA_SHOOT;a->time=a->shot_once=0;}
  unsigned hit=ka_move(a,k2_box(w,a->frame),a->vx*6,0,f,u);
  if(hit&5){a->saved_vx=hit&1?100:-100;a->time=0;a->mode=KA_TURN;}return;
 }else if(a->kind==2){
  a->frame=(a->vx>0?52:48)+(mode==KA_JUMP?3u:(c->ticks>>4)&3u);
  if(mode==KA_WALK&&ka_random(c)<18){a->vy=-(int)ka_jump(c,400);a->mode=KA_JUMP;}
 }else if(a->kind==3){
  if(mode==KA_SHOOT){a->frame=a->vx>0?97:96;a->time+=6;
   if(a->time>=30){if(!a->shot_once){a->shot_once=1;k2_shoot(w,a);}if(a->time>50)a->mode=KA_WALK;}return;}
  if(mode==KA_JUMP)a->frame=a->vx>0?98:99;
  else{a->frame=(a->vx>0?92:88)+((c->ticks>>4)&3u);
   if(ka_random(c)<12&&w->lights){a->vy=-(int)ka_jump(c,300);a->mode=KA_JUMP;}
   else if(ka_random(c)<12){a->vx=a->y+2048==ky?200:100;if(kx<a->x)a->vx=-a->vx;}
   else if(ka_random(c)<12){a->mode=KA_SHOOT;a->time=a->shot_once=0;}}
 }else{
  if(mode==KA_LOOK){a->vx=0;a->frame=82+((c->ticks>>5)&3u);a->time+=6;
   if(a->time>=80){a->vx=a->x>kx?-90:90;a->mode=KA_WALK;}}
  else if(mode==KA_JUMP)a->frame=a->vx>0?84:85;
  else{a->frame=(a->vx>0?78:74)+((c->ticks>>4)&3u);
   if(ka_random(c)<12&&w->lights){a->vy=-(int)ka_jump(c,300);a->mode=KA_JUMP;}
   else if(ka_random(c)<12){if(a->vx==90)a->vx=120;else if(a->vx==120)a->vx=90;else if(a->vx==-90)a->vx=-120;else if(a->vx==-120)a->vx=-90;}}
 }
 unsigned hit=ka_move(a,k2_box(w,a->frame),a->vx*6,k2_gravity(&a->vy),f,u);
 if(mode==KA_DYING||mode==KA_CORPSE)return;
 int speed=a->kind==2?250:a->kind==3?100:90;
 if(hit&4)a->vx=-speed;
 if(hit&1)a->vx=speed;
 if(mode==KA_JUMP&&(hit&2)){a->mode=a->kind==1?KA_LOOK:KA_WALK;a->time=0;}
 else if(!(hit&2))a->mode=KA_JUMP;
}
static inline void keen2_actors_step(KeenActors2*w,unsigned ticks,int32_t kx,int32_t ky,int cx,int cy,KeenSolid f,void*u){
 KeenActors*c=&w->core;if(!c->ready)return;if(ticks>64)ticks=64;
 for(unsigned i=0;i<c->count;i++){w->old_x[i]=c->actors[i].x;w->old_y[i]=c->actors[i].y;}
 c->pending+=ticks;
 while(c->pending>=6){
  c->pending-=6;c->ticks+=6;ka_enemy_shots(c,f,u,cx,cy);
  for(unsigned i=0;i<c->count;i++){
   KeenActor*a=&c->actors[i];if(!a->kind)continue;
   int x=kv_floor(a->x)/16,y=kv_floor(a->y)/16;
   if(!a->active){if(!(x>cx/16-2&&x<cx/16+23&&y>cy/16-2&&y<cy/16+15))continue;a->active=1;}
   else if(a->x<0||a->x>(int32_t)c->width*4096||a->y>(int32_t)c->height*4096){a->kind=0;continue;}
   else if(x<cx/16-8||x>cx/16+28||y<cy/16-8||y>cy/16+21){a->active=0;if(a->mode==KA_DYING||a->mode==KA_CORPSE)a->kind=0;continue;}
   if(a->y<0)a->y=0;
   k2_frame(w,a,kx,ky,f,u);
  }
  for(unsigned i=0;i<c->count;i++)if(c->actors[i].kind==7&&c->actors[i].active){
   KeenActor*spark=&c->actors[i];KeenActorBox sb=k2_box(w,spark->frame);int sx=kv_floor(spark->x),sy=kv_floor(spark->y);
   for(unsigned j=0;j<c->count;j++)if(i!=j&&c->actors[j].kind&&c->actors[j].active){
    KeenActor*a=&c->actors[j];KeenActorBox b=k2_box(w,a->frame);int x=kv_floor(a->x),y=kv_floor(a->y);
    if(ka_overlap(sx+sb.l,sy+sb.t,sx+sb.r,sy+sb.b,x+b.l,y+b.t,x+b.r,y+b.b)){w->targets|=keen2_city_bit(w->level);c->events|=K2_TARGET;}
   }
  }
  for(unsigned j=0;j<KA_BULLETS;j++){
   KeenEnemyShot*s=&c->shots[j];if(s->state!=1)continue;
   for(unsigned i=0;i<c->count;i++){
    KeenActor*a=&c->actors[i];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE)continue;
    KeenActorBox b=k2_box(w,a->frame);int ax=kv_floor(a->x),ay=kv_floor(a->y);
    if(!ka_overlap(kv_floor(s->x),kv_floor(s->y),kv_floor(s->x)+15,kv_floor(s->y)+7,ax+b.l,ay+b.t,ax+b.r,ay+b.b))continue;
    k2_damage(w,a,1);if(a->kind==3||a->kind==5)continue;
    s->state=ka_random(c)>128?2:3;s->age=0;c->events|=KA_SHOT_HIT;break;
   }
  }
 }
}
static inline unsigned keen2_actors_player(KeenActors2*w,KeenMotion*p,KeenVortMotion*m,int32_t old_fx,int32_t old_fy,KeenSolid f,void*u){
 KeenActors*c=&w->core;unsigned events=0;
 for(unsigned i=0;i<c->count;i++){
  KeenActor*a=&c->actors[i];if(!a->kind||!a->active||a->mode==KA_DYING||a->mode==KA_CORPSE)continue;
  KeenActorBox b=k2_box(w,a->frame);
  if(!ka_overlap(m->fx,m->fy,m->fx+2048,m->fy+5888,a->x+b.l*256,a->y+b.t*256,a->x+b.r*256,a->y+b.b*256))continue;
  if(a->kind==7){w->targets|=keen2_city_bit(w->level);events|=K2_TARGET;}
  else if(a->kind==2){w->stun_vx=a->vx;w->stun_vy=a->vy;events|=K2_STUN;}
  else if(a->kind==4||a->kind==6){
   /* CVort_carry_keen: compare motion against overlap in original 256
    * units/pixel, resolve its selected axis, then run ground collision. */
   int32_t dx=a->x-w->old_x[i],dy=a->y-w->old_y[i],kx=m->fx-1024,ky=m->fy-256;
   int32_t kl=m->fx,kr=m->fx+2048,kt=m->fy,kb=m->fy+5888;
   int32_t al=a->x+b.l*256,ar=a->x+b.r*256,at=a->y+b.t*256,ab=a->y+b.b*256;
   int16_t overlap_x=(int16_t)(a->x<kx?((ar&65535)-(kl&65535)+1):((kr&65535)-(al&65535)+1));
   int16_t overlap_y=(int16_t)(a->y<ky?((ab&65535)-(kt&65535)+1):((kb&65535)-(at&65535)+1));
   int16_t delta_y=(int16_t)(a->y<ky?dy-(m->fy-old_fy)+2:(m->fy-old_fy)-dy+2);
   int move_x=0,move_y=0,on_top=0;
   (void)old_fx;
   if(delta_y<overlap_y){
    move_x=a->x>kx?-overlap_x:overlap_x;
    if((a->x>kx&&a->vx<m->vx)||(a->x<=kx&&a->vx>m->vx))m->vx=a->vx;
   }else if(a->y>ky){
    move_x=(int)dx;move_y=-overlap_y-128;on_top=1;
    if(m->mode==KV_AIR)m->mode=KV_GROUND;
    else if(m->mode==KV_POGO_AIR){m->mode=KV_POGO_GROUND;m->time=0;m->saved=m->vx;m->vx=0;}
    m->vy=a->vy<0?a->vy/2:a->vy;
   }else{move_y=overlap_y;if(a->vy>m->vy)m->vy=a->vy;}
   (void)kv_move(m,p,move_x,move_y,f,u);if(on_top)p->grounded=1;
   events|=K2_CARRY;
  }else events|=KA_KILL_KEEN;
 }
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot*s=&c->shots[i];if(s->state!=1)continue;
  if(ka_overlap(p->x,p->y,p->x+7,p->y+22,kv_floor(s->x),kv_floor(s->y),kv_floor(s->x)+15,kv_floor(s->y)+7)){
   s->state=2;s->age=0;events|=KA_KILL_KEEN|KA_SHOT_HIT;
  }
 }
 return events;
}
#endif
