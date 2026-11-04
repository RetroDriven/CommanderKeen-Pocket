/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "refkeen.h"
#include "rd_dreams.h"
#include "opl/dbopl.h"
static Chip chip;
static uint32_t last_us,phase;
static uint64_t samples,pit;
static unsigned divisor=8514,beep_divisor;
static int started,pumping,int_calls;
static void(*service)(void);
void rd_dreams_pump(void){
 if(!started||pumping)return;pumping=1;
 uint32_t now=rd_dreams_us(),elapsed=now-last_us;last_us=now;if(elapsed>200000)elapsed=200000;
 samples+=(uint64_t)elapsed*22050;unsigned count=samples/1000000;samples%=1000000;
 static int16_t pcm[1024];static unsigned used;
 while(count){pit+=1193182;while(pit>=(uint64_t)divisor*22050){pit-=(uint64_t)divisor*22050;int_calls++;if(service)service();}
  unsigned n=1+(unsigned)(((uint64_t)divisor*22050-1-pit)/1193182);if(n>count)n=count;if(n>512-used)n=512-used;
  int32_t mono[512];Chip__GenerateBlock2(&chip,n,mono);
  for(unsigned i=0;i<n;i++){int sample=mono[i]*2;if(sample>16383)sample=16383;else if(sample< -16384)sample=-16384;
   if(beep_divisor){phase+=1193182;while(phase>=22050u*beep_divisor)phase-=22050u*beep_divisor;sample+=phase<22050u*beep_divisor/2?3500:-3500;}
   pcm[used*2]=pcm[used*2+1]=(int16_t)sample;used++;}
  pit+=(uint64_t)(n-1)*1193182;count-=n;if(used==512){rd_dreams_pcm(pcm,used);used=0;}
 }pumping=0;
}
void BE_ST_StartAudioAndTimerInt(void(*f)(void)){if(!started){DBOPL_InitTables();Chip__Chip(&chip);Chip__Setup(&chip,22050);rd_dreams_audio_start();last_us=rd_dreams_us();started=1;}service=f;}
void BE_ST_StopAudioAndTimerInt(void){service=NULL;started=0;}
void BE_ST_StartDigiAudioInt(void(*f)(void)){(void)f;}
void BE_ST_StopDigiAudioInt(void){}
void BE_ST_LockAudioRecursively(void){rd_dreams_pump();}
void BE_ST_UnlockAudioRecursively(void){}
bool BE_ST_IsEmulatedOPLChipReady(void){return true;}
void BE_ST_OPL2Write(uint8_t r,uint8_t v){Chip__WriteReg(&chip,r,v);}
void BE_ST_PCSpeakerSetInvFreq(uint16_t d){beep_divisor=d;}
void BE_ST_PCSpeakerSetConstVal(bool up){(void)up;beep_divisor=0;}
void BE_ST_SetTimer(uint16_t d){divisor=d?d:65536u;}
int BE_ST_TimerIntClearLastCalls(void){rd_dreams_pump();rd_dreams_input();int n=int_calls;int_calls=0;return n;}
void BE_ST_TimerIntCallsDelayWithOffset(int n){int start=int_calls;while(int_calls-start<n){rd_dreams_pump();rd_dreams_input();}}
void BE_ST_SetDigiSoundFreq(int n){(void)n;}
void BE_ST_PlaySoundEffect(void*p,int n,int bits){(void)p;(void)n;(void)bits;}
void BE_ST_StopSoundEffect(void){}
