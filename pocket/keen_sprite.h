/* SPDX-License-Identifier: MIT
 * Vorticons EGAHEAD/EGASPRIT masked-sprite decoder. */
#ifndef RETRODRIVEN_KEEN_SPRITE_H
#define RETRODRIVEN_KEEN_SPRITE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define KEEN_SPR_MAX 256u
#define KEEN_SPR_MAX_BYTES (256u*1024u)
#define KEEN_SPR_MAX_W 128u
#define KEEN_SPR_MAX_H 128u

typedef struct {
    uint32_t plane_size, table_offset;
    uint16_t sprite_count, compression;
} KeenSpriteInfo;
typedef struct {
    unsigned w, h, start;
    char name[13];
} KeenSprite;
/* Validate the sprite table before reading frame metadata. */
static int keen_sprite_parse_head(const uint8_t *head,size_t len,KeenSpriteInfo *info){
    if(!head||!info||len<48)return -1;
    info->plane_size=keen_le32(head+4);
    info->table_offset=keen_le32(head+12);
    info->sprite_count=keen_le16(head+40);
    info->compression=keen_le16(head+46);
    if(!info->plane_size || info->plane_size>KEEN_SPR_MAX_BYTES/5 ||
       !info->sprite_count || info->sprite_count>KEEN_SPR_MAX ||
       info->compression>3 ||
       info->table_offset<48 || info->table_offset>len ||
       (size_t)info->sprite_count*128u>len-info->table_offset)return -1;
    return 0;
}
static int keen_sprite_get(const uint8_t *head,size_t len,const KeenSpriteInfo *info,
                           unsigned idx,KeenSprite *out){
    if(!head||!info||!out||idx>=info->sprite_count ||
       info->table_offset>len || (size_t)(idx+1u)*128u>len-info->table_offset)return -1;
    const uint8_t *e=head+info->table_offset+(size_t)idx*128u;
    unsigned wb=keen_le16(e), h=keen_le16(e+2);
    unsigned start=(unsigned)keen_le16(e+6)*16u+(unsigned)keen_le16(e+4);
    if(!wb||wb>KEEN_SPR_MAX_W/8u || !h||h>KEEN_SPR_MAX_H ||
       start>info->plane_size || (size_t)wb*h>info->plane_size-start)return -1;
    out->w=wb*8u;out->h=h;out->start=start;
    memcpy(out->name,e+16,12);out->name[12]=0;
    /* Replace nonprintable frame-name bytes with spaces. */
    for(unsigned i=0;i<12 && out->name[i];i++){
        unsigned char c=(unsigned char)out->name[i];
        if(c<32 || c>126)out->name[i]='?';
    }
    return 0;
}
/* Indexed result: 0..15 for opaque pixel, -1 for transparent or invalid.
 * Original plane order: blue, green, red, intensity, transparency mask.
 * A 1 in the fifth plane is transparent. */
static int keen_sprite_pixel(const uint8_t *data,size_t len,const KeenSpriteInfo *info,
                            const KeenSprite *s,unsigned x,unsigned y){
    if(!data||!info||!s||x>=s->w||y>=s->h||
       len<(size_t)info->plane_size*5u)return -1;
    unsigned off=s->start+y*(s->w/8u)+x/8u;
    if(off>=info->plane_size)return -1;
    unsigned mask=0x80u>>(x&7u);
    if(data[(size_t)4*info->plane_size+off]&mask)return -1;
    int c=0;
    for(unsigned plane=0;plane<4;plane++){
        if(data[(size_t)plane*info->plane_size+off]&mask)c|=(1u<<plane);
    }
    return c;
}
#endif
