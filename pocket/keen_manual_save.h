/* Versioned manual-save envelope. Two 128 KiB records fit a Pocket save slot.
 * Commit the inactive header last; older world-only RDKS01 saves remain readable.
 * SPDX-License-Identifier: MIT */
#ifndef KEEN_MANUAL_SAVE_H
#define KEEN_MANUAL_SAVE_H
#include "keen_save.h"
#define KMS_RECORD (128u*1024u)
#define KMS_HEADER 64u
#define KMS_PAYLOAD (KMS_RECORD-KMS_HEADER)
typedef struct {uint32_t sequence,kind,length,abi;unsigned record;} KeenManualRecord;
static inline uint32_t kms_hash_more(uint32_t hash,const uint8_t*p,size_t n){
 for(size_t i=0;i<n;i++)hash=(hash^p[i])*16777619u;
 return hash;
}
static inline uint32_t kms_hash(const uint8_t*p,size_t n){return kms_hash_more(2166136261u,p,n);}
static inline int kms_scan(FILE*f,unsigned record,uint32_t abi,KeenManualRecord*out){
 uint8_t header[KMS_HEADER],chunk[1024];
 if(fseek(f,(long)(record*KMS_RECORD),SEEK_SET)||fread(header,1,sizeof(header),f)!=sizeof(header)||
    memcmp(header,"RDKM01\0",8)||ksv_word(header+60)!=kms_hash(header,60))return 0;
 KeenManualRecord r={ksv_word(header+8),ksv_word(header+12),ksv_word(header+16),ksv_word(header+20),record};
 if(r.kind>1||!r.length||r.length>KMS_PAYLOAD||(r.kind?r.abi!=abi:r.abi!=0))return 0;
 uint32_t hash=2166136261u,left=r.length;
 while(left){size_t n=left>sizeof(chunk)?sizeof(chunk):left;
  if(fread(chunk,1,n,f)!=n)return 0;
  hash=kms_hash_more(hash,chunk,n);left-=(uint32_t)n;
 }
 if(hash!=ksv_word(header+24))return 0;
 *out=r;return 1;
}
static inline int keen_manual_read(const char*path,uint8_t*payload,size_t capacity,
 uint32_t abi,KeenManualRecord*out){
 FILE*f=fopen(path,"rb");if(!f)return 0;
 KeenManualRecord a,b,r;int va=kms_scan(f,0,abi,&a),vb=kms_scan(f,1,abi,&b);
 if(!va&&!vb){fclose(f);return 0;}
 r=vb&&(!va||(int32_t)(b.sequence-a.sequence)>0)?b:a;
 int ok=r.length<=capacity&&!fseek(f,(long)(r.record*KMS_RECORD+KMS_HEADER),SEEK_SET)&&
        fread(payload,1,r.length,f)==r.length;
 fclose(f);if(ok)*out=r;return ok;
}
static inline int keen_manual_write(const char*path,const uint8_t*payload,size_t length,
 uint32_t kind,uint32_t abi){
 if(!payload||!length||length>KMS_PAYLOAD||kind>1)return 0;
 FILE*f=fopen(path,"r+b");if(!f)f=fopen(path,"w+b");if(!f)return 0;
 KeenManualRecord a,b;int va=kms_scan(f,0,abi,&a),vb=kms_scan(f,1,abi,&b);
 unsigned record=0;uint32_t sequence=1;
 if(va||vb){KeenManualRecord prev=vb&&(!va||(int32_t)(b.sequence-a.sequence)>0)?b:a;
  record=prev.record^1u;sequence=prev.sequence+1u;
 }else{
  /* Preserve a legacy map checkpoint until the new record is complete. */
  uint8_t raw[128];KeenSave old;
  if(!fseek(f,0,SEEK_SET)){
   size_t n=fread(raw,1,sizeof(raw),f);
   int oa=n>=64&&ksv_decode(raw,&old);uint32_t seq=oa?old.sequence:0;
   if(n>=128&&ksv_decode(raw+64,&old)&&(!oa||(int32_t)(old.sequence-seq)>0)){seq=old.sequence;oa=1;}
   if(oa){record=1;sequence=seq+1u;}
  }
 }
 clearerr(f);uint8_t header[KMS_HEADER]={0};memcpy(header,"RDKM01\0",8);
 ksv_put(header+8,sequence);ksv_put(header+12,kind);ksv_put(header+16,(uint32_t)length);
 ksv_put(header+20,kind?abi:0);ksv_put(header+24,kms_hash(payload,length));ksv_put(header+60,kms_hash(header,60));
 int ok=!fseek(f,(long)(record*KMS_RECORD+KMS_HEADER),SEEK_SET)&&fwrite(payload,1,length,f)==length&&
        !fflush(f)&&!fseek(f,(long)(record*KMS_RECORD),SEEK_SET)&&fwrite(header,1,sizeof(header),f)==sizeof(header);
 if(fclose(f))ok=0;
 if(!ok)return 0;
 f=fopen(path,"rb");if(!f)return 0;
 KeenManualRecord verify;ok=kms_scan(f,record,abi,&verify)&&verify.sequence==sequence;fclose(f);return ok;
}
#endif
