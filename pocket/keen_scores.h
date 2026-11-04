/* Original seven-row score layout; journaled Pocket storage in a separate region.
 * Original SCORES.CK1 is read-only. SPDX-License-Identifier: MIT */
#ifndef KEEN_SCORES_H
#define KEEN_SCORES_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define KHS_RAW 204u
#define KHS_RECORD 256u
#define KHS_OFFSET 4096u
static inline uint32_t kh_read(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static inline void kh_put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static inline uint32_t kh_hash(const uint8_t*p){uint32_t h=2166136261u;for(unsigned i=0;i<252;i++)h=(h^p[i])*16777619u;return h;}
static inline int keen_scores_valid(const uint8_t*raw,size_t size){
 if(!raw||size<203)return 0;
 for(unsigned i=0;i<7;i++){
  if(kh_read(raw+i*4)>0x7fffffffu||!memchr(raw+112+i*13,0,13))return 0;
 }
 return 1;
}
static inline int keen_scores_position(const uint8_t*raw,uint32_t score){
 for(unsigned i=0;i<7;i++)if(score>kh_read(raw+i*4))return (int)i;
 return -1;
}
static inline int keen_scores_insert(uint8_t*raw,uint32_t score,unsigned parts,const char*name){
 if(!keen_scores_valid(raw,KHS_RAW)||score>0x7fffffffu||parts>15||!name)return -1;
 int position=keen_scores_position(raw,score);if(position<0)return -1;
 for(int row=6;row>position;row--){
  memcpy(raw+row*4,raw+(row-1)*4,4);
  for(unsigned p=0;p<4;p++)memcpy(raw+28+(p*7+(unsigned)row)*2,raw+28+(p*7+(unsigned)row-1)*2,2);
  memcpy(raw+84+row*2,raw+84+(row-1)*2,2);memcpy(raw+98+row*2,raw+98+(row-1)*2,2);
  memcpy(raw+112+row*13,raw+112+(row-1)*13,13);
 }
 kh_put(raw+(unsigned)position*4,score);static const unsigned part_bits[]={1,2,8,4};
 for(unsigned p=0;p<4;p++){unsigned at=28+(p*7+(unsigned)position)*2;raw[at]=(uint8_t)!!(parts&part_bits[p]);raw[at+1]=0;}
 raw[84+position*2]=raw[85+position*2]=raw[98+position*2]=raw[99+position*2]=0;
 memset(raw+112+position*13,0,13);
 for(unsigned i=0;i<12&&name[i];i++)raw[112+position*13+i]=(uint8_t)((name[i]>=32&&name[i]<=126)?name[i]:' ');
 return position;
}
static inline int kh_decode(const uint8_t*record,uint8_t*raw,uint32_t*sequence){
 if(memcmp(record,"KHP1",4)||kh_read(record+8)!=KHS_RAW||kh_read(record+12)!=1||
    kh_read(record+252)!=kh_hash(record)||!keen_scores_valid(record+16,KHS_RAW))return 0;
 memcpy(raw,record+16,KHS_RAW);*sequence=kh_read(record+4);return 1;
}
/* Returns active journal record index + 1, with no changes on invalid reads. */
static inline int keen_scores_read(const char*path,uint8_t*raw,uint32_t*sequence){
 FILE*f=fopen(path,"rb");if(!f)return 0;
 uint8_t a[KHS_RECORD],b[KHS_RECORD],ra[KHS_RAW],rb[KHS_RAW];uint32_t sa=0,sb=0;
 if(fseek(f,KHS_OFFSET,SEEK_SET)){fclose(f);return 0;}
 size_t na=fread(a,1,sizeof(a),f),nb=fread(b,1,sizeof(b),f);fclose(f);
 int va=na==sizeof(a)&&kh_decode(a,ra,&sa),vb=nb==sizeof(b)&&kh_decode(b,rb,&sb);
 if(!va&&!vb)return 0;
 if(vb&&(!va||(int32_t)(sb-sa)>0)){memcpy(raw,rb,KHS_RAW);*sequence=sb;return 2;}
 memcpy(raw,ra,KHS_RAW);*sequence=sa;return 1;
}
static inline int keen_scores_write(const char*path,const uint8_t*raw){
 if(!keen_scores_valid(raw,KHS_RAW))return 0;
 uint8_t previous[KHS_RAW],record[KHS_RECORD]={0};uint32_t sequence=0;
 int active=keen_scores_read(path,previous,&sequence);sequence++;
 memcpy(record,"KHP1",4);kh_put(record+4,sequence);kh_put(record+8,KHS_RAW);kh_put(record+12,1);
 memcpy(record+16,raw,KHS_RAW);kh_put(record+252,kh_hash(record));
 FILE*f=fopen(path,"r+b");if(!f)f=fopen(path,"wb");if(!f)return 0;
 unsigned offset=KHS_OFFSET+(active==1?KHS_RECORD:0);
 int ok=!fseek(f,(long)offset,SEEK_SET)&&fwrite(record,1,sizeof(record),f)==sizeof(record);
 if(fclose(f))ok=0;
 if(!ok)return 0;
 uint32_t verify=0;return keen_scores_read(path,previous,&verify)&&verify==sequence&&!memcmp(previous,raw,KHS_RAW);
}
#endif
