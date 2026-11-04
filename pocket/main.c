/*
 * RetroDriven Commander Keen Pocket v1.0.0 - Keen 1-6 and Keen Dreams engines.
 * SPDX-License-Identifier: MIT
 * Behavioral references: bundled Chocolate Keen and Omnispeak sources.
 * Game data is loaded from keenpack.bin through APF data slot 4.
 */
#include "of.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "keen_ega.h"
#include "keen_map.h"
#include "keen_galaxy.h"
#include "keen4_motion.h"
#include "keen4_weapon.h"
#include "keen4_level.h"
#include "keen4_platform.h"
#include "keen_motion.h"
#include "keen_bounds.h"
#include "keen_weapon.h"
#include "keen_vort_motion.h"
#include "keen_anim.h"
#include "keen_sprite.h"
#include "keen_sound.h"
#include "keen_sfx.h"
#include "keen_tileinfo.h"
#include "keen_gameplay.h"
#include "keen_background.h"
#include "keen_actors.h"
#include "keen2_actors.h"
#include "keen3_actors.h"
#include "keen3_world.h"
#include "keen3_cinema.h"
#include "keen_death.h"
#include "keen_status.h"
#include "keen_save.h"
#include "keen2_save.h"
#include "keen3_save.h"
#include "keen_manual_save.h"
#include "keen_core_options.h"
#include "keen_view.h"
#include "keen_text.h"
#include "keen_image.h"
#include "keen_finale.h"
#include "keen2_finale.h"
#include "keen_scores.h"
#ifndef KEEN_STATE_FILE
#define KEEN_STATE_FILE "RetroDrivenK1_state.sav"
#endif
#ifndef KEEN2_STATE_FILE
#define KEEN2_STATE_FILE "RetroDrivenK2_state.sav"
#endif
_Static_assert(sizeof(KEEN2_STATE_FILE)-1 <= 23,"Keen 2 save filename exceeds SDK registry limit");
_Static_assert(sizeof(KEEN_STATE_FILE)-1 <= 23, "Pocket save filename exceeds SDK registry limit");

#define W 320
#define H 240
static int vort_view_height(void){return keen_core_view_enhanced()?240:200;}
#define COUNT 7
static const uint32_t base_colors[]={
        0x0C1422,0x172338,0x2B4362,0x3D536D,0xAE2E36,
        0xF2CB51,0xFFFFFF,0xB9CEE0,0x7FA6CA,
        /* Indices 9-24: the classic sixteen EGA colors. */
        0x000000,0x0000AA,0x00AA00,0x00AAAA,0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,
        0x555555,0x5555FF,0x55FF55,0x55FFFF,0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF
    };
/* Vorticons uses RGB bits 0-2 and intensity bit 4. */
static uint32_t ega_palette_color(unsigned color){return base_colors[9u+(color&7u)+((color&16u)>>1)];}
#define MAX_FILES 256
#define RECORD_BYTES 48
#define HEADER_BYTES 16

/* Small fixed 5x7 bitmap font; column bits (low bit = top pixel). */
typedef struct { char ch; uint8_t rows[7]; } Glyph;
static const Glyph glyphs[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{15,16,16,16,16,16,15}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{15,16,16,19,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{31,4,4,4,4,4,31}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,4,4,4}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {' ',{0,0,0,0,0,0,0}}, {':',{0,4,4,0,4,4,0}},
 {'-',{0,0,0,31,0,0,0}}, {'.',{0,0,0,0,0,12,12}},
 {'/',{1,1,2,4,8,16,16}}, {'!',{4,4,4,4,4,0,4}},
 {'?',{14,17,1,2,4,0,4}}
};


static const char *const titles[COUNT] = {
 "KEEN 1 - MAROONED ON MARS", "KEEN 2 - THE EARTH EXPLODES",
 "KEEN 3 - KEEN MUST DIE", "KEEN 4 - SECRET OF THE ORACLE",
 "KEEN 5 - ARMAGEDDON MACHINE", "KEEN 6 - ALIENS ATE MY BABYSITTER",
 "KEEN DREAMS"
};

/* Asset index for the SD data pack. */
typedef struct {
    uint32_t episode, offset, size, signature;
    char name[24];
} DataFile;
static DataFile entries[MAX_FILES];
static unsigned file_count, by_episode[COUNT];
static uint32_t pack_bytes;
static int pack_state; /* 0 = not loaded, 1 = readable, -1 = error */
static const char *pack_message = "NO KEENPACK.BIN IN SLOT 4";
static int selected, detail, cursor, check_state; /* check: 0 not checked, 1 OK, -1 fail */
static uint32_t checked_first;
static void rect(uint8_t *fb,int x,int y,int w,int h,uint8_t c);
static void center(uint8_t *fb,int y,const char *s,uint8_t color);
static void save_notice_frame(uint8_t *fb,int bottom,const char *s,int failed);
static void number(char *dst,unsigned n);
static uint8_t latch_file[KEEN_EGA_MAX];
static uint8_t latch_expanded[KEEN_EGA_MAX];
static KeenEgaInfo ega_info;
static uint32_t ega_len,status_font_offset;
static unsigned status_font_count;
static uint8_t status_font_atlas[256u*64u];
static unsigned tile_page;
static int tile_mode;
static const char *tile_message="NOT TESTED";
static int ega_state; /* 0 not loaded, 1 decoded, -1 error */

/* Map data and tile graphics share the asset pack. */
static uint8_t level_compressed[KEEN_MAP_CAP];
static uint8_t level_unpacked[KEEN_MAP_CAP];
static KeenMapInfo level_info;
static int map_mode, map_state, map_id=80, map_x, map_y, show_objects=1;
static uint8_t map_cached[W*H];
static int map_dirty=1;
static const char *map_message="NOT TESTED";
/* Vorticons gameplay state and collision mode. */
static int play_mode, collision_mode=1, collision_exact, player_spawned;
/* Gameplay uses the 240-line canvas with an optional HUD overlay. */
static int instance_episode;
static KeenActors2 level_actors2;
static KeenActors3 level_actors3;
static KeenWorld3 world3;
static KeenActors world_random3;
static KeenCinema3 cinema3;static uint8_t cinema3_data[9237];
static void keen3_scene_begin(unsigned kind,unsigned ship);
static void keen3_scene_update(void);
static void keen3_scene_frame(uint8_t*fb);
static const char *episode_name(const char *name){
    static char names[8][32];static unsigned next;char *out=names[(next++)&7u];
    snprintf(out,32,"%s",name);size_t n=strlen(out);if(n&&out[n-1]=='1')out[n-1]=(char)('1'+selected);return out;
}
static int campaign_namespace_ready(void){
#if (defined(__riscv) && !defined(OF_PC)) || defined(KEEN_INSTANCE_BINDING_TEST)
    return selected==instance_episode;
#else
    return 1;
#endif
}
#ifndef KEEN3_STATE_FILE
#define KEEN3_STATE_FILE "RetroDrivenK3_state.sav"
#endif
static const char *campaign_state_path(void){return selected==2?KEEN3_STATE_FILE:selected==1?KEEN2_STATE_FILE:KEEN_STATE_FILE;}
static void campaign_encode(uint8_t*p,const KeenSave*v){if(selected==2)k3sv_encode(p,v);else if(selected==1)k2sv_encode(p,v);else ksv_encode(p,v);}
static int campaign_decode(const uint8_t*p,KeenSave*v){return selected==2?k3sv_decode(p,v):selected==1?k2sv_decode(p,v):ksv_decode(p,v);}
static int campaign_state_read(const char*path,KeenSave*v){return selected==2?keen3_save_read(path,v):selected==1?keen2_save_read(path,v):keen_save_read(path,v);}
static int campaign_state_write(const char*path,const KeenSave*v){
    if(!campaign_namespace_ready())return 0;
    return selected==2?keen3_save_write(path,v):selected==1?keen2_save_write(path,v):keen_save_write(path,v);
}
static int play_hud=0;
static unsigned hud_required_keys;
static unsigned hud_map_keys(const uint8_t *map,const KeenMapInfo *info);
static void corner_hud_frame(uint8_t *fb);
static void foreground_tiles_frame(uint8_t *fb,int world);
static int campaign_active,world_mode,campaign_complete;
static KeenFinale finale;static uint8_t finale_data[5516];
static KeenFinale2 finale2;static uint8_t finale2_data[4312];
static void keen2_finale_begin(int failed,int shake);
static void finale_begin(void);
static void finale_update(void);
static void finale_frame(uint8_t*fb);
static unsigned completed_levels,world_frame_index=36,world_pending;
static int world_x,world_y;
static uint8_t world_data[34];static int world_data_ready;
static unsigned teleport_phase,teleport_pending,teleport_source,teleport_target;
static int teleport_x,teleport_y;
static void load_world_data(void);
static uint8_t special_data[1864];static int special_ready;
static unsigned play_notice,notice_step,notice_ready,notice_wait_release;
static int notice_tile_x,notice_tile_y;static const char *notice_text;
static KeenSfxClock notice_clock;
static void load_special_data(void);
static int game_lights=1;
static void set_game_lights(int on);
static int show_statue(int x,int y);
static void show_lives(void);
static int notice_tick(void);
static void notice_frame(uint8_t *fb);
static void scene_tiles_frame(uint8_t *fb,int world);
static void world_frame(uint8_t *fb);
static void world_camera(void);
static void return_world(int completed);
static void forget_current_part(void);
static void update_world(void);
static void start_campaign(void);
static int game_menu_visible,game_menu_page,game_menu_selection;
static int episode_menu_mode,episode_menu_selection,text_kind;
static int text_scroll_direction;static uint32_t text_scroll_next_us;
static uint32_t startup_loading_started_us;
static uint8_t image_planes[32768],episode_scores[204];
static unsigned preview_page;
static uint8_t ordering_objects[20];
static int ordering_ready;
static unsigned ordering3_wait,ordering3_time,ordering3_flash;static int ordering3_x,ordering3_vx;
static uint16_t ordering_jump[18];
static unsigned ordering_j1,ordering_j2,ordering_frames,ordering_offset;
static uint32_t ordering_started_us;
static unsigned ordering_random(unsigned max){
    unsigned bit=max,mask=65535;while(!(bit&32768)){bit<<=1;mask>>=1;}
    uint16_t value=(uint16_t)(ordering_jump[ordering_j1]+ordering_jump[ordering_j2]+1u);
    ordering_jump[ordering_j1]=value;value=(uint16_t)(value+ordering_jump[0]);ordering_jump[0]=value;
    ordering_j1=ordering_j1==1?17:ordering_j1-1;ordering_j2=ordering_j2==1?17:ordering_j2-1;
    unsigned result=value&mask;return result>max?result>>1:result;
}

static uint32_t autosave_notice_us;
static int autosave_prompt_pending,autosave_prompt_ready;
static KeenSave autosave_prompt_save;
static int autosave_result,autosave_present;static unsigned autosave_after_teleport;
static void campaign_autosave(void);
static void campaign_autosave_mode(unsigned mode);
static int autosave_prompt_tick(void);
static void autosave_prompt_frame(uint8_t*fb);
static int campaign_load_auto(void);
static unsigned score_flow_active,score_wait_release,score_cursor;
static int score_rank;static char score_name[13];static KeenSfxClock score_clock;
static uint8_t score_backdrop[W*H];
static void score_flow_begin(void);
static void score_flow_tick(void);
static void score_flow_frame(uint8_t*fb);
static int scores_load_table(void);
static int episode_image_open(const char*name);
static int episode_scores_open(void);
static const char *episode_menu_message;
static uint8_t text_document[8192];static KeenTextLine text_lines[512];
static size_t text_document_size;static unsigned text_line_count,text_top;
static int ordering2_x,ordering2_y,ordering2_vx,ordering2_vy;
static int episode_text_open(const char *name);
static void episode_menu_open(void);
static void episode_menu_tick(void);
static void episode_menu_frame(uint8_t *fb);
static void game_menu_open(void);
static void game_menu_tick(void);
static void game_menu_frame(uint8_t *fb);
static int play_level=1, play_spawn_x, play_spawn_y;
static unsigned play_background_tile;
static int exit_active,exit_tile_x,exit_tile_y;
static int32_t exit_fx;
static KeenMotion player;
static uint8_t tile_atlas[1024u*256u];
static unsigned tile_atlas_count;
static uint8_t tile_flags[6u*1024u];
static uint16_t menu_tile_frames[4][1024];
static KeenSfxClock menu_animation_clock;
static int16_t tile_behaviors[1024u];
static KeenGameplay gameplay;
static unsigned original_tiles;
static uint8_t air_flags[1024u];

/* Load Vorticons five-plane sprite data on first use. */
static uint8_t sprite_head_file[64u*1024u];
static uint8_t sprite_raw[KEEN_SPR_MAX_BYTES];
static uint8_t sprite_pixels[KEEN_SPR_MAX_BYTES];
static size_t sprite_head_len, sprite_len;
static KeenSpriteInfo sprite_info;
static int sprite_mode, sprite_state, sprite_episode=-1;
static unsigned sprite_index, player_sprite_index;
static int player_sprite_enabled;
/* Manual frame selection for sprite diagnostics. */
static int auto_player_animation;
static int pogo_mode;
static KeenAnimation player_animation;
static const char *sprite_message="NOT TESTED";
static void load_sprites(void);
static void sprite_frame(uint8_t *fb);
static void draw_masked_sprite(uint8_t *fb,int x,int y,unsigned idx,int flip);

static int episode_file(int episode, const char *filename);
static int read_asset(int index, uint8_t *dest, size_t cap, size_t *size);
/* One persistent DMA buffer for Vorticons PC-speaker effects. Vorticons has no background
 * music. */
static uint8_t sound_file[KS_MAX_FILE];
static KeenSoundBank sound_bank;
static int16_t sound_pcm[KS_PCM_CAP];
static size_t sound_len;
static unsigned sound_selected=0;
static int sound_mode, sound_state, sound_episode=-1;
static int sound_enabled=1, sound_voice=-1, sound_priority=-1;
static unsigned sound_active_id=0xffffffffu;
static size_t sound_samples,sound_pcm_start;
static int paused_sound_priority=-1;
static size_t paused_sound_offset;
static const char *sound_message="NOT LOADED";
/* Poll completion while idle so finished effects release their priority. */
static void sound_update(void){
    if(sound_voice>=0)of_mixer_set_volume(sound_voice,(int)(220u*keen_core_audio_gain()/256u));
    of_mixer_pump();
    uint32_t ended=of_mixer_poll_ended();
    if(sound_voice>=0 &&
       ((sound_voice<32 && (ended&(1u<<(unsigned)sound_voice))) ||
        !of_mixer_voice_active(sound_voice))){
        sound_voice=-1;sound_priority=-1;
    }
}
static void sound_stop(void){
    if(sound_voice>=0){
        of_mixer_stop(sound_voice);sound_voice=-1;
        /* Drain the completion bit after stopping a voice, before reusing it. */
        (void)of_mixer_poll_ended();
    }
    sound_priority=-1;
}
static void sound_load(void){
    sound_stop();sound_state=-1;sound_episode=selected;sound_selected=0;
    sound_len=0;
    if(selected>2){sound_message="SFX FOR KEEN 4-6 NOT READY";return;}
    if(pack_state!=1){sound_message="GAME ARCHIVE NOT LOADED";return;}
    char name[24];
    snprintf(name,sizeof(name),"SOUNDS.CK%d",selected+1);
    int index=episode_file(selected,name);
    if(index<0){sound_message="SOUNDS MISSING - REBUILD PACK";return;}
    if(read_asset(index,sound_file,sizeof(sound_file),&sound_len)){
        sound_message="SOUND FILE TOO BIG OR UNREADABLE";return;
    }
    if(ks_parse(sound_file,sound_len,&sound_bank)){
        sound_message="UNRECOGNIZED SOUNDS FORMAT";return;
    }
    sound_state=1;sound_message="ORIGINAL PC SPEAKER SOUNDS";
}
static int sound_play(unsigned number){
    sound_update();
    if(!sound_enabled)return 0;
    if(sound_episode!=selected||sound_state==0)sound_load();
    if(sound_state!=1||number>=sound_bank.count)return -1;
    int priority=(int)sound_bank.entries[number].priority;
    if(!ks_priority_allows(sound_voice>=0 && of_mixer_voice_active(sound_voice),
                           sound_priority,(unsigned)priority))return 0;
    KeenSoundStatus status=ks_status(sound_file,sound_len,&sound_bank,number);
    if(status==KS_UNUSED){sound_message="UNUSED SOUND SLOT";return 0;}
    if(status==KS_BAD || status==KS_TRUNCATED){
        sound_message="INVALID OR UNTERMINATED SOUND";return -1;
    }
    /* Silent effects still occupy the channel for their original duration. */
    sound_stop(); /* The buffer must not be modified while DMA reads it. */
    int truncated=0;
    size_t n=ks_render(sound_file,sound_len,&sound_bank,number,
                       sound_pcm,KS_PCM_CAP,&truncated);
    if(!n){sound_message="NO AUDIO DATA FOR SOUND";return -1;}
    sound_samples=n;sound_pcm_start=0;sound_active_id=number;
    sound_voice=of_mixer_play((const uint8_t*)sound_pcm,(uint32_t)n,KS_RATE,priority,(int)(220u*keen_core_audio_gain()/256u));
    if(sound_voice<0){sound_message="POCKET MIXER VOICE UNAVAILABLE";return -1;}
    sound_priority=priority;
    sound_message=truncated?"PLAYING (TRUNCATED AT 8 SEC)":"PLAYING ORIGINAL SOUND";
    return 1;
}
/* Resolve sound labels from the extracted bank. */
static int sound_play_named(const char *name){
    if(sound_episode!=selected || sound_state==0)sound_load();
    if(sound_state!=1)return -1;
    int id=ks_find_by_name(&sound_bank,name);
    return id<0?-1:sound_play((unsigned)id);
}
static void sound_frame(uint8_t *fb){
    rect(fb,0,0,W,H,0);rect(fb,0,0,W,4,4);
    center(fb,12,"KEEN PC SPEAKER SOUNDS",5);
    if(sound_state!=1){
        center(fb,94,sound_message,4);
        center(fb,139,"X RELOAD  B BACK",7);return;
    }
    char label[70],n[16];
    center(fb,49,"ORIGINAL DOS EFFECT BANK",6);
    number(n,sound_selected);strcpy(label,"SOUND ");strcat(label,n);
    number(n,sound_bank.count-1u);strcat(label," / ");strcat(label,n);
    center(fb,77,label,7);
    const KeenSoundEntry *e=&sound_bank.entries[sound_selected];
    center(fb,100,e->name[0]?e->name:"UNNAMED",5);
    number(n,e->priority);strcpy(label,"PRIORITY ");strcat(label,n);
    center(fb,123,label,7);
    KeenSoundStatus entry_status=ks_status(sound_file,sound_len,&sound_bank,sound_selected);
    const char *state=entry_status==KS_AUDIBLE?"AUDIBLE":
       entry_status==KS_SILENT?"SILENT":entry_status==KS_UNUSED?"UNUSED":
       entry_status==KS_TRUNCATED?"LONG EFFECT":"INVALID";
    center(fb,144,state,5);
    center(fb,158,sound_message,6);
    center(fb,175,sound_enabled?"A PLAY  Y MUTE":"A PLAY  Y UNMUTE",5);
    center(fb,197,"LEFT RIGHT CHOOSE SOUND",8);
    center(fb,216,"X STOP - B BACK",7);
}

static int play_camera_x, play_camera_y;
static void start_play(void);
static void play_frame(uint8_t *fb);
static void update_play(void);
static void build_collision(void);
static void load_map(void);
static void map_frame(uint8_t *fb);


static int episode_file(int episode, const char *filename){
    for(unsigned i=0;i<file_count;i++)
        if(entries[i].episode==(uint32_t)(episode+1) && strcmp(entries[i].name,filename)==0)return (int)i;
    return -1;
}
static int read_asset(int index, uint8_t *dest, size_t cap, size_t *size){
    if(index<0 || entries[index].size>cap || !size)return -1;
    FILE *f=fopen("keenpack.bin","rb");
    if(!f)return -1;
    int ok=fseek(f,(long)entries[index].offset,SEEK_SET)==0 &&
        fread(dest,1,entries[index].size,f)==entries[index].size;
    fclose(f);
    if(!ok)return -1;
    *size=entries[index].size;
    return 0;
}
static int read_asset_prefix(int index,uint8_t *dest,size_t need){
    if(index<0 || entries[index].size<need)return -1;
    FILE *f=fopen("keenpack.bin","rb");
    if(!f)return -1;
    int ok=fseek(f,(long)entries[index].offset,SEEK_SET)==0 &&
        fread(dest,1,need,f)==need;
    fclose(f);
    return ok?0:-1;
}
#include "keen4_preview.h"
static int episode_text_open(const char *name){
    size_t size=0;int index=episode_file(selected,name);
    if(index<0||read_asset(index,text_document,sizeof(text_document),&size))return 0;
    if(!strncmp(name,"ORDER.CK",8)){
        uint8_t *end=memchr(text_document,26,size);ordering_ready=0;
        if(end && (size_t)(text_document+size-end)>=25u && !memcmp(end+1,"KO01",4)){
            memcpy(ordering_objects,end+5,20);ordering_ready=1;
        }
        memset(ordering_jump,0,sizeof(ordering_jump));
        uint8_t actor_data[294];size_t actor_len=0;
        int actor_index=episode_file(selected,episode_name("ACTORS.CK1"));
        if(actor_index>=0&&!read_asset(actor_index,actor_data,sizeof(actor_data),&actor_len)&&actor_len==294)
            for(unsigned i=1;i<18;i++)ordering_jump[i]=keen_le16(actor_data+258+i*2);
        ordering_j1=17;ordering_j2=5;ordering_frames=ordering_offset=0;ordering_started_us=of_time_us();
        ordering2_x=0;ordering2_y=4;ordering2_vx=2;ordering2_vy=0;ordering3_wait=100;ordering3_time=ordering3_flash=0;ordering3_x=ordering3_vx=0;
    }
    text_scroll_direction=0;
    text_document_size=keen_text_prepare(text_document,size);
    text_line_count=keen_text_lines(text_document,text_document_size,text_lines,512,38);text_top=0;
    if(!text_line_count)return 0;
    episode_menu_mode=3;text_kind=!strncmp(name,"PREVIEWS.CK",11)?5:0;return 1;
}
static int episode_help_open(void){
    if(!episode_text_open(episode_name("HELPTEXT.CK1")))return 0;
    const char *anchor=selected==1?"When you are on the blueprint view":"When you enter a city";
    size_t anchor_length=strlen(anchor);
    size_t tail=text_document_size;
    for(size_t i=0;i+anchor_length<=text_document_size;i++)
        if(!memcmp(text_document+i,anchor,anchor_length)){tail=i;break;}
    static const char controls[]=
        "~         COMMANDER KEEN HELP\r\n"
        "\r\n"
        "D-pad         Move Keen\r\n"
        "A             Jump / choose\r\n"
        "B             Fire raygun\r\n"
        "Y             Pogo / use switch\r\n"
        "X             Score and inventory\r\n"
        "Start         Game menu\r\n"
        "Select        Help on title screen\r\n"
        "\r\n"
        "Use Start for Save, Load and Sound.\r\n"
        "On the map, A or Y enters a place.\r\n"
        "\r\n"
        "~         READING THIS HELP\r\n"
        "D-pad Up/Down scrolls one line.\r\n"
        "D-pad Left/Right changes page.\r\n"
        "B returns to the title menu.\r\n"
        "\r\n";
    size_t prefix=sizeof(controls)-1,remain=text_document_size-tail;
    if(tail==text_document_size||prefix+remain>sizeof(text_document))return 0;
    memmove(text_document+prefix,text_document+tail,remain);memcpy(text_document,controls,prefix);
    text_document_size=prefix+remain;text_line_count=keen_text_lines(text_document,text_document_size,text_lines,512,38);
    text_top=0;text_kind=3;return 1;
}

static void load_tiles(void){
    ega_state=-1; tile_page=0;status_font_count=0;
    if(selected>2){tile_message="ONLY KEEN 1-3 SUPPORTED";return;}
    if(pack_state!=1){tile_message="GAME PACK UNAVAILABLE";return;}
    char name[24];
    name[0]=0;strcpy(name,"EGAHEAD.CK");name[10]=(char)('1'+selected);name[11]=0;
    int header_index=episode_file(selected,name);
    strcpy(name,"EGALATCH.CK");name[11]=(char)('1'+selected);name[12]=0;
    int latch_index=episode_file(selected,name);
    if(header_index<0 || latch_index<0){tile_message="EGAHEAD OR EGALATCH MISSING";return;}
    uint8_t head[48];size_t raw_len=0, expanded_len=0;
    if(read_asset_prefix(header_index,head,sizeof(head)) ||
       keen_parse_egahead(head,sizeof(head),&ega_info)){
        tile_message="EGAHEAD FORMAT ERROR";return;
    }
    if(read_asset(latch_index,latch_file,sizeof(latch_file),&raw_len)){
        tile_message="EGALATCH READ FAILED / TOO BIG";return;
    }
    if(ega_info.compression & 2){
        if(keen_lzw_decode(latch_file,raw_len,latch_expanded,sizeof(latch_expanded),&expanded_len)){
            tile_message="LZW DECODE ERROR";return;
        }
    }else{
        if(raw_len>sizeof(latch_expanded)){tile_message="LATCH TOO BIG";return;}
        memcpy(latch_expanded,latch_file,raw_len);expanded_len=raw_len;
    }
    if(expanded_len != (size_t)ega_info.plane_size*4u){
        tile_message="EGA PLANE SIZE MISMATCH";return;
    }
    ega_len=(uint32_t)expanded_len;
    status_font_offset=keen_le32(head+18);
    unsigned fonts=keen_le16(head+16);
    if(fonts>=256 && (uint64_t)status_font_offset+(uint64_t)fonts*8<=ega_info.plane_size)status_font_count=fonts;
    if(status_font_count>=256)for(unsigned ch=0;ch<256;ch++)for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)
        status_font_atlas[ch*64u+y*8u+x]=(uint8_t)(9+keen_status_font_pixel(latch_expanded,ega_len,ega_info.plane_size,status_font_offset,status_font_count,ch,x,y));
    tile_atlas_count=ega_info.tiles<1024u ? ega_info.tiles : 1024u;
    for(unsigned t=0;t<tile_atlas_count;t++)
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++)
            tile_atlas[t*256u+y*16u+x]=(uint8_t)(9+keen_tile_pixel(
                latch_expanded,ega_len,&ega_info,t,x,y));
    ega_state=1;tile_message="ORIGINAL EGA TILES DECODED";
}
static uint8_t original_pixel(unsigned tile,unsigned x,unsigned y){
    if(tile<tile_atlas_count)return tile_atlas[tile*256u+y*16u+x];
    if(tile>=ega_info.tiles)return 13;
    return (uint8_t)(9+keen_tile_pixel(latch_expanded,ega_len,&ega_info,tile,x,y));
}
static void tile_frame(uint8_t *fb){
    char line[80], num[16];
    rect(fb,0,0,W,H,0);
    rect(fb,0,0,W,5,4);
    center(fb,10,"KEEN ORIGINAL EGA TILES",5);
    if(ega_state!=1){
        center(fb,88,tile_message,4);
        center(fb,115,"B BACK - X RETRY",7);
        return;
    }
    const unsigned columns=16, rows=9, per_page=columns*rows;
    unsigned first=tile_page*per_page;
    for(unsigned t=0;t<per_page && first+t<ega_info.tiles;t++){
        unsigned px=32+(t%columns)*16,py=31+(t/columns)*16;
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
            fb[(py+y)*W+px+x]=original_pixel(first+t,x,y);
        }
    }
    number(num,ega_info.tiles);
    strcpy(line,"TILES ");strcat(line,num);
    number(num,tile_page+1);strcat(line,"  PAGE ");strcat(line,num);
    center(fb,188,line,5);
    center(fb,204,(ega_info.compression&2)?"KEEN LZW EGA":"RAW 4-PLANE EGA",7);
    center(fb,221,"LEFT RIGHT PAGE - B BACK",8);
}


static void load_sprites(void){
    sprite_state=-1; sprite_episode=selected; sprite_index=0;
    player_sprite_enabled=0; player_sprite_index=0;
    auto_player_animation=0;
    sprite_head_len=0; sprite_len=0;
    if(selected>2){sprite_message="ONLY KEEN 1-3 SUPPORTED";return;}
    if(pack_state!=1){sprite_message="GAME ARCHIVE UNAVAILABLE";return;}
    char name[24];
    snprintf(name,sizeof(name),"EGAHEAD.CK%d",selected+1);
    int header=episode_file(selected,name);
    snprintf(name,sizeof(name),"EGASPRIT.CK%d",selected+1);
    int sprites=episode_file(selected,name);
    if(header<0||sprites<0){sprite_message="EGAHEAD OR EGASPRIT MISSING";return;}
    if(read_asset(header,sprite_head_file,sizeof(sprite_head_file),&sprite_head_len)||
       keen_sprite_parse_head(sprite_head_file,sprite_head_len,&sprite_info)){
        sprite_message="SPRITE HEADER INVALID";return;
    }
    size_t raw_len=0;
    if(read_asset(sprites,sprite_raw,sizeof(sprite_raw),&raw_len)){
        sprite_message="EGASPRIT READ ERROR";return;
    }
    if(sprite_info.compression&1){
        if(keen_lzw_decode(sprite_raw,raw_len,sprite_pixels,sizeof(sprite_pixels),&sprite_len)){
            sprite_message="SPRITE LZW DECODE ERROR";return;
        }
    }else{
        if(raw_len>sizeof(sprite_pixels)){sprite_message="SPRITE DATA TOO LARGE";return;}
        memcpy(sprite_pixels,sprite_raw,raw_len);sprite_len=raw_len;
    }
    if(sprite_len!=(size_t)sprite_info.plane_size*5u){
        sprite_message="SPRITE PLANE SIZE MISMATCH";return;
    }
    /* Use a named player frame when available. */
    for(unsigned i=0;i<sprite_info.sprite_count;i++){
        KeenSprite frame;
        if(!keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,i,&frame) &&
           frame.w<=64 && frame.h<=64 &&
           frame.name[0]=='K' && frame.name[1]=='E' &&
           frame.name[2]=='E' && frame.name[3]=='N'){
            sprite_index=i;player_sprite_index=i;player_sprite_enabled=1;break;
        }
    }
    /* Enable animation only when the episode profile references valid frames. */
    unsigned required[18];
    unsigned required_count=keen_animation_required_frames((unsigned)selected,required);
    if(required_count){
        int ok=1;
        for(unsigned k=0;k<required_count;k++){
            KeenSprite candidate;
            if(required[k]>=sprite_info.sprite_count ||
               keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,
                               required[k],&candidate) ||
               (required[k]<8u && !keen_animation_walk_name(required[k],candidate.name)))
                {ok=0;break;}
        }
        if(ok){
            player_sprite_enabled=1;
            player_sprite_index=0;
            auto_player_animation=1;
        }
    }
    sprite_state=1;sprite_message="ORIGINAL MASKED SPRITES READY";
}
static void draw_masked_sprite(uint8_t *fb,int x,int y,unsigned idx,int flip){
    if(sprite_state!=1)return;
    KeenSprite sp;
    if(keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,idx,&sp))return;
    for(unsigned yy=0;yy<sp.h;yy++){
        int py=y+(int)yy;
        if(py<0||py>=H)continue;
        for(unsigned xx=0;xx<sp.w;xx++){
            int px=x+(int)xx;
            if(px<0||px>=W)continue;
            int color=keen_sprite_pixel(sprite_pixels,sprite_len,&sprite_info,&sp,
                                      flip?(sp.w-xx-1u):xx,yy);
            if(color>=0)fb[py*W+px]=(uint8_t)(9+color);
        }
    }
}
static void sprite_frame(uint8_t *fb){
    rect(fb,0,0,W,H,0);rect(fb,0,0,W,4,4);
    center(fb,12,"KEEN ORIGINAL EGA SPRITES",5);
    if(sprite_state!=1){
        center(fb,92,sprite_message,4);
        center(fb,119,"X RETRY  B BACK",7);return;
    }
    KeenSprite sp;
    if(keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,sprite_index,&sp)){
        center(fb,92,"INVALID SPRITE ENTRY",4);
    }else{
        /* Draw checkerboard background to visibly confirm transparency. */
        for(int yy=40;yy<172;yy+=8)for(int xx=76;xx<244;xx+=8)
            rect(fb,xx,yy,8,8,(((xx/8)+(yy/8))&1)?3:2);
        draw_masked_sprite(fb,160-(int)sp.w/2,104-(int)sp.h/2,sprite_index,0);
        center(fb,177,sp.name,6);
    }
    char line[64],num[16];
    strcpy(line,"SPRITE ");number(num,sprite_index);strcat(line,num);
    strcat(line," OF ");number(num,sprite_info.sprite_count);strcat(line,num);
    center(fb,193,line,7);
    center(fb,207,auto_player_animation?"AUTO EPISODE FRAMES":
           (player_sprite_enabled && player_sprite_index==sprite_index?
            "MANUAL PLAYER FRAME":"UP SET MANUAL PLAYER FRAME"),5);
    center(fb,222,"LEFT RIGHT BROWSE  DOWN AUTO",8);
    center(fb,233,"X RELOAD  B BACK",7);
}

static const int browsable_levels[]={80,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,81,90};
static void load_map(void){
    map_state=-1; map_x=0; map_y=0; map_dirty=1;
    if(selected>2){map_message="ONLY KEEN 1-3 MAPS SUPPORTED";return;}
    if(pack_state!=1){map_message="GAME PACK UNAVAILABLE";return;}
    /* Load tile artwork for the selected episode. */
    if(ega_state!=1)load_tiles();
    if(ega_state!=1){map_message=tile_message;return;}
    char filename[24];
    /* Original Vorticons world maps are LEVEL80.CK1/2/3. */
    snprintf(filename,sizeof(filename),"LEVEL%02d.CK%d",map_id,selected+1);
    int index=episode_file(selected,filename);
    if(index<0){map_message="LEVEL FILE NOT FOUND";return;}
    size_t compressed_size=0;
    if(read_asset(index,level_compressed,sizeof(level_compressed),&compressed_size)){
        map_message="LEVEL READ ERROR OR TOO LARGE";return;
    }
    if(keen_map_decode(level_compressed,compressed_size,
                       level_unpacked,sizeof(level_unpacked),&level_info)){
        map_message="LEVEL RLEW / HEADER INVALID";return;
    }
    map_state=1;
    map_message="ORIGINAL LEVEL TILES LOADED";
    build_collision();
}
static void next_map(void){
    unsigned count=sizeof(browsable_levels)/sizeof(browsable_levels[0]);
    unsigned start=0;
    for(unsigned i=0;i<count;i++)if(browsable_levels[i]==map_id){start=i;break;}
    for(unsigned n=1;n<=count;n++){
        int candidate=browsable_levels[(start+n)%count];
        char name[24];
        snprintf(name,sizeof(name),"LEVEL%02d.CK%d",candidate,selected+1);
        if(episode_file(selected,name)>=0){map_id=candidate;load_map();return;}
    }
    map_message="NO OTHER LEVEL FILES";
}
static void map_frame(uint8_t *fb){
    rect(fb,0,0,W,H,0);
    rect(fb,0,0,W,3,4);
    char line[80],num[16];
    if(map_state!=1){
        center(fb,40,"KEEN ORIGINAL LEVEL VIEWER",5);
        center(fb,94,map_message,4);
        center(fb,130,"B BACK - X NEXT LEVEL",7);
        return;
    }
    /* 320x192 viewport: 20x12 native-resolution original 16px tiles. */
    for(unsigned ty=0;ty<12;ty++)for(unsigned tx=0;tx<20;tx++){
        unsigned xx=(unsigned)map_x+tx, yy=(unsigned)map_y+ty;
        if(xx>=level_info.width||yy>=level_info.height)continue;
        unsigned tile=keen_map_tile(level_unpacked,&level_info,xx,yy,0);
        unsigned px=tx*16u, py=24u+ty*16u;
        if(tile<ega_info.tiles){
            for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
                fb[(py+y)*W+px+x]=original_pixel(tile,x,y);
            }
        }else{
            /* Draw invalid tile IDs in magenta. */
            rect(fb,(int)px,(int)py,16,16,13);
        }
        if(show_objects && keen_map_tile(level_unpacked,&level_info,xx,yy,1)!=0){
            rect(fb,(int)px+12,(int)py,4,4,5);
        }
    }
    strcpy(line,"KEEN ");number(num,(unsigned)selected+1);strcat(line,num);
    strcat(line,"  LEVEL ");number(num,(unsigned)map_id);strcat(line,num);
    strcat(line,"  ");number(num,level_info.width);strcat(line,num);
    strcat(line,"X");number(num,level_info.height);strcat(line,num);
    center(fb,10,line,6);
    center(fb,218,"DPAD PAN  X NEXT  Y OBJECTS",5);
    center(fb,231,"A MOVE TEST  -  B BACK",7);
}


/* TILEINFO contains six arrays of 16-bit collision properties. The extractor unpacks LZEXE
 * data first; previews use a fallback mask when TILEINFO is absent. */
static void build_collision(void){
    for(unsigned p=0;p<4;p++)for(unsigned t=0;t<1024;t++)menu_tile_frames[p][t]=(uint16_t)t;
    memset(tile_flags,0,sizeof(tile_flags));
    memset(tile_behaviors,0,sizeof(tile_behaviors));
    memset(air_flags,0,sizeof(air_flags));
    collision_exact=0;original_tiles=0;
    if(map_state!=1)return;
    char name[24];
    snprintf(name,sizeof(name),"TILEINFO.CK%d",selected+1);
    int idx=episode_file(selected,name);
    unsigned n=keen_tileinfo_count((unsigned)selected);
    if(idx>=0 && n<=1024u){
        uint8_t raw[715u*12u];size_t sz=0;
        if(read_asset(idx,raw,sizeof(raw),&sz)==0 &&
           keen_tileinfo_valid(raw,sz,(unsigned)selected)){
            for(unsigned plane=0;plane<6;plane++)for(unsigned t=0;t<n;t++){
                int16_t value=keen_tileinfo_word(raw+((size_t)plane*n+t)*2u);
                tile_flags[plane*1024u+t]=(uint8_t)(value>0);
                if(plane==1u)tile_behaviors[t]=value;
            }
            /* Advance each two- or four-tile animation group while retaining its phase. */
            for(unsigned t=0;t<n;){
                unsigned count=(unsigned)keen_tileinfo_word(raw+t*2u);
                if((count!=2&&count!=4)||t+count>n){t++;continue;}
                for(unsigned member=0;member<count;member++)for(unsigned frame=0;frame<4;frame++)
                    menu_tile_frames[frame][t+member]=(uint16_t)(t+(member+frame)%count);
                t+=count;
            }
            original_tiles=n;
            collision_exact=1;
        }
    }
    if(collision_exact)return;
    /* Fallback collision supports preview movement only; item interactions require valid
     * TILEINFO. */
    unsigned counts[1024]={0};
    for(unsigned y=0;y<level_info.height;y++)for(unsigned x=0;x<level_info.width;x++){
        unsigned id=keen_map_tile(level_unpacked,&level_info,x,y,0);
        if(id<1024u)counts[id]++;
    }
    unsigned most=0,second=0;
    for(unsigned t=1;t<ega_info.tiles && t<1024u;t++)if(counts[t]>counts[most])most=t;
    for(unsigned t=1;t<ega_info.tiles && t<1024u;t++)if(t!=most && counts[t]>counts[second])second=t;
    air_flags[0]=1;
    air_flags[most]=1;
    if(second!=most && counts[second]>counts[most]/3u)air_flags[second]=1;
}

typedef struct {unsigned x,y,tile,background,time;} KeenDoor;
#define KEEN_DOORS 64u
static KeenDoor doors[KEEN_DOORS];
static unsigned door_count;static uint32_t door_ticks;
/* Original level door colors, independent of collected/spent inventory. */
static unsigned hud_map_keys(const uint8_t *map,const KeenMapInfo *info){
    unsigned mask=0;if(!collision_exact)return 0;
    for(unsigned y=0;y<info->height;y++)for(unsigned x=0;x<info->width;x++){
        unsigned tile=keen_map_tile(map,info,x,y,0);int type=tile<original_tiles?tile_behaviors[tile]:0;
        if(type>=2&&type<=5)mask|=1u<<(unsigned)(type-2);
    }
    return mask;
}
static unsigned runtime_tile(unsigned x,unsigned y){
    for(unsigned i=0;i<door_count;i++)if(doors[i].x==x && (doors[i].y==y || doors[i].y+1==y))return doors[i].background;
    unsigned tile=keen_map_tile(level_unpacked,&level_info,x,y,0);
    /* Keep the saved map plane intact; resolve consumed tiles consistently for collision,
     * actors, weapons and rendering. */
    if(collision_exact&&x<level_info.width&&y<level_info.height&&
       keen_gameplay_used(&gameplay,y*level_info.width+x))
        tile=keen_background_for_pickup((unsigned)selected,tile,tile<original_tiles?tile_behaviors[tile]:0,play_background_tile);
    return tile;
}
/* Resolve both key-door tiles before collision or overlap tests open either half. */
static unsigned door_top_row(unsigned x,unsigned y){
    if(!y)return y;
    unsigned tile=runtime_tile(x,y),above=runtime_tile(x,y-1);
    if(tile>=original_tiles||above>=original_tiles||tile!=above+1)return y;
    int upper=tile_behaviors[above],lower=tile_behaviors[tile];
    return upper>=2&&upper<=5&&(lower==upper||lower==0)?y-1:y;
}
static void doors_step(unsigned ticks){
    for(unsigned i=0;i<door_count;i++){
        if(doors[i].time<165){doors[i].time+=ticks;if(doors[i].time>165)doors[i].time=165;}
    }
}
static void door_tile_draw(uint8_t *fb,int x,int y,unsigned tile,int top,int bottom){
    for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++){
        int px=x+xx,py=y+yy;
        if(px>=0&&px<W&&py>=0&&py<H&&py>=top&&py<bottom)fb[py*W+px]=original_pixel(tile,(unsigned)xx,(unsigned)yy);
    }
}
static void doors_draw(uint8_t *fb){
    for(unsigned i=0;i<door_count;i++){
        KeenDoor *d=&doors[i];if(d->time>=165)continue;
        int x=(int)d->x*16-play_camera_x,y=(int)d->y*16-play_camera_y;
        int slide=(int)d->time/5;
        door_tile_draw(fb,x,y+slide,d->tile,y,y+32);
        door_tile_draw(fb,x,y+slide+16,d->tile+1,y,y+32);
    }
}
static int map_set_tile(int x,int y,unsigned tile){
    if(x<0||y<0||x>=(int)level_info.width||y>=(int)level_info.height||tile>=tile_atlas_count)return 0;
    size_t offset=32u+((size_t)y*level_info.width+(unsigned)x)*2u;
    if(offset+2u>level_info.decoded_bytes)return 0;
    level_unpacked[offset]=(uint8_t)tile;level_unpacked[offset+1]=(uint8_t)(tile>>8);return 1;
}
#define KEEN_BRIDGES 32u
typedef struct {int x,y,direction;unsigned base,count,time,mode;} KeenBridge;
static KeenBridge bridges[KEEN_BRIDGES];
static void bridges_step(unsigned ticks){
    for(unsigned i=0;i<KEEN_BRIDGES;i++){
        KeenBridge *b=&bridges[i];if(b->mode!=1&&b->mode!=2)continue;
        b->time+=ticks;
        while(b->time>=12 && (b->mode==1||b->mode==2)){
            b->time-=12;
            if(b->mode==2 && !b->count){b->mode=0;break;}
            int x=b->x+b->direction*(int)(b->mode==1?b->count:b->count-1);
            if(x<0||x>=(int)level_info.width){b->mode=b->mode==1?3u:0u;break;}
            unsigned tile=runtime_tile((unsigned)x,(unsigned)b->y);
            if(b->mode==1){
                if(tile!=b->base || !map_set_tile(x,b->y,270)){b->mode=3;break;}
                b->count++;
            }else{
                if(tile!=270 || !map_set_tile(x,b->y,b->base)){b->mode=0;break;}
                b->count--;
            }
        }
    }
}
static void keen2_blast_step(unsigned ticks){
    KeenActors2*w=&level_actors2;if(!w->blast_active)return;w->blast_time+=ticks;
    while(w->blast_active&&w->blast_time>=40){
        w->blast_time-=40;unsigned phase=w->blast_phase++;int x=(int)w->blast_x,y=(int)w->blast_y;
        if(phase==0){
            for(int col=-1;col<=1;col++){(void)map_set_tile(x+col,y-1,143);(void)map_set_tile(x+col,y,546u+(unsigned)(col+1));}
            (void)map_set_tile(x-3,y+4,506);
        }else if(phase==10){for(int xx=x-7;xx<x-4;xx++)for(int yy=y+2;yy<y+5;yy++)(void)map_set_tile(xx,yy,533);w->blast_active=0;}
        else{
            int from=phase==1?x-2:phase==2?x+2:phase<6?x:x+(int)phase-3;
            int top=phase<3?y:phase<6?y+(int)phase:y+3,bottom=phase<6?top:y+5;
            unsigned tile=phase<3?492:phase<6?505:549;
            for(int yy=top;yy<=bottom;yy++){
                (void)map_set_tile(from,yy,tile);
                for(unsigned i=0;i<KA_BULLETS;i++)if(!w->core.shots[i].state){
                    w->core.shots[i]=(KeenEnemyShot){from*4096,yy*4096,0,ka_random(&w->core)>128?2u:3u,0,1,0};break;}
                sound_play_named("SHOTHIT");
            }
        }
    }
}
static int toggle_switch(int x,int y){
    unsigned tile=runtime_tile((unsigned)x,(unsigned)y);
    unsigned marker=keen_map_tile(level_unpacked,&level_info,(unsigned)x,(unsigned)y,1);
    if(selected==1 && y+5<(int)level_info.height && x+3<(int)level_info.width && runtime_tile((unsigned)x+3,(unsigned)y+5)==479u){
        keen2_finale_begin(1,1);return 1;
    }
    if(tile==271){set_game_lights(!game_lights);sound_play_named("CLICKSND");return 1;}
    if(tile==480)(void)map_set_tile(x,y,493);
    else if(tile==493)(void)map_set_tile(x,y,480);
    int ox=(int)(marker&255u),oy=(int)(marker>>8);if(ox>=128)ox-=256;if(oy>=128)oy-=256;
    int bx=x+ox,by=y+oy;
    if(bx<0||by<0||bx>=(int)level_info.width||by>=(int)level_info.height)return 0;
    for(unsigned i=0;i<KEEN_BRIDGES;i++)if(bridges[i].mode&&bridges[i].x==bx&&bridges[i].y==by){
        bridges[i].mode=bridges[i].mode==2?1u:2u;sound_play_named("CLICKSND");return 1;
    }
    for(unsigned i=0;i<KEEN_BRIDGES;i++)if(!bridges[i].mode){
        unsigned right=bx+1<(int)level_info.width?runtime_tile((unsigned)bx+1,(unsigned)by):original_tiles;
        int direction=right>=original_tiles||tile_flags[KEEN_FACE_LEFT*1024u+right]?-1:1;
        bridges[i]=(KeenBridge){bx,by,direction,runtime_tile((unsigned)bx,(unsigned)by),0,0,1};
        sound_play_named("CLICKSND");return 1;
    }
    return 0;
}
static int try_player_switch(void){
    if(selected>=3 || !collision_exact)return 0;
    int sx=-1,sy=-1;
    for(int x=player.x/16;x<=(player.x+7)/16;x++)for(int y=player.y/16;y<=(player.y+22)/16;y++){
        if(x<0||y<0||x>=(int)level_info.width||y>=(int)level_info.height)continue;
        unsigned tile=runtime_tile((unsigned)x,(unsigned)y);
        int type=tile<original_tiles?tile_behaviors[tile]:0;
        if(type==23||type==25||type==26){sx=x;sy=y;}
    }
    if(sx<0)return 0;
    (void)toggle_switch(sx,sy);return 1;
}

static int test_face(unsigned xx,unsigned yy,int face){
    if(xx>=level_info.width||yy>=level_info.height)return 1;
    unsigned tile=runtime_tile(xx,yy);
    if(tile>=ega_info.tiles||tile>=1024u)return 0;
    if(collision_exact)return tile_flags[(unsigned)face*1024u+tile]!=0;
    return !air_flags[tile];
}
static int collision_cb(int tile_x,int tile_y,int face,void *opaque){
    (void)opaque;
    /* Apply side/top bounds separately; the bottom remains open for fall deaths. */
    if(selected>=1 && selected<=2 && tile_y>=(int)level_info.height)return 0;
    if(tile_x<0 || tile_y<0 || tile_x>=(int)level_info.width || tile_y>=(int)level_info.height)return 1;
    if(!collision_mode)return 0;
    return test_face((unsigned)tile_x,(unsigned)tile_y,face);
}
static KeenEvent check_gameplay_tile(int tilex,int tiley);
static int player_can_exit(void);
static int player_top_extra(void);
/* Apply key-door collision to Keen at each swept probe. Actors and rays use terrain flags. */
static int player_collision_cb(int x,int y,int face,void *u){
    if(collision_mode && collision_exact && x>=0 && y>=0 && x<(int)level_info.width && y<(int)level_info.height &&
       (face==KEEN_FACE_LEFT || face==KEEN_FACE_RIGHT)){
        int door_y=(int)door_top_row((unsigned)x,(unsigned)y);
        unsigned tile=runtime_tile((unsigned)x,(unsigned)door_y);
        int behavior=tile<original_tiles?tile_behaviors[tile]:0;
        if(behavior>=2&&behavior<=5){
            unsigned bit=1u<<(unsigned)(behavior-2);
            if(!(gameplay.key_mask&bit)){gameplay.event="DOOR LOCKED - KEY REQUIRED";return 1;}
            (void)check_gameplay_tile(x,door_y);
            unsigned after=runtime_tile((unsigned)x,(unsigned)door_y);
            if(after<original_tiles && tile_behaviors[after]>=2&&tile_behaviors[after]<=5)return 1;
        }
    }
    return collision_cb(x,y,face,u);
}
static void find_spawn(void){
    play_spawn_x=32;play_spawn_y=32;player_spawned=0;
    for(unsigned y=0;y<level_info.height;y++)for(unsigned x=0;x<level_info.width;x++){
        if(keen_map_tile(level_unpacked,&level_info,x,y,1)==255u){
            play_spawn_x=(int)x*16+(selected<3?4:3);
            play_spawn_y=(int)y*16+(selected!=0&&selected<3&&map_id!=80?9:selected<3?1:0);
            player_spawned=1;return;
        }
    }
}
/* Tile interactions require valid TILEINFO. */
static int player_width(void){return selected<3?8:KEEN_PLAYER_W;}
static int player_height(void){return selected<3?23:KEEN_PLAYER_H;}
static KeenEvent check_gameplay_tile(int tilex,int tiley){
    if(!collision_exact || tilex<0 || tiley<0 ||
       tilex>=(int)level_info.width || tiley>=(int)level_info.height)return KG_NONE;
    tiley=(int)door_top_row((unsigned)tilex,(unsigned)tiley);
    unsigned index=(unsigned)tiley*level_info.width+(unsigned)tilex;
    if(index>=KEEN_MAX_MAP_CELLS)return KG_NONE;
    unsigned tile=runtime_tile((unsigned)tilex,(unsigned)tiley);
    if(tile>=original_tiles)return KG_NONE;
    if(tile_behaviors[tile]>=2 && tile_behaviors[tile]<=5){
        unsigned bit=1u<<(unsigned)(tile_behaviors[tile]-2);
        if(!(gameplay.key_mask&bit)){
            int moving_right=player.vx>0 || (!player.vx && player.x+player_width()/2<tilex*16+8);
            player.x=moving_right?tilex*16-player_width():(tilex+1)*16;
            gameplay.event="DOOR LOCKED - KEY REQUIRED";return KG_LOCKED;
        }
        if(door_count>=KEEN_DOORS || tiley+1>=(int)level_info.height || tile+1>=tile_atlas_count)return KG_LOCKED;
        unsigned background=selected==2?(tilex>0?runtime_tile((unsigned)tilex-1,(unsigned)tiley):play_background_tile):143u;
        if(background>=tile_atlas_count)return KG_LOCKED;
        doors[door_count++]=(KeenDoor){(unsigned)tilex,(unsigned)tiley,tile,background,0};
        gameplay.key_mask&=~bit;gameplay.event="DOOR OPENED";sound_play_named("DOOROPENSND");return KG_NONE;
    }
    /* Chocolate Keen only enters an exit from normal grounded movement. */
    if(tile_behaviors[tile]==22 && selected<3){
        if(show_statue(tilex,tiley))return KG_MESSAGE;
    }
    if(tile_behaviors[tile]==24 && selected!=2 && campaign_active && world_data_ready){
        gameplay.reached_exit=2;gameplay.sound_cue="TELEPORTSND";return KG_EXIT;
    }
    if(tile_behaviors[tile]==17 && (!player.grounded || pogo_mode || !player_can_exit()))return KG_NONE;
    if(tile_behaviors[tile]==17 && campaign_active && !gameplay.reached_exit){
        exit_active=1;exit_tile_x=tilex+2;exit_tile_y=tiley;exit_fx=(player.x-4)*256;
    }
    if(selected==2&&tile_behaviors[tile]==1&&gameplay.ankh_ticks)return KG_NONE;
    if(selected==2&&tile_behaviors[tile]==27&&!keen_gameplay_used(&gameplay,index)){
        gameplay.ankh_ticks+=1400;gameplay.collected++;keen_gameplay_consume(&gameplay,index);sound_play_named("ANKH");return KG_PICKUP;
    }
    return keen_gameplay_touch(&gameplay,(unsigned)selected,tile_behaviors[tile],index);
}
static void award_extra_life(void);
static KeenEvent check_gameplay_interactions(void){
    int left=player.x/16,right=(player.x+player_width()-1)/16;
    int top=kv_floor((player.y-player_top_extra())*16),bottom=(player.y+player_height()-1)/16;
    for(int y=top;y<=bottom;y++)for(int x=left;x<=right;x++){
        KeenEvent result=check_gameplay_tile(x,y);
        if((result==KG_PICKUP || result==KG_EXIT) && gameplay.sound_cue)
            sound_play_named(gameplay.sound_cue);
        if(result==KG_PICKUP && keen_gameplay_extra_life(&gameplay)){
            award_extra_life();
        }
        if(result==KG_HAZARD || result==KG_EXIT || result==KG_MESSAGE)return result;
    }
    return KG_NONE;
}
static KeenSfxClock movement_sfx;
static KeenVortMotion vort_motion;
static uint32_t motion_ticks;
static KeenWeapon weapon;
static uint32_t weapon_ticks;
static KeenActors level_actors;

static uint8_t actor_data[294];
static size_t actor_data_len;
static uint32_t actor_ticks;
static unsigned chain_applied;
static KeenDeath keen_death;
static unsigned keen_lives=4;
static int keen_game_over,keen_frozen,freeze_time;
static int player_top_extra(void){return selected<3 && !keen_frozen && (vort_motion.mode==KV_POGO_GROUND||vort_motion.mode==KV_POGO_AIR);}
static int player_can_exit(void){return selected>=3 || (!keen_frozen && !weapon.shooting && vort_motion.mode==KV_GROUND);}
static void award_extra_life(void){keen_lives++;sound_play_named("EXTRAMANSND");}
static uint32_t death_ticks;
static int status_visible,status_wait_release,status_block_buttons;
static unsigned status_opening;
static uint8_t status_backdrop[W*H];
static void status_pause_audio(void){
    sound_update();paused_sound_priority=-1;
    if(sound_voice>=0){
        int position=of_mixer_get_position(sound_voice);
        if(position>=0 && sound_pcm_start+(size_t)position<sound_samples){
            paused_sound_priority=sound_priority;paused_sound_offset=sound_pcm_start+(size_t)position;
        }
        sound_stop();
    }
}
static void status_resume_audio(void){
    if(paused_sound_priority>=0 && sound_enabled && paused_sound_offset<sound_samples){
        sound_voice=of_mixer_play((const uint8_t*)(sound_pcm+paused_sound_offset),
            (uint32_t)(sound_samples-paused_sound_offset),KS_RATE,paused_sound_priority,(int)(220u*keen_core_audio_gain()/256u));
        sound_priority=sound_voice>=0?paused_sound_priority:-1;sound_pcm_start=paused_sound_offset;
    }
    paused_sound_priority=-1;
}
#define STATUS_BUTTONS (OF_BTN_UP|OF_BTN_DOWN|OF_BTN_LEFT|OF_BTN_RIGHT|OF_BTN_A|OF_BTN_B|OF_BTN_X|OF_BTN_Y|OF_BTN_L1|OF_BTN_R1|OF_BTN_START|OF_BTN_SELECT)
/* Ignore the dismissing button until release and reset the pause timer. */
static int status_tick(void){
    if(status_block_buttons){
        if(!of_btn(STATUS_BUTTONS)){
            status_block_buttons=0;movement_sfx.last_us=of_time_us();status_resume_audio();
        }
        return 1;
    }
    if(status_visible){
        if(status_opening<14)return 1;
        if(status_wait_release){if(!of_btn(STATUS_BUTTONS))status_wait_release=0;return 1;}
        if(of_btn_pressed(STATUS_BUTTONS)){status_visible=0;status_block_buttons=1;}
        return 1;
    }
    if(selected<3 && status_font_count>=256 && of_btn_pressed(OF_BTN_X)){
        if(world_mode)world_frame(status_backdrop);else play_frame(status_backdrop);
        status_pause_audio();status_visible=1;
        status_opening=0;status_wait_release=1;return 1;
    }
    return 0;
}

static unsigned death_random(void *user){
    (void)user;if(selected==2&&level_actors3.core.ready)return ka_random(&level_actors3.core);if(selected==1&&level_actors2.core.ready)return ka_random(&level_actors2.core);return level_actors.ready?ka_random(&level_actors):128u;
}
static void begin_keen_death(const char *message,int count_death){
    if(keen_death.state || keen_game_over)return;
    if(count_death)gameplay.deaths++;
    keen_death_begin(&keen_death,&player);death_ticks=movement_sfx.ticks;
    player.y=kv_floor(keen_death.y);player_animation.frame=22;
    keen_weapon_reset(&weapon);sound_play_named("KEENDIESND");gameplay.event=message;
}

/* Convert sprite-origin limits to body coordinates. CVort_check_ceiling includes an
 * 8/256-pixel left inset. */
static int keen1_level_bounds(void){
    int32_t minx=32*256+8+1024,maxx=((int)level_info.width-3)*4096+1024;
    int32_t miny=32*256+256;
    int wall=0;
    if(maxx<minx)maxx=minx;
    if(vort_motion.fx<minx){vort_motion.fx=minx;wall=1;}
    else if(vort_motion.fx>maxx){vort_motion.fx=maxx;wall=1;}
    if(wall)vort_motion.vx=player.vx=0;
    if(vort_motion.fy<miny){vort_motion.fy=miny;vort_motion.vy=player.vy=0;}
    player.x=kv_floor(vort_motion.fx);player.y=kv_floor(vort_motion.fy);
    if(vort_motion.fy-256>(int32_t)level_info.height*4096)
        begin_keen_death("FELL OUT OF LEVEL",1);
    return wall;
}

static unsigned keen3_body_tile(int x,int y,void*u){(void)u;if(x<0||y<0||x>=(int)level_info.width||y>=(int)level_info.height)return 0;return runtime_tile((unsigned)x,(unsigned)y);}
static void keen3_body_write(int x,int y,unsigned tile,void*u){(void)u;(void)map_set_tile(x,y,tile);}
static void keen3_step(unsigned dt){
    keen3_actors_step(&level_actors3,dt,vort_motion.fx-1024,vort_motion.fy-256,play_camera_x,play_camera_y,collision_cb,0);
    keen3_bodies_step(&level_actors3,dt,keen3_body_tile,keen3_body_write,0);
    unsigned events=level_actors3.core.events;
    if(events&(KA_TANK_FIRE|K3_MOM_FIRE))sound_play_named("TANKFIRE");
    if(events&K3_MEEP_FIRE)sound_play_named("MEEP");
    if(events&K3_FOOB)sound_play_named("YORPSCREAM");
    if(events&KA_VORT_DIE)sound_play_named("VORTSCREAM");
    if(events&KA_SHOT_HIT)sound_play_named("SHOTHIT");
    if(events&K3_STOMP)sound_play_named("FOOTSLAM");
    level_actors3.core.events=0;
    if(level_actors3.finished){gameplay.reached_exit=1;exit_active=0;}
}
static void actor_reset(void){
    chain_applied=0;
    memset(&level_actors,0,sizeof(level_actors));memset(&level_actors2,0,sizeof(level_actors2));memset(&level_actors3,0,sizeof(level_actors3));actor_ticks=movement_sfx.ticks;
    if(selected==0 && collision_exact && sprite_state==1 && actor_data_len==294)
        (void)keen_actors_init(&level_actors,level_unpacked,&level_info,
            sprite_head_file,sprite_head_len,&sprite_info,actor_data,actor_data_len,player.x-4);
    if(play_level==16)for(unsigned i=0;i<level_actors.count;i++)
        if(level_actors.actors[i].kind==3)level_actors.actors[i].health=104;
    if(selected==1 && collision_exact && sprite_state==1 && actor_data_len==294)
        (void)keen2_actors_init(&level_actors2,level_unpacked,&level_info,sprite_head_file,sprite_head_len,&sprite_info,actor_data,actor_data_len,player.x-4,(unsigned)play_level);
    if(selected==2 && collision_exact && sprite_state==1 && actor_data_len==294)
        (void)keen3_actors_init(&level_actors3,level_unpacked,&level_info,sprite_head_file,sprite_head_len,&sprite_info,actor_data,actor_data_len,player.x-4,player.y-1);
}
static void chain_map_step(void){
    while(chain_applied<level_actors.chain_steps){
        int y=(int)level_actors.chain_y+(int)chain_applied++;
        for(int dx=-4;dx<=4;dx++)for(int row=0;row<3;row++){
            int x=(int)level_actors.chain_x+dx,yy=y+row;
            if(x<0||yy<0||x>=(int)level_info.width||yy>=(int)level_info.height)continue;
            unsigned tile=row==0?155u:row==1?(dx==-4?476u:dx==4?478u:477u):(dx==-4?489u:dx==4?491u:490u);
            if(tile>=tile_atlas_count)continue;
            size_t off=32u+((size_t)yy*level_info.width+(unsigned)x)*2u;
            if(off+2u>level_info.decoded_bytes)continue;
            level_unpacked[off]=(uint8_t)tile;level_unpacked[off+1]=(uint8_t)(tile>>8);
        }
    }
}

static void actor_load(void){
    actor_data_len=0;
    if(selected<3){int index=episode_file(selected,episode_name("ACTORS.CK1"));
        if(index>=0 && read_asset(index,actor_data,sizeof(actor_data),&actor_data_len))actor_data_len=0;
    }
}
static int actor_shot_contact(int x,int y,void *user){
    (void)user;
    if(selected==1){
        if(!level_actors2.core.ready)return 0;
        unsigned kind=keen2_actors_shot(&level_actors2,x,y);gameplay.targets_mask|=level_actors2.targets;
        if(level_actors2.core.shot_killed){if(kind==7){gameplay.score+=10000;if(keen_gameplay_extra_life(&gameplay))award_extra_life();}else if(kind<4)sound_play_named("VORTSCREAM");}
        return kind!=0;
    }
    if(selected==2){
        if(!level_actors3.core.ready)return 0;
        unsigned kind=keen3_actors_shot(&level_actors3,x,y);
        if(level_actors3.core.shot_killed&&kind<12){sound_play_named(kind==6?"YORPSCREAM":"VORTSCREAM");}
        if(level_actors3.core.shot_killed&&(kind==12||kind==13)){gameplay.score+=1000;if(keen_gameplay_extra_life(&gameplay))award_extra_life();}
        return kind!=0;
    }
    if(selected!=0 || !level_actors.ready)return 0;
    unsigned kind=keen_actors_shot(&level_actors,x,y);
    if(kind && level_actors.shot_killed)sound_play_named(kind==1?"YORPSCREAM":kind==2?"GARGSCREAM":"VORTSCREAM");
    return kind!=0;
}

static void reset_play(void){
    door_ticks=0;exit_active=0;keen_frozen=freeze_time=0;play_notice=0;
    status_visible=status_block_buttons=0;paused_sound_priority=-1;
    sound_stop();
    memset(&keen_death,0,sizeof(keen_death));
    keen_sfx_reset(&movement_sfx,of_time_us());
    keen_weapon_reset(&weapon);weapon_ticks=0;
    pogo_mode=0;
    keen_animation_reset(&player_animation);
    keen_motion_reset(&player,play_spawn_x,play_spawn_y);
    keen_vort_reset(&vort_motion,&player);motion_ticks=0;
    actor_reset();
    play_camera_x=0;play_camera_y=0;
    if(selected<=2){
        play_camera_x=keen_bounds_camera_axis(player.x-W/2,level_info.width,W);
        play_camera_y=keen_bounds_camera_axis(player.y-vort_view_height()/2,level_info.height,vort_view_height());
    }
}
static void start_play(void){
    if(map_state!=1)return;
    set_game_lights(1);game_menu_visible=play_notice=0;finale.active=0;
    /* Open level 1 in the resource preview; LEVEL80 is the world map. */
    if(map_id==80){map_id=1;load_map();if(map_state!=1)return;}
    memset(doors,0,sizeof(doors));door_count=0;memset(bridges,0,sizeof(bridges));
    play_level=map_id;
    find_spawn();
    if(sprite_episode!=selected || sprite_state==0)load_sprites();
    if(sound_episode!=selected || sound_state==0)sound_load();
    if(!campaign_active){
        keen_gameplay_reset(&gameplay);keen_lives=4;
        gameplay.ammo=selected==1?3u:selected==2?5u:0u;gameplay.found_pogo=selected!=0;
    }
    else{
        gameplay.key_mask=0;gameplay.reached_exit=0;gameplay.collected=0;gameplay.ankh_ticks=0;
        memset(gameplay.consumed,0,sizeof(gameplay.consumed));
    }
    keen_game_over=0;world_mode=0;
    actor_load();load_special_data();hud_required_keys=hud_map_keys(level_unpacked,&level_info);
    /* Use the most common passive level tile as the fallback background; tile 0 is a
     * checkerboard marker. */
    play_background_tile=keen_background_choose(level_unpacked,&level_info,
                           tile_atlas_count,tile_behaviors,collision_exact);
    reset_play();
    gameplay.event=selected==0?(level_actors.ready?"YORPS AND GARGS ACTIVE":"ACTOR DATA NOT READY"):
        (collision_exact?"ORIGINAL TILEINFO ACTIVE":"TILEINFO MISSING - MOVE ONLY");
    play_mode=1;
    if(selected==2&&play_level==16)keen3_scene_begin(0,0);
}
static void frozen_step(unsigned ticks){
    vort_motion.pending+=ticks;
    while(vort_motion.pending>=6){
        vort_motion.pending-=6;freeze_time-=6;
        unsigned frame=freeze_time<0?(selected==2?30u:31u):28u+((movement_sfx.ticks>>5)&1u);
        const uint8_t *m=sprite_head_file+sprite_info.table_offset+frame*128u;
        KeenActorBox b={keen_le16(m+8)/256,keen_le16(m+10)/256,keen_le16(m+12)/256,keen_le16(m+14)/256};
        KeenActor frozen;memset(&frozen,0,sizeof(frozen));frozen.x=vort_motion.fx-1024;frozen.y=vort_motion.fy-256;
        frozen.vx=vort_motion.vx;frozen.vy=vort_motion.vy;int dy=0;
        for(unsigned tick=0;tick<6;tick++){frozen.vy+=3;if(frozen.vy>200)frozen.vy=200;if(tick<5)dy+=frozen.vy;}
        int dx=frozen.vx*6;
        if(selected!=0){int drag=0;kv_friction(&vort_motion,3,&drag);frozen.vx=vort_motion.vx;dx=drag+frozen.vx*6;}
        unsigned hit=ka_move(&frozen,b,dx,dy+frozen.vy*6,player_collision_cb,0);
        vort_motion.fx=frozen.x+1024;vort_motion.fy=frozen.y+256;vort_motion.vx=frozen.vx;vort_motion.vy=frozen.vy;
        player.x=kv_floor(vort_motion.fx);player.y=kv_floor(vort_motion.fy);player.vx=frozen.vx;player.vy=frozen.vy;player.grounded=!!(hit&2);
        if(freeze_time< (selected!=0?-40:-50)){keen_frozen=0;vort_motion.mode=player.grounded?KV_GROUND:KV_AIR;break;}
    }
}

static void update_play(void){
    if(cinema3.active){keen3_scene_update();return;}
    if(score_flow_active){score_flow_tick();return;}
    if(notice_tick())return;
    if(game_menu_visible){game_menu_tick();return;}
    if(status_block_buttons){(void)status_tick();return;}
    if(autosave_prompt_tick())return;
    if(finale.active){finale_update();return;}
    if(status_tick())return;
    if(world_mode){update_world();return;}
    if(selected<3 && keen_game_over){
        if(of_btn_pressed(OF_BTN_A)){
            if(campaign_active){start_campaign();return;}
            cinema3.active=0;keen_gameplay_reset(&gameplay);keen_lives=4;keen_game_over=0;
    if(selected==1||selected==2){gameplay.ammo=selected==1?3:5;gameplay.found_pogo=1;}reset_play();
            gameplay.event="NEW ATTEMPT";
        }
        return;
    }
    if(selected<3 && keen_death.state){
        keen_sfx_clock_step(&movement_sfx,of_time_us());
        unsigned dt=movement_sfx.ticks-death_ticks;death_ticks=movement_sfx.ticks;
        keen_actors_step(&level_actors,dt,keen_death.x-1024,keen_death.y-256,
            play_camera_x,play_camera_y,collision_cb,0);
        keen_death_step(&keen_death,dt,movement_sfx.ticks,play_camera_y,death_random,0);
        player.x=kv_floor(keen_death.x);player.y=kv_floor(keen_death.y);
        player_animation.frame=keen_death.frame;
        if(keen_death.state==3){
            if(campaign_active && keen_lives){keen_lives--;return_world(0);show_lives();return;}
            if(keen_lives)keen_lives--;
            if(!keen_lives){
                keen_game_over=1;gameplay.event="GAME OVER";
                if(campaign_active){forget_current_part();if(selected==1)keen2_finale_begin(1,0);else score_flow_begin();}else sound_play_named("GAMEOVERSND");
            }
            else{reset_play();gameplay.event="LIFE LOST - SPAWN RESET";}
        }
        return;
    }
    if(gameplay.reached_exit){
        if(campaign_active){
            if(gameplay.reached_exit==2){
                return_world(-1);teleport_source=teleport_target=2;
                world_x=(int)(keen_le32(world_data+24)/256u);world_y=(int)(keen_le32(world_data+28)/256u);
                teleport_x=world_x/16;teleport_y=world_y/16;teleport_phase=17;teleport_pending=0;autosave_after_teleport=1;world_camera();return;
            }
            if(!exit_active){return_world(1);return;}
            keen_sfx_clock_step(&movement_sfx,of_time_us());
            unsigned dt=movement_sfx.ticks-motion_ticks;motion_ticks=movement_sfx.ticks;
            if(dt>64)dt=64;
            if(selected==1&&level_actors2.core.ready){
                keen2_actors_step(&level_actors2,dt,vort_motion.fx-1024,vort_motion.fy-256,play_camera_x,play_camera_y,collision_cb,0);
                keen2_blast_step(dt);gameplay.targets_mask|=level_actors2.targets;actor_ticks=movement_sfx.ticks;
            }
            if(selected==2&&level_actors3.core.ready){
                unsigned actor_dt=movement_sfx.ticks-actor_ticks;actor_ticks=movement_sfx.ticks;
                keen3_step(actor_dt);
            }
            if(level_actors.ready){
                keen_actors_step(&level_actors,dt,vort_motion.fx-1024,vort_motion.fy-256,play_camera_x,play_camera_y,collision_cb,0);
                chain_map_step();actor_ticks=movement_sfx.ticks;
            }
            (void)keen_weapon_step_contacts(&weapon,dt,0,0,1,player.x-4,player.y-1,
                &gameplay.ammo,collision_cb,0,actor_shot_contact,0);weapon_ticks=movement_sfx.ticks;
            exit_fx+=(int32_t)dt*60;player.x=kv_floor(exit_fx)+4;
            player_animation.frame=(movement_sfx.ticks>>4)&3u;
            if(exit_fx>=exit_tile_x*4096)return_world(1);
            return;
        }
        if(of_btn_pressed(OF_BTN_A)){
            sound_stop();play_mode=0;map_mode=1;map_id=80;load_map();
        }
        return;
    }
    if(!campaign_active && of_btn_pressed(OF_BTN_SELECT))collision_mode=!collision_mode;
    if(of_btn_pressed(OF_BTN_R1))play_hud=!play_hud;
    if(!keen_frozen && of_btn_pressed(OF_BTN_Y)){
        if(try_player_switch())gameplay.event="SWITCH ACTIVATED";
        else if(selected!=0 || gameplay.found_pogo)pogo_mode=!pogo_mode;
        else gameplay.event="FIND THE POGO STICK FIRST";
    }
    if(selected>=3 && of_btn_pressed(OF_BTN_X))reset_play();
    if(finale.active)return;
    int jump=of_btn_pressed(OF_BTN_A);
    int firing=selected<3 && !keen_frozen && (weapon.shooting || of_btn_pressed(OF_BTN_B));
    int direction=firing?0:!!of_btn(OF_BTN_RIGHT)-!!of_btn(OF_BTN_LEFT);
    if(firing){jump=0;pogo_mode=0;}
    KeenMotion before=player;int32_t before_fx=vort_motion.fx,before_fy=vort_motion.fy;
    keen_sfx_clock_step(&movement_sfx,of_time_us());
    doors_step(movement_sfx.ticks-door_ticks);bridges_step(movement_sfx.ticks-door_ticks);door_ticks=movement_sfx.ticks;
    unsigned motion_events=0,ankh_elapsed=0;
    if(selected<3){
        unsigned elapsed=movement_sfx.ticks-motion_ticks;motion_ticks=movement_sfx.ticks;
        if(keen_frozen)frozen_step(elapsed);
        else motion_events=keen_vort_step(&vort_motion,&player,elapsed,direction,
            !firing && of_btn(OF_BTN_A),pogo_mode,firing,player_collision_cb,0);
    }else keen_motion_step_mode(&player,direction,jump,pogo_mode,player_collision_cb,0);
    if(selected<3){if(keen1_level_bounds())motion_events|=KV_WALL;if(keen_death.state)return;}
    int fell_out=selected>=3 &&
        keen_bounds_apply(&player,level_info.width,level_info.height);
    if(fell_out){
        gameplay.deaths++;
        reset_play();sound_play_named("KEENDIESND");
        gameplay.event="FELL OUT OF LEVEL - SPAWN RESET";
    }else{
        const char *cue=keen_sfx_movement(&movement_sfx,&before,&player,jump,pogo_mode);
        if(selected<3){
            if((motion_events&KV_WALL) && ((movement_sfx.ticks>>4)&1u))cue="KEENBLOKSND";
            else if(motion_events&KV_HEAD)cue="BUMPHEADSND";
            else if(motion_events&KV_LAND)cue="KEENLANDSND";
            else if(motion_events&KV_JUMP)cue="KEENJUMPSND";
            else if(vort_motion.mode!=KV_GROUND)cue=0;
        }
        if(cue)sound_play_named(cue);
        keen_animation_step_for_episode(&player_animation,&player,pogo_mode,(unsigned)selected);
        if(selected<3){
            unsigned base=player_animation.facing<0?14u:8u;
            if(vort_motion.mode==KV_GROUND)player_animation.frame=(player_animation.facing<0?4u:0u)+(player.vx?((movement_sfx.ticks>>4)&3u):0u);
            else if(vort_motion.mode==KV_CHARGE)player_animation.frame=base+vort_motion.time/6u;
            else if(vort_motion.mode==KV_AIR)player_animation.frame=player_animation.facing<0?19u:13u;
            else if(vort_motion.mode==KV_POGO_GROUND)player_animation.frame=player_animation.facing<0?27u:25u;
            else if(vort_motion.mode==KV_POGO_AIR)player_animation.frame=player_animation.facing<0?26u:24u;
            if(keen_frozen)player_animation.frame=freeze_time<0?(selected==2?30u:31u):28u+((movement_sfx.ticks>>5)&1u);
            unsigned dt=movement_sfx.ticks-weapon_ticks;weapon_ticks=movement_sfx.ticks;
            if(selected==1 && level_actors2.core.ready){
                unsigned actor_dt=movement_sfx.ticks-actor_ticks;actor_ticks=movement_sfx.ticks;
                level_actors2.core.events=0;level_actors2.lights=(unsigned)game_lights;
                keen2_actors_step(&level_actors2,actor_dt,vort_motion.fx-1024,vort_motion.fy-256,play_camera_x,play_camera_y,collision_cb,0);
                keen2_blast_step(actor_dt);
                gameplay.targets_mask|=level_actors2.targets;
                if(level_actors2.core.events&KA_TANK_FIRE)sound_play_named("TANKFIRE");
                if(level_actors2.core.events&KA_VORT_DIE)sound_play_named("VORTSCREAM");
                if(level_actors2.core.events&KA_SHOT_HIT)sound_play_named("SHOTHIT");
            }
            if(selected==2&&level_actors3.core.ready){
                unsigned actor_dt=movement_sfx.ticks-actor_ticks;actor_ticks=movement_sfx.ticks;
                keen3_step(actor_dt);ankh_elapsed=actor_dt;
            }
            if(level_actors.ready){
                level_actors.events=0;
                unsigned actor_dt=movement_sfx.ticks-actor_ticks;actor_ticks=movement_sfx.ticks;
                keen_actors_step(&level_actors,actor_dt,vort_motion.fx-1024,vort_motion.fy-256,
                    play_camera_x,play_camera_y,collision_cb,0);
                chain_map_step();
                if(level_actors.events&KA_YORP_DIE)sound_play_named("YORPSCREAM");
                if(level_actors.events&KA_GARG_DIE)sound_play_named("GARGSCREAM");
                if(level_actors.events&KA_VORT_DIE)sound_play_named("VORTSCREAM");
                if(level_actors.events&KA_CANNON_FIRE)sound_play_named("CANNONFIRE");
                if(level_actors.events&KA_CHUNK_SMASH)sound_play_named("CHUNKSMASH");
                if(level_actors.events&KA_TANK_FIRE)sound_play_named("TANKFIRE");
                if(level_actors.events&KA_SHOT_HIT)sound_play_named("SHOTHIT");
            }
            int ox=player.x+(player_width()-16)/2,oy=player.y+player_height()-24;
            unsigned event=keen_weapon_step_contacts(&weapon,dt,!keen_frozen&&of_btn_pressed(OF_BTN_B),!keen_frozen&&of_btn(OF_BTN_B),
                player_animation.facing,ox,oy,&gameplay.ammo,collision_cb,0,actor_shot_contact,0);
            if(event&KW_FIRE){sound_play_named("KEENFIRESND");gameplay.event="RAYGUN FIRED";}
            if(event&KW_EMPTY){sound_play_named("GUNCLICK");gameplay.event="RAYGUN EMPTY - FIND AMMO";}
            if(event&KW_HIT)sound_play_named("SHOTHIT");
            if(weapon.shooting)player_animation.frame=weapon.facing<0?21u:20u;
        }
        if(selected==1 && level_actors2.core.ready){
            unsigned contact=keen2_actors_player(&level_actors2,&player,&vort_motion,before_fx,before_fy,player_collision_cb,0);
            gameplay.targets_mask|=level_actors2.targets;
            if(contact&KA_KILL_KEEN)begin_keen_death("ENEMY CONTACT",1);
            else if((contact&K2_STUN)&&!keen_frozen){keen_frozen=1;freeze_time=400;
                vort_motion.vx=level_actors2.stun_vx;vort_motion.vy=level_actors2.stun_vy;pogo_mode=0;weapon.shooting=0;}
        }
        if(selected==2&&level_actors3.core.ready){
            unsigned contact=keen3_actors_player(&level_actors3,&player,&vort_motion,before_fy,gameplay.ankh_ticks!=0,player_collision_cb,0);
            if(contact&KA_KILL_KEEN)begin_keen_death("ENEMY CONTACT",1);
            else if((contact&K2_STUN)&&!keen_frozen){keen_frozen=1;freeze_time=400;vort_motion.vx=level_actors3.stun_vx;vort_motion.vy=level_actors3.stun_vy;pogo_mode=0;weapon.shooting=0;}
        }
        if(selected==0 && level_actors.ready){
            unsigned contact=keen_actors_player_box(&level_actors,&player,vort_motion.fy-256,player_top_extra());
            if(contact&KA_BULLET_CONTACT)sound_play_named("SHOTHIT");
            if(contact&KA_FREEZE){
                keen_frozen=1;freeze_time=800;vort_motion.vx=level_actors.freeze_vx;vort_motion.vy=level_actors.freeze_vy;
                pogo_mode=0;weapon.shooting=0;sound_play_named("KEENCICLE");
            }
            if((contact&KA_KILL_KEEN) && keen_frozen && !(contact&KA_LETHAL_ACTOR))freeze_time=0;
            else if(contact&KA_KILL_KEEN){
                begin_keen_death("ENEMY CONTACT",1);
            }else if(contact&KA_STOMP){
                vort_motion.vy=player.vy=0;vort_motion.mode=KV_AIR;
                sound_play_named("YORPBOPSND");gameplay.event="YORP STUNNED";
            }else if(contact&(KA_PUSH_LEFT|KA_PUSH_RIGHT)){
                vort_motion.vx=player.vx=contact&KA_PUSH_RIGHT?240:-240;
                vort_motion.vy=player.vy=0;sound_play_named("YORPBUMPSND");gameplay.event="YORP PUSH";
            }
        }
        if(collision_exact && !(selected<3 && keen_death.state)){
            KeenEvent e=check_gameplay_interactions();
            if(e==KG_HAZARD){
                if(selected<3)begin_keen_death("HAZARD",0);
                else{reset_play();sound_play_named("KEENDIESND");gameplay.event="HAZARD - SPAWN RESET";}
            }
        }
    }
    if(selected==2)gameplay.ankh_ticks=gameplay.ankh_ticks>ankh_elapsed?gameplay.ankh_ticks-ankh_elapsed:0;
    if(selected<3 && keen_death.state)return;
    /* Clamp preview movement when collision is disabled. */
    int mapw=(int)level_info.width*16, maph=(int)level_info.height*16;
    if(player.x<0)player.x=0;
    if(player.x>mapw-player_width())player.x=mapw-player_width();
    if(player.y>maph+64)reset_play();
    if(selected<3){
        if(kv_floor(vort_motion.fx)!=player.x)vort_motion.fx=player.x*256;
        if(kv_floor(vort_motion.fy)!=player.y)vort_motion.fy=player.y*256;
    }
    play_camera_x=player.x-W/2;
    play_camera_y=player.y-vort_view_height()/2;
    int maxx=mapw-W,maxy=maph-vort_view_height();
    if(maxx<0)maxx=0;
    if(maxy<0)maxy=0;
    if(play_camera_x<0)play_camera_x=0;
    if(play_camera_y<0)play_camera_y=0;
    if(play_camera_x>maxx)play_camera_x=maxx;
    if(play_camera_y>maxy)play_camera_y=maxy;
    if(selected<=2){
        play_camera_x=keen_bounds_camera_axis(play_camera_x,level_info.width,W);
        play_camera_y=keen_bounds_camera_axis(play_camera_y,level_info.height,vort_view_height());
    }
}
static void play_frame(uint8_t *fb){
    rect(fb,0,0,W,H,0);
    /* Draw visible map pixels directly on the 320x240 canvas; the HUD overlays the scene. */
    for(int sy=0;sy<H;sy++){
        unsigned wy=(unsigned)(play_camera_y+sy),ty=wy/16u,py=wy%16u;
        if(ty>=level_info.height)continue;
        uint8_t *line=fb+sy*W;
        int sx=0;
        while(sx<W){
            unsigned wx=(unsigned)(play_camera_x+sx),tx=wx/16u;
            int run=16-(int)(wx%16u);
            if(run>W-sx)run=W-sx;
            if(tx<level_info.width){
                unsigned tile=runtime_tile(tx,ty);
                if(selected!=0&&selected<3&&tile<tile_atlas_count)tile=menu_tile_frames[(movement_sfx.ticks>>4)&3u][tile];
                if(tile<tile_atlas_count){
                    memcpy(line+sx,tile_atlas+tile*256u+py*16u+wx%16u,(size_t)run);
                }else for(int k=0;k<run;k++){
                    line[sx+k]=original_pixel(tile,(wx+(unsigned)k)%16u,py);
                }
            }
            sx+=run;
        }
    }
    doors_draw(fb);
    if(selected<3 && sprite_state==1){
        for(unsigned i=0;i<KW_SHOTS;i++)if(weapon.shots[i].state){
            KeenShot *shot=&weapon.shots[i];
            draw_masked_sprite(fb,shot->x/256-play_camera_x,shot->y/256-play_camera_y,
                shot->state==1?(selected==2?102u:selected==1?122u:108u):(shot->direction>0?(selected==2?105u:selected==1?124u:110u):(selected==2?106u:selected==1?125u:111u)),0);
        }
    }
    if(selected==1 && sprite_state==1 && level_actors2.core.ready){
        KeenActors *c=&level_actors2.core;
        for(unsigned i=0;i<KA_BULLETS;i++)if(c->shots[i].state){KeenEnemyShot *shot=&c->shots[i];
            draw_masked_sprite(fb,kv_floor(shot->x)-play_camera_x,kv_floor(shot->y)-play_camera_y,shot->state==1?123u:shot->state==2?124u:125u,0);}
        for(unsigned i=c->count;i>0;i--){KeenActor*a=&c->actors[i-1];if(a->kind&&a->active)
            draw_masked_sprite(fb,kv_floor(a->x)-play_camera_x,kv_floor(a->y)-play_camera_y,a->frame,0);}
    }
    if(selected==2 && sprite_state==1 && level_actors3.core.ready){
        KeenActors*c=&level_actors3.core;
        for(unsigned i=0;i<KA_BULLETS;i++)if(c->shots[i].state){KeenEnemyShot*shot=&c->shots[i];draw_masked_sprite(fb,kv_floor(shot->x)-play_camera_x,kv_floor(shot->y)-play_camera_y,k3_shot_frame(shot,c->ticks),0);}
        for(unsigned i=c->count;i>0;i--){KeenActor*a=&c->actors[i-1];if(a->kind&&a->active)draw_masked_sprite(fb,kv_floor(a->x)-play_camera_x,kv_floor(a->y)-play_camera_y,a->frame,0);}
    }
    if(selected==0 && sprite_state==1 && level_actors.ready){
        for(unsigned i=0;i<KA_BULLETS;i++)if(level_actors.shots[i].state){
            KeenEnemyShot *s=&level_actors.shots[i];
            draw_masked_sprite(fb,kv_floor(s->x)-play_camera_x,kv_floor(s->y)-play_camera_y,
                s->kind==3?113u:s->kind==2?112u:s->state==1?109u:s->state==2?110u:111u,0);
        }
        for(unsigned i=level_actors.count;i>0;i--){
            KeenActor *a=&level_actors.actors[i-1];
            if(a->kind && a->active)draw_masked_sprite(fb,kv_floor(a->x)-play_camera_x,
                kv_floor(a->y)-play_camera_y,a->frame,0);
        }
    }
    /* Draw the selected sprite independently of the collision hitbox. */
    int px=player.x-play_camera_x,py=player.y-play_camera_y;
    int drew_sprite=0;
    if(sprite_state==1 && player_sprite_enabled){
        unsigned draw_index=auto_player_animation?player_animation.frame:player_sprite_index;
        KeenSprite sp;
        if(keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,
                           draw_index,&sp)==0){
            /* Animation frames encode facing. Diagnostic frames may still be mirrored. */
            draw_masked_sprite(fb,px+(player_width()-(int)sp.w)/2,
                               selected<3 && (keen_death.state || keen_frozen) ? keen_death_draw_y(py) : py+player_height()-(int)sp.h,
                               draw_index,auto_player_animation?0:player.vx<0);
            drew_sprite=1;
        }
    }
    if(campaign_active && exit_active && gameplay.reached_exit){
        int x=exit_tile_x*16-play_camera_x,y=exit_tile_y*16-play_camera_y;
        door_tile_draw(fb,x,y,selected==2?257:160,0,H);door_tile_draw(fb,x,y+16,selected==2?257:160,0,H);
        door_tile_draw(fb,x+16,y,selected==2?246:143,0,H);door_tile_draw(fb,x+16,y+16,selected==2?246:143,0,H);
    }
    if(!drew_sprite){
        rect(fb,px,py,player_width(),player_height(),5);
        rect(fb,px+2,py+2,player_width()-4,player_height()-6,4);
        rect(fb,px+2,py+player_height()-3,player_width()-4,2,6);
    }
    if(selected==2&&gameplay.ankh_ticks&&!keen_death.state&&(gameplay.ankh_ticks>250||((movement_sfx.ticks>>4)&1u)))draw_masked_sprite(fb,px-12,py-9,61+((movement_sfx.ticks>>(gameplay.ankh_ticks>250?4:5))&1u),0);
    foreground_tiles_frame(fb,0);
    char label[64],v[16];
    strcpy(label,"KEEN ");number(v,(unsigned)selected+1);strcat(label,v);
    strcat(label," LVL ");number(v,(unsigned)play_level);strcat(label,v);
    if(auto_player_animation){
        strcat(label," ");strcat(label,keen_animation_pose_name(player_animation.pose));
    }
    if(play_hud){
        /* Draw diagnostics over the scene. */
        rect(fb,0,0,W,10,0);
        center(fb,1,label,6);
    }
    char stats[70],score[16];
    number(score,gameplay.score);
    strcpy(stats,"SCORE ");strcat(stats,score);
    if(collision_exact){
        char num[16];
        strcat(stats," ITEMS ");number(num,gameplay.collected);strcat(stats,num);
        strcat(stats," DEATHS ");number(num,gameplay.deaths);strcat(stats,num);
    } else strcat(stats," COLLISION TEST");
    if(selected==0){char ammo[16];number(ammo,gameplay.ammo);strcat(stats," AMMO ");strcat(stats,ammo);}
    if(play_hud){
        rect(fb,0,H-20,W,20,0);
        center(fb,H-19,stats,5);
        center(fb,H-10,gameplay.event?gameplay.event:"A JUMP Y POGO START MENU X STATUS",7);
    }
    if(selected==0 && keen_game_over && !campaign_active){
        rect(fb,0,H/2-6,W,16,0);center(fb,H/2-4,"GAME OVER - A RETRY B BACK",5);
    }
    corner_hud_frame(fb);
    if(!keen_core_view_enhanced())keen_view_center_original(fb,9);
}

static void load_world_data(void){
    if(selected==2){actor_load();memset(&world_random3,0,sizeof(world_random3));
        if(actor_data_len==294){memcpy(world_random3.random,actor_data+4,256);for(unsigned i=1;i<18;i++)world_random3.jump[i]=keen_le16(actor_data+258+i*2);}
        world_random3.j1=17;world_random3.j2=5;
    }
    size_t len=0;int idx=episode_file(0,"WORLD.CK1");
    world_data_ready=idx>=0&&!read_asset(idx,world_data,sizeof(world_data),&len)&&len==34&&!memcmp(world_data,"KW01",4);
}
static void world_camera(void){
    int maxx=(int)level_info.width*16-W,maxy=(int)level_info.height*16-vort_view_height();
    if(maxx<0)maxx=0;
    if(maxy<0)maxy=0;
    play_camera_x=world_x-144;play_camera_y=world_y-48;
    if(play_camera_x<0)play_camera_x=0;
    if(play_camera_y<0)play_camera_y=0;
    if(play_camera_x>maxx)play_camera_x=maxx;
    if(play_camera_y>maxy)play_camera_y=maxy;
    play_camera_x=keen_bounds_camera_axis(play_camera_x,level_info.width,W);
    play_camera_y=keen_bounds_camera_axis(play_camera_y,level_info.height,vort_view_height());
}
static unsigned world_object(unsigned x,unsigned y){
    unsigned id=keen_map_tile(level_unpacked,&level_info,x,y,1);
    unsigned level=id&0x7fffu;
    if(level>=1 && level<=16 && (completed_levels&(1u<<(level-1))))return 0;
    return id;
}
static unsigned world_tile(unsigned x,unsigned y){
    if(teleport_phase && (int)x==teleport_x && (int)y==teleport_y){
        unsigned index=teleport_phase<=(selected==2?8u:16u)?teleport_source:teleport_target;
        if(selected==2)return 130u+((teleport_phase-1u)&3u);
        unsigned snow=keen_le16(world_data+4+index*10+8);
        return (snow?342u:338u)+(((teleport_phase+1)/2)&3u);
    }
    unsigned id=keen_map_tile(level_unpacked,&level_info,x,y,1)&0x7fffu;
    if(id>=1 && id<=16 && (completed_levels&(1u<<(id-1)))){
        int left=x>0 && (keen_map_tile(level_unpacked,&level_info,x-1,y,1)&0x7fffu)==id;
        int right=x+1<level_info.width && (keen_map_tile(level_unpacked,&level_info,x+1,y,1)&0x7fffu)==id;
        int up=y>0 && (keen_map_tile(level_unpacked,&level_info,x,y-1,1)&0x7fffu)==id;
        int down=y+1<level_info.height && (keen_map_tile(level_unpacked,&level_info,x,y+1,1)&0x7fffu)==id;
        if((left||right)&&(up||down))return (selected==2?52u:78u)+(unsigned)left+(unsigned)up*2u;
        return selected==2?56:77;
    }
    return keen_map_tile(level_unpacked,&level_info,x,y,0);
}
/* Original draw order: sprites first, then negative-type foreground tiles.
 * Type -1 is opaque. Type -2 takes its four-plane mask from the next tile:
 * preserve destination bits selected by the mask, replace the other bits. */
static void foreground_tiles_frame(uint8_t *fb,int world){
    if(!collision_exact||map_state!=1)return;
    unsigned phase=selected!=0&&selected<3?(movement_sfx.ticks>>4)&3u:0;
    for(unsigned ty=(unsigned)play_camera_y/16u;ty<level_info.height&&ty<=(unsigned)(play_camera_y+H-1)/16u;ty++)
    for(unsigned tx=(unsigned)play_camera_x/16u;tx<level_info.width&&tx<=(unsigned)(play_camera_x+W-1)/16u;tx++){
        unsigned tile=world?world_tile(tx,ty):runtime_tile(tx,ty);
        if(tile>=original_tiles||tile>=tile_atlas_count||tile_behaviors[tile]>=0)continue;
        int masked=tile_behaviors[tile]==-2;
        if(masked&&tile+1>=tile_atlas_count)continue;
        unsigned image=masked?tile:menu_tile_frames[phase][tile];
        if(image>=tile_atlas_count)continue;
        int sx=(int)tx*16-play_camera_x,sy=(int)ty*16-play_camera_y;
        int left=sx<0?-sx:0,right=sx+16>W?W-sx:16;
        int top=sy<0?-sy:0,bottom=sy+16>H?H-sy:16;
        for(int y=top;y<bottom;y++)for(int x=left;x<right;x++){
            uint8_t*pixel=fb+(sy+y)*W+sx+x;unsigned color=original_pixel(image,(unsigned)x,(unsigned)y);
            if(masked){unsigned mask=original_pixel(tile+1,(unsigned)x,(unsigned)y)-9u;
                unsigned behind=*pixel>=9?(unsigned)*pixel-9u:0;
                color=9u+((behind&mask)|((color-9u)&(mask^15u)));
            }
            *pixel=(uint8_t)color;
        }
    }
}
static int world_block(int x,int y,int face){
    if(x<0||y<0||x>=(int)level_info.width||y>=(int)level_info.height)return 1;
    unsigned tile=world_tile((unsigned)x,(unsigned)y);
    return tile>=original_tiles || tile_flags[(unsigned)face*1024u+tile] || (world_object((unsigned)x,(unsigned)y)&0x8000u);
}
static void forget_current_part(void){
    if(selected==2)return;
    if(selected==1){gameplay.targets_mask&=~keen2_city_bit((unsigned)play_level);return;}
    unsigned bit=play_level==4?1u:play_level==3?2u:play_level==8?4u:play_level==16?8u:0u;
    gameplay.ship_mask&=~bit;
}
static void return_world(int completed){
    unsigned checkpoint_mode=keen_core_autosave_mode(); /* Capture before SD/map work. */
    finale.active=0;set_game_lights(1);
    if(completed>0 && play_level>=1 && play_level<=16)completed_levels|=1u<<(unsigned)(play_level-1);
    if(completed==0)forget_current_part();
    sound_stop();map_id=80;load_map();
    if(selected==2&&map_state==1)keen3_world_init(&world3,level_unpacked,&level_info);
    if(map_state!=1){play_mode=world_mode=0;map_mode=1;campaign_active=0;return;}
    gameplay.key_mask=0;gameplay.reached_exit=0;memset(gameplay.consumed,0,sizeof(gameplay.consumed));
    memset(&keen_death,0,sizeof(keen_death));memset(&level_actors,0,sizeof(level_actors));memset(&level_actors2,0,sizeof(level_actors2));memset(&level_actors3,0,sizeof(level_actors3));
    memset(doors,0,sizeof(doors));door_count=0;memset(bridges,0,sizeof(bridges));keen_weapon_reset(&weapon);
    keen_frozen=freeze_time=0;exit_active=0;world_mode=play_mode=1;map_mode=0;world_pending=teleport_phase=teleport_pending=0;motion_ticks=0;world_frame_index=selected==2?35:36;
    keen_sfx_reset(&movement_sfx,of_time_us());status_visible=status_block_buttons=0;paused_sound_priority=-1;
    campaign_complete=completed>0 && (selected==2?!!(completed_levels&32768u):selected==1?gameplay.targets_mask==255:gameplay.ship_mask==15);
    world_camera();
    if(completed>=0)campaign_autosave_mode(checkpoint_mode);
    if(campaign_complete && !autosave_prompt_pending)finale_begin();
}
static void start_campaign(void){
    /* This controller handles Vorticons; Galaxy and Dreams use their own engines. */
    if(selected>=3)return;
    if(!campaign_namespace_ready()){campaign_active=play_mode=world_mode=0;episode_menu_message=selected==2?"LOAD KEEN 3.JSON FIRST":selected==1?"LOAD KEEN 2.JSON FIRST":"LOAD KEEN.JSON FIRST";return;}
    ega_state=0;load_tiles();load_sprites();sound_load();load_world_data();load_special_data();map_id=80;load_map();
    if(map_state!=1 || !collision_exact || sprite_state!=1)return;
    find_spawn();if(!player_spawned)return;
    cinema3.active=0;keen_gameplay_reset(&gameplay);keen_lives=4;keen_game_over=0;
    if(selected==1||selected==2){gameplay.ammo=selected==1?3:5;gameplay.found_pogo=1;}
    if(selected==2){actor_load();memset(&world_random3,0,sizeof(world_random3));if(actor_data_len==294){memcpy(world_random3.random,actor_data+4,256);for(unsigned i=1;i<18;i++)world_random3.jump[i]=keen_le16(actor_data+258+i*2);}world_random3.j1=17;world_random3.j2=5;}
    autosave_prompt_pending=autosave_prompt_ready=0;
    completed_levels=0;campaign_active=1;campaign_complete=0;collision_mode=1;autosave_result=0;autosave_after_teleport=0;
    world_x=play_spawn_x-4;world_y=play_spawn_y-1;return_world(-1);show_lives();
}
static void update_world(void){
    if(campaign_complete)return;
    keen_sfx_clock_step(&movement_sfx,of_time_us());
    unsigned ticks=movement_sfx.ticks-motion_ticks;motion_ticks=movement_sfx.ticks;
    if(ticks>64)ticks=64;
    if(teleport_phase){
        teleport_pending+=ticks;
        while(teleport_pending>=(selected==2?4u:6u) && teleport_phase){
            teleport_pending-=selected==2?4u:6u;teleport_phase++;
            if(teleport_phase==(selected==2?9u:17u)){
                if(selected==2)(void)map_set_tile((int)world3.teleport_x[teleport_source],(int)world3.teleport_y[teleport_source],134);
                world_x=selected==2?(int)world3.teleport_x[teleport_target]*16:(int)(keen_le32(world_data+4+teleport_target*10)/256u);
                world_y=selected==2?(int)world3.teleport_y[teleport_target]*16:(int)(keen_le32(world_data+8+teleport_target*10)/256u);
                teleport_x=world_x/16;teleport_y=world_y/16;world_camera();
            }
            if(teleport_phase>(selected==2?16u:32u)){if(selected==2){(void)map_set_tile((int)world3.teleport_x[teleport_source],(int)world3.teleport_y[teleport_source],134);(void)map_set_tile((int)world3.teleport_x[teleport_target],(int)world3.teleport_y[teleport_target],134);}teleport_phase=0;if(autosave_after_teleport){autosave_after_teleport=0;campaign_autosave();}}
        }
        return;
    }
    int dx=!!of_btn(OF_BTN_RIGHT)-!!of_btn(OF_BTN_LEFT),dy=!!of_btn(OF_BTN_DOWN)-!!of_btn(OF_BTN_UP);
    if(dy)world_frame_index=dy<0?(selected==2?43u:44u):(selected==2?35u:36u);
    else if(dx)world_frame_index=dx<0?(selected==2?39u:40u):(selected==2?31u:32u);
    KeenSprite sp;
    if(keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,world_frame_index,&sp))return;
    const uint8_t *meta=sprite_head_file+sprite_info.table_offset+world_frame_index*128u;
    int l=keen_le16(meta+8)/256,t=keen_le16(meta+10)/256,r=keen_le16(meta+12)/256,b=keen_le16(meta+14)/256;
    if(selected==2){
        KeenSprite messie;unsigned width=48,height=32;if(!keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,130,&messie)){width=messie.w;height=messie.h;}
        unsigned mounted=world3.mounted;keen3_world_step(&world3,ticks,level_unpacked,&level_info,&world_x,&world_y,dx,dy,width,height);
        if(!mounted&&world3.mounted)sound_play_named("GETCARDSND");
        if(world3.mounted){int shore_x=world_x,shore_y=world_y;world_x=world3.x/256;world_y=world3.y/256;world_camera();world_x=shore_x;world_y=shore_y;return;}
    }
    int map_blocked=0;unsigned map_steps=0;
    world_pending+=ticks;
    while(world_pending>=6){
        world_pending-=6;map_steps++;
        for(int step=0;step<4;step++){
            if(dy){int yy=world_y+(dy>0?b+1:t-1),blocked=yy<0;
                for(int xx=(world_x+l)/16;xx<=(world_x+r)/16;xx++)if(world_block(xx,yy/16,dy>0?KEEN_FACE_TOP:KEEN_FACE_BOTTOM))blocked=1;
                if(!blocked)world_y+=dy;else map_blocked=1;
            }
            if(dx){int xx=world_x+(dx>0?r+1:l-1),blocked=xx<0;
                for(int yy=(world_y+t)/16;yy<=(world_y+b)/16;yy++)if(world_block(xx/16,yy,dx>0?KEEN_FACE_LEFT:KEEN_FACE_RIGHT))blocked=1;
                if(!blocked)world_x+=dx;else map_blocked=1;
            }
        }
    }
    world_camera();
    if(map_steps && (dx||dy) && !((movement_sfx.ticks>>3)&3u))sound_play_named(map_blocked?"WLDBLOCKSND":"WLDWALKSND");
    if(of_btn_pressed(OF_BTN_A)||of_btn_pressed(OF_BTN_Y)){
        for(int y=(world_y+t)/16;y<=(world_y+b)/16;y++)for(int x=(world_x+l)/16;x<=(world_x+r)/16;x++){
            if(x<0||y<0||x>=(int)level_info.width||y>=(int)level_info.height)continue;
            unsigned id=world_object((unsigned)x,(unsigned)y),level=id&0x7fffu;
            if(selected==2&&(id&0xf00)==0xf00){unsigned target=id&15u,source=(id>>4)&15u;
                if((world3.teleport_mask&(1u<<target))&&(world3.teleport_mask&(1u<<source))){teleport_source=source;teleport_target=target;teleport_x=x;teleport_y=y;teleport_phase=1;teleport_pending=0;sound_play_named("TELEPORTSND");return;}
            }
            if(selected==2&&id==20){keen3_scene_begin(2,ka_jump(&world_random3,3));return;}
            if(selected==2&&(id&0xff00)>=0x2000)continue;
            if(selected==0 && world_data_ready && (id==38 || id==41 || id==46) && ((id>>2)&3u) && (id&3u)){
                unsigned target=(id&3u)-1u,source=((id>>2)&3u)-1u;
                if(target<3 && source<3 && keen_le32(world_data+4+target*10)/256u+16u<=level_info.width*16u &&
                   keen_le32(world_data+8+target*10)/256u+16u<=level_info.height*16u){
                    teleport_source=source;teleport_target=target;teleport_x=x;teleport_y=y;
                    teleport_phase=1;teleport_pending=0;sound_play_named("TELEPORTSND");return;
                }
            }
            if(level>=1&&level<=16){
                map_id=(int)level;load_map();
                if(map_state==1){start_play();sound_play_named("WLDENTERSND");}
                else return_world(0);
                return;
            }
        }
    }
}
static void scene_tiles_frame_mode(uint8_t *fb,int world,int animate){
    memset(fb,9,W*H);
    /* Resolve map markers once per visible tile, then copy atlas scanlines. */
    for(unsigned ty=(unsigned)play_camera_y/16u;ty<level_info.height && ty<=(unsigned)(play_camera_y+H-1)/16u;ty++)
    for(unsigned tx=(unsigned)play_camera_x/16u;tx<level_info.width && tx<=(unsigned)(play_camera_x+W-1)/16u;tx++){
        unsigned tile=world?world_tile(tx,ty):keen_map_tile(level_unpacked,&level_info,tx,ty,0);if(tile>=tile_atlas_count)continue;
        if(animate)tile=menu_tile_frames[(menu_animation_clock.ticks>>4)&3u][tile];
        int x=(int)tx*16-play_camera_x,y=(int)ty*16-play_camera_y;
        int left=x<0?-x:0,right=x+16>W?W-x:16;
        for(int row=0;row<16;row++){
            int py=y+row;if(py<0||py>=H)continue;
            memcpy(fb+py*W+x+left,tile_atlas+tile*256u+(unsigned)row*16u+(unsigned)left,(size_t)(right-left));
        }
    }
}
static void scene_tiles_frame(uint8_t *fb,int world){scene_tiles_frame_mode(fb,world,0);}
static void world_frame(uint8_t *fb){
    if(autosave_result && keen_core_autosave_mode()==KCO_AUTOSAVE_OFF)autosave_result=0;
    if(selected!=0&&selected<3){menu_animation_clock.ticks=movement_sfx.ticks;scene_tiles_frame_mode(fb,1,1);}else scene_tiles_frame(fb,1);
    unsigned animated=of_btn(OF_BTN_LEFT|OF_BTN_RIGHT|OF_BTN_UP|OF_BTN_DOWN)?((movement_sfx.ticks>>4)&3u):0u;
    if(selected!=2||!world3.mounted)draw_masked_sprite(fb,world_x-play_camera_x,world_y-play_camera_y,world_frame_index+animated,0);
    if(selected==2&&world3.ready)draw_masked_sprite(fb,world3.x/256-play_camera_x,world3.y/256-play_camera_y,world3.frame+((movement_sfx.ticks>>1)&1u),0);
    foreground_tiles_frame(fb,1);
    if(autosave_result>0 && (uint32_t)(of_time_us()-autosave_notice_us)<3000000u){
        save_notice_frame(fb,vort_view_height()-2,"AUTO SAVE COMPLETE",0);
    }
    if(autosave_result<0){save_notice_frame(fb,vort_view_height()-2,"AUTO SAVE FAILED - USE MANUAL SAVE",1);}
    if(campaign_complete){rect(fb,0,96,W,38,0);center(fb,101,"ALL FOUR SHIP PARTS RECOVERED",5);center(fb,119,"B GAME MENU",7);}
    corner_hud_frame(fb);
    if(!keen_core_view_enhanced())keen_view_center_original(fb,9);
}

static void status_glyph(void *ctx,int x,int y,unsigned ch){
    uint8_t *fb=ctx;int valid=ch<256&&status_font_count>=256;
    if(valid && x>=0 && y>=0 && x+8<=W && y+8<=H){
        for(unsigned row=0;row<8;row++)memcpy(fb+(y+(int)row)*W+x,status_font_atlas+ch*64u+row*8u,8);
        return;
    }
    for(unsigned yy=0;yy<8;yy++)for(unsigned xx=0;xx<8;xx++){
        int px=x+(int)xx,py=y+(int)yy;
        if(px>=0&&px<W&&py>=0&&py<H)fb[py*W+px]=valid?status_font_atlas[ch*64u+yy*8u+xx]:9;
    }
}
/* Original Vorticons dialog glyphs keep the notification in each episode's style. */
static void save_notice_frame(uint8_t *fb,int bottom,const char *s,int failed){
    int chars=(int)strlen(s),width=chars+2,x=(W-(width+2)*8)/2,y=bottom-24;
    if(width>38)width=38;
    for(int row=0;row<3;row++)for(int col=0;col<width+2;col++){
        unsigned ch=row==0?(!col?1u:col==width+1?3u:2u):row==2?(!col?6u:col==width+1?8u:7u):!col?4u:col==width+1?5u:32u;
        status_glyph(fb,x+col*8,y+row*8,ch);
    }
    for(int i=0;i<chars;i++){
        int gx=(W-chars*8)/2+i*8;status_glyph(fb,gx,y+8,(unsigned char)s[i]);
        for(int row=0;row<8;row++)for(int col=0;col<8;col++){
            uint8_t *pixel=fb+(y+8+row)*W+gx+col;
            if(*pixel==9)*pixel=(uint8_t)(failed?13:11);
        }
    }
}
static void status_tile(void *ctx,int x,int y,unsigned tile){
    uint8_t *fb=ctx;if(tile>=tile_atlas_count)return;
    for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++)
        if(x+xx>=0&&x+xx<W&&y+yy>=0&&y+yy<H)fb[(y+yy)*W+x+xx]=original_pixel(tile,(unsigned)xx,(unsigned)yy);
}
static void status_sprite(void *ctx,int x,int y,unsigned index){draw_masked_sprite(ctx,x,y,index,0);}
/* HUD drawing reads gameplay state without updating it. */
static void corner_hud_icon(uint8_t *fb,int x,int y,unsigned tile,int owned){
    if(tile>=tile_atlas_count)return;
    for(unsigned yy=0;yy<16;yy++)for(unsigned xx=0;xx<16;xx++){
        unsigned color=original_pixel(tile,xx,yy);
        if(color==16||color==24)continue;
        fb[(y+(int)yy)*W+x+(int)xx]=(uint8_t)(owned?color:16);
    }
}
static void corner_hud_frame(uint8_t *fb){
    if(selected>=3||!campaign_active||!play_mode||finale.active||campaign_complete||
       ega_state!=1||sprite_state!=1||status_font_count<256||!keen_core_hud_enabled())return;
    char score[16],lives[16],ammo[16];
    snprintf(score,sizeof(score),"%07lu",(unsigned long)gameplay.score);
    snprintf(lives,sizeof(lives),"%u",keen_lives);snprintf(ammo,sizeof(ammo),"%02u",gameplay.ammo);
    int left=16+(int)strlen(lives)*8,right=16+(int)strlen(ammo)*8;
    if(left<32)left=32;
    if(right<32)right=32;
    int inner=left+right;if(inner<(int)strlen(score)*8)inner=(int)strlen(score)*8;
    unsigned keys=world_mode?0:hud_required_keys;int height=keys?58:40;
    char extra[16]={0};
    if(keen_core_hud_enhanced()){
        if(world_mode&&selected<2){unsigned count=0,mask=selected?gameplay.targets_mask:gameplay.ship_mask;for(;mask;mask>>=1)count+=mask&1u;snprintf(extra,sizeof(extra),selected?"CITIES %u/8":"PARTS %u/4",count);}
        else if(!world_mode&&selected==2&&gameplay.ankh_ticks)snprintf(extra,sizeof(extra),"ANKH %u",(gameplay.ankh_ticks+143)/144);
    }
    int extra_y=height;
    if(extra[0]){height+=14;if(inner<80)inner=80;}
    int x=8,y=8,width=inner+16;KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    rect(fb,x,y,width,height,9);rect(fb,x+1,y+1,width-2,height-2,24);
    rect(fb,x+2,y+2,width-4,1,16);rect(fb,x+2,y+3,1,height-5,16);
    rect(fb,x+2,y+height-2,width-4,1,17);rect(fb,x+width-2,y+2,1,height-3,17);
    ks_text(&c,x+8,y+6,score,0);
    rect(fb,x+4,y+16,width-8,1,16);rect(fb,x+6+left,y+17,1,19,16);
    draw_masked_sprite(fb,x+6,y+19,selected==2?31:32,0);ks_text(&c,x+24,y+23,lives,0);
    corner_hud_icon(fb,x+8+left,y+19,selected==2?216:414,1);ks_text(&c,x+24+left,y+23,ammo,0);
    if(keys){
        unsigned count=0;for(unsigned i=0;i<4;i++)count+=(keys>>i)&1u;
        rect(fb,x+4,y+36,width-8,1,16);int key_x=x+(width-(int)count*16)/2;
        for(unsigned i=0;i<4;i++)if(keys&(1u<<i)){corner_hud_icon(fb,key_x,y+39,(selected==2?217:424)+i,(gameplay.key_mask>>i)&1u);key_x+=16;}
    }
    if(extra[0]){rect(fb,x+4,y+extra_y-2,width-8,1,16);ks_text(&c,x+8,y+extra_y+1,extra,0);}
}
static void status_frame(uint8_t *fb){
    memcpy(fb,status_backdrop,sizeof(status_backdrop));if(status_opening<14)status_opening++;
    KeenStatusCanvas canvas={fb,status_glyph,status_tile,status_sprite};
    KeenStatusValues values={gameplay.score,gameplay.extra_life_base+20000u,keen_lives,gameplay.ammo,
        gameplay.key_mask,gameplay.ship_mask,(unsigned)gameplay.found_pogo,gameplay.targets_mask,gameplay.ankh_ticks/144u};
    keen_status_draw(&canvas,(unsigned)selected,&values,status_opening);
}

static void load_special_data(void){
    size_t size=0;int index=episode_file(selected,episode_name("SPECIAL.CK1"));
    special_ready=selected<3 && index>=0 && !read_asset(index,special_data,sizeof(special_data),&size) &&
        size==sizeof(special_data) && !memcmp(special_data,selected==2?"KS03":selected==1?"KS02":"KS01",4);
    if(special_ready)for(unsigned i=0;i<(selected==2?0u:selected==1?2u:7u);i++)if(!memchr(special_data+74+i*(selected==1?512u:256u),0,selected==1?510u:254u))special_ready=0;
}
static void set_game_lights(int on){
    if(!on && !special_ready)return;
    uint32_t colors[25];memcpy(colors,base_colors,sizeof(colors));game_lights=!!on;
    if(!on)for(unsigned i=0;i<16;i++){
        unsigned c=special_data[4+17+i];
        colors[9+i]=ega_palette_color(c);
    }
    of_video_palette_bulk(colors,25);
}

static void show_lives(void){
    if(!campaign_active||!world_mode)return;
    world_frame(status_backdrop);play_notice=1;notice_step=notice_ready=0;notice_wait_release=1;
    keen_sfx_reset(&notice_clock,of_time_us());paused_sound_priority=-1;
}
static int show_statue(int x,int y){
    if(!special_ready||play_notice)return 0;
    const char *message=0;
    for(unsigned i=0;i<(selected==1?2u:7u);i++)if(special_data[72+i*(selected==1?512u:256u)]==play_level){message=(const char*)special_data+74+i*(selected==1?512u:256u);break;}
    if(!message)return 0;
    play_frame(status_backdrop);status_pause_audio();play_notice=2;notice_text=message;
    notice_tile_x=x;notice_tile_y=y;notice_wait_release=1;return 1;
}
static int notice_tick(void){
    if(!play_notice)return 0;
    if(play_notice==1 && notice_step<8)return 1;
    if(notice_wait_release){if(!of_btn(STATUS_BUTTONS))notice_wait_release=0;}
    if(play_notice==1){
        keen_sfx_clock_step(&notice_clock,of_time_us());
        if(!kt_delay_elapsed(notice_clock.ticks,60) && (notice_wait_release||!of_btn_pressed(STATUS_BUTTONS)))return 1;
    }else{
        if(notice_wait_release||!of_btn_pressed(OF_BTN_A))return 1;
        (void)map_set_tile(notice_tile_x,notice_tile_y,selected==1?143u:play_level==11?434u:315u);
    }
    play_notice=0;status_block_buttons=1;return 1;
}
static void notice_frame(uint8_t *fb){
    memcpy(fb,status_backdrop,sizeof(status_backdrop));KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    if(play_notice==1){
        if(notice_step<8)notice_step++;
        int width=(int)notice_step*2,height=4-(16-width);if(height<1)height=1;
        keen_text_box(&c,width,height);if(notice_step<8)return;
        if(!notice_ready){notice_ready=1;keen_sfx_reset(&notice_clock,of_time_us());sound_play_named("KEENSLEFT");}
        ks_text(&c,96,104,"   Keens Left",0);
        for(unsigned i=0;i<keen_lives && i<6;i++)draw_masked_sprite(fb,104+(int)i*16,115,selected==2?35:36,0);
    }else{
        keen_text_box(&c,selected==1?22:21,selected==1?13:10);int x=selected==1?72:76,y=selected==1?64:80;
        for(const unsigned char*p=(const unsigned char*)notice_text;*p;p++){
            if(*p==10){x=selected==1?72:76;y+=8;continue;}
            if(x<248 && y<(selected==1?160:144))status_glyph(fb,x,y,*p);
            x+=8;
        }
        ks_text(&c,116,selected==1?168:144,"Press A:",0);status_glyph(fb,188,selected==1?168:144,9+(of_time_us()/85610u)%3u);
    }
}

static int game_menu_release;
static unsigned game_menu_save_slot;
static const char *game_menu_message;
static void campaign_save_path(char *path,unsigned slot){snprintf(path,40,"RetroDrivenKeen%u_%u.sav",(unsigned)selected+1,slot);}
static void game_menu_open(void){
    if(finale.active)finale_frame(status_backdrop);
    else if(world_mode)world_frame(status_backdrop);else play_frame(status_backdrop);
    status_pause_audio();game_menu_visible=1;game_menu_page=game_menu_selection=0;
    game_menu_release=1;game_menu_message=0;
}
static void game_menu_close(void){game_menu_visible=0;status_block_buttons=1;}
static KeenSave campaign_snapshot(void){
    KeenSave save={0,gameplay.score,gameplay.extra_life_base,keen_lives,gameplay.ammo,
        selected==1?gameplay.targets_mask:gameplay.ship_mask,(unsigned)gameplay.found_pogo,completed_levels,(unsigned)world_x,(unsigned)world_y,gameplay.deaths};
    return save;
}
static void autosave_write_snapshot(const KeenSave *save,unsigned policy,int confirmed){
    unsigned current=keen_core_autosave_mode();
    /* Recheck Auto Save here to honor Off if settings changed after scheduling. */
    if(policy==KCO_AUTOSAVE_OFF || current==KCO_AUTOSAVE_OFF ||
       (!confirmed && (policy!=KCO_AUTOSAVE_ON || current!=KCO_AUTOSAVE_ON))){
        autosave_result=0;return;
    }
    autosave_result=campaign_state_write(campaign_state_path(),save)?1:-1;
    if(autosave_result>0){autosave_present=1;autosave_notice_us=of_time_us();}
    movement_sfx.last_us=of_time_us();
}
static void campaign_autosave_mode(unsigned mode){
    if(!campaign_active||!world_mode||map_state!=1||map_id!=80)return;
    unsigned current=keen_core_autosave_mode();
    if(current==KCO_AUTOSAVE_OFF)mode=KCO_AUTOSAVE_OFF;
    else if(current==KCO_AUTOSAVE_PROMPT && mode==KCO_AUTOSAVE_ON)mode=KCO_AUTOSAVE_PROMPT;
    if(mode==KCO_AUTOSAVE_OFF){autosave_result=0;return;}
    KeenSave save=campaign_snapshot();
    if(mode==KCO_AUTOSAVE_PROMPT){
        autosave_prompt_save=save;autosave_prompt_pending=1;autosave_prompt_ready=0;autosave_result=0;return;
    }
    autosave_write_snapshot(&save,mode,0);
}
static void campaign_autosave(void){campaign_autosave_mode(keen_core_autosave_mode());}
static int autosave_prompt_tick(void){
    if(!autosave_prompt_pending)return 0;
    int choice=-1;
    if(keen_core_autosave_mode()==KCO_AUTOSAVE_OFF)choice=0;
    else if(!autosave_prompt_ready){
        if(!of_btn(STATUS_BUTTONS))autosave_prompt_ready=1;
        return 1;
    }else if(of_btn_pressed(OF_BTN_A))choice=1;
    else if(of_btn_pressed(OF_BTN_B))choice=0;
    if(choice<0)return 1;
    if(choice)autosave_write_snapshot(&autosave_prompt_save,KCO_AUTOSAVE_PROMPT,1);
    autosave_prompt_pending=autosave_prompt_ready=0;status_block_buttons=1;
    movement_sfx.last_us=of_time_us();
    if(campaign_complete)finale_begin();
    return 1;
}
static void autosave_prompt_frame(uint8_t*fb){
    world_frame(fb);KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    keen_text_box(&c,26,5);
    ks_text(&c,72,100,"Save this checkpoint?",0);
    ks_text(&c,104,124,"A Save   B Skip",0);
}
static int campaign_restore(const KeenSave*save){
    /* Validation reuses only compressed-byte scratch, leaving the current scene intact on failure. */
    KeenMapInfo world_info;size_t world_size=0;int world_index=episode_file(selected,episode_name("LEVEL80.CK1"));
    if(world_index<0 || read_asset(world_index,level_compressed,sizeof(level_compressed),&world_size) ||
       keen_map_decode(level_compressed,world_size,sprite_raw,sizeof(sprite_raw),&world_info) ||
       save->x+16u>world_info.width*16u || save->y+16u>world_info.height*16u)return 0;
    cinema3.active=0;keen_gameplay_reset(&gameplay);gameplay.score=save->score;gameplay.extra_life_base=save->extra;
    gameplay.ammo=save->ammo;if(selected==1)gameplay.targets_mask=save->parts;else gameplay.ship_mask=save->parts;gameplay.found_pogo=(int)save->pogo;
    gameplay.deaths=save->deaths;keen_lives=save->lives;completed_levels=save->levels;
    autosave_prompt_pending=autosave_prompt_ready=0;
    world_x=(int)save->x;world_y=(int)save->y;campaign_active=1;keen_game_over=campaign_complete=0;autosave_result=0;
    autosave_after_teleport=0;score_flow_active=0;return_world(-1);return world_mode;
}
static int campaign_load(unsigned slot);
static int campaign_load_auto(void){
    if(selected>=3||!campaign_namespace_ready())return 0;
    KeenSave save;if(!campaign_state_read(campaign_state_path(),&save)||!campaign_restore(&save))return 0;
    if(selected==2?!!(completed_levels&32768u):selected==1?gameplay.targets_mask==255:gameplay.ship_mask==15){campaign_complete=1;finale_begin();}
    else show_lives();
    return 1;
}
static void campaign_save_selected(void);
/* Pointer-free, fixed-width application snapshot; never stores FILE/mixer/GPU handles. */
typedef struct {
 uint32_t score,extra,targets,ankh,collected,deaths,ammo,keys,parts,pogo,exit;
 uint8_t consumed[KEEN_CONSUMED_BYTES];
} KeenSavedGameplay;
typedef struct {
 uint32_t level,map_bytes,asset_hash;
 KeenSave campaign;KeenSavedGameplay gameplay;
 KeenMotion player;KeenVortMotion motion;KeenAnimation animation;
 KeenWeapon weapon;KeenActors actors;KeenDoor doors[KEEN_DOORS];KeenBridge bridges[KEEN_BRIDGES];
 uint32_t ticks,remainder_lo,remainder_hi,motion_ticks,actor_ticks,weapon_ticks,door_ticks;
 uint32_t door_count,chain_applied,background,pogo,lights,frozen;
 int32_t freeze_time,camera_x,camera_y;
 uint32_t audio_id,audio_offset;int32_t audio_priority;
} KeenLevelSave;
_Static_assert(sizeof(int)==4&&sizeof(unsigned)==4&&_Alignof(KeenLevelSave)==4,"Manual level format requires 32-bit fields");
#define KLV_ABI (selected==2?0x33000000u+(uint32_t)(sizeof(KeenLevelSave)+sizeof(KeenActors3)):selected==1?0x22000000u+(uint32_t)(sizeof(KeenLevelSave)+sizeof(KeenActors2)):0x14000000u+(uint32_t)sizeof(KeenLevelSave))
static union {KeenLevelSave level;uint8_t raw[KMS_PAYLOAD];} manual_blob;
static size_t manual_header_bytes(void){return sizeof(KeenLevelSave)+(selected==2?sizeof(KeenActors3):selected==1?sizeof(KeenActors2):0);}
static KeenActors3 *manual_actors3(void){return (KeenActors3*)(void*)(manual_blob.raw+sizeof(KeenLevelSave));}
static KeenActors2 *manual_actors2(void){return (KeenActors2*)(void*)(manual_blob.raw+sizeof(KeenLevelSave));}
static KeenMapInfo manual_validated_map;
static unsigned manual_validated_keys;
static uint8_t manual_actor_metadata[294];
static int manual_level_allowed(void){
 return selected<3&&campaign_namespace_ready()&&campaign_active&&play_mode&&!world_mode&&map_state==1&&map_id>=1&&map_id<=16&&
        !keen_death.state&&!keen_game_over&&!exit_active&&!finale.active&&!cinema3.active&&!score_flow_active&&!play_notice;
}
static uint32_t manual_assets_hash(unsigned level){
 char name[24];snprintf(name,sizeof(name),"LEVEL%02u.CK%u",level,(unsigned)selected+1);size_t n=0;
 int index=episode_file(selected,name);
 if(index<0||read_asset(index,level_compressed,sizeof(level_compressed),&n))return 0;
 return kms_hash(level_compressed,n)^kms_hash(sprite_head_file,sprite_head_len)^kms_hash(tile_flags,sizeof(tile_flags));
}
static void manual_gameplay_capture(KeenSavedGameplay*s){
 s->score=gameplay.score;s->extra=gameplay.extra_life_base;s->targets=gameplay.targets_mask;s->ankh=gameplay.ankh_ticks;
 s->collected=gameplay.collected;s->deaths=gameplay.deaths;s->ammo=gameplay.ammo;s->keys=gameplay.key_mask;
 s->parts=gameplay.ship_mask;s->pogo=(unsigned)gameplay.found_pogo;s->exit=(unsigned)gameplay.reached_exit;
 memcpy(s->consumed,gameplay.consumed,sizeof(s->consumed));
}
static void manual_gameplay_restore(const KeenSavedGameplay*s){
 keen_gameplay_reset(&gameplay);gameplay.score=s->score;gameplay.extra_life_base=s->extra;
 gameplay.targets_mask=s->targets;gameplay.ankh_ticks=s->ankh;gameplay.collected=s->collected;
 gameplay.deaths=s->deaths;gameplay.ammo=s->ammo;gameplay.key_mask=s->keys;gameplay.ship_mask=s->parts;
 gameplay.found_pogo=(int)s->pogo;gameplay.reached_exit=(int)s->exit;
 memcpy(gameplay.consumed,s->consumed,sizeof(s->consumed));gameplay.event="LEVEL GAME LOADED";
}
static int manual_level_capture(void){
 if(!manual_level_allowed()||level_info.decoded_bytes>KMS_PAYLOAD-manual_header_bytes())return 0;
 KeenLevelSave*s=&manual_blob.level;memset(s,0,sizeof(*s));
 s->level=(unsigned)map_id;s->map_bytes=level_info.decoded_bytes;s->asset_hash=manual_assets_hash(s->level);
 if(!s->asset_hash)return 0;
 s->campaign=campaign_snapshot();manual_gameplay_capture(&s->gameplay);
 s->player=player;s->motion=vort_motion;s->animation=player_animation;s->weapon=weapon;s->actors=level_actors;
 memcpy(s->doors,doors,sizeof(doors));memcpy(s->bridges,bridges,sizeof(bridges));
 s->ticks=movement_sfx.ticks;s->remainder_lo=(uint32_t)movement_sfx.remainder;s->remainder_hi=(uint32_t)(movement_sfx.remainder>>32);
 s->motion_ticks=motion_ticks;s->actor_ticks=actor_ticks;s->weapon_ticks=weapon_ticks;s->door_ticks=door_ticks;
 s->door_count=door_count;s->chain_applied=chain_applied;s->background=play_background_tile;s->pogo=(unsigned)pogo_mode;
 s->lights=(unsigned)game_lights;s->frozen=(unsigned)keen_frozen;s->freeze_time=freeze_time;
 s->camera_x=play_camera_x;s->camera_y=play_camera_y;
 s->audio_id=sound_active_id;s->audio_priority=paused_sound_priority;s->audio_offset=(uint32_t)paused_sound_offset;
 if(selected==1)*manual_actors2()=level_actors2;
 if(selected==2)*manual_actors3()=level_actors3;
 memcpy(manual_blob.raw+manual_header_bytes(),level_unpacked,s->map_bytes);return 1;
}
static int manual_velocity(int v){return v>=-4096&&v<=4096;}
static int manual_position(int32_t p,unsigned tiles){return (int64_t)p>=-1048576&&(int64_t)p<=(int64_t)tiles*4096+1048576;}
static int manual_level_valid(size_t length){
 const KeenLevelSave*s=&manual_blob.level;
 if(length<manual_header_bytes()||s->level<1||s->level>16||s->map_bytes>KMS_PAYLOAD-manual_header_bytes()||length!=manual_header_bytes()+s->map_bytes||
    s->asset_hash!=manual_assets_hash(s->level)||s->door_count>KEEN_DOORS||s->chain_applied>4||
    s->pogo>1||s->lights>1||s->frozen>1||s->gameplay.keys>15||s->gameplay.parts>15||s->gameplay.pogo>1||s->gameplay.exit||
    s->motion.mode>KV_POGO_AIR||s->motion.pending>=6||s->player.grounded<0||s->player.grounded>1||
    !manual_velocity(s->motion.vx)||!manual_velocity(s->motion.vy)||!manual_velocity(s->motion.saved)||
    !manual_velocity(s->player.vx)||!manual_velocity(s->player.vy)||s->motion.charge>1000||
    (((uint64_t)s->remainder_hi<<32)|s->remainder_lo)>=(uint64_t)KS_TICK_DIVISOR*1000000u||
    s->animation.pose>KEEN_POSE_POGO||s->animation.frame>=sprite_info.sprite_count||
    (s->animation.facing!=-1&&s->animation.facing!=1)||s->weapon.shooting>1||s->audio_priority< -1||
    (s->audio_priority>=0&&(s->audio_id>=sound_bank.count||s->audio_offset>=KS_PCM_CAP)))return 0;
 uint8_t raw[KSV_BYTES];KeenSave valid;campaign_encode(raw,&s->campaign);if(!campaign_decode(raw,&valid))return 0;
 if(s->gameplay.score!=s->campaign.score||s->gameplay.ammo!=s->campaign.ammo||s->gameplay.extra!=s->campaign.extra||
    (selected==1?s->gameplay.targets:s->gameplay.parts)!=s->campaign.parts||s->gameplay.pogo!=s->campaign.pogo||s->gameplay.deaths!=s->campaign.deaths)return 0;
 char name[24];snprintf(name,sizeof(name),"LEVEL%02u.CK%u",s->level,(unsigned)selected+1);size_t n=0;KeenMapInfo info;
 int index=episode_file(selected,name);
 if(index<0||read_asset(index,level_compressed,sizeof(level_compressed),&n)||
    keen_map_decode(level_compressed,n,sprite_raw,sizeof(sprite_raw),&info)||info.decoded_bytes!=s->map_bytes||
    memcmp(sprite_raw,manual_blob.raw+manual_header_bytes(),32))return 0;
 manual_validated_map=info;manual_validated_keys=hud_map_keys(sprite_raw,&info);
 unsigned width=info.width,height=info.height;
 if(s->player.x<0||s->player.x>(int)width*16||s->player.y<0||s->player.y>(int)height*16+32||
    !manual_position(s->motion.fx,width)||!manual_position(s->motion.fy,height)||s->camera_x<0||s->camera_y<0||
    s->camera_x>(int)width*16||s->camera_y>(int)height*16||s->background>=tile_atlas_count)return 0;
 for(unsigned i=0;i<(unsigned)info.width*info.height;i++)
  if(keen_le16(manual_blob.raw+manual_header_bytes()+32+i*2)>=tile_atlas_count)return 0;
 if(selected==2){
  const KeenActors3*w=manual_actors3();const KeenActors*a=&w->core;
  if(!a->ready||a->count>KA_MAX||a->width!=width||a->height!=height||a->pending>=6||a->rnd>255||a->j1<1||a->j1>17||a->j2<1||a->j2>17||w->body_count>32||w->sparks>6||w->arm_destruct>1||w->destruct>1||w->finished>1||w->arm_row>19||w->row>24||w->body_pending>=6)return 0;
  for(unsigned i=0;i<a->count;i++){const KeenActor*o=&a->actors[i];if(o->kind&&(o->kind>13||o->mode>K3_RUN||o->frame<47||o->frame>=148||!manual_position(w->old_x[i],width)||!manual_position(w->old_y[i],height)||!manual_position(o->x,width)||!manual_position(o->y,height)||!manual_velocity(o->vx)||!manual_velocity(o->vy)))return 0;}
  for(unsigned i=0;i<101;i++){KeenActorBox b=w->boxes[i];if(b.l<0||b.t<0||b.r>128||b.b>128||b.l>b.r||b.t>b.b)return 0;}
  for(unsigned i=0;i<w->body_count;i++){const KeenBody3*b=&w->bodies[i];if(b->kind&&(b->kind<14||b->kind>18||b->x<0||b->y<0||b->x>=(int)width||b->y>=(int)height||b->time>400||b->wait>201||(b->direction!=-1&&b->direction!=1)||(b->side!=-1&&b->side!=1)||(b->kind==16&&b->period!=15&&b->period!=40)))return 0;}
  for(unsigned i=0;i<KA_BULLETS;i++)if(a->shots[i].state>3||a->shots[i].kind>3||!manual_position(a->shots[i].x,width)||!manual_position(a->shots[i].y,height)||!manual_velocity(a->shots[i].vx)||!manual_velocity(a->shots[i].vy))return 0;
 }else if(selected==1){
  const KeenActors2*w=manual_actors2();const KeenActors*a=&w->core;
  if(!a->ready||a->count>KA_MAX||a->width!=width||a->height!=height||a->pending>=6||a->rnd>255||
     a->j1<1||a->j1>17||a->j2<1||a->j2>17||w->level!=s->level||w->targets>255||w->lights>1||
     w->blast_active>1||(w->blast_active&&w->blast_phase>=11)||w->blast_phase>11||w->blast_x>=width||w->blast_y>=height||w->blast_time>=40)return 0;
  for(unsigned i=0;i<a->count;i++){
   const KeenActor*o=&a->actors[i];if(o->kind&&(o->kind>7||o->mode>K2_SCRUB_FALL||o->frame<48||o->frame>=134||
       !manual_position(o->x,width)||!manual_position(o->y,height)||!manual_position(w->old_x[i],width)||!manual_position(w->old_y[i],height)||
       !manual_velocity(o->vx)||!manual_velocity(o->vy)))return 0;
  }
  for(unsigned i=0;i<86;i++){KeenActorBox box=w->boxes[i];
   if(box.l< -32||box.t< -32||box.r>128||box.b>128||box.l>box.r||box.t>box.b)return 0;}
  for(unsigned i=0;i<KA_BULLETS;i++)if(a->shots[i].state>3||a->shots[i].kind>1||
      !manual_position(a->shots[i].x,width)||!manual_position(a->shots[i].y,height))return 0;
 }else{
 const KeenActors*a=&s->actors;
 if(!a->ready||a->count>KA_MAX||a->cannon_count>KA_MAX||a->j1<1||a->j1>17||a->j2<1||a->j2>17||a->rnd>255||
    a->pending>=6||a->width!=width||a->height!=height||a->chain_steps>4)return 0;
 for(unsigned i=0;i<a->count;i++){
  const KeenActor*o=&a->actors[i];
  if(o->kind && ((o->kind>5&&o->kind!=10)||o->mode>KA_CORPSE||o->frame<48||o->frame>114||
     !manual_position(o->x,width)||!manual_position(o->y,height)||!manual_velocity(o->vx)||!manual_velocity(o->vy)))return 0;
 }
 for(unsigned i=0;i<67;i++){
  KeenActorBox box=a->boxes[i];if(box.l< -32||box.t< -32||box.r>128||box.b>128||box.l>box.r||box.t>box.b)return 0;
 }
 for(unsigned i=0;i<a->cannon_count;i++)if(a->cannons[i].x>=width||a->cannons[i].y>=height||a->cannons[i].variant>3)return 0;
 for(unsigned i=0;i<KA_BULLETS;i++)if(a->shots[i].state>3||a->shots[i].kind>3||
    !manual_position(a->shots[i].x,width)||!manual_position(a->shots[i].y,height))return 0;
 }
 for(unsigned i=0;i<KW_SHOTS;i++)if(s->weapon.shots[i].state>3||
    !manual_position(s->weapon.shots[i].x,width)||!manual_position(s->weapon.shots[i].y,height))return 0;
 for(unsigned i=0;i<s->door_count;i++){
  const KeenDoor*d=&s->doors[i];if(d->x>=width||d->y+1>=height||d->tile+1>=tile_atlas_count||d->background>=tile_atlas_count||d->time>165)return 0;
 }
 for(unsigned i=0;i<KEEN_BRIDGES;i++)if(s->bridges[i].mode){
  const KeenBridge*b=&s->bridges[i];if(b->mode>3||b->x<0||b->y<0||b->x>=(int)width||b->y>=(int)height||
      (b->direction!=-1&&b->direction!=1)||b->count>width||b->base>=tile_atlas_count)return 0;
 }
 index=episode_file(selected,episode_name("LEVEL80.CK1"));
 if(index<0||read_asset(index,level_compressed,sizeof(level_compressed),&n)||
    keen_map_decode(level_compressed,n,sprite_raw,sizeof(sprite_raw),&info)||
    s->campaign.x+16>info.width*16u||s->campaign.y+16>info.height*16u)return 0;
 index=episode_file(selected,episode_name("ACTORS.CK1"));
 if(index<0||read_asset(index,manual_actor_metadata,sizeof(manual_actor_metadata),&n)||
    n!=sizeof(manual_actor_metadata)||memcmp(manual_actor_metadata,selected==2?"KA03":selected==1?"KA02":"KA01",4))return 0;
 return 1;
}
static int manual_level_restore(size_t length){
 if(selected>=3||!campaign_namespace_ready()||!manual_level_valid(length))return 0;
 const KeenLevelSave*s=&manual_blob.level;
 /* Commit the validated scratch snapshot without another SD read. */
 sound_stop();paused_sound_priority=-1;campaign_active=1;map_id=play_level=(int)s->level;
 level_info=manual_validated_map;hud_required_keys=manual_validated_keys;map_state=1;memcpy(level_unpacked,manual_blob.raw+manual_header_bytes(),s->map_bytes);
 memcpy(actor_data,manual_actor_metadata,sizeof(actor_data));actor_data_len=sizeof(actor_data);
 find_spawn();memset(&keen_death,0,sizeof(keen_death));exit_active=0;finale.active=0;cinema3.active=0;score_flow_active=0;
 world_pending=teleport_phase=teleport_pending=0;collision_mode=1;
 manual_gameplay_restore(&s->gameplay);keen_lives=s->campaign.lives;completed_levels=s->campaign.levels;
 world_x=(int)s->campaign.x;world_y=(int)s->campaign.y;
 if(selected==1)level_actors2=*manual_actors2();
 if(selected==2)level_actors3=*manual_actors3();
 player=s->player;vort_motion=s->motion;player_animation=s->animation;weapon=s->weapon;level_actors=s->actors;
 memcpy(doors,s->doors,sizeof(doors));memcpy(bridges,s->bridges,sizeof(bridges));
 movement_sfx.ticks=s->ticks;movement_sfx.remainder=((uint64_t)s->remainder_hi<<32)|s->remainder_lo;movement_sfx.last_us=of_time_us();
 motion_ticks=s->motion_ticks;actor_ticks=s->actor_ticks;weapon_ticks=s->weapon_ticks;door_ticks=s->door_ticks;
 door_count=s->door_count;chain_applied=s->chain_applied;play_background_tile=s->background;pogo_mode=(int)s->pogo;
 keen_frozen=(int)s->frozen;freeze_time=s->freeze_time;play_camera_x=s->camera_x;play_camera_y=s->camera_y;
 set_game_lights((int)s->lights);keen_game_over=campaign_complete=world_mode=game_menu_visible=0;play_mode=1;
 autosave_prompt_pending=autosave_prompt_ready=0;autosave_after_teleport=0;play_notice=status_visible=0;status_block_buttons=1;
 if(s->audio_priority>=0&&s->audio_id<sound_bank.count){
  int truncated=0;size_t n=ks_render(sound_file,sound_len,&sound_bank,s->audio_id,sound_pcm,KS_PCM_CAP,&truncated);
  if(n>s->audio_offset){sound_samples=n;sound_active_id=s->audio_id;sound_pcm_start=0;
   paused_sound_priority=(int)sound_bank.entries[s->audio_id].priority;paused_sound_offset=s->audio_offset;}
 }
 return 1;
}
static int campaign_load(unsigned slot){
 if(selected>=3||!campaign_namespace_ready())return 0;
 char path[40];campaign_save_path(path,slot);KeenManualRecord record;KeenSave save;
 if(keen_manual_read(path,manual_blob.raw,sizeof(manual_blob.raw),KLV_ABI,&record)){
  if(record.kind)return manual_level_restore(record.length);
  if(record.length!=KSV_BYTES||!campaign_decode(manual_blob.raw,&save))return 0;
 }else if(!campaign_state_read(path,&save))return 0;
 if(!campaign_restore(&save))return 0;
 show_lives();return 1;
}
static void campaign_save_selected(void){
 char path[40];campaign_save_path(path,game_menu_save_slot);int ok=0;
 if(world_mode){KeenSave save=campaign_snapshot(),valid;campaign_encode(manual_blob.raw,&save);
  if(campaign_decode(manual_blob.raw,&valid)&&campaign_namespace_ready())ok=keen_manual_write(path,manual_blob.raw,KSV_BYTES,0,KLV_ABI);
 }else if(manual_level_capture()&&manual_level_valid(manual_header_bytes()+manual_blob.level.map_bytes))
  ok=keen_manual_write(path,manual_blob.raw,manual_header_bytes()+manual_blob.level.map_bytes,1,KLV_ABI);
 game_menu_message=ok?"GAME SAVED":"SAVE FAILED";
 game_menu_page=1;game_menu_selection=(int)game_menu_save_slot;
}

/* Menu and text scrolling use separate repeat timers; reset on menu or direction changes. */
static int menu_repeat_context,menu_repeat_direction;
static uint32_t menu_repeat_next_us;
static int menu_navigation(int context){
    int down=of_btn(OF_BTN_DOWN)||of_btn_pressed(OF_BTN_DOWN);
    int up=of_btn(OF_BTN_UP)||of_btn_pressed(OF_BTN_UP);
    int direction=down-up;uint32_t now=of_time_us();
    if(context!=menu_repeat_context){
        menu_repeat_context=context;menu_repeat_direction=0;
        if(!of_btn_pressed(OF_BTN_UP|OF_BTN_DOWN)){
            menu_repeat_direction=direction;menu_repeat_next_us=now+300000u;return 0;
        }
    }
    if(!direction){menu_repeat_direction=0;return 0;}
    if(direction!=menu_repeat_direction || of_btn_pressed(OF_BTN_UP|OF_BTN_DOWN) ||
       (int32_t)(now-menu_repeat_next_us)>=0){
        menu_repeat_next_us=now+(direction!=menu_repeat_direction?300000u:110000u);
        menu_repeat_direction=direction;return direction;
    }
    return 0;
}

static void game_menu_tick(void){
    if(game_menu_release){if(!of_btn(STATUS_BUTTONS))game_menu_release=0;return;}
    if(of_btn_pressed(OF_BTN_B)){
        if(game_menu_page){game_menu_page=game_menu_selection=0;game_menu_message=0;game_menu_release=1;}
        else game_menu_close();
        return;
    }
    unsigned count=game_menu_page==0?5u:game_menu_page>=3?2u:game_menu_page==2?10u:9u;
    int navigation=menu_navigation(100+game_menu_page);
    game_menu_selection=(game_menu_selection+(int)count+navigation)%(int)count;
    if(!of_btn_pressed(OF_BTN_A))return;
    game_menu_release=1;
    if(game_menu_page==0){
        if(game_menu_selection==0){game_menu_close();return;}
        if((game_menu_selection==1||game_menu_selection==2) && (!campaign_active||selected>=3||!campaign_namespace_ready())){game_menu_message="START A CAMPAIGN FIRST";return;}
        if(game_menu_selection==1 && !world_mode && !manual_level_allowed()){game_menu_message="CANNOT SAVE DURING THIS TRANSITION";return;}
        if(game_menu_selection==1||game_menu_selection==2){
            game_menu_page=game_menu_selection;game_menu_selection=0;game_menu_message=0;
            KeenSave saved;autosave_present=campaign_state_read(campaign_state_path(),&saved)>0;return;
        }
        if(game_menu_selection==3){sound_enabled=!sound_enabled;game_menu_message=sound_enabled?"SOUND ON":"SOUND OFF";return;}
        game_menu_page=3;game_menu_selection=0;game_menu_message=0;return;
    }
    if(game_menu_page==4){
        if(game_menu_selection==0)campaign_save_selected();
        else{game_menu_page=1;game_menu_selection=(int)game_menu_save_slot;}
        return;
    }
    if(game_menu_page==3){
        if(game_menu_selection==0){game_menu_close();return;}
        game_menu_visible=play_mode=world_mode=campaign_active=0;paused_sound_priority=-1;sound_stop();episode_menu_open();return;
    }
    if(game_menu_page==2){
        if(game_menu_selection==0?campaign_load_auto():campaign_load((unsigned)game_menu_selection-1)){game_menu_close();}
        else game_menu_message="EMPTY OR INVALID SAVE";
        return;
    }
    game_menu_save_slot=(unsigned)game_menu_selection;
    char path[40];campaign_save_path(path,game_menu_save_slot);FILE *existing=fopen(path,"rb");
    if(existing){fclose(existing);game_menu_page=4;game_menu_selection=0;game_menu_message=0;}
    else campaign_save_selected();
}
static void game_menu_frame(uint8_t *fb){
    memcpy(fb,status_backdrop,sizeof(status_backdrop));KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    keen_status_box(&c,0,14);int x=64,y=76;
    ks_text(&c,x,y,game_menu_page==1?"SAVE GAME":game_menu_page==2?"LOAD GAME":game_menu_page==3?"END GAME?":game_menu_page==4?"OVERWRITE SAVE?":"COMMANDER KEEN",1);
    if(game_menu_page==0){
        static const char *const labels[]={"RESUME GAME","SAVE GAME","LOAD GAME","SOUND","END GAME"};
        for(int i=0;i<5;i++){ks_text(&c,x,y+16+i*12,i==game_menu_selection?">":" ",0);ks_text(&c,x+16,y+16+i*12,labels[i],0);}
    }else if(game_menu_page==4){
        ks_text(&c,x,y+24,game_menu_selection==0?"> OVERWRITE":"  OVERWRITE",0);
        ks_text(&c,x,y+40,game_menu_selection==1?"> CANCEL":"  CANCEL",0);
    }else if(game_menu_page==3){
        ks_text(&c,x,y+24,game_menu_selection==0?"> KEEP PLAYING":"  KEEP PLAYING",0);
        ks_text(&c,x,y+40,game_menu_selection==1?"> END GAME":"  END GAME",0);
    }else{
        if(game_menu_page==2)ks_text(&c,x,y+16,game_menu_selection==0?"> Autosave":"  Autosave",0);
        for(unsigned i=0;i<9;i++){
            int xx=x+(int)(i%3)*64,yy=y+(game_menu_page==2?40:24)+(int)(i/3)*16;
            int selected_slot=game_menu_page==2?game_menu_selection-1:game_menu_selection;
            ks_text(&c,xx,yy,(int)i==selected_slot?">":" ",0);ks_number(&c,xx+8,yy,i+1,0);
        }
    }
    ks_text(&c,48,164,game_menu_message?game_menu_message:"A CHOOSE  B BACK",0);
}

static int draw_original_bitmap_at(uint8_t *fb,unsigned index,int x,int y){
    if(sprite_head_len<48 || ega_state!=1)return 0;
    unsigned count=keen_le16(sprite_head_file+34);uint32_t table=keen_le32(sprite_head_file+8);
    if(index>=count || (uint64_t)table+(index+1u)*16u>sprite_head_len)return 0;
    const uint8_t *m=sprite_head_file+table+index*16u;
    unsigned stride=keen_le16(m),height=keen_le16(m+2),width=stride*8u;
    uint64_t offset=(uint64_t)keen_le32(sprite_head_file+36)+keen_le32(m+4);
    if(!stride||width>W||!height||height>H||offset+(uint64_t)stride*height>ega_info.plane_size)return 0;
    if(x<0)x=(W-(int)width)/2;
    for(unsigned yy=0;yy<height;yy++)for(unsigned xx=0;xx<width;xx++){
        unsigned color=0,mask=128u>>(xx&7u);size_t at=(size_t)offset+yy*stride+xx/8u;
        for(unsigned plane=0;plane<4;plane++)if(latch_expanded[at+plane*ega_info.plane_size]&mask)color|=1u<<plane;
        int py=y+(int)yy;if(py>=0&&py<H&&x+(int)xx>=0&&x+(int)xx<W)fb[py*W+x+(int)xx]=(uint8_t)(9+color);
    }
    return 1;
}
static int draw_original_bitmap(uint8_t*fb,unsigned index,int y){return draw_original_bitmap_at(fb,index,-1,y);}
static int load_original_image(const char*name){
    size_t size=0;int index=episode_file(selected,name);
    if(index<0||read_asset(index,sprite_raw,sizeof(sprite_raw),&size)||
       keen_image_decode(sprite_raw,size,image_planes,sizeof(image_planes),sizeof(image_planes)))return 0;
    memset(map_cached,9,sizeof(map_cached));
    for(unsigned y=0;y<200;y++)for(unsigned x=0;x<320;x++)map_cached[(y+20)*W+x]=(uint8_t)(9+keen_image_pixel(image_planes,sizeof(image_planes),x,y));
    return 1;
}
static int episode_image_open(const char*name){
    if(!load_original_image(name))return 0;
    episode_menu_mode=4;return 1;
}
static int scores_load_table(void){
    uint32_t sequence=0;if(keen_scores_read(campaign_state_path(),episode_scores,&sequence))return 1;
    size_t size=0;int index=episode_file(selected,episode_name("SCORES.CK1"));
    memset(episode_scores,0,sizeof(episode_scores));
    if(index<0||read_asset(index,episode_scores,sizeof(episode_scores),&size)||!keen_scores_valid(episode_scores,size)){
        memset(episode_scores,0,sizeof(episode_scores));return 0;
    }
    return 1;
}
static int episode_scores_open(void){
    (void)scores_load_table();episode_menu_mode=5;play_camera_x=1344;play_camera_y=32;return 1;
}
static void dos_scene_frame(uint8_t *fb){
    keen_sfx_clock_step(&menu_animation_clock,of_time_us());
    scene_tiles_frame_mode(fb,0,1);
    keen_view_center_original(fb,9);
    if(keen_core_view_enhanced())keen_view_flat_margins(fb,1,9);
}
static void episode_scores_frame(uint8_t*fb){
    dos_scene_frame(fb);(void)draw_original_bitmap_at(fb,3,88,27);
    (void)draw_original_bitmap_at(fb,4,selected==2?80:40,64);(void)draw_original_bitmap_at(fb,5,selected==2?192:152,64);if(selected!=2)(void)draw_original_bitmap_at(fb,6,232,selected==1?60:64);
    KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    for(unsigned i=0;i<7;i++){
        char name[14];memcpy(name,episode_scores+112+i*13u,13);name[13]=0;
        ks_text(&c,selected==2?80:40,84+(int)i*16,name,1);
        char score[16];snprintf(score,sizeof(score),"%lu",(unsigned long)keen_le32(episode_scores+i*4u));
        ks_text(&c,(selected==2?240:200)-(int)strlen(score)*8,84+(int)i*16,score,1);
        if(selected==1){char cities[8];snprintf(cities,sizeof(cities),"%u",(unsigned)keen_le16(episode_scores+84+i*2));ks_text(&c,256,84+(int)i*16,cities,1);continue;}
        if(selected==2)continue;
        static const unsigned icons[]={221,237,241,245},part_order[]={0,1,3,2};
        for(unsigned part=0;part<4;part++)if(keen_le16(episode_scores+28+(part_order[part]*7u+i)*2u))status_tile(fb,224+(int)part*16,84+(int)i*16,menu_tile_frames[(menu_animation_clock.ticks>>4)&3u][icons[part]+i%4u]);
    }
    if(episode_menu_message)ks_text(&c,40,220,episode_menu_message,0);
}

static int startup_loading_frame(uint8_t *fb){
    memset(fb,9,W*H);
    /* CVort_engine_decompGraphics: bitmap 14 at screen (15*8,76).
     * This precedes map scrolling: no 32-pixel page-margin subtraction. */
    return draw_original_bitmap_at(fb,selected==2?10:14,120,96);
}
static void episode_assets_open(int show_loading){
    if(selected>=3)return;
    autosave_prompt_pending=autosave_prompt_ready=0;
    set_game_lights(1);game_menu_visible=play_notice=status_visible=status_block_buttons=0;finale.active=0;
    ega_state=0;load_tiles();
    if(show_loading && ega_state==1){
        int header=episode_file(selected,episode_name("EGAHEAD.CK1"));
        if(header>=0 && !read_asset(header,sprite_head_file,sizeof(sprite_head_file),&sprite_head_len) &&
           startup_loading_frame(of_video_surface())){
            startup_loading_started_us=of_time_us();
            of_video_flip();of_video_wait_flip();
        }
    }
    load_sprites();sound_load();load_world_data();load_special_data();
    map_id=90;load_map();play_camera_x=play_camera_y=32;
    if(ega_state!=1||sprite_state!=1||status_font_count<256)return;
    episode_menu_mode=1;episode_menu_selection=0;episode_menu_message=0;
    keen_sfx_reset(&menu_animation_clock,of_time_us());
}
static void episode_menu_open(void){episode_assets_open(0);}
/* Original CVort_scroll_up_logo uses 300 drawScreen iterations (nominal
 * six PIT ticks each); palette fades use VGA retraces, not game ticks. */
static uint32_t intro_started_us;
static unsigned intro_phase;
/* Retail CVort_do_intro_and_menu: four screens, 2400 PIT ticks each.
 * Restart Demo skips the already-shown Apogee scroll, as reshow_scroll_up=0. */
static unsigned attract_screen,attract_wait_release,attract_restart_pending;static KeenSfxClock attract_clock;
static void episode_attract_select(unsigned screen){
    int ok=1;attract_screen=screen;play_camera_x=play_camera_y=32;
    if(screen==1){ok=episode_text_open(episode_name("ABOUT.CK1"));text_kind=1;play_camera_x=1008;play_camera_y=28;}
    else if(screen==2)ok=episode_scores_open();
    else if(screen==3){ok=episode_text_open(episode_name("ORDER.CK1"));text_kind=2;text_line_count=keen_text_lines(text_document,text_document_size,text_lines,512,80);play_camera_x=352;play_camera_y=selected==1?28:32;}
    if(!ok){attract_screen=0;play_camera_x=play_camera_y=32;}
    episode_menu_mode=6;
}

static uint32_t intro_elapsed(void){return (uint32_t)(of_time_us()-intro_started_us);}
static unsigned intro_refresh(void){return (unsigned)((uint64_t)intro_elapsed()*70086u/1000000000u);}
static void intro_palette(unsigned table){
    if(!special_ready){set_game_lights(1);return;}
    uint32_t colors[25];memcpy(colors,base_colors,sizeof(colors));
    for(unsigned i=0;i<16;i++){
        unsigned c=special_data[4+17*table+i];
        colors[9+i]=ega_palette_color(c);
    }
    of_video_palette_bulk(colors,25);
}
static void intro_next(unsigned phase){intro_phase=phase;intro_started_us=of_time_us();}
static void episode_attract_restart(void){
    sound_stop();campaign_active=play_mode=world_mode=0;episode_menu_mode=6;episode_menu_message=0;
    attract_screen=0;attract_wait_release=1;attract_restart_pending=1;play_camera_x=play_camera_y=32;intro_next(6);
}
static void episode_intro_open(void){
    episode_assets_open(1);if(episode_menu_mode!=1)return;
    sound_stop();episode_menu_mode=6;play_camera_x=1664;play_camera_y=32;attract_screen=attract_wait_release=attract_restart_pending=0;
    if((uint32_t)(of_time_us()-startup_loading_started_us)<1200000u)intro_next(5);
    else{intro_next(0);intro_palette(0);}
}
static void episode_intro_tick(void){
    if(intro_phase==5){
        if((uint32_t)(of_time_us()-startup_loading_started_us)>=1200000u){intro_next(0);intro_palette(0);}
        return;
    }
    if(attract_wait_release){if(!of_btn(STATUS_BUTTONS))attract_wait_release=0;}
    if(!attract_wait_release && of_btn_pressed(OF_BTN_SELECT)){
        set_game_lights(1);play_camera_x=play_camera_y=32;
        (void)episode_help_open();return;
    }
    if(!attract_wait_release && of_btn_pressed(intro_phase>=4?STATUS_BUTTONS:(OF_BTN_A|OF_BTN_B|OF_BTN_X|OF_BTN_Y|OF_BTN_START))){
        set_game_lights(1);episode_menu_mode=1;play_camera_x=play_camera_y=32;return;
    }
    unsigned refresh=intro_refresh();
    if(intro_phase==0 || intro_phase==3){
        intro_palette(refresh<1?0:refresh<9?0:refresh<17?1:3);
        if(refresh>=17){unsigned next=intro_phase==0?1:4;intro_next(next);if(next==4)keen_sfx_reset(&attract_clock,of_time_us());}
    }else if(intro_phase==1){
        unsigned frame=(unsigned)((uint64_t)intro_elapsed()*1193182u/(8192ull*6000000u));
        if(frame>=300)intro_next(2);
    }else if(intro_phase==2){
        intro_palette(refresh<1?3:refresh<9?3:refresh<17?2:refresh<25?1:0);
        if(refresh>=25){play_camera_x=play_camera_y=32;intro_next(3);}
    }else if(intro_phase==4){
        keen_sfx_clock_step(&attract_clock,of_time_us());if(attract_clock.ticks>=2400)intro_next(6);
    }else if(intro_phase==6){
        intro_palette(refresh<9?3:refresh<17?2:refresh<25?1:0);
        if(refresh>=25){episode_attract_select(attract_restart_pending?0:(attract_screen+1u)&3u);attract_restart_pending=0;intro_next(7);}
    }else if(intro_phase==7){
        intro_palette(refresh<9?0:refresh<17?1:3);
        if(refresh>=17){intro_next(4);keen_sfx_reset(&attract_clock,of_time_us());}
    }
}
static void title_help_prompt(uint8_t *fb){
    /* Keep the original bitmap frame and wording, widened for Select. */
    const int x=selected==2?104:80,y=202,width=96;
    if(!draw_original_bitmap_at(fb,2,x,y))return;
    uint8_t original[16][64];
    for(unsigned row=0;row<16;row++)memcpy(original[row],fb+(y+(int)row)*W+x,64);
    for(unsigned row=0;row<16;row++)for(unsigned col=0;col<96;col++){
        unsigned source=col<8?col:col>=88?col-32:7;
        fb[(y+(int)row)*W+x+(int)col]=original[row][source];
    }
    for(unsigned row=0;row<5;row++){
        memcpy(fb+(y+2+(int)row)*W+x+8,original[row+2]+9,33);
        memcpy(fb+(y+9+(int)row)*W+x+(width-47)/2,original[row+9]+8,47);
    }
    static const struct {char letter;uint8_t rows[5];} small[]={
        {'E',{126,96,120,96,126}},
        {'L',{96,96,96,96,126}},{'C',{62,96,96,96,62}},{'T',{126,24,24,24,24}}
    };
    const char *label="SELECT";
    for(unsigned letter=0;letter<6;letter++){
        if(label[letter]=='S'){
            for(unsigned row=0;row<5;row++)for(unsigned col=0;col<7;col++)if(original[row+2][35+col]==original[2][9])
                fb[(y+2+(int)row)*W+x+47+(int)letter*7+(int)col]=13;
        }else for(unsigned glyph=0;glyph<sizeof(small)/sizeof(small[0]);glyph++)if(small[glyph].letter==label[letter])
            for(unsigned row=0;row<5;row++)for(unsigned col=0;col<7;col++)if(small[glyph].rows[row]&(64u>>col))
                fb[(y+2+(int)row)*W+x+47+(int)letter*7+(int)col]=13;
    }
}
static void episode_intro_frame(uint8_t *fb){
    if(intro_phase==5){(void)startup_loading_frame(fb);return;}
    if(intro_phase>=4 && attract_screen){
        episode_menu_mode=attract_screen==2?5:3;episode_menu_frame(fb);episode_menu_mode=6;return;
    }
    dos_scene_frame(fb);
    if(intro_phase<=2){
        unsigned frame=intro_phase==0?0:intro_phase==2?300:
            (unsigned)((uint64_t)intro_elapsed()*1193182u/(8192ull*6000000u));
        int y=frame<=32?200:frame<177?200-(int)(frame-32):55;
        /* Bitmap x coordinates include the original 32-pixel page margin. */
        if(frame>=31)(void)draw_original_bitmap_at(fb,selected==2?9:10,96,y+20);
        if(frame>=178){
            (void)draw_original_bitmap_at(fb,selected==2?7:8,144,y+10);
            (void)draw_original_bitmap_at(fb,selected==2?8:9,96,y+52);
            (void)draw_original_bitmap_at(fb,selected==2?11:15,136,119);
            (void)draw_original_bitmap_at(fb,selected==2?13:17,120,134);
            (void)draw_original_bitmap_at(fb,selected==2?12:16,120,179);
        }
    }else{
        (void)draw_original_bitmap(fb,0,21);
        title_help_prompt(fb);
    }
    if(!keen_core_view_enhanced())memset(fb+220*W,9,20*W);
}

static void episode_ordering_frame(uint8_t *fb){
    unsigned wanted=(unsigned)((uint64_t)(uint32_t)(of_time_us()-ordering_started_us)*1193182u/(8192ull*6000000u));
    if(selected==2){
        while(ordering_frames<wanted){
            if(ordering3_flash){ordering3_flash--;if(!ordering3_flash){ordering3_vx=1536;ordering3_time=(unsigned)(87040-ordering3_x)/1536u;}}
            else{
                if(ordering3_wait&&!--ordering3_wait){ordering3_x=87040;ordering3_vx=-256;ordering3_time=ordering_random(175)+100;}
                if(ordering3_time){ordering3_x+=ordering3_vx;if(!--ordering3_time){
                    if(ordering3_vx>0){ordering3_wait=10+ordering_random(50);ordering3_vx=0;}
                    else{ordering3_flash=4;sound_play_named("YORPSCREAM");}
                }}
            }
            ordering_frames++;
        }
        for(unsigned line=0;line<text_line_count;line++){KeenTextLine l=text_lines[line];unsigned start=l.start,length=l.length;if(length&&text_document[start]=='~'){start++;length--;}
            for(unsigned col=0;col<length;col++)status_glyph(fb,(int)col*8,52+(int)line*8,text_document[start+col]+128);}
        draw_masked_sprite(fb,8,196,ordering3_flash||(ordering3_time&&ordering3_time<=10&&ordering3_vx<0)?20:0,0);
        if(ordering3_time||ordering3_flash)draw_masked_sprite(fb,ordering3_x/256,204,ordering3_flash?(ordering3_flash>2?97u:98u):(ordering3_vx>0?95u:93u)+((ordering_frames&2u)/2u),0);
        return;
    }
    if(selected==1){
        unsigned frame=106;
        while(ordering_frames<wanted){
            if(ordering2_vx>0){ordering2_x+=2;if(ordering2_x>302){ordering2_vx=0;ordering2_vy=2;}}
            if(ordering2_vx<0){ordering2_x-=2;if(ordering2_x<2){ordering2_vx=0;ordering2_vy=-2;}}
            if(ordering2_vy>0){ordering2_y+=2;if(ordering2_y>184){ordering2_vx=-2;ordering2_vy=0;}}
            if(ordering2_vy<0){ordering2_y-=2;if(ordering2_y<6){ordering2_vx=2;ordering2_vy=0;}}
            ordering_frames++;
        }
        frame=ordering2_vx>0?106u:ordering2_vx<0?102u:ordering2_vy>0?108u:104u;
        for(unsigned line=0;line<text_line_count;line++){
            KeenTextLine l=text_lines[line];unsigned start=l.start,length=l.length;
            if(length&&text_document[start]=='~'){start++;length--;}
            for(unsigned col=0;col<length;col++)status_glyph(fb,16+(int)col*8,44+(int)line*8,text_document[start+col]+128);
        }
        draw_masked_sprite(fb,ordering2_x,20+ordering2_y,frame+((ordering_frames/4)&1u),0);return;
    }
    while(ordering_frames<wanted){
        if(!(ordering_frames%6)){
            int offset=(int)ordering_offset+(int)ordering_random(3)-2;
            ordering_offset=offset<0||offset>7?ordering_random(49)/7:(unsigned)offset;
        }
        ordering_frames++;
    }

    for(unsigned line=0;line<text_line_count;line++){
        KeenTextLine l=text_lines[line];unsigned start=l.start,length=l.length;
        if(length&&text_document[start]=='~'){start++;length--;}
        int x=line<7?64:line==7?8:line+1==text_line_count?-32:0;
        for(unsigned col=0;col<length;col++)
            status_glyph(fb,x+(int)col*8,52+(int)line*8,text_document[start+col]+128);
    }
    if(ordering_ready){
        unsigned creature=(ordering_frames/501u)&1u;
        draw_masked_sprite(fb,kv_floor((int32_t)keen_le32(ordering_objects+12+creature*4)),
            20+kv_floor((int32_t)keen_le32(ordering_objects+4+creature*4)),
            keen_le16(ordering_objects+creature*2)+ordering_offset,0);
    }
}

static void episode_menu_tick(void){
    if(episode_menu_mode==6){episode_intro_tick();return;}
    if(episode_menu_mode==3 && text_kind==4 && of_btn_pressed(OF_BTN_A|OF_BTN_B)){
        score_flow_begin();return;
    }
    if(episode_menu_mode==1 && of_btn_pressed(OF_BTN_SELECT)){(void)episode_help_open();return;}
    if(of_btn_pressed(OF_BTN_B)){
        if(episode_menu_mode>=2){episode_menu_mode=1;episode_menu_selection=0;episode_menu_message=0;play_camera_x=play_camera_y=32;}
        else episode_attract_restart();
        return;
    }
    if(episode_menu_mode==4){
        if(of_btn_pressed(OF_BTN_A)){
            if(preview_page==0){preview_page=1;if(!episode_image_open(episode_name("PREVIEW3.CK1")))episode_menu_message="IMAGE DATA NOT AVAILABLE";}
            else if(!episode_text_open(episode_name("PREVIEWS.CK1")))episode_menu_message="TEXT DATA NOT AVAILABLE";
        }
        return;
    }
    if(episode_menu_mode==5){if(of_btn_pressed(OF_BTN_A)){episode_menu_mode=1;play_camera_x=play_camera_y=32;}return;}
    if(episode_menu_mode==3){
        unsigned rows=text_kind==5||text_kind==4?21u:text_kind==0?15u:18u;
        unsigned max=text_line_count>rows?text_line_count-rows:0;
        int direction=!!(of_btn(OF_BTN_DOWN)||of_btn_pressed(OF_BTN_DOWN))-
                      !!(of_btn(OF_BTN_UP)||of_btn_pressed(OF_BTN_UP));
        uint32_t now=of_time_us();
        if(text_kind==1||text_kind==2)direction=0;
        if(!direction)text_scroll_direction=0;
        else if(direction!=text_scroll_direction || (int32_t)(now-text_scroll_next_us)>=0){
            if(direction<0&&text_top)text_top--;
            if(direction>0&&text_top<max)text_top++;
            text_scroll_next_us=now+(direction!=text_scroll_direction?300000u:75000u);
            text_scroll_direction=direction;
        }
        if(of_btn_pressed(OF_BTN_LEFT))text_top=text_top>rows?text_top-rows:0;
        if(of_btn_pressed(OF_BTN_RIGHT)){text_top+=rows;if(text_top>max)text_top=max;}
        if(of_btn_pressed(OF_BTN_A)){episode_menu_mode=1;episode_menu_selection=0;play_camera_x=play_camera_y=32;}
        return;
    }
    int count=episode_menu_mode==2?10:8;
    int navigation=menu_navigation(episode_menu_mode);
    episode_menu_selection=(episode_menu_selection+count+navigation)%count;
    if(!of_btn_pressed(OF_BTN_A))return;
    if(episode_menu_mode==2){
        if(episode_menu_selection==0?campaign_load_auto():campaign_load((unsigned)episode_menu_selection-1)){episode_menu_mode=0;status_block_buttons=1;}
        else episode_menu_message="EMPTY OR INVALID SAVE";
        return;
    }
    if(episode_menu_selection==0){start_campaign();if(campaign_active){episode_menu_mode=0;status_block_buttons=1;}}
    else if(episode_menu_selection==1){
        episode_menu_mode=2;episode_menu_selection=0;episode_menu_message=0;
        KeenSave saved;autosave_present=campaign_state_read(campaign_state_path(),&saved)>0;
    }
    else if(episode_menu_selection==2){if(episode_text_open(episode_name("STORYTXT.CK1"))){play_camera_x=672;play_camera_y=32;}else episode_menu_message="TEXT DATA NOT AVAILABLE";}
    else if(episode_menu_selection==3){if(episode_text_open(episode_name("ABOUT.CK1"))){text_kind=1;play_camera_x=1008;play_camera_y=28;}else episode_menu_message="TEXT DATA NOT AVAILABLE";}
    else if(episode_menu_selection==4){if(!episode_scores_open())episode_menu_message="SCORES DATA NOT AVAILABLE";}
    else if(episode_menu_selection==5){if(episode_text_open(episode_name("ORDER.CK1"))){text_kind=2;text_line_count=keen_text_lines(text_document,text_document_size,text_lines,512,80);play_camera_x=352;play_camera_y=selected==1?28:32;}else episode_menu_message="TEXT DATA NOT AVAILABLE";}
    else if(episode_menu_selection==6){preview_page=0;if(selected!=0){if(!episode_text_open(episode_name("PREVIEWS.CK1")))episode_menu_message="TEXT DATA NOT AVAILABLE";}else if(!episode_image_open(episode_name("PREVIEW2.CK1")))episode_menu_message="IMAGE DATA NOT AVAILABLE";}
    else episode_attract_restart();
}
static void episode_menu_frame(uint8_t *fb){
    if(episode_menu_mode==6){episode_intro_frame(fb);return;}
    if(episode_menu_mode==4){memcpy(fb,map_cached,sizeof(map_cached));keen_view_flat_margins(fb,keen_core_view_enhanced(),9);return;}
    if(episode_menu_mode==5){episode_scores_frame(fb);return;}
    if(text_kind==4 && episode_menu_mode==3)memcpy(fb,status_backdrop,sizeof(status_backdrop));
    else if(map_state==1 && map_id==90)dos_scene_frame(fb);else memset(fb,9,W*H);
    if(episode_menu_mode==1||episode_menu_mode==2)(void)draw_original_bitmap(fb,0,20);
    KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};
    if(episode_menu_mode==3){
        if(text_kind==2){episode_ordering_frame(fb);return;}
        if(text_kind==1){(void)draw_original_bitmap(fb,1,43);}
        unsigned bottom=text_kind==5||text_kind==4?22u:text_kind==0?16u:20u,top=text_kind==3?1u:0u;
        if(text_kind!=1){
            for(unsigned row=top;row<=bottom;row++)for(unsigned col=0;col<40;col++){
                unsigned ch=row==top?(col==0?1:col==39?3:2):row==bottom?(col==0?6:col==39?8:7):col==0?4:col==39?5:32;
                status_glyph(fb,(int)col*8,20+(int)row*8,ch);
            }
            for(unsigned col=0;col<40;col++){
                status_glyph(fb,(int)col*8,20+(int)(bottom+1)*8,col==0||col==39?4:160);
                status_glyph(fb,(int)col*8,20+(int)(bottom+2)*8,col==0?6:col==39?8:7);
            }
            ks_text(&c,48,20+(int)(bottom+1)*8,"B to Exit / ",1);
            status_glyph(fb,144,20+(int)(bottom+1)*8,15+128);
            status_glyph(fb,152,20+(int)(bottom+1)*8,19+128);
            ks_text(&c,168,20+(int)(bottom+1)*8,"to Read",1);
        }
        unsigned rows=text_kind==1?14u:bottom-top-1;
        for(unsigned line=0;line<rows&&text_top+line<text_line_count;line++){
            KeenTextLine l=text_lines[text_top+line];unsigned start=l.start,length=l.length,alternate=0;
            if(length&&text_document[start]=='~'){start++;length--;alternate=128;}
            for(unsigned col=0;col<(text_kind==1?length:38u);col++)status_glyph(fb,(text_kind==1?24:8)+(int)col*8,
                (text_kind==1?92:20+(int)(top+1)*8)+(int)line*8,(col<length?text_document[start+col]:32)+alternate);
        }
        return;
    }
    if(episode_menu_mode==2){
        keen_text_box(&c,22,10);ks_text(&c,80,80,"Continue Game",0);
        ks_text(&c,80,96,episode_menu_selection==0?">":" ",0);
        ks_text(&c,96,96,autosave_present?"Autosave":"Autosave (empty)",0);
        for(int i=0;i<9;i++){int x=96+(i%3)*48,y=112+(i/3)*16;
            ks_text(&c,x-8,y,i+1==episode_menu_selection?">":" ",0);ks_number(&c,x,y,(unsigned)i+1,0);}
        ks_text(&c,80,152,"A CHOOSE  B BACK",0);
    }else{
        keen_text_box(&c,18,10);
        static const char *const labels[]={"New Game","Continue Game","Story","About ID...","High Scores","Ordering Info","Previews!","Restart Demo"};
        for(int i=0;i<8;i++)ks_text(&c,112,80+i*8,labels[i],0);
        status_glyph(fb,96,80+episode_menu_selection*8,9+(of_time_us()/85610u)%3u);
        ks_text(&c,88,152,"Use the arrows",0);
        title_help_prompt(fb);
    }
    if(episode_menu_message)ks_text(&c,48,192,episode_menu_message,0);
}

static void score_flow_begin(void){
    if((episode_menu_mode==3&&text_kind==4)||(selected==1&&finale2.failed&&!finale2.active&&finale2.phase==25))memcpy(score_backdrop,status_backdrop,sizeof(score_backdrop));
    else play_frame(score_backdrop);
    (void)scores_load_table();score_rank=keen_scores_position(episode_scores,gameplay.score);
    memset(score_name,' ',12);memcpy(score_name,"KEEN",4);score_name[12]=0;score_cursor=0;
    score_flow_active=score_wait_release=1;episode_menu_mode=0;play_mode=1;world_mode=0;
    finale.active=0;status_visible=status_block_buttons=0;game_menu_visible=play_notice=0;
    paused_sound_priority=-1;keen_sfx_reset(&score_clock,of_time_us());sound_play_named("GAMEOVERSND");
}
static void score_flow_finish(int save){
    int saved=1;
    if(save){
        char name[13];memcpy(name,score_name,13);int end=12;while(end>0&&name[end-1]==' ')end--;name[end]=0;
        if(!end)strcpy(name,"KEEN");
        int rank=keen_scores_insert(episode_scores,gameplay.score,selected==1?0:gameplay.ship_mask,name);
        if(rank>=0&&selected==1){unsigned cities=0;for(unsigned i=0;i<8;i++)cities+=(gameplay.targets_mask>>i)&1u;episode_scores[84+(unsigned)rank*2]=(uint8_t)cities;}
        if(rank>=0)saved=campaign_namespace_ready()&&keen_scores_write(campaign_state_path(),episode_scores);
    }
    score_flow_active=campaign_active=play_mode=world_mode=keen_game_over=0;
    episode_menu_open();episode_menu_mode=5;play_camera_x=1344;play_camera_y=32;
    if(!saved)episode_menu_message="SCORE SAVE FAILED";
}
static void score_flow_tick(void){
    keen_sfx_clock_step(&score_clock,of_time_us());
    if(score_wait_release){if(!of_btn(STATUS_BUTTONS))score_wait_release=0;return;}
    if(score_rank<0){if(kt_delay_elapsed(score_clock.ticks,360)||of_btn_pressed(STATUS_BUTTONS))score_flow_finish(0);return;}
    if(of_btn_pressed(OF_BTN_LEFT)&&score_cursor)score_cursor--;
    if(of_btn_pressed(OF_BTN_RIGHT|OF_BTN_A)&&score_cursor<11)score_cursor++;
    if(of_btn_pressed(OF_BTN_UP|OF_BTN_DOWN)){
        static const char letters[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-";
        const char *found=strchr(letters,score_name[score_cursor]);unsigned index=found?(unsigned)(found-letters):0;
        unsigned count=(unsigned)sizeof(letters)-1u;
        index=of_btn_pressed(OF_BTN_UP)?(index+1)%count:(index+count-1)%count;score_name[score_cursor]=letters[index];
    }
    if(of_btn_pressed(OF_BTN_B)){if(score_cursor)score_cursor--;score_name[score_cursor]=' ';}
    if(of_btn_pressed(OF_BTN_START))score_flow_finish(1);
}
static void score_flow_frame(uint8_t*fb){
    memcpy(fb,score_backdrop,sizeof(score_backdrop));
    if(score_rank<0){(void)draw_original_bitmap(fb,selected==2?6:7,100);return;}
    KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};keen_text_box(&c,32,15);
    (void)draw_original_bitmap(fb,selected==2?6:7,64);ks_text(&c,104,92,"SCORE:",0);ks_number(&c,160,92,gameplay.score,0);
    if(selected==1){unsigned cities=0;for(unsigned i=0;i<8;i++)cities+=(gameplay.targets_mask>>i)&1u;
        ks_text(&c,72,108,"CITIES SAVED:",0);ks_number(&c,184,108,cities,0);}
    else for(unsigned i=0;i<4;i++)if(gameplay.ship_mask&(1u<<i))status_tile(fb,136+(int)i*16,108,448+i);
    ks_text(&c,64,132,"CONGRATULATIONS!",0);ks_text(&c,64,148,"Enter your name:",0);
    ks_text(&c,64,164,score_name,0);status_glyph(fb,64+(int)score_cursor*8,172,2);
    ks_text(&c,40,200,"START DONE  A NEXT  B ERASE",0);
}

static void finale_box(uint8_t*fb,int left,int top,int right,int bottom){
    int x=left*8-32,y=top*8+20,width=right-left,height=bottom-top;
    for(int row=0;row<=height;row++)for(int col=0;col<=width;col++){
        unsigned glyph_index=32;
        if(!row)glyph_index=!col?1u:col==width?3u:2u;
        else if(row==height)glyph_index=!col?6u:col==width?8u:7u;
        else if(!col)glyph_index=4;else if(col==width)glyph_index=5;
        status_glyph(fb,x+col*8,y+row*8,glyph_index);
    }
}
static void keen2_finale_frame(uint8_t*fb);
static void finale_frame(uint8_t*fb){
    if(selected==2){keen3_scene_frame(fb);return;}
    if(selected==1){keen2_finale_frame(fb);return;}
    if(!finale.scene){memcpy(fb,map_cached,sizeof(map_cached));keen_view_flat_margins(fb,keen_core_view_enhanced(),9);}
    else{
        play_camera_x=kv_floor(finale.camera_x);play_camera_y=kv_floor(finale.camera_y);scene_tiles_frame(fb,0);
        draw_masked_sprite(fb,kv_floor(finale.x)-play_camera_x,kv_floor(finale.y)-play_camera_y,finale.frame,0);
        if(finale.overlay)draw_masked_sprite(fb,kv_floor(finale.x)+6-play_camera_x,kv_floor(finale.y)-14-play_camera_y,finale.overlay,0);
    }
    if(!finale.scene && finale.window)(void)draw_original_bitmap_at(fb,finale.window,80,20);
    if(finale.dialog>=0){
        int left=finale.dialog==0?5:finale.dialog==9?12:4,top=finale.dialog==9?3:15;
        int right=finale.dialog==0?42:finale.dialog==5||finale.dialog==8?29:finale.dialog==9?32:27;
        int bottom=finale.dialog==0?20:finale.dialog==3||finale.dialog==5?20:finale.dialog==4?18:
            finale.dialog==6||finale.dialog==7?17:finale.dialog==8?21:finale.dialog==9?5:19;
        finale_box(fb,left,top,right,bottom);int x=(left+1)*8-32,y=(top+1)*8+20;
        const unsigned char*text=(const unsigned char*)kf_message(&finale,(unsigned)finale.dialog);
        for(unsigned i=0;i<finale.shown&&text[i];i++){
            if(text[i]==10){x=(left+1)*8-32;y+=8;continue;}
            status_glyph(fb,x,y,text[i]);x+=8;
        }
    }
}
static void finale_begin(void){
    if(selected==2){keen3_scene_begin(1,0);return;}
    if(selected==1){keen2_finale_begin(0,0);return;}
    size_t size=0;int index=episode_file(0,"FINALE1.CK1");
    if(index<0||read_asset(index,finale_data,sizeof(finale_data),&size)||keen_finale_init(&finale,finale_data,size)){
        gameplay.event="FINALE DATA UNAVAILABLE";return;
    }
    world_mode=0;motion_ticks=movement_sfx.ticks;keen_weapon_reset(&weapon);
}
static void keen2_finale_update(void);
static void finale_update(void){
    if(selected==2){keen3_scene_update();return;}
    if(selected==1){keen2_finale_update();return;}
    keen_sfx_clock_step(&movement_sfx,of_time_us());unsigned ticks=movement_sfx.ticks-motion_ticks;motion_ticks=movement_sfx.ticks;
    unsigned events=keen_finale_step(&finale,ticks);
    if(events&KF_BAD){gameplay.event="INVALID FINALE SEQUENCE";return;}
    if(events&KF_SCENE){
        if(!finale.scene){if(!load_original_image("FINALE.CK1")){finale.active=0;gameplay.event="FINALE IMAGE UNAVAILABLE";return;}}
        else{map_id=(int)finale.scene;load_map();if(map_state!=1){finale.active=0;gameplay.event="FINALE MAP UNAVAILABLE";return;}}
    }
    if(events&KF_CLICK)sound_play_named("CLICKSND");
    if(events&KF_FINISH){
        finale_frame(status_backdrop);campaign_active=play_mode=world_mode=0;
        episode_menu_open();if(episode_text_open(episode_name("ENDTEXT.CK1")))text_kind=4;
        sound_play_named("KEENSLEFT");
    }
}


static void keen2_finale_begin(int failed,int shake){
    size_t size=0;int index=episode_file(1,"FINALE2.CK2");
    if(index<0||read_asset(index,finale2_data,sizeof(finale2_data),&size)||keen2_finale_init(&finale2,finale2_data,size,failed,shake)){
        gameplay.event="KEEN 2 FINALE DATA UNAVAILABLE";return;
    }
    if(failed&&shake){finale2.scene=map_id;finale2.x=(player.x-4)*256;finale2.y=(player.y-1)*256;
        finale2.frame=player_animation.frame;finale2.camera_x=finale2.saved_x=play_camera_x*256;finale2.camera_y=finale2.saved_y=play_camera_y*256;
    }else{map_id=81;load_map();if(map_state!=1){finale2.active=0;return;}}
    memset(&finale,0,sizeof(finale));finale.active=1;finale.scene=(unsigned)finale2.scene;
    world_mode=0;play_mode=1;episode_menu_mode=game_menu_visible=play_notice=status_visible=status_block_buttons=0;
    motion_ticks=movement_sfx.ticks;keen_weapon_reset(&weapon);sound_stop();
}
static void keen2_finale_frame(uint8_t*fb){
    KeenFinale2*f=&finale2;
    if(!f->scene){memcpy(fb,map_cached,sizeof(map_cached));keen_view_flat_margins(fb,keen_core_view_enhanced(),9);}
    else{
        play_camera_x=kv_floor(f->camera_x);play_camera_y=kv_floor(f->camera_y);scene_tiles_frame(fb,0);
        if(f->frame)draw_masked_sprite(fb,kv_floor(f->x)-play_camera_x,kv_floor(f->y)-play_camera_y,f->frame,0);
    }
    if(f->failed&&f->phase==24){
        int32_t ex=0x2f000,ey=0x16000;unsigned tick=f->count;
        if(tick>=15){int32_t distance=(int32_t)(tick-15)*0xa00;
            const int dx[]={-1,1,-1,1,-2,2,-2,2},dy[]={-2,-2,2,2,-1,-1,1,1};
            for(unsigned i=0;i<8;i++)draw_masked_sprite(fb,kv_floor(ex+distance*dx[i]/2+(dx[i]>0?0x1000:0))-play_camera_x,
                kv_floor(ey+distance*dy[i]/2+(dy[i]>0?0x1000:0))-play_camera_y,68+(tick/2)%4,0);
        }
        if(tick>=20){int32_t distance=(int32_t)(tick-20)*2048;
            for(unsigned i=0;i<4;i++)draw_masked_sprite(fb,kv_floor(ex+(i&1?distance+0x1000:-distance))-play_camera_x,
                kv_floor(ey+(i&2?distance+0x1000:-distance))-play_camera_y,64+(tick/2)%4,0);
        }
        for(unsigned i=0;i<9;i++){
            int age=(int)tick-kf_i16(f->data+4+194+i*2);if(age<0||age>=79)continue;
            int frame=kf_i16(f->data+4+(unsigned)age*2);if(frame<0||frame>3)continue;
            int x=(kf_i16(f->data+4+158+i*2)+94)*8,y=(kf_i16(f->data+4+176+i*2)+44)*8;
            draw_masked_sprite(fb,x-play_camera_x,y-play_camera_y,60u+(unsigned)frame,0);
        }
    }
    if(f->dialog>=0){
        if(f->phase==21){KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};keen_text_box(&c,5,1);ks_text(&c,140,116,k2f_message(f,7),0);return;}
        static const int bounds[7][4]={{4,17,36,24},{4,17,36,24},{4,18,29,22},{4,16,29,24},{4,18,29,23},{4,18,16,20},{12,3,32,5}};
        const int*b=bounds[f->dialog];finale_box(fb,b[0],b[1],b[2],b[3]);
        int x=(b[0]+1)*8-32,y=(b[1]+1)*8+20;const unsigned char*text=(const unsigned char*)k2f_message(f,(unsigned)f->dialog);
        for(unsigned i=0;i<f->shown&&text[i];i++){
            if(text[i]==10){x=(b[0]+1)*8-32;y+=8;continue;}
            status_glyph(fb,x,y,text[i]);x+=8;
        }
    }
}
static void keen2_finale_update(void){
    keen_sfx_clock_step(&movement_sfx,of_time_us());unsigned ticks=movement_sfx.ticks-motion_ticks;motion_ticks=movement_sfx.ticks;
    unsigned events=keen2_finale_step(&finale2,ticks,&level_actors2.core);
    if(events&KF_BAD){finale.active=0;gameplay.event="INVALID KEEN 2 FINALE";return;}
    if(events&KF_SCENE){
        if(!finale2.scene){if(!load_original_image("FINALE.CK2")){finale.active=0;return;}}
        else{map_id=finale2.scene;load_map();if(map_state!=1){finale.active=0;return;}}
    }
    finale.active=finale2.active;finale.scene=(unsigned)finale2.scene;
    if(events&K2F_TANK)sound_play_named("TANKFIRE");
    if(events&K2F_EARTH)sound_play_named("EARTHPOW");
    if(events&KF_FINISH){
        keen2_finale_frame(status_backdrop);
        if(finale2.failed){score_flow_begin();return;}
        campaign_active=play_mode=world_mode=0;episode_menu_open();
        if(episode_text_open("ENDTEXT.CK2"))text_kind=4;
        sound_play_named("KEENSLEFT");
    }
}
static KeenSfxClock cinema3_clock;
static void keen3_scene_begin(unsigned kind,unsigned ship){
    size_t size=0;int index=episode_file(2,"CINEMA3.CK3");
    if(index<0||read_asset(index,cinema3_data,sizeof(cinema3_data),&size)||keen3_cinema_init(&cinema3,cinema3_data,size,kind,ship)){
        cinema3.active=0;if(kind==1){finale.active=0;score_flow_begin();episode_menu_message="ENDING DATA NOT AVAILABLE";}return;
    }
    sound_stop();keen_sfx_reset(&cinema3_clock,of_time_us());
    if(kind==1){finale.active=1;finale.scene=81;world_mode=0;map_id=81;load_map();play_camera_x=play_camera_y=32;}
    else{if(world_mode)world_frame(status_backdrop);else play_frame(status_backdrop);if(!kind){sound_play_named("MORTIMER");sound_play_named("DOOROPENSND");}}
}
static void keen3_flash_palette(int on){
    if(!on){set_game_lights(1);return;}
    uint32_t colors[25];memcpy(colors,base_colors,sizeof(colors));
    for(unsigned i=0;i<16;i++){unsigned c=cinema3_data[9220+i];colors[9+i]=ega_palette_color(c);}
    of_video_palette_bulk(colors,25);
}
static void keen3_scene_update(void){
    unsigned before=cinema3_clock.ticks;keen_sfx_clock_step(&cinema3_clock,of_time_us());
    keen3_cinema_step(&cinema3,cinema3_clock.ticks-before,of_btn_pressed(OF_BTN_A|OF_BTN_B|OF_BTN_START|OF_BTN_X|OF_BTN_Y),of_btn(STATUS_BUTTONS));
    if(cinema3.kind==1){
        static const int x[]={19,3,5,20,8},y[]={5,8,8,5,8};
        for(unsigned i=0;i<5;i++)if(cinema3.events&(1u<<(i+4))){(void)map_set_tile(x[i],y[i],y[i]==5?58:60);sound_play_named("CLICKSND");}
        if(cinema3.phase>=4&&cinema3.phase<9&&cinema3.age>=2){unsigned i=cinema3.phase-4;(void)map_set_tile(x[i],y[i],y[i]==5?72:61);}
        if(cinema3.events&(1u<<10))if(!load_original_image("FINALE.CK3")){cinema3.active=0;episode_menu_message="ENDING IMAGE NOT AVAILABLE";}
        keen3_flash_palette(cinema3.phase>=4&&cinema3.phase<9&&cinema3.age<2);
    }
    if(cinema3.active)return;
    movement_sfx.last_us=of_time_us();status_block_buttons=1;
    if(cinema3.kind==1){finale.active=0;campaign_active=play_mode=world_mode=0;episode_menu_open();
        if(episode_text_open("ENDTEXT.CK3"))text_kind=4;
        sound_play_named("KEENSLEFT");}
}
static void keen3_scene_frame(uint8_t*fb){
    if(cinema3.kind==1){
        if(cinema3.phase>=10){memcpy(fb,map_cached,sizeof(map_cached));keen_view_flat_margins(fb,keen_core_view_enhanced(),9);}
        else{dos_scene_frame(fb);draw_masked_sprite(fb,192,84,67,0);draw_masked_sprite(fb,216,92,0,0);}
    }else memcpy(fb,status_backdrop,sizeof(status_backdrop));
    if(cinema3.kind==1&&cinema3.phase>=4&&cinema3.phase!=11)return;
    int left,top,right,bottom;
    if(cinema3.kind==0){static const int widths[]={5,12,18,38,38,31,35,34,38},heights[]={1,1,1,5,5,2,4,1,3};
        unsigned phase=cinema3.phase;if(phase>=9)return;left=23-widths[phase]/2;top=(phase==6||phase==8?18:20)-heights[phase]/2;right=left+widths[phase]+1;bottom=top+heights[phase]+1;
    }else if(cinema3.kind==2){left=7;top=8;right=40;bottom=15;}
    else{left=cinema3.phase==3?17:cinema3.phase==11?18:4;top=cinema3.phase==11?20:18;right=cinema3.phase==3?30:cinema3.phase==11?29:43;bottom=cinema3.phase==0?23:cinema3.phase==3?20:cinema3.phase==11?24:22;}
    finale_box(fb,left,top,right,bottom);int x=(left+1)*8-32,y=(top+1)*8+20;
    const unsigned char*text=(const unsigned char*)k3c_text(&cinema3);
    for(unsigned i=0;i<cinema3.shown&&text[i];i++){if(text[i]==10){x=(left+1)*8-32;y+=8;continue;}status_glyph(fb,x,y,text[i]);x+=8;}
    if(cinema3.kind==2){KeenStatusCanvas c={fb,status_glyph,status_tile,status_sprite};ks_text(&c,112,132,"Press A:",0);}
}
static void update_play_input(void){
    if(!cinema3.active && !score_flow_active && !play_notice && !autosave_prompt_pending && !game_menu_visible && !status_visible && !status_block_buttons && of_btn_pressed(OF_BTN_START))game_menu_open();
    else update_play();
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int file_for_selection(int episode, int index) {
    unsigned current = 0;
    for (unsigned i=0; i<file_count; ++i) {
        if (entries[i].episode == (uint32_t)(episode + 1)) {
            if ((int)current == index) return (int)i;
            current++;
        }
    }
    return -1;
}
static void load_pack(void) {
    uint8_t head[HEADER_BYTES], record[RECORD_BYTES];
    FILE *fp;
    file_count = 0; pack_bytes = 0; pack_state = -1;
    for (int i=0; i<COUNT; ++i) by_episode[i]=0;
    /* Register the data pack on APF slot 4. */
    of_file_slot_register(4, "keenpack.bin");
    fp = fopen("keenpack.bin", "rb");
    if (!fp) { pack_message = "PACK MISSING - CHECK INSTALL"; return; }
    if (fseek(fp,0,SEEK_END) != 0) { pack_message="PACK SEEK FAILED"; goto done; }
    long end = ftell(fp);
    if (end < HEADER_BYTES || (uint64_t)end >= 0x7fffffffu) {
        pack_message="PACK HAS INVALID SIZE"; goto done;
    }
    pack_bytes = (uint32_t)end;
    if (fseek(fp,0,SEEK_SET) != 0 || fread(head,1,HEADER_BYTES,fp)!=HEADER_BYTES) {
        pack_message="PACK HEADER READ FAILED"; goto done;
    }
    if (memcmp(head,"RDKP019",7)!=0 || head[7]!=0 || le32(head+8)!=1) {
        pack_message="PACK FORMAT VERSION MISMATCH"; goto done;
    }
    uint32_t count = le32(head+12);
    if (count<1 || count>MAX_FILES || HEADER_BYTES + RECORD_BYTES*count > pack_bytes) {
        pack_message="PACK INDEX INVALID"; goto done;
    }
    uint32_t min_offset = HEADER_BYTES + RECORD_BYTES*count;
    for (uint32_t i=0;i<count;i++) {
        if (fread(record,1,RECORD_BYTES,fp)!=RECORD_BYTES) {
            pack_message="PACK DIRECTORY READ FAILED"; goto done;
        }
        uint32_t ep=le32(record), off=le32(record+28), size=le32(record+32);
        if (ep<1 || ep>COUNT || !memchr(record+4,0,24) ||
            off<min_offset || off>pack_bytes || size>pack_bytes-off) {
            pack_message="PACK DIRECTORY INVALID"; goto done;
        }
        entries[i].episode=ep; entries[i].offset=off;
        entries[i].size=size; entries[i].signature=le32(record+36);
        memcpy(entries[i].name,record+4,24);
        by_episode[ep-1]++;
        file_count++;
    }
    pack_state=1;
    pack_message="ASSET INDEX LOADED";
done:
    fclose(fp);
}
static void check_file(void) {
    check_state = -1;
    int idx = file_for_selection(selected,cursor);
    if (idx<0 || !pack_state) return;
    FILE *fp=fopen("keenpack.bin","rb");
    if (!fp) return;
    uint8_t sample[4]={0,0,0,0};
    unsigned need = entries[idx].size < 4 ? (unsigned)entries[idx].size : 4u;
    if (fseek(fp,(long)entries[idx].offset,SEEK_SET)==0 &&
        fread(sample,1,need,fp)==need) {
        checked_first=le32(sample);
        check_state=(checked_first==entries[idx].signature) ? 1 : -1;
    }
    fclose(fp);
}
static void rect(uint8_t *fb,int x,int y,int w,int h,uint8_t c) {
    if(x<0||y<0||w<=0||h<=0||x+w>W||y+h>H)return;
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)fb[yy*W+xx]=c;
}
static void glyph(uint8_t *fb,int x,int y,char c,uint8_t color) {
    const uint8_t *bits=0;
    for(unsigned i=0;i<sizeof(glyphs)/sizeof(glyphs[0]);i++)
        if(glyphs[i].ch==c) {bits=glyphs[i].rows;break;}
    if(!bits)return;
    for(int dy=0;dy<7;dy++)for(int dx=0;dx<5;dx++)
        if(bits[dy]&(1<<(4-dx))) {
            int px=x+dx,py=y+dy;
            if(px>=0&&py>=0&&px<W&&py<H)fb[py*W+px]=color;
        }
}
static int tw(const char *s){int n=0;while(*s++)n++;return n?6*n-1:0;}
static void txt(uint8_t *fb,int x,int y,const char *s,uint8_t color){
    for(;*s;s++,x+=6)glyph(fb,x,y,*s,color);
}
static void center(uint8_t *fb,int y,const char *s,uint8_t color){
    txt(fb,(W-tw(s))/2,y,s,color);
}
static void number(char *dst,unsigned n){
    char temp[12];int i=0,j=0;
    do{temp[i++]=(char)('0'+n%10);n/=10;}while(n&&i<11);
    while(i) dst[j++]=temp[--i];
    dst[j]=0;
}
static void frame(uint8_t *fb){
    char line[64],val[16];
    rect(fb,0,0,W,H,0);
    rect(fb,0,0,W,5,4);
    center(fb,12,"RETRODRIVEN",5);
    center(fb,27,"COMMANDER KEEN",6);
    rect(fb,12,43,296,1,3);
    if(!detail){
        for(int i=0;i<COUNT;i++){
            int y=51+i*20;
            if(i==selected)rect(fb,10,y-4,300,17,2);
            txt(fb,14,y,titles[i],i==selected?6:7);
            txt(fb,275,y,(pack_state>0 && by_episode[i])?"OK":"--",(pack_state>0 && by_episode[i])?5:8);
        }
        rect(fb,12,200,296,1,3);
        center(fb,208,pack_state>0?"DATA FOUND - A INSPECT":pack_message,pack_state>0?5:4);
        center(fb,222,pack_state>0?"V1.0.0":"CHECK BUILD AND INSTALL",8);
    }else{
        center(fb,54,titles[selected],5);
        number(val,by_episode[selected]);
        strcpy(line,"FILES: ");strcat(line,val);
        center(fb,78,line,7);
        if(!by_episode[selected]) {
            center(fb,107,"NO DATA FOR THIS EPISODE",4);
        }else {
            int idx=file_for_selection(selected,cursor);
            if(idx>=0) {
                center(fb,99,entries[idx].name,6);
                number(val,(unsigned)(cursor+1));
                strcpy(line,"FILE ");strcat(line,val);strcat(line," OF ");
                number(val,by_episode[selected]);strcat(line,val);
                center(fb,117,line,7);
                number(val,entries[idx].size);
                strcpy(line,"BYTES: ");strcat(line,val);center(fb,134,line,8);
                center(fb,155,check_state==1?"FIRST BYTES MATCH":(check_state<0?"FILE READ FAILED":"PRESS A TO CHECK BYTES"),check_state==1?5:6);
            }
        }
        center(fb,182,"START NEW GAME - L MAP VIEWER",8);
        center(fb,194,"UP SPRITES  DOWN SOUNDS",8);
        center(fb,206,"A READ - X RELOAD",8);
        center(fb,218,"LEFT RIGHT BROWSE",8);
        center(fb,230,"B BACK",7);
    }
}
int main(void){

    of_video_init();
    of_video_palette_bulk(base_colors,25);
    of_mixer_init(32,48000);
    load_pack();
#if defined(__riscv) && !defined(OF_PC)
    uint32_t save_slot=0;if(of_file_slot_find("RetroDrivenK2_state.sav",&save_slot)==0&&save_slot==19)instance_episode=1;
    if(of_file_slot_find("RetroDrivenK3_state.sav",&save_slot)==0&&save_slot==19)instance_episode=2;
    if(of_file_slot_find("RetroDrivenK4_state.sav",&save_slot)==0&&save_slot==19)instance_episode=3;
    if(of_file_slot_find("RetroDrivenK5_state.sav",&save_slot)==0&&save_slot==19)instance_episode=4;
    if(of_file_slot_find("RetroDrivenK6_state.sav",&save_slot)==0&&save_slot==19)instance_episode=5;
    if(of_file_slot_find("RetroDrivenK7_state.sav",&save_slot)==0&&save_slot==19)instance_episode=6;
#endif
    selected=instance_episode;
#ifdef RD_GALAXY_FULL
    if(selected>=3&&selected<=5){extern void rd_galaxy_run(unsigned);rd_galaxy_run((unsigned)selected+1);}
    if(selected==6){extern void rd_dreams_run(void);rd_dreams_run();}
#endif
    if(selected==3)galaxy_open();
    if(selected<3 && pack_state==1 && by_episode[selected])episode_intro_open();
    while(1){
        sound_update();
        of_input_poll();
        if(galaxy_mode){galaxy_tick();
        }else if(sound_mode){
            if(of_btn_pressed(OF_BTN_B)||of_btn_pressed(OF_BTN_SELECT)){
                sound_stop();sound_mode=0;
            }else if(of_btn_pressed(OF_BTN_Y)){
                sound_enabled=!sound_enabled;
                if(!sound_enabled)sound_stop();
                sound_message=sound_enabled?"SOUND ON":"MUTED";
            }else if(of_btn_pressed(OF_BTN_X)){
                sound_stop();sound_message="STOPPED";
                if(sound_state!=1)sound_load();
            }else if(sound_state==1){
                if(of_btn_pressed(OF_BTN_RIGHT))sound_selected=(sound_selected+1u)%sound_bank.count;
                if(of_btn_pressed(OF_BTN_LEFT))sound_selected=(sound_selected+sound_bank.count-1u)%sound_bank.count;
                if(of_btn_pressed(OF_BTN_A)){sound_stop();sound_play(sound_selected);}
            }
        }else if(episode_menu_mode){episode_menu_tick();
        }else if(play_mode){
            update_play_input();
        }else if(map_mode){
            if(of_btn_pressed(OF_BTN_A) && map_state==1){
                start_play();
            }else if(of_btn_pressed(OF_BTN_B) || of_btn_pressed(OF_BTN_SELECT))map_mode=0;
            else {
                if(of_btn_pressed(OF_BTN_X))next_map();
                if(of_btn_pressed(OF_BTN_Y)){show_objects=!show_objects;map_dirty=1;}
                if(map_state==1){
                    int max_x=level_info.width>20 ? (int)level_info.width-20 : 0;
                    int max_y=level_info.height>12 ? (int)level_info.height-12 : 0;
                    if(of_btn_pressed(OF_BTN_LEFT)&&map_x>0){map_x--;map_dirty=1;}
                    if(of_btn_pressed(OF_BTN_RIGHT)&&map_x<max_x){map_x++;map_dirty=1;}
                    if(of_btn_pressed(OF_BTN_UP)&&map_y>0){map_y--;map_dirty=1;}
                    if(of_btn_pressed(OF_BTN_DOWN)&&map_y<max_y){map_y++;map_dirty=1;}
                }
            }
        }else if(sprite_mode){
            if(of_btn_pressed(OF_BTN_B) || of_btn_pressed(OF_BTN_SELECT))sprite_mode=0;
            else if(of_btn_pressed(OF_BTN_X))load_sprites();
            else if(sprite_state==1){
                if(of_btn_pressed(OF_BTN_RIGHT))sprite_index=(sprite_index+1u)%sprite_info.sprite_count;
                if(of_btn_pressed(OF_BTN_LEFT))sprite_index=(sprite_index+sprite_info.sprite_count-1u)%sprite_info.sprite_count;
                if(of_btn_pressed(OF_BTN_UP)){
                    KeenSprite sp;
                    if(!keen_sprite_get(sprite_head_file,sprite_head_len,&sprite_info,sprite_index,&sp)){
                        player_sprite_index=sprite_index;player_sprite_enabled=1;auto_player_animation=0;
                    }
                }
                if(of_btn_pressed(OF_BTN_DOWN)){
                    /* Reset to a named player frame, or use the preview placeholder. */
                    load_sprites();
                }
            }
        }else if(tile_mode){
            if(of_btn_pressed(OF_BTN_B) || of_btn_pressed(OF_BTN_SELECT))tile_mode=0;
            if(of_btn_pressed(OF_BTN_X))load_tiles();
            if(ega_state==1){
                unsigned pages=(ega_info.tiles+143u)/144u;
                if(of_btn_pressed(OF_BTN_RIGHT))tile_page=(tile_page+1)%pages;
                if(of_btn_pressed(OF_BTN_LEFT))tile_page=(tile_page+pages-1)%pages;
            }
        }else if(!detail){
            if(of_btn_pressed(OF_BTN_UP)) selected=(selected+COUNT-1)%COUNT;
            if(of_btn_pressed(OF_BTN_DOWN)) selected=(selected+1)%COUNT;
            if(of_btn_pressed(OF_BTN_A)||of_btn_pressed(OF_BTN_START)) {
                detail=1;cursor=0;check_state=0;
            }
        }else{
            if(of_btn_pressed(OF_BTN_B)||of_btn_pressed(OF_BTN_SELECT)) detail=0;
            if(of_btn_pressed(OF_BTN_X)) {load_pack();cursor=0;check_state=0;ega_state=0;sprite_state=0;sprite_episode=-1;}
            if(of_btn_pressed(OF_BTN_Y)){tile_mode=1;load_tiles();}
            if(of_btn_pressed(OF_BTN_DOWN)){
                sound_mode=1;
                if(sound_episode!=selected || sound_state==0)sound_load();
            }
            if(of_btn_pressed(OF_BTN_UP)){
                sprite_mode=1;
                if(sprite_episode!=selected || sprite_state==0)load_sprites();
            }
            if(of_btn_pressed(OF_BTN_START)){
                if(selected<3)episode_intro_open();
                else{map_mode=1;map_id=80;ega_state=0;load_map();}
            }
            if(of_btn_pressed(OF_BTN_L1)){campaign_active=world_mode=0;map_mode=1;map_id=80;ega_state=0;load_map();}
            if(by_episode[selected]) {
                if(of_btn_pressed(OF_BTN_LEFT)) {
                    cursor=(cursor+(int)by_episode[selected]-1)%(int)by_episode[selected];check_state=0;
                }
                if(of_btn_pressed(OF_BTN_RIGHT)) {
                    cursor=(cursor+1)%(int)by_episode[selected];check_state=0;
                }
                if(of_btn_pressed(OF_BTN_A)) check_file();
            }
        }
        if(galaxy_mode)galaxy_frame(of_video_surface());
        else if(episode_menu_mode)episode_menu_frame(of_video_surface());
        else if(sound_mode)sound_frame(of_video_surface());
        else if(play_mode){if(score_flow_active)score_flow_frame(of_video_surface());else if(play_notice)notice_frame(of_video_surface());else if(autosave_prompt_pending)autosave_prompt_frame(of_video_surface());else if(game_menu_visible)game_menu_frame(of_video_surface());else if(status_visible)status_frame(of_video_surface());else if(cinema3.active)keen3_scene_frame(of_video_surface());else if(finale.active)finale_frame(of_video_surface());else if(world_mode)world_frame(of_video_surface());else play_frame(of_video_surface());}
        else if(map_mode){
            if(map_dirty){map_frame(map_cached);map_dirty=0;}
            memcpy(of_video_surface(),map_cached,sizeof(map_cached));
        }
        else if(tile_mode)tile_frame(of_video_surface());
        else if(sprite_mode)sprite_frame(of_video_surface());
        else frame(of_video_surface());
        if(play_mode&&selected<3&&!episode_menu_mode&&!keen_core_view_enhanced())keen_view_flat_margins(of_video_surface(),0,9);
        of_video_flip();
        of_video_wait_flip();
    }
}

#ifdef RD_GALAXY_FULL
const char*rd_galaxy_notice(void);
extern int rd_in_text,rd_in_help,rd_in_menu,rd_in_play;
int rd_galaxy_hud_inventory(unsigned*,unsigned*);
static void galaxy_inventory(uint8_t*fb){
 unsigned needed,held;if(!rd_galaxy_hud_inventory(&needed,&held))return;
 unsigned count=0;for(unsigned i=0;i<4;i++)count+=(needed>>i)&1u;
 int width=(int)count*14+8,x=4,y=59;rect(fb,x,y,width,14,0);rect(fb,x+1,y+1,width-2,12,7);
 static const uint8_t colors[]={4,14,1,2};int gx=x+5;
 for(unsigned i=0;i<4;i++)if(needed&(1u<<i)){unsigned color=(held&(1u<<i))?colors[i]:8;
  for(int row=0;row<9;row++)for(int col=0;col<9;col++)if(abs(row-4)+abs(col-4)<=4)fb[(y+2+row)*W+gx+col]=(uint8_t)color;
  fb[(y+5)*W+gx+3]=15;gx+=14;
 }
}
static void galaxy_notice_box(uint8_t*fb,const char*s){
 int w=tw(s)+16;if(w>318)w=318;int x=(W-w)/2,y=218;
 rect(fb,x,y,w,20,19);rect(fb,x+1,y+1,w-2,18,20);
 rect(fb,x+2,y+2,w-4,16,17);rect(fb,x+3,y+3,w-6,14,21);
 rect(fb,x+4,y+4,w-8,12,17);
 center(fb,y+7,s,strstr(s,"FAILED")?4:18);
}
void rd_galaxy_notice_draw(uint8_t*fb,int stride){galaxy_inventory(fb);(void)stride;const char*s=rd_galaxy_notice();if(s)galaxy_notice_box(fb,s);}
void rd_galaxy_error_screen(const char*message){for(;;){uint8_t*fb=of_video_surface();memset(fb,9,W*H);of_video_palette_bulk(base_colors,25);center(fb,70,"COMMANDER KEEN",7);center(fb,96,"GAME ENGINE STOPPED",6);char line[39];size_t len=strlen(message?message:"");for(unsigned i=0;i<4&&i*38<len;i++){size_t n=len-i*38;if(n>38)n=38;memcpy(line,message+i*38,n);line[n]=0;center(fb,120+(int)i*12,line,7);}of_video_flip();of_video_wait_flip();}}
#endif
