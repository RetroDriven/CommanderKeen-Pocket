/*
 * RetroDriven Commander Keen 1-3 level file decoder (LEVELxx.CK1/2/3).
 * Format: LE32 expanded length, RLEW-compressed 32-byte header + two tile
 * planes. Supports the common 0xFEFE and 0xABCD RLEW markers, plus raw files.
 * SPDX-License-Identifier: MIT
 */
#ifndef RETRODRIVEN_KEEN_MAP_H
#define RETRODRIVEN_KEEN_MAP_H
#include <stdint.h>
#include <stddef.h>

#define KEEN_MAP_CAP (256u * 1024u)

typedef struct {
    uint16_t width, height, planes, plane_bytes;
    uint32_t decoded_bytes;
    uint16_t marker; /* zero for uncompressed */
} KeenMapInfo;

static uint16_t km_word(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t km_dword(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
/* Compressed input bytes begin immediately after the four-byte length. */
static int keen_map_rlew(const uint8_t *src,size_t len,uint8_t *dst,
                          size_t expected,uint16_t tag){
    if(!src||!dst||!tag||(expected&1u))return -1;
    size_t si=0,di=0;
    while(di<expected){
        if(si+2>len)return -1;
        uint16_t word=km_word(src+si);si+=2;
        if(word==tag){
            if(si+4>len)return -1;
            size_t count=km_word(src+si);
            uint16_t repeated=km_word(src+si+2);si+=4;
            if(count>(expected-di)/2u)return -1;
            for(size_t n=0;n<count;n++){
                dst[di++]=(uint8_t)repeated;
                dst[di++]=(uint8_t)(repeated>>8);
            }
        }else{
            if(di+2>expected)return -1;
            dst[di++]=(uint8_t)word;
            dst[di++]=(uint8_t)(word>>8);
        }
    }
    return 0;
}
static int keen_map_header(const uint8_t *raw,size_t decoded_len,KeenMapInfo *map){
    if(!raw||!map||decoded_len<36)return -1;
    unsigned width=km_word(raw),height=km_word(raw+2);
    unsigned planes=km_word(raw+4),plane_bytes=km_word(raw+14);
    uint64_t footprint=(uint64_t)width*height*2u;
    if(!width||!height||width>2048||height>2048||planes!=2 ||
       !plane_bytes || (plane_bytes&15u) ||
       footprint>plane_bytes || plane_bytes-footprint>=16u ||
       32u+(size_t)plane_bytes*2u>decoded_len)return -1;
    map->width=(uint16_t)width;
    map->height=(uint16_t)height;
    map->planes=(uint16_t)planes;
    map->plane_bytes=(uint16_t)plane_bytes;
    map->decoded_bytes=(uint32_t)decoded_len;
    return 0;
}
/* Try supported encodings and validate the map header. Each output buffer needs
 * KEEN_MAP_CAP bytes. Returns 0 on success. */
static int keen_map_decode(const uint8_t *src,size_t len,uint8_t *dst,
                            size_t capacity,KeenMapInfo *info){
    if(!src||!dst||!info||len<32)return -1;
    if(len>=4){
        size_t expected=km_dword(src);
        if(expected>=36 && expected<=capacity && !(expected&1u)){
            static const uint16_t tags[]={0xFEFE,0xABCD};
            for(unsigned t=0;t<sizeof(tags)/sizeof(tags[0]);t++){
                if(!keen_map_rlew(src+4,len-4,dst,expected,tags[t]) &&
                   !keen_map_header(dst,expected,info)){
                    info->marker=tags[t];return 0;
                }
            }
        }
    }
    if(len<=capacity && !keen_map_header(src,len,info)){
        for(size_t i=0;i<len;i++)dst[i]=src[i];
        info->marker=0;
        return 0;
    }
    return -1;
}
static uint16_t keen_map_tile(const uint8_t *raw,const KeenMapInfo *info,
                               unsigned x,unsigned y,unsigned plane){
    if(!raw||!info||x>=info->width||y>=info->height||plane>=2)return 0xffffu;
    size_t off=32u+(size_t)plane*info->plane_bytes+
               ((size_t)y*info->width+x)*2u;
    if(off+2u>info->decoded_bytes)return 0xffffu;
    return km_word(raw+off);
}
#endif
