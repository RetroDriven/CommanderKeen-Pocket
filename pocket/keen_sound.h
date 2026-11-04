/* SPDX-License-Identifier: MIT
 * Vorticons PC-speaker decoder for SOUNDS.CK1/2/3. Sound indices are zero-based: jump 5,
 * landing 6. */
#ifndef RETRODRIVEN_KEEN_SOUND_H
#define RETRODRIVEN_KEEN_SOUND_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define KS_MAX_FILE 65536u
#define KS_MAX_SOUNDS 128u
#define KS_RATE 24000u
#define KS_PIT_HZ 1193182u
#define KS_TICK_DIVISOR 8192u /* Chocolate Keen engine_timing.c: pitAltCounter */
#define KS_MAX_TICKS ((KS_PIT_HZ*8u+KS_TICK_DIVISOR-1u)/KS_TICK_DIVISOR) /* ~8 seconds */
#define KS_PCM_CAP ((size_t)(((uint64_t)KS_RATE*KS_MAX_TICKS*KS_TICK_DIVISOR + KS_PIT_HZ-1u)/KS_PIT_HZ))
static inline size_t ks_samples_at_tick(unsigned tick){
    return (size_t)(((uint64_t)tick*KS_RATE*KS_TICK_DIVISOR)/KS_PIT_HZ);
}
/* One voice: equal priority replaces the current effect; lower priority is dropped. */
static inline int ks_priority_allows(int active,int current,unsigned requested){
    return !active || (int)requested>=current;
}

typedef struct {
    uint16_t offset;
    uint8_t priority, rate;
    char name[13];
} KeenSoundEntry;
typedef struct {
    unsigned count;
    size_t file_size;
    KeenSoundEntry entries[KS_MAX_SOUNDS];
} KeenSoundBank;
static uint16_t ks_le16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
/* Sound count is at +6; +8 holds a constant (usually 0x0032). Zero-offset entries are
 * unused. */
static int ks_parse(const uint8_t *data,size_t len,KeenSoundBank *out){
    if(!data||!out||len<32u||len>KS_MAX_FILE || memcmp(data,"SND\0",4))return -1;
    unsigned count=ks_le16(data+6);
    if(count<1u||count>KS_MAX_SOUNDS || 16u+count*16u>len)return -1;
    unsigned valid=0;
    for(unsigned i=0;i<count;i++){
        unsigned off=ks_le16(data+16u+i*16u);
        if(off>=16u+count*16u && off+1u<len)valid++;
    }
    if(valid*2u<count)return -1;
    memset(out,0,sizeof(*out));
    out->count=count;out->file_size=len;
    for(unsigned i=0;i<count;i++){
        const uint8_t *item=data+16u+i*16u;
        KeenSoundEntry *e=&out->entries[i];
        e->offset=ks_le16(item);e->priority=item[2];e->rate=item[3];
        for(unsigned j=0;j<12u;j++){
            unsigned c=item[4u+j];
            if(!c)break;
            if(c>='a'&&c<='z')c-=32u;
            e->name[j]=(char)((c>=32u && c<=126u)?c:'_');
        }
        e->name[12]=0;
    }
    return 0;
}
/* Resolve by label to allow per-episode bank ordering. KEENJUMPSND is normally index 5;
 * KEENLANDSND is index 6. */
static int ks_find_by_name(const KeenSoundBank *bank,const char *name){
    if(!bank || !name || !*name)return -1;
    for(unsigned i=0;i<bank->count;i++)
        if(strcmp(bank->entries[i].name,name)==0)return (int)i;
    return -1;
}
/* Distinguish intentionally silent/unpopulated effects from bad data. */
typedef enum { KS_BAD=-1, KS_UNUSED=0, KS_SILENT=1, KS_AUDIBLE=2,
               KS_TRUNCATED=3 } KeenSoundStatus;
static KeenSoundStatus ks_status(const uint8_t *data,size_t len,
                                 const KeenSoundBank *bank,unsigned id){
    if(!data||!bank||id>=bank->count)return KS_BAD;
    size_t pos=bank->entries[id].offset;
    if(!pos)return KS_UNUSED;
    if(pos<16u+bank->count*16u || pos+1u>=len)return KS_BAD;
    unsigned audible=0;
    for(unsigned t=0;t<KS_MAX_TICKS && pos+1u<len;t++,pos+=2u){
        unsigned divisor=ks_le16(data+pos);
        if(divisor==0xffffu)return audible?KS_AUDIBLE:KS_SILENT;
        if(divisor)audible=1;
    }
    return KS_TRUNCATED;
}
/* Generate mono signed-16 square waves. PIT/8192 timing uses rational sample boundaries to
 * avoid drift. */
static size_t ks_render(const uint8_t *data,size_t len,const KeenSoundBank *bank,
                        unsigned sound_id,int16_t *pcm,size_t cap,int *truncated){
    if(truncated)*truncated=0;
    if(!data||!bank||!pcm||sound_id>=bank->count)return 0;
    const KeenSoundEntry *entry=&bank->entries[sound_id];
    size_t start=entry->offset;
    if(start<16u+bank->count*16u || start+1u>=len)return 0;
    /* FFFF terminates an effect. Shared or overlapping data makes adjacent offsets
     * unsuitable as end markers. */
    size_t total=0;
    for(unsigned t=0;t<KS_MAX_TICKS && start+2u<=len;t++,start+=2u){
        unsigned divisor=ks_le16(data+start);
        if(divisor==0xffffu)return total;
        size_t next=ks_samples_at_tick(t+1u);
        if(next>cap){if(truncated)*truncated=1;return total;}
        /* Each divisor write restarts the PIT counter at positive polarity, as in
         * Chocolate Keen. */
        uint32_t phase=0;
        uint32_t step=divisor? (uint32_t)(((uint64_t)KS_PIT_HZ<<32)/
                       ((uint64_t)divisor*KS_RATE)):0u;
        while(total<next){
            if(!divisor)pcm[total++]=0;
            else{
                pcm[total++]=(phase&0x80000000u)?-10500:10500;
                phase+=step;
            }
        }
    }
    if(truncated)*truncated=1;
    return total;
}
#endif
