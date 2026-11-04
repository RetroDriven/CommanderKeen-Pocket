/* SPDX-License-Identifier: MIT
 * Keen 1 actors and chains. Reference: Chocolate Keen episode1.c, enemies.c and physics.c.
 * RNG and jump tables come from ACTORS.CK1. */
#ifndef KEEN_ACTORS_H
#define KEEN_ACTORS_H
#include <string.h>
#include "keen_ega.h"
#include "keen_sprite.h"
#include "keen_map.h"
#include "keen_vort_motion.h"
#define KA_MAX 80u
enum {KA_WALK,KA_LOOK,KA_STUN,KA_JUMP,KA_TURN,KA_SHOOT,KA_SPAWN,KA_DYING,KA_CORPSE};
#define KA_PUSH_LEFT 1u
#define KA_PUSH_RIGHT 2u
#define KA_STOMP 4u
#define KA_KILL_KEEN 8u
#define KA_TANK_FIRE 16u
#define KA_SHOT_HIT 32u
#define KA_BULLETS 32u
#define KA_CANNON_FIRE 64u
#define KA_CHUNK_SMASH 128u
#define KA_FREEZE 256u
#define KA_BULLET_CONTACT 512u
#define KA_LETHAL_ACTOR 1024u
#define KA_YORP_DIE 2048u
#define KA_GARG_DIE 4096u
#define KA_VORT_DIE 8192u
typedef struct {int l,t,r,b;} KeenActorBox;
typedef struct {int32_t x,y;int vx,vy;unsigned kind,mode,time,frame,active,edge_jump,health,death_left;int saved_vx;unsigned shot_once;} KeenActor;
typedef struct {int32_t x,y;int vx;unsigned state,age,kind;int vy;} KeenEnemyShot;
typedef struct {unsigned x,y,variant,last;} KeenCannon;
typedef struct {
 KeenActor actors[KA_MAX];KeenActorBox boxes[67];
 KeenEnemyShot shots[KA_BULLETS];unsigned events;
 KeenCannon cannons[KA_MAX];unsigned cannon_count;int freeze_vx,freeze_vy;
 uint8_t random[256];uint16_t jump[18];unsigned rnd,j1,j2,count,pending,ticks;
 unsigned width,height,unsupported,shot_killed;
 unsigned chain_active,chain_x,chain_y,chain_steps,chain_last;int ready;
} KeenActors;
static inline unsigned ka_random(KeenActors *w){w->rnd=(w->rnd+1u)&255u;return w->random[w->rnd];}
static inline unsigned ka_jump(KeenActors *w,unsigned max){
 if(!max)return 0;
 unsigned bit=max,mask=65535u;
 while(!(bit&32768u)){bit<<=1;mask>>=1;}
 uint16_t value=(uint16_t)(w->jump[w->j1]+w->jump[w->j2]+1u);
 w->jump[w->j1]=value;value=(uint16_t)(value+w->jump[0]);w->jump[0]=value;
 w->j1=w->j1==1?17:w->j1-1;w->j2=w->j2==1?17:w->j2-1;
 unsigned result=value&mask;return result>max?result>>1:result;
}
static inline int keen_actors_init(KeenActors *w,const uint8_t *map,const KeenMapInfo *mi,
 const uint8_t *head,size_t hlen,const KeenSpriteInfo *si,const uint8_t *data,size_t len,int keen_x){
 memset(w,0,sizeof(*w));
 if(!data||len!=294u||memcmp(data,"KA01",4)||!map||!mi||!si||si->sprite_count<70)return -1;
 for(unsigned f=48;f<115 && f<si->sprite_count;f++){
  KeenSprite sp;if(keen_sprite_get(head,hlen,si,f,&sp))return -1;
  const uint8_t *m=head+si->table_offset+f*128u;
  KeenActorBox *b=&w->boxes[f-48];
  b->l=keen_le16(m+8)/256;b->t=keen_le16(m+10)/256;
  b->r=keen_le16(m+12)/256;b->b=keen_le16(m+14)/256;
  if(b->l>b->r||b->t>b->b||b->r>=(int)sp.w||b->b>=(int)sp.h)return -1;
 }
 memcpy(w->random,data+4,256);
 for(unsigned i=1;i<18;i++)w->jump[i]=keen_le16(data+258+i*2u);
 w->j1=17;w->j2=5;w->width=mi->width;w->height=mi->height;
 for(unsigned y=0;y<mi->height;y++)for(unsigned x=0;x<mi->width;x++){
  unsigned id=keen_map_tile(map,mi,x,y,1);
  if(id>=6&&id<=9){
   if(w->cannon_count>=KA_MAX || si->sprite_count<114)return -1;
   w->cannons[w->cannon_count++]=(KeenCannon){x,y,id-6,0};continue;
  }
  if((id<1||id>5)&&id!=10)continue;
  if(id==10 && si->sprite_count<115)return -1;
  if(id>=4 && si->sprite_count<114)return -1;
  if(id==3 && si->sprite_count<88)return -1;
  if(w->count>=KA_MAX){w->count=0;return -1;}
  KeenActor *a=&w->actors[w->count++];a->kind=id;
  a->x=(int32_t)x*4096;a->y=(int32_t)y*4096+(id==1?2048:0);
  a->mode=id==2?KA_LOOK:id==5?KA_SPAWN:KA_WALK;
  a->frame=id==1?48:id==2?60:id==3?78:id==4?96:106;
  if(id==4||id==5){a->vx=a->x<keen_x*256?-90:90;if(id==5)a->y+=2048;}
  if(id==10){a->frame=114;a->vx=0;}
  if(id==3){a->vx=a->x>keen_x*256?-90:90;a->health=3;}
  if(id==1)a->vx=a->x<keen_x*256?60:-60;
 }
 w->ready=1;return 0;
}
static inline int ka_overlap(int l,int t,int r,int b,int ll,int tt,int rr,int bb){
 return l<=rr&&r>=ll&&t<=bb&&b>=tt;
}
static inline int ka_column(KeenSolid f,void*u,int x,int top,int bottom,int face){
 if(x<0||top<0)return 1;
 for(int y=top/16;y<=bottom/16;y++)if(f(x/16,y,face,u))return 1;
 return 0;
}
static inline int ka_row(KeenSolid f,void*u,int y,int left,int right,int face){
 if(y<0||left<0)return 1;
 for(int x=left/16;x<=right/16;x++)if(f(x,y/16,face,u))return 1;
 return 0;
}
static inline unsigned ka_move(KeenActor *a,KeenActorBox b,int dx,int dy,KeenSolid f,void*u){
 unsigned contacts=0;int x=kv_floor(a->x),y=kv_floor(a->y);
 int32_t next=a->x+dx;int target=kv_floor(next),step=dx<0?-1:1;
 while(x!=target){int edge=x+(step>0?b.r+1:b.l-1);
  if(ka_column(f,u,edge,y+b.t,y+b.b,step>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT)){
   contacts|=step>0?4u:1u;next=x*256;break;
  }x+=step;
 }a->x=next;
 next=a->y+dy;target=kv_floor(next);step=dy<0?-1:1;
 while(y!=target){int edge=y+(step>0?b.b+1:b.t-1);
  if(ka_row(f,u,edge,x+b.l,x+b.r,step>0?KEEN_FACE_TOP:KEEN_FACE_BOTTOM)){
   contacts|=step>0?2u:8u;a->vy=0;next=y*256;break;
  }y+=step;
 }a->y=next;
 if(a->vy>=0 && ka_row(f,u,y+b.b+1,x+b.l,x+b.r,KEEN_FACE_TOP)){
  contacts|=2u;a->vy=0;a->y=y*256;
 }
 return contacts;
}
static inline void ka_tank_shot(KeenActors *w,KeenActor *a){
 for(unsigned i=0;i<KA_BULLETS;i++)if(!w->shots[i].state){
  w->shots[i]=(KeenEnemyShot){a->x,a->y+1280,a->saved_vx<0?-350:350,1,0,1,0};
  w->events|=KA_TANK_FIRE;return;
 }
}
static inline void ka_robot_frame(KeenActors *w,KeenActor *a,int32_t kx,KeenSolid f,void*u){
 unsigned mode=a->mode;int32_t ox=a->x,oy=a->y;int tank=a->kind==5;
 if(mode==KA_TURN){
  a->frame=(tank?106u:96u)+((w->ticks>>(tank?4:5))&1u);a->time+=6;
  if(a->time>50){a->mode=KA_WALK;a->vx=a->saved_vx;}
  if(tank)return;
 }else if(mode==KA_SHOOT){
  a->time+=6;
  if(!a->shot_once&&a->time>80){ka_tank_shot(w,a);a->shot_once=1;}
  if(a->time>120){a->saved_vx=a->x>kx?-90:90;a->time=0;a->mode=KA_TURN;}
  return;
 }else if(mode==KA_WALK){
  a->frame=(tank?(a->vx>0?98u:102u):(a->vx>0?88u:92u))+((w->ticks>>(tank?3:5))&3u);
  if(tank&&ka_random(w)<6){a->mode=KA_SHOOT;a->time=a->shot_once=0;a->saved_vx=a->vx;a->vx=0;}
 }
 int dy=0;for(unsigned tick=0;tick<6;tick++){a->vy+=3;if(a->vy>200)a->vy=200;if(tick<5)dy+=a->vy;}
 int dx=a->vx*6;
 unsigned hit=ka_move(a,w->boxes[a->frame-48],dx,dy+a->vy*6,f,u);
 if(mode==KA_SPAWN){if(hit&2)a->mode=KA_WALK;return;}
 if(mode!=KA_WALK)return;
 if(!(hit&2)){
  a->x=ox;a->y=oy;
  (void)ka_move(a,w->boxes[a->frame-48],tank?(a->frame<102?-1080:1080):-dx*2,0,f,u);
  a->saved_vx=tank?(a->frame<102?-90:90):-a->vx;
  a->mode=KA_TURN;a->time=0;a->vx=0;
 }
 if(hit&4){a->saved_vx=-90;a->mode=KA_TURN;a->time=0;a->vx=0;}
 if(hit&1){a->saved_vx=90;a->mode=KA_TURN;a->time=0;a->vx=0;}
 if(tank){a->y=oy;a->vy=0;}
}
static inline void ka_cannons(KeenActors*w,int cx,int cy){
 unsigned phase=(w->ticks&65535u)>>7;
 for(unsigned i=0;i<w->cannon_count;i++){
  KeenCannon *c=&w->cannons[i];if(c->last==phase)continue;c->last=phase;
  if((int)c->x<=cx-4||(int)c->x>=cx+24||(int)c->y<=cy-4||(int)c->y>=cy+17)continue;
  for(unsigned j=0;j<KA_BULLETS;j++)if(!w->shots[j].state){
   w->shots[j]=(KeenEnemyShot){(int32_t)c->x*4096+(c->variant==0?4096:0),(int32_t)c->y*4096,
       c->variant==0?200:c->variant==3?-200:0,1,0,2,c->variant==2?200:-200};
   w->events|=KA_CANNON_FIRE;break;
  }
 }
}
static inline void ka_cube_chunks(KeenActors*w,int32_t x,int32_t y){
 for(unsigned n=0;n<4;n++)for(unsigned j=0;j<KA_BULLETS;j++)if(!w->shots[j].state){
  w->shots[j]=(KeenEnemyShot){x,y,n<2?300:-300,1,0,3,(n&1u)?-300:300};break;
 }
 w->events|=KA_CHUNK_SMASH;
}
static inline void ka_enemy_shots(KeenActors*w,KeenSolid f,void*u,int camera_x,int camera_y){
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot *s=&w->shots[i];if(!s->state)continue;
  if(s->state>=2){s->age+=6;if(s->age>20)s->state=0;continue;}
  if(s->kind==3){
   s->age+=6;if(s->age>60){s->state=0;continue;}
   int dy=0;for(unsigned tick=0;tick<6;tick++){s->vy+=3;if(s->vy>200)s->vy=200;if(tick<5)dy+=s->vy;}
   s->x+=s->vx*6;s->y+=dy+s->vy*6;continue;
  }
  if(s->kind==2){
   KeenActor cube;memset(&cube,0,sizeof(cube));cube.x=s->x;cube.y=s->y;cube.vy=s->vy;
   unsigned hit=ka_move(&cube,w->boxes[112-48],s->vx*6,s->vy*6,f,u);
   s->x=cube.x;s->y=cube.y;
   if(hit){int32_t x=s->x,y=s->y;s->state=0;ka_cube_chunks(w,x,y);}
   continue;
  }
  int32_t next=s->x+s->vx*6;int x=kv_floor(s->x),target=kv_floor(next),step=s->vx<0?-1:1;
  for(;;){
   int edge=x+(step>0?15:0),y=kv_floor(s->y),face=step>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT;
   if(edge<0||y<0||f(edge/16,y/16,face,u)||f(edge/16,(y+7)/16,face,u)){
    s->x=x*256;s->state=ka_random(w)>128?2u:3u;s->age=0;w->events|=KA_SHOT_HIT;break;
   }
   if(x==target){s->x=next;break;}x+=step;
  }
  int px=kv_floor(s->x),py=kv_floor(s->y);
  if(px<camera_x-128||px>camera_x+448||py<camera_y-128||py>camera_y+368)s->state=0;
 }
}

static inline void ka_frame(KeenActors *w,KeenActor *a,int32_t kx,int32_t ky,KeenSolid f,void*u){
 if(a->kind==10){if(a->mode==KA_DYING){a->time+=6;if(a->time>20)a->kind=0;}return;}
 if(a->kind>=4){ka_robot_frame(w,a,kx,f,u);return;}
 unsigned think=a->mode;
 if(think==KA_DYING){a->vx=0;a->time+=6;if(a->time>40){a->time-=40;a->frame++;
  if(a->death_left>1)a->death_left--;
  if(a->death_left<=1)a->mode=KA_CORPSE;}}
 else if(think==KA_CORPSE)a->vx=0;
 else if(think==KA_STUN){
  a->frame=56+((w->ticks>>4)&1u);a->vx=0;a->time+=6;
  if(a->time>=800){a->time=0;a->mode=KA_LOOK;}
 }else if(a->kind==3){
  if(think==KA_LOOK){
   a->vx=0;a->frame=78+((w->ticks>>5)&1u);a->time+=6;
   if(a->time>=80){a->vx=a->x>kx?-90:90;a->mode=KA_WALK;}
  }else if(think==KA_JUMP)a->frame=a->vx>0?80u:81u;
  else{
   a->frame=(a->vx>0?74u:70u)+((w->ticks>>4)&3u);
   if(ka_random(w)<12){a->vy=-(int)ka_jump(w,300);a->mode=KA_JUMP;}
   else if(ka_random(w)<12){if(a->vx==90)a->vx=120;else if(a->vx==120)a->vx=90;
    else if(a->vx==-90)a->vx=-120;else if(a->vx==-120)a->vx=-90;}
  }
 }else if(think==KA_LOOK){
  a->vx=0;a->frame=(a->kind==1?48u:60u)+((w->ticks>>5)&3u);a->time+=6;
  if(a->kind==1 && a->time>=200){a->vx=a->x<kx?60:-60;a->mode=KA_WALK;}
 }else{
  if(!a->vy){
   if(a->kind==1){if(ka_random(w)<6)a->vy=-(int)ka_jump(w,128);else if(ka_random(w)<6){a->mode=KA_LOOK;a->time=0;}}
   else if(a->vx> -220&&a->vx<220&&ka_random(w)<6){a->mode=KA_LOOK;a->time=0;}
  }
  a->frame=(a->kind==1?(a->vx>0?52u:54u):(a->vx>0?64u:66u))+((w->ticks>>4)&1u);
 }
 int dy=0;
 for(unsigned tick=0;tick<6;tick++){a->vy+=3;if(a->vy>200)a->vy=200;if(a->vy< -400)a->vy=-400;if(tick<5)dy+=a->vy;}
 unsigned hit=ka_move(a,w->boxes[a->frame-48],a->vx*6,dy+a->vy*6,f,u);
 if(a->kind==3 && think<KA_DYING){
  if(hit&4)a->vx=-90;
  if(hit&1)a->vx=90;
  if(think==KA_JUMP && (hit&2)){a->mode=KA_LOOK;a->time=0;}
 }else if(think==KA_WALK){
  if(hit&4)a->vx=-60;
  if(hit&1)a->vx=60;
  if(a->kind==2){
   if(hit&2)a->edge_jump=0;
   else if((a->vx==220||a->vx== -220)&&!a->edge_jump){a->edge_jump=1;a->vy=-200;}
  }
 }else if(think==KA_LOOK && a->kind==2 && a->time>=80){
  a->vx=a->kind==2 && a->y+2048==ky?220:60;
  if(a->x>kx)a->vx=-a->vx;
  a->mode=KA_WALK;
 }
}
static inline void ka_enemy_contacts(KeenActors*w);
static inline void ka_enemy_impact(KeenActors*w,KeenEnemyShot*s);
static inline void keen_actors_step(KeenActors *w,unsigned ticks,int32_t kx,int32_t ky,
 int camera_x,int camera_y,KeenSolid f,void*u){
 if(!w->ready)return;
 if(ticks>64)ticks=64;
 w->pending+=ticks;
 while(w->pending>=6){
  w->pending-=6;w->ticks+=6;
  if(w->chain_active && (w->ticks>>5)!=w->chain_last){
   w->chain_last=w->ticks>>5;if(++w->chain_steps>=4)w->chain_active=0;
  }
  ka_cannons(w,camera_x/16,camera_y/16);ka_enemy_shots(w,f,u,camera_x,camera_y);int cx=camera_x/16,cy=camera_y/16;
  for(unsigned i=0;i<w->count;i++){
   KeenActor *a=&w->actors[i];if(!a->kind)continue;
   int x=kv_floor(a->x)/16,y=kv_floor(a->y)/16;
   if(!a->active){
    if(!(x>cx-2&&x<cx+23&&y>cy-2&&y<cy+15))continue;
    a->active=1;if(a->mode==KA_STUN){a->mode=KA_LOOK;a->time=0;}
   }else if(a->x<0||a->x>((int)w->width-2)*4096||a->y>(int)w->height*4096){a->kind=0;continue;}
   else if(x<cx-8||x>cx+28||y<cy-8||y>cy+21){a->active=0;if(a->mode>=KA_DYING)a->kind=0;continue;}
   if(a->y<0)a->y=0;
   ka_frame(w,a,kx,ky,f,u);
  }
  ka_enemy_contacts(w);
 }
}
/* Actor contact callback: enemy shots do not reset butlers or shoot chains. */
static inline void ka_damage_actor(KeenActors *w,KeenActor *a,int enemy){
 if(a->mode>=KA_DYING)return;
  if(a->kind==10){
   if(enemy)return;
   a->mode=KA_DYING;a->time=0;a->frame=ka_random(w)>128?110:111;
   w->chain_active=1;w->chain_x=(unsigned)(kv_floor(a->x)/16);w->chain_y=(unsigned)(kv_floor(a->y)/16)+1;
   w->chain_steps=w->chain_last=0;
   for(unsigned j=w->count;j>0;j--){KeenActor *victim=&w->actors[j-1];
    if(victim->kind==3 && victim->mode<KA_DYING){victim->mode=KA_DYING;victim->time=0;victim->frame=82;victim->death_left=6;break;}}
   w->shot_killed=1;return;
  }
  if(a->kind>=4){if(a->kind==4 && !enemy)a->time=0;return;}
  /* Keen 1 tests pre-decrement health: ordinary Vorticons take four shots. */
  if(a->kind==3 && a->health){a->health--;return;}
  w->shot_killed=1;a->mode=KA_DYING;a->time=0;
  if(a->kind!=3)a->vy=-80;
  a->frame=a->kind==1?58:a->kind==2?68:82;a->death_left=a->kind==3?6:2;
  if(enemy)w->events|=a->kind==1?KA_YORP_DIE:a->kind==2?KA_GARG_DIE:KA_VORT_DIE;
}
static inline void ka_enemy_impact(KeenActors*w,KeenEnemyShot*s){
 s->state=ka_random(w)>128?2u:3u;s->age=0;w->events|=KA_SHOT_HIT;
}
static inline void ka_enemy_contacts(KeenActors*w){
 /* Original level_detect_collisions visits level actors before spawned shots. */
 for(unsigned i=0;i<w->count;i++){
  KeenActor *a=&w->actors[i];if(!a->kind||!a->active||a->mode>=KA_DYING||a->kind==5)continue;
  KeenActorBox b=w->boxes[a->frame-48];int ax=kv_floor(a->x),ay=kv_floor(a->y);
  for(unsigned j=0;j<KA_BULLETS;j++){
   KeenEnemyShot *s=&w->shots[j];if(s->state!=1||s->kind!=1)continue;
   int x=kv_floor(s->x),y=kv_floor(s->y);
   if(!ka_overlap(x,y,x+15,y+7,ax+b.l,ay+b.t,ax+b.r,ay+b.b))continue;
   ka_damage_actor(w,a,1);ka_enemy_impact(w,s);
  }
 }
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot *s=&w->shots[i];if(s->state!=1||s->kind!=1)continue;
  int x=kv_floor(s->x),y=kv_floor(s->y);
  for(unsigned j=0;j<KA_BULLETS;j++){
   KeenEnemyShot *other=&w->shots[j];if(i==j||other->state!=1)continue;
   KeenActorBox b=other->kind==2?w->boxes[112-48]:other->kind==3?w->boxes[113-48]:(KeenActorBox){0,0,15,7};
   int ox=kv_floor(other->x),oy=kv_floor(other->y);
   if(!ka_overlap(x,y,x+15,y+7,ox+b.l,oy+b.t,ox+b.r,oy+b.b))continue;
   ka_enemy_impact(w,s);if(other->kind==1)ka_enemy_impact(w,other);break;
  }
 }
}
/* Returns original actor kind when a live actor is struck, once per shot. */
static inline unsigned keen_actors_shot(KeenActors *w,int x,int y){
 w->shot_killed=0;
 for(unsigned i=0;i<w->count;i++){
  KeenActor *a=&w->actors[i];if(!a->kind||!a->active||a->mode>=KA_DYING)continue;
  KeenActorBox b=w->boxes[a->frame-48];int ax=kv_floor(a->x),ay=kv_floor(a->y);
  if(!ka_overlap(x,y,x+15,y+7,ax+b.l,ay+b.t,ax+b.r,ay+b.b))continue;
  ka_damage_actor(w,a,0);return a->kind;
 }
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot *shot=&w->shots[i];if(shot->state!=1)continue;
  KeenActorBox b=shot->kind==2?w->boxes[112-48]:shot->kind==3?w->boxes[113-48]:(KeenActorBox){0,0,15,7};
  int sx=kv_floor(shot->x),sy=kv_floor(shot->y);
  if(!ka_overlap(x,y,x+15,y+7,sx+b.l,sy+b.t,sx+b.r,sy+b.b))continue;
  if(shot->kind==1)ka_enemy_impact(w,shot);
  return shot->kind==1?11u:shot->kind==2?12u:13u;
 }
 return 0;
}
static inline unsigned keen_actors_player_box(KeenActors *w,const KeenMotion *p,int32_t origin_y,int top_extra){
 unsigned events=0;
 for(unsigned i=0;i<KA_BULLETS;i++){
  KeenEnemyShot *s=&w->shots[i];int x=kv_floor(s->x),y=kv_floor(s->y);
  if(s->state!=1 || s->kind==3)continue;
  KeenActorBox box=s->kind==2?w->boxes[112-48]:(KeenActorBox){0,0,15,7};
  if(ka_overlap(p->x,p->y-top_extra,p->x+7,p->y+22,x+box.l,y+box.t,x+box.r,y+box.b)){
   if(s->kind==2){events|=KA_FREEZE;w->freeze_vx=s->vx;w->freeze_vy=s->vy;s->state=0;}
   else{events|=KA_KILL_KEEN|KA_BULLET_CONTACT;ka_enemy_impact(w,s);}
  }
 }
 for(unsigned i=0;i<w->count;i++){
  KeenActor *a=&w->actors[i];if(!a->kind||!a->active||a->mode>=KA_DYING)continue;
  KeenActorBox b=w->boxes[a->frame-48];int ax=kv_floor(a->x),ay=kv_floor(a->y);
  if(!ka_overlap(p->x,p->y-top_extra,p->x+7,p->y+22,ax+b.l,ay+b.t,ax+b.r,ay+b.b))continue;
  if(a->kind==10)continue;
  if(a->kind==2 || a->kind==3){events|=KA_KILL_KEEN|KA_LETHAL_ACTOR;continue;}
  if(a->kind>=4){events|=a->vx>0?KA_PUSH_RIGHT:KA_PUSH_LEFT;continue;}
  if(a->mode==KA_STUN)continue;
  if(p->vy>0 && origin_y+2048<=a->y){a->mode=KA_STUN;a->time=0;events|=KA_STOMP;}
  else events|=a->vx>0?KA_PUSH_RIGHT:KA_PUSH_LEFT;
 }return events;
}
static inline unsigned keen_actors_player(KeenActors *w,const KeenMotion*p,int32_t origin_y){
 return keen_actors_player_box(w,p,origin_y,0);
}
#endif
