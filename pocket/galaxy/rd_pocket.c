/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rd_platform.h"
#include "of.h"
#include "of_audio.h"
#include "of_file.h"
#include "../keen_core_options.h"
#include "../keen_audio_volume.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
void rd_galaxy_audio_flush(void);
uint32_t rd_galaxy_us(void){return of_time_us();}
unsigned rd_galaxy_buttons(void){
 rd_galaxy_audio_flush();of_input_poll();unsigned b=0;
 if(of_btn(OF_BTN_UP))b|=RD_UP;
 if(of_btn(OF_BTN_DOWN))b|=RD_DOWN;
 if(of_btn(OF_BTN_LEFT))b|=RD_LEFT;
 if(of_btn(OF_BTN_RIGHT))b|=RD_RIGHT;
 if(of_btn(OF_BTN_A))b|=RD_A;
 if(of_btn(OF_BTN_B))b|=RD_B;
 if(of_btn(OF_BTN_X))b|=RD_X;
 if(of_btn(OF_BTN_Y))b|=RD_Y;
 if(of_btn(OF_BTN_START))b|=RD_START;
 if(of_btn(OF_BTN_SELECT))b|=RD_SELECT;
 return b;
}
unsigned rd_galaxy_options(void){return keen_core_settings_word();}
/* Runtime 618a3eb stream_write counts interleaved stereo pairs. */
int rd_galaxy_stream_open(int rate){return of_audio_stream_open(rate);}
int rd_galaxy_stream_write(const int16_t*p,int pairs){
 return keen_core_stream_write(p,pairs,of_audio_stream_write);
}
void rd_galaxy_vbls(int n){uint32_t start=of_time_us(),duration=(uint32_t)(n>0?n:1)*14286;while((uint32_t)(of_time_us()-start)<duration)rd_galaxy_pump();}
void rd_galaxy_notice_draw(uint8_t*,int);
void rd_galaxy_present(const uint8_t*p,int stride,int height,int x,int y,const uint8_t*pal){
 uint32_t colors[22];for(unsigned i=0;i<16;i++){unsigned c=pal[i];colors[i]=(((c&4?170u:0u)+(c&8?85u:0u))<<16)|(((c&2?170u:0u)+(c&8?85u:0u))<<8)|((c&1?170u:0u)+(c&8?85u:0u));}for(unsigned i=0;i<16;i++)if(pal[i]==6)colors[i]=0xaa5500;colors[16]=0x55ff55;colors[17]=0xffffff;colors[18]=0x008000;colors[19]=0x000000;colors[20]=0xaaaaaa;colors[21]=0x555555;
 of_video_palette_bulk(colors,22);uint8_t*fb=of_video_surface();
 rd_galaxy_compose_frame(fb,p,stride,height,x,y);
 rd_galaxy_notice_draw(fb,320);rd_galaxy_pump();of_video_flip();of_video_wait_flip();rd_galaxy_audio_flush();
}
_Noreturn void rd_galaxy_fatal(const char*message){extern void rd_galaxy_error_screen(const char*);of_audio_stream_close();rd_galaxy_error_screen(message);for(;;){}}
int rd_galaxy_main(int,char**);
void rd_galaxy_run(unsigned episode){rd_galaxy_select_episode(episode);char number[2]={(char)('0'+episode),0};
 const char*names[]={"EGAGRAPH.CK4","GAMEMAPS.CK4","AUDIO.CK4"};
 for(unsigned i=0;i<3;i++){char name[16];snprintf(name,sizeof(name),"%s",names[i]);name[strlen(name)-1]=(char)('0'+episode);size_t n;void*p=rd_galaxy_asset(name,&n);if(!p)rd_galaxy_fatal("Missing original game data. Import this episode and run PACK-GAMES.cmd.");free(p);}
 char*args[]={"Commander Keen","/EPISODE",number,"/NOBORDER","/NOCOPY",NULL};rd_galaxy_main(5,args);}
