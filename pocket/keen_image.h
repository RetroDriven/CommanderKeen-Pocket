/* Bounded original Vorticons image RLE and 320x200 four-plane decoding.
 * Reference: Chocolate Keen imageRLE.c/gfx.c. SPDX-License-Identifier: MIT */
#ifndef KEEN_IMAGE_H
#define KEEN_IMAGE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static inline int keen_image_decode(const uint8_t*src,size_t size,uint8_t*dst,size_t cap,size_t expected){
 if(!src||!dst||size<4||!expected||cap<expected)return -1;
 uint32_t declared=(uint32_t)src[0]|((uint32_t)src[1]<<8)|((uint32_t)src[2]<<16)|((uint32_t)src[3]<<24);
 if(declared!=expected)return -1;
 size_t p=4,written=0;
 while(written<expected){
  if(p>=size)return -1;
  unsigned action=src[p++],count=action>=128?action-127:action+3;
  size_t copy=count;if(copy>expected-written)copy=expected-written;
  if(action>=128){if(count>size-p)return -1;memcpy(dst+written,src+p,copy);p+=count;}
  else{if(p>=size)return -1;memset(dst+written,src[p++],copy);}
  written+=copy;
 }
 /* The last RLE packet may exceed the declared plane length. Validate the full packet,
  * then clip writes to the output buffer. */
 return 0;
}
static inline unsigned keen_image_pixel(const uint8_t*planes,size_t size,unsigned x,unsigned y){
 if(!planes||size<32768||x>=320||y>=200)return 0;
 size_t byte=y*40u+x/8u;unsigned mask=128u>>(x&7u),color=0;
 for(unsigned p=0;p<4;p++)if(planes[byte+p*8192u]&mask)color|=1u<<p;
 return color;
}
#endif
