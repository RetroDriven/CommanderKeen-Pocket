/* SPDX-License-Identifier: GPL-2.0-or-later
 * Single-threaded PIT service and block-based OPL2/PC-speaker stream. */
#include "id_sd.h"
#include "rd_platform.h"
#include "opl/dbopl.h"
static Chip chip;
static int boost_movement_sfx;
static SynthHandler movement_synth;
/* Scale effect channel 0 before mixing the music channels. */
static Channel *movement_block(Channel *channel,Chip *opl,Bit32u count,Bit32s *out){
 Channel *next=movement_synth(channel,opl,count,out);
 for(Bit32u i=0;i<count;i++)out[i]=out[i]*(boost_movement_sfx==2?45:32)/16;
 return next;
}
void rd_galaxy_sfx_begin(int sound){
 unsigned episode=rd_galaxy_episode();
 boost_movement_sfx=episode>=4&&episode<=6?(sound==2?2:sound==7?1:0):0;
}
#ifdef RD_GALAXY_TEST
int rd_test_movement_gain(void){return boost_movement_sfx;}
#endif
static uint32_t last_us;
static uint64_t sample_credit,pit_credit;
static unsigned divisor=8514,beep_divisor;
static uint32_t phase;
static int pumping,started;
void SDL_t0Service(void);
void rd_galaxy_pump(void){
 if(!started||pumping)return;
 pumping=1;
 uint32_t now=rd_galaxy_us(),elapsed=now-last_us;last_us=now;
 if(elapsed>200000)elapsed=200000;
 sample_credit+=(uint64_t)elapsed*22050;unsigned count=(unsigned)(sample_credit/1000000);sample_credit%=1000000;
 static int16_t pcm[512*2];static unsigned used;
 while(count){
  /* Render up to the next PIT event before applying register writes. */
  pit_credit+=1193182;
  while(pit_credit>=(uint64_t)divisor*22050){pit_credit-=(uint64_t)divisor*22050;SDL_t0Service();}
  uint64_t remaining=(uint64_t)divisor*22050-1-pit_credit;
  unsigned n=1+(unsigned)(remaining/1193182);
  if(n>count)n=count;
  if(n>512-used)n=512-used;
  int32_t mono[512];
  movement_synth=chip.chan[0].synthHandler;
  if(boost_movement_sfx)chip.chan[0].synthHandler=movement_block;
  Chip__GenerateBlock2(&chip,n,mono);
  chip.chan[0].synthHandler=movement_synth;
  for(unsigned i=0;i<n;i++){
   int sample=mono[i]*2;
   /* Jump range: -32768..32767. Other effects: -16384..16383. */
   int limit=boost_movement_sfx==2?32767:16383;
   if(sample>limit)sample=limit;else if(sample< -limit-1)sample=-limit-1;
   int beep=0;
   if(beep_divisor){phase+=1193182;while(phase>=22050u*beep_divisor)phase-=22050u*beep_divisor;beep=phase<22050u*beep_divisor/2?3500:-3500;}
   int mixed=sample+beep;
   if(mixed>32767)mixed=32767;else if(mixed< -32768)mixed=-32768;
   pcm[used*2]=(int16_t)mixed;pcm[used*2+1]=(int16_t)mixed;
   used++;
  }
  pit_credit+=(uint64_t)(n-1)*1193182;count-=n;
  if(used==512){rd_galaxy_pcm(pcm,(int)used);used=0;}
 }
 pumping=0;
}
static void startup(void){boost_movement_sfx=0;DBOPL_InitTables();Chip__Chip(&chip);Chip__Setup(&chip,22050);rd_galaxy_audio_start();last_us=rd_galaxy_us();sample_credit=pit_credit=0;started=1;}
static void shutdown(void){started=0;}
static void noop(void){}
static void alout(uint8_t r,uint8_t v){Chip__WriteReg(&chip,r,v);}
static void speaker(bool on,int d){beep_divisor=on&&d>0?(unsigned)d:0;}
static void timer(int16_t d){if(d)divisor=(uint16_t)d;}
static unsigned detect(void){return SD_CARD_PC_SPEAKER|SD_CARD_OPL2;}
static void opl3(bool on){(void)on;}
static SD_Backend backend={startup,shutdown,noop,noop,alout,speaker,timer,rd_galaxy_pump,detect,opl3};
SD_Backend*SD_Impl_GetBackend(void){return &backend;}
