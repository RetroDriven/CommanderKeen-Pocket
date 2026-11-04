/* Pocket campaign saves: explicit little-endian format and two-record journal.
 * Never reads or writes original DOS save formats. SPDX-License-Identifier: MIT */
#ifndef KEEN_SAVE_H
#define KEEN_SAVE_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define KSV_BYTES 64u
typedef struct {uint32_t sequence,score,extra,lives,ammo,parts,pogo,levels,x,y,deaths;} KeenSave;
static inline uint32_t ksv_word(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static inline void ksv_put(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static inline uint32_t ksv_hash(const uint8_t*p){uint32_t h=2166136261u;for(unsigned i=0;i<60;i++)h=(h^p[i])*16777619u;return h;}
static inline void ksv_encode(uint8_t*p,const KeenSave*v){
 memset(p,0,KSV_BYTES);memcpy(p,"RDKS01\0",8);
 const uint32_t a[]={v->sequence,v->score,v->extra,v->lives,v->ammo,v->parts,v->pogo,v->levels,v->x,v->y,v->deaths};
 for(unsigned i=0;i<11;i++)ksv_put(p+8+i*4,a[i]);
 ksv_put(p+60,ksv_hash(p));
}
static inline int ksv_decode(const uint8_t*p,KeenSave*v){
 if(memcmp(p,"RDKS01\0",8)||ksv_word(p+60)!=ksv_hash(p))return 0;
 KeenSave t={ksv_word(p+8),ksv_word(p+12),ksv_word(p+16),ksv_word(p+20),ksv_word(p+24),ksv_word(p+28),
  ksv_word(p+32),ksv_word(p+36),ksv_word(p+40),ksv_word(p+44),ksv_word(p+48)};
 if(t.score>0x7fffffffu||t.extra>t.score||t.extra%20000u||t.lives>65535||t.ammo>65535||t.parts>15||t.pogo>1||t.levels>65535||t.x>8192||t.y>8192)return 0;
 *v=t;return 1;
}
/* Positive result is the active record index + 1. Invalid loads leave output untouched. */
static inline int keen_save_read(const char*path,KeenSave*v){
 FILE*f=fopen(path,"rb");if(!f)return 0;
 uint8_t raw[KSV_BYTES*2];size_t n=fread(raw,1,sizeof(raw),f);fclose(f);
 KeenSave a,b;int va=n>=64&&ksv_decode(raw,&a),vb=n>=128&&ksv_decode(raw+64,&b);
 if(!va&&!vb)return 0;
 if(vb&&(!va||(int32_t)(b.sequence-a.sequence)>0)){*v=b;return 2;}
 *v=a;return 1;
}
static inline int keen_save_write(const char*path,const KeenSave*v){
 KeenSave prev;int which=keen_save_read(path,&prev);KeenSave next=*v;
 next.sequence=which?prev.sequence+1u:1u;uint8_t raw[KSV_BYTES];ksv_encode(raw,&next);
 KeenSave valid;if(!ksv_decode(raw,&valid))return 0;
 FILE*f=fopen(path,"r+b");if(!f)f=fopen(path,"wb");if(!f)return 0;
 unsigned off=which==1?64u:0u;int ok=fseek(f,(long)off,SEEK_SET)==0&&fwrite(raw,1,sizeof(raw),f)==sizeof(raw);
 if(fclose(f)!=0)ok=0;
 if(!ok)return 0;
 KeenSave verify;return keen_save_read(path,&verify)>0 && verify.sequence==next.sequence;
}
#endif
