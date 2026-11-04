/* SPDX-License-Identifier: MIT
 * Galaxy resource formats: Omnispeak id_ca.c / id_vh.h, Apogee Keen 4 1.4.
 * Byte-oriented decoding keeps DOS alignment and endianness independent.
 */
#ifndef RETRODRIVEN_KEEN_GALAXY_H
#define RETRODRIVEN_KEEN_GALAXY_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define KG_MAP_CELLS 16384u
#define KG_CHUNK_CAP 65536u
#define KG_BG_COUNT 1296u
#define KG_FG_COUNT 2916u
#define KG_BG_START 523u
#define KG_FG_START 1819u
#define KG_SPR_START 124u
#define KG_SPR_COUNT 397u
static uint16_t kg_u16(const uint8_t*p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t kg_u24(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16);}
static uint32_t kg_u32(const uint8_t*p){return kg_u16(p)|((uint32_t)kg_u16(p+2)<<16);}
/* LSB-first Huffman bits, root node 254, leaves 0..255. */
static int kg_huffman(const uint8_t*src,size_t len,const uint8_t*dict,size_t dict_len,uint8_t*out,size_t need){
    if(!src||!dict||!out||dict_len<1020)return -1;
    size_t pos=0;unsigned bit=0,node=254;
    for(size_t i=0;i<need;){
        if(pos>=len||node>254)return -1;
        unsigned v=kg_u16(dict+node*4+((src[pos]>>bit)&1u)*2u);
        if(++bit==8){bit=0;pos++;}
        if(v<256){out[i++]=(uint8_t)v;node=254;}else node=v-256;
    }
    return 0;
}
/* Carmack references address already-produced words, including overlap. */
static int kg_carmack(const uint8_t*src,size_t len,uint8_t*out,size_t need){
    if(!src||!out||(need&1u))return -1;
    size_t si=0,di=0;
    while(di<need){
        if(len-si<2)return -1;
        uint16_t word=kg_u16(src+si);si+=2;unsigned tag=word>>8,count=word&255u;
        if(tag!=0xa7&&tag!=0xa8){out[di++]=(uint8_t)word;out[di++]=(uint8_t)tag;continue;}
        if(!count){if(si>=len)return -1;out[di++]=src[si++];out[di++]=(uint8_t)tag;continue;}
        size_t from;
        if(tag==0xa7){if(si>=len)return -1;size_t back=(size_t)src[si++]*2u;if(!back||back>di)return -1;from=di-back;}
        else{if(len-si<2)return -1;from=(size_t)kg_u16(src+si)*2u;si+=2;if(from>=di)return -1;}
        if((size_t)count*2>need-di)return -1;
        for(unsigned n=0;n<count*2u;n++){out[di++]=out[from++];}
    }
    return 0;
}
static int kg_rlew(const uint8_t*src,size_t len,uint16_t tag,uint16_t*out,size_t cells){
    if(!src||!out)return -1;
    size_t si=0,di=0;
    while(di<cells){
        if(len-si<2)return -1;
        uint16_t v=kg_u16(src+si);si+=2;
        if(v!=tag){out[di++]=v;continue;}
        if(len-si<4)return -1;
        unsigned n=kg_u16(src+si);v=kg_u16(src+si+2);si+=4;
        if(!n||n>cells-di)return -1;
        while(n--)out[di++]=v;
    }
    return si==len?0:-1;
}
typedef struct{uint16_t width,height;char name[17];uint16_t planes[3][KG_MAP_CELLS];} KeenGalaxyMap;
static int kg_map_decode(const uint8_t*maps,size_t maps_len,const uint8_t*head,size_t head_len,unsigned id,KeenGalaxyMap*out,uint8_t*scratch,size_t cap){
    if(!maps||!head||!out||!scratch||head_len<6||id>(head_len-6)/4)return -1;
    uint32_t off=kg_u32(head+2+id*4u);
    if(off>maps_len||maps_len-off<38)return -1;
    const uint8_t*h=maps+off;
    unsigned width=kg_u16(h+18),height=kg_u16(h+20);size_t cells=(size_t)width*height;
    if(!width||!height||width>512||height>512||cells>KG_MAP_CELLS)return -1;
    for(unsigned p=0;p<3;p++){
        uint32_t at=kg_u32(h+p*4);size_t len=kg_u16(h+12+p*2);
        if(at>maps_len||len<2||len>maps_len-at)return -1;
        size_t expanded=kg_u16(maps+at);
        if(expanded<2||expanded>cap||(expanded&1u)||kg_carmack(maps+at+2,len-2,scratch,expanded))return -1;
        if(kg_u16(scratch)!=cells*2u||kg_rlew(scratch+2,expanded-2,kg_u16(head),out->planes[p],cells))return -1;
    }
    out->width=(uint16_t)width;out->height=(uint16_t)height;memcpy(out->name,h+22,16);out->name[16]=0;return 0;
}
typedef struct{const uint8_t*graph,*head,*dict;size_t graph_len,head_len,dict_len;} KeenGalaxyGraphics;
static int kg_graph_chunk(const KeenGalaxyGraphics*g,unsigned id,uint8_t*out,size_t cap,size_t*size){
    if(!g||!out||!size||!g->graph||!g->head||!g->dict||g->head_len<6||id>(g->head_len-6)/3)return -1;
    uint32_t start=kg_u24(g->head+id*3u),end=0xffffffu;size_t next=id+1u;
    if(start==0xffffffu)return -1;
    while(next<g->head_len/3){end=kg_u24(g->head+next*3u);next++;if(end!=0xffffffu)break;}
    if(end==0xffffffu)end=(uint32_t)g->graph_len;
    if(start>=end||end>g->graph_len)return -1;
    size_t need;const uint8_t*src=g->graph+start;size_t len=end-start;
    if(id>=KG_BG_START&&id<KG_FG_START)need=128;
    else if(id>=KG_FG_START&&id<4735u)need=160;
    else if(id==521u)need=104u*32u;
    else if(id==522u)need=20u*40u;
    else{if(len<4)return -1;need=kg_u32(src);src+=4;len-=4;}
    if(!need||need>cap||kg_huffman(src,len,g->dict,g->dict_len,out,need))return -1;
    *size=need;return 0;
}
/* Masked graphics have mask first, then B/G/R/intensity planes. */
static unsigned kg_pixel(const uint8_t*data,unsigned width_bytes,unsigned height,unsigned x,unsigned y,int masked){
    if(!data||x>=width_bytes*8u||y>=height)return 255;
    size_t plane=(size_t)width_bytes*height,at=(size_t)y*width_bytes+x/8u;unsigned bit=0x80u>>(x&7u),c=0;
    if(masked){if(data[at]&bit)return 255;data+=plane;}
    for(unsigned p=0;p<4;p++)if(data[at+plane*p]&bit)c|=1u<<p;
    return c;
}
static void kg_blit(uint8_t*fb,int stride,int screen_h,int x,int y,const uint8_t*data,unsigned width_bytes,unsigned height,int masked){
    for(unsigned py=0;py<height;py++)for(unsigned px=0;px<width_bytes*8u;px++){
        int dx=x+(int)px,dy=y+(int)py;
        if(dx<0||dx>=stride||dy<0||dy>=screen_h)continue;
        unsigned c=kg_pixel(data,width_bytes,height,px,py,masked);
        if(c!=255)fb[(size_t)dy*stride+dx]=(uint8_t)(9u+c);
    }
}
#endif
