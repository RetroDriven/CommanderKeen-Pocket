/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef RD_GALAXY_PLATFORM_H
#define RD_GALAXY_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
unsigned rd_galaxy_episode(void);
void rd_galaxy_select_episode(unsigned episode);
extern unsigned rd_galaxy_buffer_rows;
unsigned rd_galaxy_view_height(void);
void rd_galaxy_view_begin(void);
void rd_galaxy_view_update(void);
void rd_galaxy_compose_frame(uint8_t*,const uint8_t*,int,int,int,int);
void rd_galaxy_pump(void);
uint32_t rd_galaxy_us(void);
unsigned rd_galaxy_buttons(void);
void rd_galaxy_vbls(int n);
void rd_galaxy_present(const uint8_t*p,int stride,int height,int x,int y,const uint8_t*palette);
void rd_galaxy_pcm(const int16_t*p,int pairs);
void rd_galaxy_audio_start(void);
void rd_galaxy_input(void);
void *rd_galaxy_asset(const char*name,size_t*size);
_Noreturn void rd_galaxy_fatal(const char*message);
enum {RD_UP=1,RD_DOWN=2,RD_LEFT=4,RD_RIGHT=8,RD_A=16,RD_B=32,RD_X=64,RD_Y=128,RD_START=256,RD_SELECT=512};
#endif
