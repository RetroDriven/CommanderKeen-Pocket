/* RetroDriven Pocket options for the matched os25 runtime (618a3eb).
 * core_top.v mirrors the complete F7000000 settings word at CPU 40000080.
 * Firmware analogizer_refresh preserves 23:16 and clears bits above 23.
 * Keen reserves 23:22 for autosave, bit 21 for the optional HUD,
 * bit 20 for the expanded viewport and bit 19 for enhanced HUD inventory;
 * this core does not declare/use the legacy mouse-speed setting there.
 * Do not use F7000010: that address selects the runtime app ID.
 * SPDX-License-Identifier: MIT */
#ifndef KEEN_CORE_OPTIONS_H
#define KEEN_CORE_OPTIONS_H
#include <stdint.h>
enum {KCO_AUTOSAVE_ON,KCO_AUTOSAVE_PROMPT,KCO_AUTOSAVE_OFF};
#ifdef KEEN_CORE_OPTIONS_TEST
uint32_t keen_core_settings_word(void);
#elif defined(__riscv) && !defined(OF_PC)
static inline uint32_t keen_core_settings_word(void){
 return *(volatile const uint32_t *)(uintptr_t)0x40000080u;
}
#else
static inline uint32_t keen_core_settings_word(void){return 0;}
#endif
/* Dedicated F700000C latch, mirrored at CPU 4000006C in 618a3eb.
 * Keen has no mouse-speed option. Zero means unset; one encodes Mute. */
#ifdef KEEN_CORE_VOLUME_TEST
uint32_t keen_core_volume_word(void);
#elif defined(__riscv) && !defined(OF_PC)
static inline uint32_t keen_core_volume_word(void){return *(volatile const uint32_t *)(uintptr_t)0x4000006cu;}
#else
static inline uint32_t keen_core_volume_word(void){return 0;}
#endif
static inline unsigned keen_core_audio_gain(void){
 switch(keen_core_volume_word()){
 case 75:return 192;case 50:return 128;case 25:return 64;case 1:return 0;
 default:return 256;
 }
}
static inline int16_t keen_core_scale_sample(int16_t sample,unsigned gain){return (int16_t)((int32_t)sample*(int32_t)gain/256);}
static inline int keen_core_hud_enabled(void){return !!(keen_core_settings_word()&0x00200000u);}
static inline int keen_core_view_enhanced(void){return !!(keen_core_settings_word()&0x00100000u);}
static inline int keen_core_hud_enhanced(void){return (keen_core_settings_word()&0x00280000u)==0x00280000u;}
static inline unsigned keen_core_autosave_mode(void){
 unsigned mode=(keen_core_settings_word()>>22)&3u;
 return mode>2?KCO_AUTOSAVE_OFF:mode;
}
#endif
