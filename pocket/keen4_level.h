/* SPDX-License-Identifier: MIT
 * Galaxy pickups, gem sockets and switches. Reference: Omnispeak
 * ck_keen.c CK_KeenCheckSpecialTileInfo/CK_KeenPlaceGem,
 * ck_obj.c CK_DoorOpen/CK_SpawnItem and ck4_misc.c info-layer scan.
 */
#ifndef RETRODRIVEN_KEEN4_LEVEL_H
#define RETRODRIVEN_KEEN4_LEVEL_H
#include "keen4_weapon.h"
#define K4_ITEMS 256u
#define K4_OPEN_DOORS 32u
typedef struct{K4Body body;unsigned kind,base,active,time,notify;} K4Item;
typedef struct{unsigned x,y,height,phase,time,active;} K4Opening;
typedef struct{
    K4Item items[K4_ITEMS];K4Opening doors[K4_OPEN_DOORS];unsigned item_count;
    uint32_t score,next_life;unsigned lives,drops,gems[4],pickups,opened,switches;
    uint8_t dirty[KG_MAP_CELLS];uint16_t dirty_cells[KG_MAP_CELLS];unsigned dirty_count;
} Keen4Level;
static const unsigned k4_item_base[12]={242,244,246,248,227,229,231,233,235,237,239,251};
static const unsigned k4_item_shadow[12]={250,250,250,250,219,220,221,222,223,224,225,226};
static const unsigned k4_item_points[12]={0,0,0,0,100,200,500,1000,2000,5000,0,0};
static void k4_level_score(Keen4Level*l,unsigned points){l->score+=points;if(l->score>=l->next_life){l->lives++;l->next_life=l->next_life<=UINT32_MAX/2?l->next_life*2:UINT32_MAX;}}
static int k4_level_tile(Keen4Level*l,KeenGalaxyMap*m,int x,int y,unsigned tile){
    if(x<0||y<0||x>=m->width||y>=m->height||tile>=KG_FG_COUNT)return 0;
    size_t cell=(size_t)y*m->width+x;if(m->planes[1][cell]==tile)return 1;m->planes[1][cell]=(uint16_t)tile;
    if(!l->dirty[cell]){l->dirty[cell]=1;l->dirty_cells[l->dirty_count++]=(uint16_t)cell;}return 1;
}
static K4Item*k4_level_notify(Keen4Level*l,const K4Terrain*w,int x,int y,unsigned frame,unsigned splash){
    for(unsigned n=0;n<K4_ITEMS;n++)if(!l->items[n].active){
        K4Item*i=&l->items[n];memset(i,0,sizeof(*i));i->active=2;i->notify=splash;i->body.x=x;i->body.y=y;i->base=frame;i->body.frame=(int)frame;k4_rect(w,&i->body);return i;
    }return NULL;
}
static int k4_level_init(Keen4Level*l,const K4Terrain*w,unsigned ammo){
    memset(l,0,sizeof(*l));l->lives=3;l->next_life=20000;
    for(unsigned y=0;y<w->map->height;y++)for(unsigned x=0;x<w->map->width;x++){
        unsigned v=w->map->planes[2][(size_t)y*w->map->width+x];if(v==34&&ammo<5)v=68;if(v<57||v>68)continue;
        if(l->item_count>=K4_ITEMS)return 0;
        K4Item*i=&l->items[l->item_count++];i->kind=v-57;i->base=k4_item_base[i->kind];i->active=1;i->body.x=(int)x*256;i->body.y=(int)y*256;i->body.frame=(int)i->base;k4_rect(w,&i->body);
    }return 1;
}
static void k4_level_award(Keen4Level*l,Keen4Weapons*gun,unsigned kind,int easy){
    if(kind>=12)return;
    k4_level_score(l,k4_item_points[kind]);
    if(kind<4)l->gems[kind]++;else if(kind==10)l->lives++;else if(kind==11)gun->ammo+=easy?8u:5u;l->pickups++;
}
static int k4_overlap(const K4Body*a,const K4Body*b){return a->box.x1<b->box.x2&&a->box.x2>b->box.x1&&a->box.y1<b->box.y2&&a->box.y2>b->box.y1;}
static int k4_socket_plan(const K4Terrain*w,int x,int y,unsigned*tx,unsigned*ty,unsigned*height){
    if(!w||!w->map||x<0||y<0||x>=w->map->width||y>=w->map->height)return 0;
    unsigned tile=k4_tile(w,x,y),flag=k4_info(w,tile,5)&127u;if(flag<7||flag>10||tile+18>=KG_FG_COUNT)return 0;
    unsigned dest=w->map->planes[2][(size_t)y*w->map->width+x];*tx=dest>>8;*ty=dest&255;
    if(*tx<2||*ty<2||*tx>=w->map->width||*ty>=w->map->height)return 0;
    unsigned door=k4_tile(w,(int)*tx,(int)*ty),n=0;if(!door)return 0;
    do{n++;}while(*ty+n<w->map->height&&k4_tile(w,(int)*tx,(int)(*ty+n))==door);
    if(n+2>50||*ty+n+2>w->map->height)return 0;
    for(unsigned yy=0;yy<n+2;yy++)if(k4_tile(w,(int)*tx,(int)(*ty+yy))+3>=KG_FG_COUNT)return 0;
    *height=n;return 1;
}
static int k4_open_slot(const Keen4Level*l){for(unsigned n=0;n<K4_OPEN_DOORS;n++)if(!l->doors[n].active)return (int)n;return -1;}
static void k4_level_switch(Keen4Level*l,const K4Terrain*w,KeenGalaxyMap*m,int x,int y){
    if(x<0||y<0||x>=m->width||y>=m->height)return;
    unsigned tile=k4_tile(w,x,y),flag=k4_info(w,tile,5);int next=(int)tile+(int8_t)k4_info(w,tile,4);
    if((flag!=5&&flag!=6&&flag!=15)||next<0||next>=(int)KG_FG_COUNT)return;
    unsigned destination=m->planes[2][(size_t)y*m->width+x],tx=destination>>8,ty=destination&255u;
    if(tx>=m->width||ty>=m->height)return;
    if(flag==15){
        if(tx==0||ty+1>=m->height)return;
        for(unsigned row=0;row<2;row++)for(unsigned xx=tx-row;xx<m->width;xx++){
            unsigned t=k4_tile(w,(int)xx,(int)(ty+row));int delta=(int8_t)k4_info(w,t,4);if(!delta)break;
            if((int)t+delta<0||(int)t+delta>=(int)KG_FG_COUNT)return;
        }
        for(unsigned row=0;row<2;row++)for(unsigned xx=tx-row;xx<m->width;xx++){
            unsigned t=k4_tile(w,(int)xx,(int)(ty+row));int delta=(int8_t)k4_info(w,t,4);if(!delta)break;
            (void)k4_level_tile(l,m,(int)xx,(int)(ty+row),(unsigned)((int)t+delta));
        }
    }else m->planes[2][(size_t)ty*m->width+tx]^=0x1f;
    (void)k4_level_tile(l,m,x,y,(unsigned)next);l->switches++;
}
static void k4_level_step(Keen4Level*l,const K4Terrain*w,KeenGalaxyMap*m,Keen4Player*p,Keen4Weapons*gun,int easy){
    K4Body*b=&p->body;
    for(unsigned n=0;n<K4_OPEN_DOORS;n++){
        K4Opening*d=&l->doors[n];if(!d->active)continue;d->time+=2;if(d->time<10)continue;d->time-=10;
        for(unsigned yy=0;yy<d->height+2;yy++)(void)k4_level_tile(l,m,(int)d->x,(int)(d->y+yy),k4_tile(w,(int)d->x,(int)(d->y+yy))+1);
        if(++d->phase==3)d->active=0;
    }
    int can_collect=p->state!=K4_DEAD&&p->state!=K4_DOOR_ENTER&&p->state!=K4_DOOR_ALIGN&&p->state!=K4_USE_ALIGN&&p->state!=K4_USE_GEM&&p->state!=K4_USE_SWITCH&&p->state!=K4_USE_WAIT;
    for(unsigned n=0;n<K4_ITEMS;n++){
        K4Item*i=&l->items[n];if(!i->active)continue;i->time+=2;
        if(i->active==2){if(i->time>=(i->notify?30u:40u)){i->active=0;continue;}if(i->notify)i->body.frame=215+(int)(i->time/10);else i->body.y-=16;k4_rect(w,&i->body);continue;}
        i->body.frame=(int)i->base+(int)((i->time/20)%2);k4_rect(w,&i->body);
        if(can_collect&&p->state!=K4_POLE&&p->state!=K4_HANG&&p->state!=K4_PULL&&!(p->state==K4_LOOKUP&&p->timer>=30)&&!(p->state==K4_LOOKDOWN&&p->timer>=6)&&!(p->state==K4_SHOOT&&p->shoot_kind==2)&&k4_overlap(b,&i->body)){k4_level_award(l,gun,i->kind,easy);i->active=2;i->time=0;i->base=k4_item_shadow[i->kind];i->body.frame=(int)i->base;k4_rect(w,&i->body);}
    }
    if(can_collect){
        for(int y=b->box.ty1;y<=b->box.ty2;y++)for(int x=b->box.tx1;x<=b->box.tx2;x++){
            if(x<0||y<0||x>=m->width||y>=m->height)continue;
            unsigned flag=k4_at(w,x,y,5)&127u;
            if(flag==4){(void)k4_level_tile(l,m,x,y,0);k4_level_notify(l,w,x*256,y*256,215,1);if(++l->drops==100){l->drops=0;l->lives++;k4_level_notify(l,w,x*256,y*256,215,0);}l->pickups++;}
            else if(flag>=21&&flag<=28){unsigned kind=flag-17;k4_level_award(l,gun,kind,easy);(void)k4_level_tile(l,m,x,y,0);k4_level_notify(l,w,x*256,y*256,k4_item_shadow[kind],0);}
        }
    }
    if(p->state==K4_USE_GEM&&!p->use_committed){
        unsigned flag=k4_at(w,p->use_tx,p->use_ty,5)&127u,tx,ty,height;
        if(flag<7||flag>10||!l->gems[flag-7]||!k4_socket_plan(w,p->use_tx,p->use_ty,&tx,&ty,&height)||k4_open_slot(l)<0){k4_state(w,p,K4_STAND);return;}
        l->gems[flag-7]--;p->use_committed=1;
    }
    if(p->events&K4_USE_EVENT){
        if(p->use_kind==2)k4_level_switch(l,w,m,p->use_tx,p->use_ty);
        else if(p->use_committed){
            unsigned tx,ty,height;int slot=k4_open_slot(l);
            if(slot>=0&&k4_socket_plan(w,p->use_tx,p->use_ty,&tx,&ty,&height)){
                (void)k4_level_tile(l,m,p->use_tx,p->use_ty,k4_tile(w,p->use_tx,p->use_ty)+18);
                l->doors[slot]=(K4Opening){tx,ty,height,0,0,1};l->opened++;
            }
        }
    }
    if(p->state==K4_STAND||p->state==K4_RUN||p->state==K4_LOOKUP||p->state==K4_LOOKDOWN){
        int y=b->box.ty2;if(!b->top||y<0||y>=m->height)return;
        for(int x=b->box.tx1;x<=b->box.tx2;x++){
            unsigned flag=k4_at(w,x,y,5)&127u,tx,ty,height;
            if(flag>=7&&flag<=10&&l->gems[flag-7]&&k4_open_slot(l)>=0&&k4_socket_plan(w,x,y,&tx,&ty,&height)){
                k4_use_begin(w,p,1,x,y,x*256-64);
                if(p->state==K4_USE_GEM){l->gems[flag-7]--;p->use_committed=1;}return;
            }
        }
    }
}
#endif
