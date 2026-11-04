/* SPDX-License-Identifier: MIT
 * Galaxy fixed-point locomotion. Format/behavior reference: Omnispeak
 * ck_phys.c, ck_keen.c and Apogee 1.4 ACTION.CK4. Units: 16 per pixel.
 */
#ifndef RETRODRIVEN_KEEN4_MOTION_H
#define RETRODRIVEN_KEEN4_MOTION_H
#include "keen_galaxy.h"
#include <stdlib.h>
typedef struct {int x1,y1,x2,y2,xm,tx1,ty1,tx2,ty2,txm;} K4Rect;
typedef struct {const KeenGalaxyMap*map;const uint8_t*ti,*sprites;} K4Terrain;
typedef struct {int x,y,vx,vy,dir,frame,top,bottom,left,right,dx,dy,stick,clipped;K4Rect box;} K4Body;
enum {K4_STAND,K4_RUN,K4_JUMP,K4_JUMP2,K4_FALL,K4_FALL2,K4_POGO,K4_POGO2,K4_POGO3,K4_LOOKUP,K4_LOOKDOWN,K4_POLE,K4_HANG,K4_PULL,K4_DEAD,K4_DOOR_ALIGN,K4_DOOR_ENTER,K4_SHOOT,K4_USE_ALIGN,K4_USE_GEM,K4_USE_SWITCH,K4_USE_WAIT};
enum {K4_JUMP_EVENT=1,K4_LAND_EVENT=2,K4_POGO_EVENT=4,K4_HEAD_EVENT=8,K4_WALK_EVENT=16,K4_DEAD_EVENT=32,K4_DOOR_EVENT=64,K4_SHOT_EVENT=128,K4_USE_EVENT=256};
typedef struct {int x,y,jump,pogo,shoot;} K4Input;
typedef struct {K4Body body;unsigned state,timer,jump_timer,ticks,pole_time,pull_phase;int jump_latch,pogo_latch,pole_dir,z,up_slide,door_x,door_start_x,door_start_y;unsigned door_stage,door_time,invuln,events,shoot_kind,shoot_time,shoot_fired;int shoot_latch,shoot_aim,fire_x,fire_y,fire_dx,fire_dy,use_x,use_tx,use_ty;unsigned use_kind,use_time,use_committed;} Keen4Player;
/* fields: top, right, bottom, left, animation delta, misc, animation time */
static unsigned k4_info(const K4Terrain*w,unsigned tile,unsigned field){return !w||!w->ti||tile>=KG_FG_COUNT||field>6?0:w->ti[KG_BG_COUNT*2+KG_FG_COUNT*field+tile];}
static unsigned k4_tile(const K4Terrain*w,int x,int y){return !w||!w->map||x<0||y<0||x>=w->map->width||y>=w->map->height?0:w->map->planes[1][(size_t)y*w->map->width+x];}
static unsigned k4_at(const K4Terrain*w,int x,int y,unsigned field){return k4_info(w,k4_tile(w,x,y),field);}
static int k4_slope(unsigned shape,unsigned x){
    x&=15;switch(shape&7){case 0:return 256;case 1:return 0;case 2:return (int)x*8;case 3:return 128+(int)x*8;case 4:return (int)x*16;case 5:return 120-(int)x*8;case 6:return 248-(int)x*8;default:return 240-(int)x*16;}
}
static const uint8_t*k4_sprite(const K4Terrain*w,int frame){return w&&w->sprites&&frame>=124&&frame<521?w->sprites+(frame-124)*18:NULL;}
static void k4_rect(const K4Terrain*w,K4Body*b){
    const uint8_t*s=k4_sprite(w,b->frame);if(!s)return;
    K4Rect*r=&b->box;r->x1=b->x+(int16_t)kg_u16(s+8);r->y1=b->y+(int16_t)kg_u16(s+10);
    r->x2=b->x+(int16_t)kg_u16(s+12);r->y2=b->y+(int16_t)kg_u16(s+14);r->xm=r->x1+(r->x2-r->x1)/2;
    r->tx1=r->x1>>8;r->tx2=r->x2>>8;r->ty1=r->y1>>8;r->ty2=r->y2>>8;r->txm=r->xm>>8;
}
static void k4_shift_x(K4Body*b,int d){b->x+=d;b->box.x1+=d;b->box.x2+=d;b->box.tx1=b->box.x1>>8;b->box.tx2=b->box.x2>>8;/* Keen 4 leaves the cached midpoint unchanged until the next reset. */}
static void k4_shift_y(K4Body*b,int d){b->y+=d;b->box.y1+=d;b->box.y2+=d;b->box.ty1=b->box.y1>>8;b->box.ty2=b->box.y2>>8;}
static void k4_clip_vertical(const K4Terrain*w,K4Body*b,const K4Rect*old,int ddx,int ddy1,int ddy2){
    int px=(b->box.xm>>4)&15,limit=-abs(ddx)-ddy2-16;
    for(int y=old->ty2-1;y<=b->box.ty2;y++){
        unsigned ti=k4_at(w,b->box.txm,y,0);if(!ti)continue;
        int d=y*256+k4_slope(ti,(unsigned)px)-b->box.y2-1;
        if(d<0&&d>=limit){b->top=(int)ti;k4_shift_y(b,d);return;}
    }
    limit=abs(ddx)-ddy1+16;
    for(int y=old->ty1+1;y>=b->box.ty1;y--){
        unsigned ti=k4_at(w,b->box.txm,y,2);if(!ti)continue;
        int d=(y+1)*256-k4_slope(ti,(unsigned)px)-b->box.y1;
        if(d>0&&d<=limit&&b->dy+d<256&&b->dy+d>-256){b->bottom=(int)ti;k4_shift_y(b,d);}
    }
}
static void k4_clip_edge(const K4Terrain*w,K4Body*b,int ddy1,int ddy2){
    int x=b->dir==1?b->box.tx2:b->box.tx1,side=b->dir==1?3:1;
    if(!b->top&&ddy2>0){
        int y=b->box.ty2;unsigned ti=k4_at(w,x,y,0);
        if(!k4_at(w,x,y-2,(unsigned)side)&&!k4_at(w,x,y-1,(unsigned)side)&&!k4_at(w,x,y-1,0)&&ti==1){
            int d=y*256-b->box.y2-1;
            if(d<=0&&-ddy2<=d){b->top=1;k4_shift_y(b,d);k4_shift_x(b,(b->dir==1?b->box.x2:b->box.x1)-b->box.xm);}
        }
    }
    if(!b->bottom&&ddy1<0){
        int y=b->box.ty1;unsigned ti=k4_at(w,x,y,2);
        if(!k4_at(w,x,y+2,(unsigned)side)&&!k4_at(w,x,y+3,(unsigned)side)&&!k4_at(w,x,y+1,2)&&ti){
            int d=(y+1)*256-k4_slope(ti,b->dir==1?0u:15u)-b->box.y1;
            if(d>=0&&-ddy1>=d){b->bottom=(int)ti;k4_shift_y(b,d);}
        }
    }
}
static void k4_clip_horizontal(const K4Terrain*w,K4Body*b){
    int y1=b->box.ty1+(b->bottom>1),y2=b->box.ty2-(b->top>1);
    for(int y=y1;y<=y2;y++){b->right=(int)k4_at(w,b->box.tx1,y,1);if(b->right){k4_shift_x(b,(b->box.tx1+1)*256-b->box.x1);return;}}
    for(int y=y1;y<=y2;y++){b->left=(int)k4_at(w,b->box.tx2,y,3);if(b->left){k4_shift_x(b,b->box.tx2*256-b->box.x2-1);return;}}
}
static void k4_physics_base(const K4Terrain*w,K4Body*b,int keen){
    int ox=b->x,oy=b->y;K4Rect old=b->box;int grounded=b->stick&&b->top!=0x19;
    if(b->stick)b->dy=b->top==0x19?145:abs(b->dx)+16;
    if(b->dx>239)b->dx=239;
    if(b->dx< -239)b->dx=-239;
    if(b->dy>255)b->dy=255;
    if(b->dy< -239)b->dy=-239;
    b->x+=b->dx;b->y+=b->dy;k4_rect(w,b);
    b->top=b->bottom=b->left=b->right=0;
    if(b->clipped){int ddx=b->box.xm-old.xm,dy1=b->box.y1-old.y1,dy2=b->box.y2-old.y2;k4_clip_vertical(w,b,&old,ddx,dy1,dy2);if(keen)k4_clip_edge(w,b,dy1,dy2);k4_clip_horizontal(w,b);}
    if(!b->top&&grounded){b->x=ox+b->dx;b->y=oy;k4_rect(w,b);}
    b->dx=b->x-ox;b->dy=b->y-oy;
}
static void k4_physics(const K4Terrain*w,K4Body*b){k4_physics_base(w,b,1);}
/* The original updates acceleration on odd 70 Hz ticks. */
static int k4_gravity(K4Body*b,unsigned first,unsigned count,unsigned strength){
    int d=0;for(unsigned t=first;t<first+count;t++){
        if((strength==1?t==0:(t&1u))){if(strength>1&&b->vy<0&&b->vy>=-(int)strength){d+=b->vy;b->vy=0;return d;}b->vy+=(int)strength;if(b->vy>70)b->vy=70;}d+=b->vy;
    }return d;
}
static int k4_accel(K4Body*b,unsigned first,unsigned count,int accel,int limit){
    int d=0;for(unsigned t=first;t<first+count;t++){if(t&1u){int negative=b->vx<0;b->vx+=accel;if((b->vx<0)!=negative)b->dir=b->vx<0?-1:1;if(b->vx>limit)b->vx=limit;if(b->vx< -limit)b->vx=-limit;}d+=b->vx;}return d;
}
static int k4_damp(K4Body*b,unsigned first,unsigned count){
    int d=0,a=b->vx>0?-1:b->vx<0?1:0,negative=b->vx<0;
    for(unsigned t=first;t<first+count;t++){if(t&1u){b->vx+=a;if((b->vx<0)!=negative)b->vx=0;}d+=b->vx;}return d;
}
static unsigned k4_frame(const Keen4Player*p){
    int l=p->body.dir<0;switch(p->state){
    case K4_RUN:return (unsigned)(l?139:131)+(p->timer/6u)%4u;
    case K4_JUMP:return l?143u:135u;case K4_JUMP2:return l?144u:136u;case K4_FALL:return l?145u:137u;case K4_FALL2:return l?144u:136u;
    case K4_POGO:case K4_POGO2:return l?214u:212u;case K4_POGO3:return l?213u:211u;
    case K4_LOOKUP:return 146;case K4_LOOKDOWN:return p->timer<6?160u:161u;
    case K4_POLE:return p->pole_dir>0?183u+(p->timer/8)%4u:(unsigned)(l?187:180)+(p->pole_dir<0?(p->timer/8)%3u:0);
    case K4_HANG:return l?201u:202u;case K4_PULL:return p->pull_phase<4?(unsigned)(l?203:207)+p->pull_phase:(l?138u:130u);
    case K4_SHOOT:
        if(p->shoot_kind==2)return p->shoot_aim<0?(l?190u:191u):p->shoot_aim>0?(l?193u:192u):(l?194u:195u);
        if(p->shoot_kind==1)return p->shoot_aim<0?170u:p->shoot_aim>0?169u:(l?168u:173u);
        return p->shoot_aim<0?171u:(l?167u:172u);
    case K4_USE_ALIGN:case K4_USE_GEM:case K4_USE_SWITCH:case K4_USE_WAIT:return 196;
    case K4_DOOR_ALIGN:return 196;case K4_DOOR_ENTER:return 196u+p->door_stage;
    case K4_DEAD:return 162;default:return l?138u:130u;
    }
}
static void k4_pose(const K4Terrain*w,Keen4Player*p){(void)w;p->body.frame=(int)k4_frame(p);}
static void k4_state(const K4Terrain*w,Keen4Player*p,unsigned state){if(p->state!=state)p->timer=0;p->state=state;k4_pose(w,p);}
static void k4_spawn(const K4Terrain*w,Keen4Player*p,int tx,int ty,int dir){memset(p,0,sizeof(*p));p->body.x=tx*256;p->body.y=ty*256-241;p->body.dir=dir;p->body.clipped=1;p->z=1;p->ticks=20;k4_state(w,p,K4_STAND);k4_rect(w,&p->body);}
static void k4_begin_shoot(const K4Terrain*w,Keen4Player*p,unsigned kind,int aim){p->shoot_kind=kind;p->shoot_aim=kind==0&&aim>0?0:aim;p->shoot_time=p->shoot_fired=0;k4_state(w,p,K4_SHOOT);}
static void k4_fire_event(Keen4Player*p){
    K4Body*b=&p->body;int pole=p->shoot_kind==2,air=p->shoot_kind==1;
    p->fire_dx=p->shoot_aim?0:b->dir;p->fire_dy=p->shoot_aim;
    if(!p->shoot_aim){p->fire_x=b->x+(b->dir>0?256:air?0:-128);p->fire_y=b->y+(air?32:64);}
    else if(pole){p->fire_x=b->x+(b->dir>0?96:192);p->fire_y=b->y+(p->shoot_aim<0?64:384);}
    else{p->fire_x=b->x+(p->shoot_aim<0?80:128);p->fire_y=b->y+(p->shoot_aim<0?-160:288);}
    p->events|=K4_SHOT_EVENT;
}
static void k4_use_begin(const K4Terrain*w,Keen4Player*p,unsigned kind,int tx,int ty,int target_x){
    p->use_x=target_x;p->use_tx=tx;p->use_ty=ty;p->use_kind=kind;p->use_time=p->use_committed=0;
    if(kind==2)p->up_slide=1;
    p->body.dx=p->body.dy=0;
    k4_state(w,p,p->body.x==target_x?(kind==1?K4_USE_GEM:K4_USE_SWITCH):K4_USE_ALIGN);
}
/* Ordinary room doors (misc 2) use the map's encoded destination, not a key. */
static int k4_press_up(const K4Terrain*w,Keen4Player*p){
    K4Body*b=&p->body;int tx=b->box.txm,ty=b->box.ty1;
    unsigned flag=k4_at(w,tx,ty,5);
    if(flag==5||flag==6||flag==15){k4_use_begin(w,p,2,tx,ty,tx*256-64);return 1;}
    if(flag!=2)return 0;
    p->door_x=tx*256+96;if(k4_at(w,tx-1,ty,5)==2)p->door_x-=256;
    p->up_slide=1;
    if(b->x!=p->door_x)k4_state(w,p,K4_DOOR_ALIGN);
    else{p->door_start_x=b->x;p->door_start_y=b->y;p->door_stage=p->door_time=0;p->invuln=110;p->z=0;b->dx=b->dy=0;k4_state(w,p,K4_DOOR_ENTER);}
    return 1;
}
static void k4_door_enter(const K4Terrain*w,Keen4Player*p){
    K4Body*b=&p->body;p->door_time+=2;
    if(p->door_time<9)return;
    p->door_time-=9;
    if(p->door_stage<4){b->dy=-64;p->door_stage++;p->events|=K4_WALK_EVENT;return;}
    int x=b->box.tx1,y=b->box.ty2;
    unsigned target=x>=0&&y>=0&&x<w->map->width&&y<w->map->height?w->map->planes[2][(size_t)y*w->map->width+x]:0;
    unsigned tx=target>>8,ty=target&255u;
    if(tx>=2&&ty>=2&&tx<(unsigned)w->map->width-2u&&ty<(unsigned)w->map->height-2u){b->x=(int)tx*256;b->y=(int)ty*256-241;p->events|=K4_DOOR_EVENT;}
    else{b->x=p->door_start_x;b->y=p->door_start_y;}
    b->dx=b->dy=b->vx=b->vy=0;b->clipped=0;p->z=1;p->jump_timer=0;k4_state(w,p,K4_STAND);k4_rect(w,b);
}
static int k4_pole_try(const K4Terrain*w,Keen4Player*p,K4Input in){
    K4Body*b=&p->body;if(!in.y||(p->ticks-p->pole_time)<19)return 0;
    int y=in.y<0?(b->box.y1+96)>>8:b->box.ty2+1;
    if((k4_at(w,b->box.txm,y,5)&127u)!=1)return 0;
    b->x=(b->box.txm-1)*256+128;b->dx=0;b->dy=in.y*32;b->clipped=0;p->pole_dir=0;k4_state(w,p,K4_POLE);return 1;
}
static void k4_ledge(const K4Terrain*w,Keen4Player*p,K4Input in,int old_top){
    K4Body*b=&p->body;if(b->dy<=0||!in.x)return;
    int boundary=((b->box.y1-64)&~255)+64;
    if(old_top>=boundary||b->box.y1<boundary)return;
    int tx=in.x<0?b->box.tx1-(b->right!=0):b->box.tx2+(b->left!=0),ty=(boundary>>8)-1;
    if(k4_at(w,tx,ty,0)||k4_at(w,tx,ty,1)||k4_at(w,tx,ty,2)||k4_at(w,tx,ty,3)||!k4_at(w,tx,ty+1,in.x<0?1u:3u)||!k4_at(w,tx,ty+1,0))return;
    b->dir=in.x;b->clipped=0;b->x=(b->x&~255)+(in.x<0?128:256);b->y=boundary-64;b->vy=b->dy=0;k4_state(w,p,K4_HANG);
}
static void k4_kill(const K4Terrain*w,Keen4Player*p){p->body.clipped=0;p->body.vy=-40;p->body.vx=16;p->z=3;p->events|=K4_DEAD_EVENT;k4_state(w,p,K4_DEAD);}
/* One DOS-style two-tick simulation frame, independent of display refresh. */
static void k4_step_objects(const K4Terrain*w,Keen4Player*p,K4Input in,int easy,void(*objects)(const K4Terrain*,Keen4Player*,void*),void*context){
    K4Body*b=&p->body;unsigned state=p->state;int fresh_jump=in.jump&&!p->jump_latch,fresh_pogo=in.pogo&&!p->pogo_latch,fresh_shoot=in.shoot&&!p->shoot_latch;
    p->jump_latch=in.jump;p->pogo_latch=in.pogo;p->shoot_latch=in.shoot;p->events=0;
    if(in.y!=-1)p->up_slide=0;
    if(p->invuln)p->invuln=p->invuln>2?p->invuln-2:0;
    b->dx=b->dy=0;b->stick=0;
    int old_top=b->box.y1;
    if(state==K4_DEAD){b->dy=k4_gravity(b,p->ticks,2,3);b->dx=b->vx*2;}
    else if(state==K4_SHOOT){
        p->shoot_time+=2;
        if(p->shoot_kind==0)b->stick=1;
        else if(p->shoot_kind==1){b->dy=k4_gravity(b,p->ticks,2,4);b->dx=b->vx*2;}
        if(!p->shoot_fired&&p->shoot_time>=9){k4_fire_event(p);p->shoot_fired=1;}
        if(p->shoot_time>=(p->shoot_kind==1?16u:15u)){if(p->shoot_kind==2){p->pole_dir=0;k4_state(w,p,K4_POLE);}else k4_state(w,p,p->shoot_kind==1?K4_FALL:p->shoot_aim<0?K4_LOOKUP:K4_STAND);}
    }
    else if(state==K4_USE_ALIGN){
        int d=p->use_x-b->x;b->dx=d>32?32:d< -32?-32:d;
        if(!d){p->use_time=0;k4_state(w,p,p->use_kind==1?K4_USE_GEM:K4_USE_SWITCH);}
    }else if(state==K4_USE_GEM||state==K4_USE_SWITCH||state==K4_USE_WAIT){
        b->stick=1;p->use_time+=2;
        if(p->use_time>=(state==K4_USE_GEM?6u:8u)){
            if(state==K4_USE_WAIT)k4_state(w,p,K4_STAND);
            else{p->events|=K4_USE_EVENT;p->use_time=0;k4_state(w,p,K4_USE_WAIT);}
        }
    }
    else if(state==K4_DOOR_ALIGN){
        int d=p->door_x-b->x;b->dx=d>32?32:d< -32?-32:d;
        if(!d&&!k4_press_up(w,p))k4_state(w,p,K4_STAND);
    }else if(state==K4_DOOR_ENTER){k4_door_enter(w,p);}
    else if(state==K4_PULL){
        p->timer+=2;
        if(p->timer>=(p->pull_phase==4?6u:10u)){
            switch(p->pull_phase){case 0:if(b->dir==1)b->dx=128;else b->dy=-128;break;case 1:b->dx=b->dir*128;b->dy=-128;break;case 2:b->dy=-128;break;case 3:b->clipped=1;p->z=1;b->dy=128;break;default:break;}
            p->timer=0;if(++p->pull_phase==5)k4_state(w,p,K4_STAND);
        }
    }else if(state==K4_HANG){
        if(p->timer>=12){if(in.y<0||in.x==b->dir){b->dy=b->dir==1?-256:-128;p->pull_phase=0;k4_state(w,p,K4_PULL);p->z=(k4_at(w,b->dir==1?b->box.tx2:b->box.tx1,b->box.ty1-1,5)&128u)?1:3;}
        else if(in.y>0||(in.x&&in.x!=b->dir)){b->clipped=1;k4_state(w,p,K4_FALL);}}
    }else if(state==K4_POLE){
        if(in.x)b->dir=in.x;
        if(fresh_jump){b->vx=in.x*8;b->vy=-20;b->clipped=1;p->jump_timer=10;p->pole_time=p->ticks;k4_state(w,p,K4_JUMP);p->events|=K4_JUMP_EVENT;}
        else if(fresh_shoot){k4_begin_shoot(w,p,2,in.y);}
        else{
            p->pole_dir=in.y;
            if(in.y<0&&(k4_at(w,b->box.txm,b->box.ty1,5)&127u)!=1)p->pole_dir=0;
            if(in.y>0&&(k4_at(w,b->box.txm,(b->box.y2-64)>>8,5)&127u)!=1){b->vx=in.x*8;b->vy=0;b->clipped=1;p->jump_timer=0;k4_state(w,p,K4_FALL);}
            else{b->dy=p->pole_dir*(p->pole_dir>0?24:8)*2;k4_pose(w,p);}
        }
    }else if(state==K4_STAND||state==K4_RUN||state==K4_LOOKUP||state==K4_LOOKDOWN){
        b->stick=1;
        if(state==K4_LOOKUP&&in.y==-1&&!in.x&&!fresh_jump&&!fresh_pogo&&!fresh_shoot)goto apply_motion;
        if(in.x){b->dir=in.x;k4_state(w,p,K4_RUN);b->dx=in.x*(state==K4_RUN?48:12);static const int slope_bias[8]={0,0,4,4,8,-4,-4,-8};b->dx+=slope_bias[b->top&7]*2;}
        else k4_state(w,p,in.y<0?K4_LOOKUP:in.y>0?K4_LOOKDOWN:K4_STAND);
        if(state==K4_LOOKDOWN&&fresh_jump&&(b->top&7)==1){
            fresh_jump=0;int tx=b->box.txm,ty=b->box.ty2;
            if(!k4_at(w,tx,ty,1)&&!k4_at(w,tx,ty,2)&&!k4_at(w,tx,ty,3)&&!k4_at(w,tx,ty+1,1)&&!k4_at(w,tx,ty+1,2)&&!k4_at(w,tx,ty+1,3)){
                b->y+=64;b->box.y2+=64;b->vx=b->vy=b->dx=b->dy=0;b->stick=0;p->jump_timer=0;k4_state(w,p,K4_FALL);goto apply_motion;
            }
        }
        if(fresh_shoot&&!in.x){b->dx=0;k4_begin_shoot(w,p,0,in.y);}
        else if(in.y&&k4_pole_try(w,p,in)){b->stick=0;}
        else if(in.y<0&&!p->up_slide&&k4_press_up(w,p)){b->stick=0;}
        else if(fresh_shoot){if(state!=K4_RUN)b->dx=0;k4_begin_shoot(w,p,0,in.y);}
        else if(fresh_jump||fresh_pogo){b->vx=in.x*16;b->vy=fresh_pogo?-48:-40;p->jump_timer=fresh_pogo?24:18;b->dx=b->dy=0;b->stick=0;k4_state(w,p,fresh_pogo?K4_POGO:K4_JUMP);p->events|=K4_JUMP_EVENT;}
    }else{
        int pogo=state==K4_POGO||state==K4_POGO2||state==K4_POGO3;
        if(state==K4_POGO){b->vy=-48;b->dy=-288;p->jump_timer=24;p->events|=K4_POGO_EVENT;k4_state(w,p,K4_POGO2);}
        else if(pogo){unsigned g=!p->jump_timer?(easy?3u:4u):(in.jump||p->jump_timer<=9?1u:4u);b->dy=k4_gravity(b,p->ticks,2,g);if(p->jump_timer)p->jump_timer=p->jump_timer>2?p->jump_timer-2:0;if(!p->jump_timer)k4_state(w,p,K4_POGO3);}
        else if(p->jump_timer){b->dy=b->vy*(int)(p->jump_timer<2?p->jump_timer:2);p->jump_timer=p->jump_timer>2?p->jump_timer-2:0;if(!in.jump)p->jump_timer=0;if(!p->jump_timer)k4_state(w,p,K4_JUMP2);}
        else{b->dy=k4_gravity(b,p->ticks,2,easy?3u:4u);if(b->vy>0&&state!=K4_FALL&&state!=K4_FALL2)k4_state(w,p,K4_FALL);}
        if(in.x){if(!pogo||!b->vx)b->dir=in.x;b->dx=k4_accel(b,p->ticks,2,in.x*(pogo?1:2),24);}
        else b->dx=pogo?b->vx*2:k4_damp(b,p->ticks,2);
        if(fresh_shoot){k4_begin_shoot(w,p,1,in.y);}
        else if(fresh_pogo){p->jump_timer=0;k4_state(w,p,pogo?K4_FALL:K4_POGO2);}
        if(!pogo&&!fresh_shoot&&!fresh_pogo&&in.y<0)k4_pole_try(w,p,in);
    }
apply_motion:
    k4_pose(w,p);k4_physics(w,b);
    if(objects)objects(w,p,context);
    if(p->state!=K4_DEAD&&p->state!=K4_HANG&&p->state!=K4_PULL&&p->state!=K4_POLE&&p->state!=K4_DOOR_ALIGN&&p->state!=K4_DOOR_ENTER&&p->state!=K4_USE_ALIGN&&!(p->state==K4_SHOOT&&p->shoot_kind==2)&&!(p->events&K4_DOOR_EVENT)){
        if((b->right&&b->dir<0)||(b->left&&b->dir>0)){b->vx=0;if(p->state==K4_RUN)k4_state(w,p,K4_STAND);}
        if(b->bottom){if(b->bottom==17){b->y-=32;b->x=b->box.txm*256-32;b->vx=0;k4_rect(w,b);}else{b->vy=b->bottom>1?b->vy+16:0;if(b->vy<0)b->vy=0;p->jump_timer=0;p->events|=K4_HEAD_EVENT;}}
        if(b->top){
            if((b->top&~7)==8)k4_kill(w,p);
            else if(p->state==K4_POGO||p->state==K4_POGO2||p->state==K4_POGO3){if(b->top!=0x19||!p->jump_timer){b->vy=-48;p->jump_timer=24;k4_state(w,p,K4_POGO2);p->events|=K4_POGO_EVENT;}}
            else if(!b->stick&&(b->top!=0x19||!p->jump_timer)){
                if(p->state==K4_SHOOT&&p->shoot_kind==1&&!p->shoot_fired&&p->shoot_aim<=0){p->shoot_kind=0;p->shoot_time=0;k4_state(w,p,K4_SHOOT);}
                else k4_state(w,p,in.x?K4_RUN:K4_STAND);
                p->events|=K4_LAND_EVENT;
            }
        }else if(b->stick){b->vx=b->dir*8;b->vy=0;p->jump_timer=0;k4_state(w,p,K4_FALL);}
        else if(p->state!=K4_POGO&&p->state!=K4_POGO2&&p->state!=K4_POGO3)k4_ledge(w,p,in,old_top);
    }
    if(p->state==K4_POLE&&p->pole_dir>0&&k4_at(w,b->box.txm,b->box.ty2,0)==1){k4_shift_y(b,-(b->box.y2&255)-1);b->clipped=1;k4_state(w,p,K4_LOOKDOWN);}
    if(state==K4_DOOR_ENTER&&p->state!=K4_DOOR_ENTER)b->clipped=1;
    if(p->state!=K4_DEAD&&!p->invuln){for(int y=b->box.ty1;y<=b->box.ty2;y++)for(int x=b->box.tx1;x<=b->box.tx2;x++)if((k4_at(w,x,y,5)&127u)==3)k4_kill(w,p);}
    if(p->state!=K4_PULL)p->timer+=2;
    if(p->state==K4_RUN&&(p->timer==2||p->timer==14))p->events|=K4_WALK_EVENT;
    if(p->state==K4_FALL&&p->timer>=50)k4_state(w,p,K4_FALL2);else if(p->state==K4_FALL2&&p->timer>=40)k4_state(w,p,K4_FALL);
    p->ticks+=2;k4_pose(w,p);k4_rect(w,b);
}
static void k4_step(const K4Terrain*w,Keen4Player*p,K4Input in,int easy){k4_step_objects(w,p,in,easy,NULL,NULL);}
#endif
