/* SPDX-License-Identifier: MIT
 * Keen 1-3 EGA tile decoding. Keen 1 uses MSB-first LZW with 9-12 bit codes (256 error,
 * 257 end). Keen 2/3 store uncompressed planar data. */
#ifndef RETRODRIVEN_KEEN_EGA_H
#define RETRODRIVEN_KEEN_EGA_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define KEEN_EGA_MAX (256u * 1024u)

typedef struct {
    uint32_t plane_size, tile_offset;
    uint16_t tiles, compression;
} KeenEgaInfo;
static uint16_t keen_le16(const uint8_t *p) {return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t keen_le32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static int keen_parse_egahead(const uint8_t *head, size_t n, KeenEgaInfo *info){
    if(n<48 || !info)return -1;
    info->plane_size=keen_le32(head);
    info->tiles=keen_le16(head+28);
    info->tile_offset=keen_le32(head+30);
    info->compression=keen_le16(head+46);
    if(!info->plane_size || info->plane_size > KEEN_EGA_MAX/4 ||
       !info->tiles || info->tiles>4096 ||
       info->tile_offset>info->plane_size ||
       (uint64_t)info->tiles * 32 > info->plane_size-info->tile_offset ||
       (info->compression&~3u))return -1;
    return 0;
}
/* The six-byte header stores expanded length and maximum code width. The caller validates
 * it; decoding starts at offset 6. */
typedef struct {const uint8_t *data; size_t n, pos; unsigned remaining; uint32_t scratch;} KeenBits;
static int keen_read_bits(KeenBits *b, unsigned width){
    if(width<9||width>12)return -1;
    while(b->remaining < width){
        if(b->pos==b->n)return -1;
        b->scratch=(b->scratch<<8)|b->data[b->pos++];
        b->remaining +=8;
    }
    b->remaining-=width;
    int value=(int)((b->scratch>>b->remaining)&((1u<<width)-1));
    b->scratch &= (1u<<b->remaining)-1;
    return value;
}
static int keen_lzw_decode(const uint8_t *src, size_t n, uint8_t *dst, size_t capacity, size_t *wrote){
    static uint16_t prefix[4096];
    static uint8_t suffix[4096], stack[4096];
    if(!src||n<8||!dst||!wrote)return -1;
    *wrote=0;
    uint32_t expected=keen_le32(src);
    uint16_t maxbits=keen_le16(src+4);
    if(maxbits<9||maxbits>12 || !expected || expected>capacity)return -1;
    KeenBits bits={src+6,n-6,0,0,0};
    unsigned width=9, next=258;
    for(unsigned i=0;i<256;i++){prefix[i]=0xFFFFu;suffix[i]=(uint8_t)i;}
    int old=keen_read_bits(&bits,width);
    if(old<0||old>=256)return -1;
    dst[(*wrote)++]=(uint8_t)old;
    int ended=0;
    while(1){
        int code=keen_read_bits(&bits,width);
        if(code<0 || code==256)return -1;
        if(code==257){ended=1;break;}
        if(code>(int)next || code>=4096)return -1;
        unsigned sp=0;
        unsigned walk=(unsigned)code;
        int special=(walk==next);
        if(special)walk=(unsigned)old;
        while(walk>=256){
            if(walk>=next || sp==4096)return -1;
            stack[sp++]=suffix[walk];
            walk=prefix[walk];
        }
        if(sp==4096)return -1;
        uint8_t first=(uint8_t)walk;
        stack[sp++]=first;
        if(*wrote+sp+(unsigned)special>expected)return -1;
        while(sp)dst[(*wrote)++]=stack[--sp];
        if(special)dst[(*wrote)++]=first;
        if(next<4096){
            prefix[next]=(uint16_t)old;
            suffix[next]=first;
            next++;
            /* Original Keen bit-width transition uses this early boundary. */
            if(width<maxbits && next==((1u<<width)-1u))width++;
        }
        old=code;
    }
    return (ended && *wrote==expected)?0:-1;
}
static uint8_t keen_tile_pixel(const uint8_t *latch,size_t len,const KeenEgaInfo *info,
                               unsigned tile,unsigned x,unsigned y){
    if(!latch||!info||tile>=info->tiles||x>=16||y>=16)return 0;
    size_t byte_offset=(size_t)info->tile_offset+(size_t)tile*32u+y*2u+x/8u;
    if(byte_offset>=info->plane_size || len < (size_t)info->plane_size*4u)return 0;
    uint8_t mask=(uint8_t)(0x80u>>(x&7u));
    uint8_t color=0;
    for(unsigned p=0;p<4;p++)if(latch[(size_t)p*info->plane_size+byte_offset]&mask)color|=(uint8_t)(1u<<p);
    return color;
}
#endif
