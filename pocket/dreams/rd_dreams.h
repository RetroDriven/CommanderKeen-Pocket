/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef RD_DREAMS_PLATFORM_H
#define RD_DREAMS_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
uint32_t rd_dreams_us(void);
unsigned rd_dreams_buttons(void);
void rd_dreams_present(const uint8_t *pixels,const uint8_t *palette);
void rd_dreams_pcm(const int16_t *pcm,int pairs);
void rd_dreams_audio_start(void);
_Noreturn void rd_dreams_fatal(const char *message);
void rd_dreams_pump(void);
void rd_dreams_input(void);
void rd_dreams_run(void);
unsigned rd_dreams_options(void);
unsigned rd_dreams_view_height(void);
void rd_dreams_view_begin(void);
void rd_dreams_compose_frame(uint8_t*fb,const uint8_t*pixels);
void rd_dreams_inventory_overlay(uint8_t*,int);
void rd_dreams_present_pending(void);
extern int rd_dreams_port_rows;
#endif
