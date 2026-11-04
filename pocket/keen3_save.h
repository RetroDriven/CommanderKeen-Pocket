/* SPDX-License-Identifier: MIT
 * Keen 3 checkpoints: 64-byte journal records in a separate save namespace. */
#ifndef KEEN3_SAVE_H
#define KEEN3_SAVE_H
#include "keen_save.h"
static inline void k3sv_encode(uint8_t*p,const KeenSave*v){
 KeenSave base=*v;base.parts&=15u;ksv_encode(p,&base);
 memcpy(p,"RDKS03\0",8);ksv_put(p+52,v->parts);ksv_put(p+56,3);ksv_put(p+60,ksv_hash(p));
}
static inline int k3sv_decode(const uint8_t*p,KeenSave*v){
 if(memcmp(p,"RDKS03\0",8)||ksv_word(p+60)!=ksv_hash(p)||ksv_word(p+52)!=0||ksv_word(p+56)!=3||
    ksv_word(p+28)!=(ksv_word(p+52)&15u))return 0;
 uint8_t base[64];memcpy(base,p,64);memcpy(base,"RDKS01\0",8);ksv_put(base+52,0);ksv_put(base+56,0);ksv_put(base+60,ksv_hash(base));
 KeenSave valid;if(!ksv_decode(base,&valid))return 0;valid.parts=ksv_word(p+52);*v=valid;return 1;
}
static inline int keen3_save_read(const char*path,KeenSave*v){
 FILE*f=fopen(path,"rb");if(!f)return 0;uint8_t raw[128];size_t n=fread(raw,1,sizeof(raw),f);fclose(f);
 KeenSave a,b;int va=n>=64&&k3sv_decode(raw,&a),vb=n>=128&&k3sv_decode(raw+64,&b);
 if(!va&&!vb)return 0;
 if(vb&&(!va||(int32_t)(b.sequence-a.sequence)>0)){*v=b;return 2;}*v=a;return 1;
}
static inline int keen3_save_write(const char*path,const KeenSave*v){
 KeenSave prev,valid,next=*v;int active=keen3_save_read(path,&prev);next.sequence=active?prev.sequence+1u:1;
 uint8_t raw[64];k3sv_encode(raw,&next);if(!k3sv_decode(raw,&valid))return 0;
 FILE*f=fopen(path,"r+b");if(!f)f=fopen(path,"w+b");if(!f)return 0;
 int ok=!fseek(f,active==1?64:0,SEEK_SET)&&fwrite(raw,1,64,f)==64;if(fclose(f))ok=0;
 return ok&&keen3_save_read(path,&valid)&&valid.sequence==next.sequence;
}
#endif
