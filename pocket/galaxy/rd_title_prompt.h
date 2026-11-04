/* SPDX-License-Identifier: MIT
 * Pocket captions patched into decoded DOS bitmaps. Original files stay intact. */
#ifndef RD_TITLE_PROMPT_H
#define RD_TITLE_PROMPT_H
#include <stdint.h>
#include <string.h>
static inline unsigned rd_title_colour(const uint8_t*data,int x,int y){
 unsigned at=(unsigned)y*40u+(unsigned)x/8u,mask=128u>>((unsigned)x&7u),c=0;
 for(unsigned p=0;p<4;p++)if(data[at+p*8000u]&mask)c|=1u<<p;
 return c;
}
static inline void rd_title_pixel(uint8_t*data,int x,int y,unsigned color){
 unsigned at=(unsigned)y*40u+(unsigned)x/8u,mask=128u>>((unsigned)x&7u);
 for(unsigned p=0;p<4;p++){uint8_t*b=data+at+p*8000u;*b=(uint8_t)((*b&~mask)|((color&(1u<<p))?mask:0));}
}
static inline void rd_title_prompt_pixels(uint8_t*data,unsigned episode){
 static const uint8_t select[6][5]={{15,24,14,3,30},{31,24,30,24,31},{24,24,24,24,31},{31,24,30,24,31},{15,24,24,24,15},{31,6,6,6,6}};
 int left=episode==5?272:271;unsigned correct=1;
 for(int i=0;i<6;i++)for(int y=0;y<5;y++)for(int x=0;x<5;x++)if(select[i][y]&(16u>>x))
  if(rd_title_colour(data,left+i*6+x,182+y)!=12)correct=0;
 if(correct)return;
 /* Restore the old F1 area from adjacent background pixels. */
 uint8_t background[8][13];int old=episode==5?284:283;
 for(int y=0;y<8;y++)for(int x=0;x<13;x++)background[y][x]=(uint8_t)rd_title_colour(data,old-16+x,181+y);
 for(int y=0;y<8;y++)for(int x=0;x<13;x++)rd_title_pixel(data,old+x,181+y,background[y][x]);
 /* Add the black outline used by the surrounding artwork. */
 for(int i=0;i<6;i++)for(int y=0;y<5;y++)for(int x=0;x<5;x++)if(select[i][y]&(16u>>x))
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)rd_title_pixel(data,left+i*6+x+dx,182+y+dy,0);
 for(int i=0;i<6;i++)for(int y=0;y<5;y++)for(int x=0;x<5;x++)if(select[i][y]&(16u>>x))
  rd_title_pixel(data,left+i*6+x,182+y,12);
}
static inline void rd_watch_prompt_pixels(uint8_t*data){
 /* Patch the shareware watch caption once; retail watches have no F1 caption. */
 for(int x=85;x<=90;x++)if(rd_title_colour(data,x,182)!=14)return;
 uint8_t original[14][240];
 for(int y=0;y<14;y++)for(int x=0;x<240;x++)original[y][x]=(uint8_t)rd_title_colour(data,x+38,y+180);
 /* Restore the narrow lower chrome/shadow band obscured by the old caption. */
 for(int y=180;y<195;y++)for(int x=39;x<286;x++){
  unsigned c=y<187?(x<=275?7:x==276?15:3):3;
  if(y==187&&x<=275)c=0;else if(y==187&&x==276)c=8;
  if(y==188&&x<=274)c=0;else if(y==188&&x==275)c=8;
  if(y>=189&&y<=191){if(x>=55&&x<=257)c=0;if((x>=56&&x<=64)||(x>=248&&x<=256))c=y==189?15:y==190?7:8;}
  if(y==192&&x>=55&&x<=257)c=0;
  if((y==193||y==194)&&x>=65&&x<=247)c=0;
  rd_title_pixel(data,x,y,c);
 }
 /* Reuse the existing Press / for Help and Order Info lettering. */
 uint8_t ink[14][270];memset(ink,0,sizeof(ink));
 for(int y=0;y<14;y++){
  for(int x=41;x<=78;x++){unsigned c=original[y][x-38];if(c==4||c==14)ink[y][x-41]=(uint8_t)c;}
  for(int x=102;x<=267;x++){unsigned c=original[y][x-38];if(c==4||c==14)ink[y][83+x-102]=(uint8_t)c;}
 }
 /* Select uses a gold face, red outline and black shadow. */
 static const uint8_t capital_s[7]={31,48,48,30,3,3,62};
 static const uint8_t letter_t[7]={12,12,63,12,12,12,7};
 uint8_t face[10][40];memset(face,0,sizeof(face));
 for(int y=0;y<7;y++)for(int x=0;x<6;x++){
  if(capital_s[y]&(32u>>x))face[y+2][x]=1;
  if(letter_t[y]&(32u>>x))face[y+2][31+x]=1;
 }
 for(int y=0;y<9;y++)for(int x=0;x<6;x++){
  unsigned e=original[y+1][139+x-38]==14;
  face[y+1][7+x]=(uint8_t)e;face[y+1][19+x]=(uint8_t)e;
  unsigned o=original[y+1][110+x-38]==14;
  if(o&&!(y>=4&&y<=6&&x>=3))face[y+1][26+x]=1;
 }
 for(int y=0;y<9;y++)for(int x=0;x<3;x++)face[y+1][14+x]=(uint8_t)(original[y+1][146+x-38]==14);
 for(int y=0;y<10;y++)for(int x=0;x<38;x++)if(face[y][x]){
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if((dx==0||dy==0)&&y+dy>=0&&y+dy<14&&42+x+dx>=0)ink[y+dy][42+x+dx]=4;
 }
 for(int y=0;y<10;y++)for(int x=0;x<38;x++)if(face[y][x])ink[y][42+x]=14;
 for(int y=0;y<14;y++)for(int x=0;x<249;x++)if(ink[y][x]){if(y+182<200)rd_title_pixel(data,33+x,182+y,0);}
 for(int y=0;y<14;y++)for(int x=0;x<249;x++)if(ink[y][x])rd_title_pixel(data,31+x,180+y,ink[y][x]);
}
#endif
