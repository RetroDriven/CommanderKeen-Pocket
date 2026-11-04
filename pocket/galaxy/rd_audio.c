/* SPDX-License-Identifier: GPL-2.0-or-later
 * Stereo-pair queue for the matched Pocket runtime's hardware audio ring. */
#include "rd_platform.h"
#include <string.h>
enum {RD_AUDIO_PAIRS=8192,RD_AUDIO_PRIME=2048};
static int16_t queue[RD_AUDIO_PAIRS*2];
static unsigned head,queued;
static int opened;
int rd_galaxy_stream_open(int rate);
int rd_galaxy_stream_write(const int16_t*p,int pairs);
void rd_galaxy_audio_flush(void){
 if(!opened)return;
 while(queued){
  unsigned n=RD_AUDIO_PAIRS-head;if(n>queued)n=queued;
  int written=rd_galaxy_stream_write(queue+head*2,(int)n);
  if(written<=0||(unsigned)written>n)return;
  head=(head+(unsigned)written)%RD_AUDIO_PAIRS;queued-=(unsigned)written;
 }
}
void rd_galaxy_audio_start(void){
 head=0;queued=0;opened=rd_galaxy_stream_open(22050)==0;
 if(!opened)return;
 /* Prime 2048 stereo pairs (about 93 ms) to cover producer batches and frame-time jitter. */
 memset(queue,0,RD_AUDIO_PRIME*2*sizeof(int16_t));queued=RD_AUDIO_PRIME;
 rd_galaxy_audio_flush();
}
void rd_galaxy_pcm(const int16_t*p,int pairs){
 if(!opened||!p||pairs<=0)return;
 rd_galaxy_audio_flush();
 while(pairs>0){
  unsigned tail=(head+queued)%RD_AUDIO_PAIRS;
  unsigned n=RD_AUDIO_PAIRS-tail;
  if(n>RD_AUDIO_PAIRS-queued)n=RD_AUDIO_PAIRS-queued;
  if(n>(unsigned)pairs)n=(unsigned)pairs;
  if(!n)return; /* Keep queued audio when the ring is full. */
  memcpy(queue+tail*2,p,n*2*sizeof(int16_t));queued+=n;p+=n*2;pairs-=(int)n;
  rd_galaxy_audio_flush();
 }
}
