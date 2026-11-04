/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rd_dreams.h"
#include "of.h"
#include "of_audio.h"
#include "../keen_core_options.h"
#include <string.h>
uint32_t rd_dreams_us(void){return of_time_us();}
unsigned rd_galaxy_buttons(void);
void rd_galaxy_pcm(const int16_t*,int),rd_galaxy_audio_start(void),rd_galaxy_audio_flush(void);
_Noreturn void rd_galaxy_fatal(const char*);
unsigned rd_dreams_buttons(void){return rd_galaxy_buttons();}
unsigned rd_dreams_options(void){return keen_core_settings_word();}
void rd_dreams_pcm(const int16_t*p,int n){rd_galaxy_pcm(p,n);}
void rd_dreams_audio_start(void){rd_galaxy_audio_start();}
_Noreturn void rd_dreams_fatal(const char*s){rd_galaxy_fatal(s);}
void rd_dreams_overlay(uint8_t*,int);
void rd_dreams_inventory_overlay(uint8_t*,int);
void rd_dreams_present(const uint8_t*p,const uint8_t*pal){
 uint32_t colors[22];for(unsigned i=0;i<16;i++){unsigned c=pal[i];colors[i]=(((c&4?170u:0u)+(c&8?85u:0u))<<16)|(((c&2?170u:0u)+(c&8?85u:0u))<<8)|((c&1?170u:0u)+(c&8?85u:0u));if(c==6)colors[i]=0xaa5500;}colors[16]=0x55ff55;colors[17]=0xffffff;colors[18]=0x008000;colors[19]=0x000000;colors[20]=0xaaaaaa;colors[21]=0x555555;
 of_video_palette_bulk(colors,22);uint8_t*fb=of_video_surface();rd_dreams_compose_frame(fb,p);rd_dreams_inventory_overlay(fb,320);rd_dreams_overlay(fb,320);rd_dreams_pump();of_video_flip();of_video_wait_flip();rd_galaxy_audio_flush();
}
