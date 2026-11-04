/* SPDX-License-Identifier: MIT
 * Galaxy resource and gameplay preview. */
#ifndef RETRODRIVEN_KEEN4_PREVIEW_H
#define RETRODRIVEN_KEEN4_PREVIEW_H
static uint8_t galaxy_graph[600000],galaxy_head[15000],galaxy_dict[1024];
static uint8_t galaxy_maps[120000],galaxy_maphead[402],galaxy_scratch[KG_CHUNK_CAP];
static uint8_t galaxy_bg[KG_BG_COUNT][128],galaxy_fg[KG_FG_COUNT][160];
static uint8_t galaxy_bg_pixels[KG_BG_COUNT][256],galaxy_fg_pixels[KG_FG_COUNT][256];
static uint8_t galaxy_scene_pixels[KG_MAP_CELLS*256u];
static uint8_t galaxy_title_pixels[32000];
static unsigned galaxy_title_wb,galaxy_title_height;
static uint8_t galaxy_bg_ready[KG_BG_COUNT],galaxy_fg_ready[KG_FG_COUNT];
static uint8_t galaxy_tileinfo[23004],galaxy_player_pixels[8192];
static Keen4Player galaxy_player;
static Keen4Weapons galaxy_gun;
static Keen4Level galaxy_level_state;
static Keen4Platforms galaxy_platforms;
static uint8_t galaxy_platform_pixels[7][8192],galaxy_platform_ready[7];
static uint8_t galaxy_item_pixels[38][4096],galaxy_item_ready[38];
static uint8_t galaxy_shot_pixels[6][8192];
static uint8_t galaxy_shot_ready[6];
static K4Terrain galaxy_terrain;
static uint32_t galaxy_last_us,galaxy_pending;
static uint64_t galaxy_credit;
static int galaxy_sprite_cached=-1,galaxy_camera_x,galaxy_camera_y,galaxy_screen_y=140;
static uint8_t galaxy_pictures[460],galaxy_sprites[KG_SPR_COUNT*18];
static KeenGalaxyGraphics galaxy_gfx;
static KeenGalaxyMap galaxy_map;
static size_t galaxy_maps_len,galaxy_maphead_len;
static int galaxy_mode,galaxy_ready,galaxy_page,galaxy_level,galaxy_x,galaxy_y;
static char galaxy_label[17];
static const char*galaxy_message="KEEN 4 DATA NOT AVAILABLE";
static int galaxy_asset(const char*name,uint8_t*dst,size_t cap,size_t*len){return read_asset(episode_file(3,name),dst,cap,len);}
static int galaxy_prepare_fg(unsigned fg){
    if(fg>=KG_FG_COUNT)return 0;
    if(!fg||galaxy_fg_ready[fg])return 1;
    size_t size;if(kg_graph_chunk(&galaxy_gfx,KG_FG_START+fg,galaxy_fg[fg],160,&size)||size!=160)return 0;
    for(unsigned py=0;py<16;py++)for(unsigned px=0;px<16;px++)galaxy_fg_pixels[fg][py*16+px]=(uint8_t)kg_pixel(galaxy_fg[fg],2,16,px,py,1);
    galaxy_fg_ready[fg]=1;return 1;
}
static int galaxy_cache_cell(size_t cell){
    unsigned bg=galaxy_map.planes[0][cell],fg=galaxy_map.planes[1][cell],tx=(unsigned)(cell%galaxy_map.width),ty=(unsigned)(cell/galaxy_map.width),stride=(unsigned)galaxy_map.width*16u;
    if(!galaxy_prepare_fg(fg))return 0;
    for(unsigned py=0;py<16;py++){
        uint8_t*row=galaxy_scene_pixels+((size_t)ty*16+py)*stride+tx*16;
        for(unsigned px=0;px<16;px++){unsigned at=py*16+px,c=galaxy_bg_pixels[bg][at],f=fg?galaxy_fg_pixels[fg][at]:255;row[px]=(uint8_t)(9+(f==255?c:f));}
    }return 1;
}
static void galaxy_flush_level(void){
    for(unsigned n=0;n<galaxy_level_state.dirty_count;n++){
        unsigned cell=galaxy_level_state.dirty_cells[n];if(!galaxy_cache_cell(cell)){galaxy_ready=0;galaxy_message="KEEN 4 TILE UPDATE FAILED";return;}galaxy_level_state.dirty[cell]=0;
    }galaxy_level_state.dirty_count=0;
}
static int galaxy_load_map(unsigned id){
    if(!galaxy_ready||kg_map_decode(galaxy_maps,galaxy_maps_len,galaxy_maphead,galaxy_maphead_len,id,&galaxy_map,galaxy_scratch,sizeof(galaxy_scratch)))return 0;
    galaxy_level=(int)id;galaxy_x=galaxy_y=0;
    for(unsigned i=0;i<17;i++){char c=galaxy_map.name[i];galaxy_label[i]=(c>='a'&&c<='z')?(char)(c-32):c;}
    for(size_t n=0;n<(size_t)galaxy_map.width*galaxy_map.height;n++){
        unsigned bg=galaxy_map.planes[0][n],fg=galaxy_map.planes[1][n];size_t size;
        if(bg>=KG_BG_COUNT||fg>=KG_FG_COUNT)return 0;
        if(!galaxy_bg_ready[bg]){
            if(kg_graph_chunk(&galaxy_gfx,KG_BG_START+bg,galaxy_bg[bg],128,&size)||size!=128)return 0;
            for(unsigned py=0;py<16;py++)for(unsigned px=0;px<16;px++)galaxy_bg_pixels[bg][py*16+px]=(uint8_t)kg_pixel(galaxy_bg[bg],2,16,px,py,0);
            galaxy_bg_ready[bg]=1;
        }
        if(fg&&!galaxy_fg_ready[fg]){
            if(kg_graph_chunk(&galaxy_gfx,KG_FG_START+fg,galaxy_fg[fg],160,&size)||size!=160)return 0;
            for(unsigned py=0;py<16;py++)for(unsigned px=0;px<16;px++)galaxy_fg_pixels[fg][py*16+px]=(uint8_t)kg_pixel(galaxy_fg[fg],2,16,px,py,1);
            galaxy_fg_ready[fg]=1;
        }
        unsigned spawn=galaxy_map.planes[2][n];
        if(spawn==1||spawn==2||spawn==3){galaxy_x=(int)(n%galaxy_map.width)*16-144;galaxy_y=(int)(n/galaxy_map.width)*16-96;}
    }
    for(size_t cell=0;cell<(size_t)galaxy_map.width*galaxy_map.height;cell++)if(!galaxy_cache_cell(cell))return 0;
    int max_x=(int)galaxy_map.width*16-W,max_y=(int)galaxy_map.height*16-200;
    if(galaxy_x>max_x)galaxy_x=max_x;
    if(galaxy_y>max_y)galaxy_y=max_y;
    if(galaxy_x<0)galaxy_x=0;
    if(galaxy_y<0)galaxy_y=0;
    return 1;
}
static int galaxy_open(void){
    size_t n;galaxy_mode=1;galaxy_page=0;galaxy_ready=0;
    memset(galaxy_bg_ready,0,sizeof(galaxy_bg_ready));memset(galaxy_fg_ready,0,sizeof(galaxy_fg_ready));
    if(galaxy_asset("EGAGRAPH.CK4",galaxy_graph,sizeof(galaxy_graph),&galaxy_gfx.graph_len)||
       galaxy_asset("EGAHEAD.CK4",galaxy_head,sizeof(galaxy_head),&galaxy_gfx.head_len)||
       galaxy_asset("EGADICT.CK4",galaxy_dict,sizeof(galaxy_dict),&galaxy_gfx.dict_len)||
       galaxy_asset("GAMEMAPS.CK4",galaxy_maps,sizeof(galaxy_maps),&galaxy_maps_len)||
       galaxy_asset("MAPHEAD.CK4",galaxy_maphead,sizeof(galaxy_maphead),&galaxy_maphead_len))return 0;
    if(galaxy_asset("TILEINFO.CK4",galaxy_tileinfo,sizeof(galaxy_tileinfo),&n)||n!=sizeof(galaxy_tileinfo))return 0;
    galaxy_terrain=(K4Terrain){&galaxy_map,galaxy_tileinfo,galaxy_sprites};
    galaxy_gfx.graph=galaxy_graph;galaxy_gfx.head=galaxy_head;galaxy_gfx.dict=galaxy_dict;
    if(kg_graph_chunk(&galaxy_gfx,0,galaxy_pictures,sizeof(galaxy_pictures),&n)||n!=sizeof(galaxy_pictures)||
       kg_graph_chunk(&galaxy_gfx,2,galaxy_sprites,sizeof(galaxy_sprites),&n)||n!=sizeof(galaxy_sprites))return 0;
    galaxy_title_wb=kg_u16(galaxy_pictures+(109-6)*4);galaxy_title_height=kg_u16(galaxy_pictures+(109-6)*4+2);
    if(galaxy_title_wb!=40||galaxy_title_height!=200||kg_graph_chunk(&galaxy_gfx,109,galaxy_title_pixels,sizeof(galaxy_title_pixels),&n)||n!=sizeof(galaxy_title_pixels))return 0;
    galaxy_ready=1;
    if(!galaxy_load_map(0)){galaxy_ready=0;galaxy_message="KEEN 4 MAP DECODE FAILED";return 0;}
    return 1;
}
static void galaxy_title_frame(uint8_t*fb){
    kg_blit(fb,W,H,0,20,galaxy_title_pixels,galaxy_title_wb,galaxy_title_height,0);
}
static void galaxy_map_frame(uint8_t*fb){
    unsigned stride=(unsigned)galaxy_map.width*16u;
    unsigned copy_width=stride>(unsigned)galaxy_x?stride-(unsigned)galaxy_x:0;
    if(copy_width>W)copy_width=W;
    for(unsigned sy=0;sy<200;sy++){
        unsigned wy=sy+(unsigned)galaxy_y;if(wy>=(unsigned)galaxy_map.height*16u)break;
        memcpy(fb+(sy+20)*W,galaxy_scene_pixels+(size_t)wy*stride+(unsigned)galaxy_x,copy_width);
    }
}
static int galaxy_camera_bound(int n,int max){return max<32?0:n<32?32:n>max?max:n;}
static int galaxy_start_player(unsigned id){
    if(id==0)id=1;
    if(!galaxy_load_map(id))return 0;
    for(unsigned y=0;y<galaxy_map.height;y++)for(unsigned x=0;x<galaxy_map.width;x++){
        unsigned obj=galaxy_map.planes[2][(size_t)y*galaxy_map.width+x];
        if(obj!=1&&obj!=2)continue;
        k4_spawn(&galaxy_terrain,&galaxy_player,(int)x,(int)y,obj==1?1:-1);k4_weapon_init(&galaxy_gun);
        if(!k4_level_init(&galaxy_level_state,&galaxy_terrain,galaxy_gun.ammo))return 0;
        if(!k4_platform_init(&galaxy_platforms,&galaxy_terrain))return 0;
        memset(galaxy_platform_ready,0,sizeof(galaxy_platform_ready));
        memset(galaxy_item_ready,0,sizeof(galaxy_item_ready));
        memset(galaxy_shot_ready,0,sizeof(galaxy_shot_ready));galaxy_player.shoot_latch=!!of_btn(OF_BTN_X);
        galaxy_player.jump_latch=!!of_btn(OF_BTN_A);galaxy_player.pogo_latch=!!of_btn(OF_BTN_Y);
        galaxy_camera_x=galaxy_player.body.x-152*16;galaxy_camera_y=galaxy_player.body.box.y2-140*16;galaxy_screen_y=140;
        galaxy_x=galaxy_camera_bound(galaxy_camera_x/16,(int)galaxy_map.width*16-352);galaxy_y=galaxy_camera_bound(galaxy_camera_y/16,(int)galaxy_map.height*16-240);
        galaxy_camera_x=galaxy_x*16;galaxy_camera_y=galaxy_y*16;
        galaxy_page=2;galaxy_last_us=of_time_us();galaxy_credit=galaxy_pending=0;galaxy_sprite_cached=-1;return 1;
    }
    return 0;
}
static void galaxy_camera(void){
    K4Body*b=&galaxy_player.body;if(galaxy_player.state==K4_DEAD)return;
    int dx=0,dy=0;
    if(b->x<galaxy_camera_x+144*16)dx=b->x-galaxy_camera_x-144*16;
    if(b->x>galaxy_camera_x+192*16)dx=b->x-galaxy_camera_x-192*16;
    if(galaxy_player.state==K4_LOOKUP&&galaxy_player.timer>=30){int n=galaxy_screen_y<166?2:167-galaxy_screen_y;galaxy_screen_y+=n;dy-=n*16;}
    if(galaxy_player.state==K4_LOOKDOWN&&galaxy_player.timer>=30){int n=galaxy_screen_y>34?2:galaxy_screen_y-33;galaxy_screen_y-=n;dy+=n*16;}
    if(b->top||!b->clipped||galaxy_player.state==K4_HANG){
        if(galaxy_player.state!=K4_PULL){dy+=b->dy;int d=b->box.y2-(galaxy_screen_y*16+galaxy_camera_y+dy),n=abs(d)/8;if(n>48)n=48;n*=2;if(n<16)n=abs(d)<16?abs(d):16;dy+=d<0?-n:n;}
    }else galaxy_screen_y=140;
    if(b->box.y2<galaxy_camera_y+dy+32*16)dy+=b->box.y2-galaxy_camera_y-dy-32*16;
    if(b->box.y2>galaxy_camera_y+dy+168*16)dy+=b->box.y2-galaxy_camera_y-dy-168*16;
    if(dx>255)dx=255;
    if(dx< -255)dx=-255;
    if(dy>255)dy=255;
    if(dy< -255)dy=-255;
    galaxy_camera_x+=dx;galaxy_camera_y+=dy;
    int max_x=((int)galaxy_map.width*16-352)*16,max_y=((int)galaxy_map.height*16-240)*16;
    if(galaxy_camera_x<512)galaxy_camera_x=512;
    if(galaxy_camera_x>max_x)galaxy_camera_x=max_x;
    if(galaxy_camera_y<512)galaxy_camera_y=512;
    if(galaxy_camera_y>max_y)galaxy_camera_y=max_y;
    galaxy_x=galaxy_camera_x/16;galaxy_y=galaxy_camera_y/16;
}
static void galaxy_player_frame(uint8_t*fb){
    const K4Body*b=&galaxy_player.body;const uint8_t*sp=k4_sprite(&galaxy_terrain,b->frame);if(!sp)return;
    if(galaxy_sprite_cached!=b->frame){
        size_t n;if(kg_graph_chunk(&galaxy_gfx,(unsigned)b->frame,galaxy_player_pixels,sizeof(galaxy_player_pixels),&n)||n!=(size_t)kg_u16(sp)*kg_u16(sp+2)*5u)return;
        galaxy_sprite_cached=b->frame;
    }
    int x=(b->x+(int16_t)kg_u16(sp+4))/16-galaxy_x,y=(b->y+(int16_t)kg_u16(sp+6))/16-galaxy_y;
    kg_blit(fb+20*W,W,200,x,y,galaxy_player_pixels,kg_u16(sp),kg_u16(sp+2),1);
    if(galaxy_player.z<3)for(int sy=y<0?0:y;sy<200&&sy<y+(int)kg_u16(sp+2);sy++)for(int sx=x<0?0:x;sx<W&&sx<x+(int)kg_u16(sp)*8;sx++){
        unsigned wx=sx+(unsigned)galaxy_x,wy=sy+(unsigned)galaxy_y,tx=wx/16,ty=wy/16;
        if(tx>=galaxy_map.width||ty>=galaxy_map.height)continue;
        unsigned fg=galaxy_map.planes[1][(size_t)ty*galaxy_map.width+tx];
        if(!fg||(galaxy_player.z>0&&!(k4_info(&galaxy_terrain,fg,5)&128u)))continue;
        unsigned c=galaxy_fg_pixels[fg][(wy&15)*16+(wx&15)];if(c!=255)fb[(sy+20)*W+sx]=(uint8_t)(9+c);
    }
}
static void galaxy_platform_sprite(uint8_t*fb,int frame,int ux,int uy){
    unsigned slot=(unsigned)frame-484;const uint8_t*sp=k4_sprite(&galaxy_terrain,frame);if(slot>=7||!sp)return;
    int x=(ux+(int16_t)kg_u16(sp+4))/16-galaxy_x,y=(uy+(int16_t)kg_u16(sp+6))/16-galaxy_y;
    if(x>=W||y>=200||x+(int)kg_u16(sp)*8<=0||y+(int)kg_u16(sp+2)<=0)return;
    if(!galaxy_platform_ready[slot]){size_t n;if(kg_graph_chunk(&galaxy_gfx,(unsigned)frame,galaxy_platform_pixels[slot],sizeof(galaxy_platform_pixels[slot]),&n)||n!=(size_t)kg_u16(sp)*kg_u16(sp+2)*5u)return;galaxy_platform_ready[slot]=1;}
    kg_blit(fb+20*W,W,200,x,y,galaxy_platform_pixels[slot],kg_u16(sp),kg_u16(sp+2),1);
    for(int py=y<0?0:y;py<200&&py<y+(int)kg_u16(sp+2);py++)for(int px=x<0?0:x;px<W&&px<x+(int)kg_u16(sp)*8;px++){
        unsigned wx=(unsigned)(px+galaxy_x),wy=(unsigned)(py+galaxy_y),tx=wx>>4,ty=wy>>4;
        if(tx>=galaxy_map.width||ty>=galaxy_map.height)continue;
        unsigned fg=galaxy_map.planes[1][(size_t)ty*galaxy_map.width+tx];if(!fg)continue;
        unsigned c=galaxy_fg_pixels[fg][(wy&15)*16+(wx&15)];if(c!=255)fb[(py+20)*W+px]=(uint8_t)(9+c);
    }
}
static void galaxy_platforms_frame(uint8_t*fb){
    int flame=(int)(galaxy_player.ticks/4u)&1;
    for(unsigned n=0;n<galaxy_platforms.count;n++){
        K4Platform*a=&galaxy_platforms.objects[n];int x=a->body.x,y=a->body.y;
        galaxy_platform_sprite(fb,484,x,y);if(a->kind==32)continue;
        if(a->xd==1)galaxy_platform_sprite(fb,485+flame,x-16,y+48);
        else if(a->xd==-1)galaxy_platform_sprite(fb,485+flame,x+768,y+80);
        else if(a->yd==-1||flame){galaxy_platform_sprite(fb,489+flame,x+32,y+144);galaxy_platform_sprite(fb,487+flame,x+736,y+128);}
    }
}
static void galaxy_weapons_frame(uint8_t*fb){
    for(unsigned n=0;n<K4_SHOTS;n++){
        K4Shot*s=&galaxy_gun.shots[n];if(!s->active||s->body.frame<174||s->body.frame>179)continue;
        unsigned id=(unsigned)s->body.frame,slot=id-174;const uint8_t*sp=k4_sprite(&galaxy_terrain,(int)id);size_t size;
        if(!galaxy_shot_ready[slot]){if(kg_graph_chunk(&galaxy_gfx,id,galaxy_shot_pixels[slot],sizeof(galaxy_shot_pixels[slot]),&size)||size!=(size_t)kg_u16(sp)*kg_u16(sp+2)*5u)continue;galaxy_shot_ready[slot]=1;}
        int x=(s->body.x+(int16_t)kg_u16(sp+4))/16-galaxy_x,y=(s->body.y+(int16_t)kg_u16(sp+6))/16-galaxy_y;
        kg_blit(fb+20*W,W,200,x,y,galaxy_shot_pixels[slot],kg_u16(sp),kg_u16(sp+2),1);
        for(int py=y<0?0:y;py<200&&py<y+(int)kg_u16(sp+2);py++)for(int px=x<0?0:x;px<W&&px<x+(int)kg_u16(sp)*8;px++){
            unsigned wx=(unsigned)(px+galaxy_x),wy=(unsigned)(py+galaxy_y),tx=wx>>4,ty=wy>>4;
            if(tx>=galaxy_map.width||ty>=galaxy_map.height)continue;
            unsigned fg=galaxy_map.planes[1][(size_t)ty*galaxy_map.width+tx];if(!fg)continue;
            unsigned c=galaxy_fg_pixels[fg][(wy&15)*16+(wx&15)];if(c!=255)fb[(py+20)*W+px]=(uint8_t)(9+c);
        }
    }
}
static void galaxy_items_frame(uint8_t*fb,int notifications){
    for(unsigned n=0;n<K4_ITEMS;n++){
        K4Item*i=&galaxy_level_state.items[n];if(!i->active||(i->active==2)!=notifications)continue;
        unsigned frame=(unsigned)i->body.frame;if(frame<215||frame>252)continue;unsigned slot=frame-215;const uint8_t*sp=k4_sprite(&galaxy_terrain,(int)frame);
        int x=(i->body.x+(int16_t)kg_u16(sp+4))/16-galaxy_x,y=(i->body.y+(int16_t)kg_u16(sp+6))/16-galaxy_y;
        if(x>=W||y>=200||x+(int)kg_u16(sp)*8<=0||y+(int)kg_u16(sp+2)<=0)continue;
        if(!galaxy_item_ready[slot]){size_t size;if(kg_graph_chunk(&galaxy_gfx,frame,galaxy_item_pixels[slot],sizeof(galaxy_item_pixels[slot]),&size)||size!=(size_t)kg_u16(sp)*kg_u16(sp+2)*5u)continue;galaxy_item_ready[slot]=1;}
        kg_blit(fb+20*W,W,200,x,y,galaxy_item_pixels[slot],kg_u16(sp),kg_u16(sp+2),1);
        if(!notifications)for(int py=y<0?0:y;py<200&&py<y+(int)kg_u16(sp+2);py++)for(int px=x<0?0:x;px<W&&px<x+(int)kg_u16(sp)*8;px++){
            unsigned wx=(unsigned)(px+galaxy_x),wy=(unsigned)(py+galaxy_y),tx=wx>>4,ty=wy>>4;if(tx>=galaxy_map.width||ty>=galaxy_map.height)continue;
            unsigned fg=galaxy_map.planes[1][(size_t)ty*galaxy_map.width+tx];if(!fg||!(k4_info(&galaxy_terrain,fg,5)&128u))continue;
            unsigned c=galaxy_fg_pixels[fg][(wy&15)*16+(wx&15)];if(c!=255)fb[(py+20)*W+px]=(uint8_t)(9+c);
        }
    }
}
static void galaxy_player_tick(void){
    if(of_btn_pressed(OF_BTN_START)){galaxy_page=0;return;}
    uint32_t now=of_time_us(),elapsed=now-galaxy_last_us;galaxy_last_us=now;
    if(elapsed>200000u)elapsed=200000u;
    /* Apogee PIT: timer interrupt divisor 8514, one game tick per two interrupts. */
    galaxy_credit+=(uint64_t)elapsed*1193182u;
    if(of_btn_pressed(OF_BTN_A))galaxy_pending|=OF_BTN_A;
    if(of_btn_pressed(OF_BTN_Y))galaxy_pending|=OF_BTN_Y;
    if(of_btn_pressed(OF_BTN_X))galaxy_pending|=OF_BTN_X;
    while(galaxy_credit>=34056000000ull){
        galaxy_credit-=34056000000ull;
        K4Input in={!!of_btn(OF_BTN_RIGHT)-!!of_btn(OF_BTN_LEFT),!!of_btn(OF_BTN_DOWN)-!!of_btn(OF_BTN_UP),!!of_btn(OF_BTN_A)||!!(galaxy_pending&OF_BTN_A),!!of_btn(OF_BTN_Y)||!!(galaxy_pending&OF_BTN_Y),!!of_btn(OF_BTN_X)||!!(galaxy_pending&OF_BTN_X)};
        galaxy_pending=0;k4_platform_step(&galaxy_terrain,&galaxy_platforms,&galaxy_player,in,0);
        if(galaxy_player.events&K4_SHOT_EVENT)(void)k4_weapon_fire(&galaxy_gun,&galaxy_terrain,&galaxy_player);
        k4_weapons_tick(&galaxy_gun,&galaxy_terrain,galaxy_x,galaxy_y);
        k4_level_step(&galaxy_level_state,&galaxy_terrain,&galaxy_map,&galaxy_player,&galaxy_gun,0);
        galaxy_flush_level();
        K4Body*b=&galaxy_player.body;
        if(galaxy_player.state!=K4_DEAD&&b->box.y2>((int)galaxy_map.height-2)*256){b->y-=b->box.y2-((int)galaxy_map.height-2)*256;k4_kill(&galaxy_terrain,&galaxy_player);}
        if(galaxy_player.state!=K4_DEAD&&(b->box.x1<512||b->box.x2>((int)galaxy_map.width-2)*256)){galaxy_page=0;return;}
        if(galaxy_player.events&K4_DOOR_EVENT){
            galaxy_screen_y=140;
            galaxy_x=galaxy_camera_bound(b->x/16-152,(int)galaxy_map.width*16-352);
            galaxy_y=galaxy_camera_bound(b->box.y2/16-140,(int)galaxy_map.height*16-240);
            galaxy_camera_x=galaxy_x*16;galaxy_camera_y=galaxy_y*16;
        }else galaxy_camera();
        if(galaxy_player.state==K4_DEAD&&(b->y/16-galaxy_y)>220){galaxy_page=0;return;}
    }
}
static void galaxy_frame(uint8_t*fb){
    memset(fb,9,W*H);
    if(!galaxy_ready){center(fb,96,galaxy_message,6);center(fb,120,"REBUILD THE GAME PACK",7);return;}
    if(galaxy_page==0)galaxy_title_frame(fb);else{galaxy_map_frame(fb);if(galaxy_page==2){galaxy_platforms_frame(fb);galaxy_weapons_frame(fb);if(galaxy_player.z==3){galaxy_items_frame(fb,0);galaxy_player_frame(fb);}else{galaxy_player_frame(fb);galaxy_items_frame(fb,0);}galaxy_items_frame(fb,1);}}
    rect(fb,0,220,W,20,9);
    char label[48];if(galaxy_page==2)snprintf(label,sizeof(label),"SCORE %u AMMO %u LIVES %u",galaxy_level_state.score,galaxy_gun.ammo,galaxy_level_state.lives);
    center(fb,222,galaxy_page==2?label:galaxy_page?galaxy_label:"KEEN 4 DEVELOPMENT - MOVEMENT",24);
    center(fb,232,galaxy_page==2?"A JUMP X FIRE Y POGO START TITLE":galaxy_page?"A PLAY TEST  X NEXT  B TITLE":"A MOVE TEST  X MAP VIEWER",16);
}
static void galaxy_tick(void){
    if(!galaxy_ready)return;
    if(galaxy_page==2){galaxy_player_tick();return;}
    if(!galaxy_page){if(of_btn_pressed(OF_BTN_A)||of_btn_pressed(OF_BTN_START)){if(!galaxy_start_player(1))galaxy_page=1;}else if(of_btn_pressed(OF_BTN_X))galaxy_page=1;return;}
    if(of_btn_pressed(OF_BTN_A)){(void)galaxy_start_player((unsigned)galaxy_level);return;}
    if(of_btn_pressed(OF_BTN_B)){galaxy_page=0;return;}
    if(of_btn_pressed(OF_BTN_X)){if(!galaxy_load_map((unsigned)(galaxy_level+1)%20)){galaxy_ready=0;galaxy_message="KEEN 4 MAP DECODE FAILED";}return;}
    int max_x=(int)galaxy_map.width*16-W,max_y=(int)galaxy_map.height*16-200;
    if(of_btn(OF_BTN_LEFT))galaxy_x-=4;
    if(of_btn(OF_BTN_RIGHT))galaxy_x+=4;
    if(of_btn(OF_BTN_UP))galaxy_y-=4;
    if(of_btn(OF_BTN_DOWN))galaxy_y+=4;
    if(galaxy_x>max_x)galaxy_x=max_x;
    if(galaxy_y>max_y)galaxy_y=max_y;
    if(galaxy_x<0)galaxy_x=0;
    if(galaxy_y<0)galaxy_y=0;
}
#endif
