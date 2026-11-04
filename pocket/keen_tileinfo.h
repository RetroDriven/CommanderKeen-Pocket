/* SPDX-License-Identifier: MIT
 * Vorticons TILEINFO tables: six planes of signed little-endian 16-bit properties. Steam
 * v1.31 uses 1 for non-animated tiles; 0 is also accepted. */
#ifndef RETRODRIVEN_KEEN_TILEINFO_H
#define RETRODRIVEN_KEEN_TILEINFO_H
#include <stdint.h>
#include <stddef.h>
static unsigned keen_tileinfo_count(unsigned ep){
    return ep==0u?611u:ep==1u?689u:ep==2u?715u:0u;
}
static int16_t keen_tileinfo_word(const uint8_t *p){
    return (int16_t)(uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));
}
static int keen_tileinfo_valid(const uint8_t *raw,size_t size,unsigned ep){
    unsigned n=keen_tileinfo_count(ep);
    if(!raw||!n||size!=n*12u)return 0;
    unsigned anim=0,beh=0,faces=0,solids=0,partial=0,items=0;
    for(unsigned i=0;i<n;i++){
        int a=keen_tileinfo_word(raw+i*2u);
        int b=keen_tileinfo_word(raw+(n+i)*2u);
        if(a==0||a==1||a==2||a==4)anim++;
        if(b>=-2&&b<=28)beh++;
        if(b>=6&&b<=21)items++;
        unsigned s=0;
        for(unsigned face=2;face<6;face++){
            int v=keen_tileinfo_word(raw+(face*n+i)*2u);
            if(v>=0 && v<=4)faces++;
            if(v>0)s++;
        }
        if(s==4)solids++;
        if(s>0&&s<4)partial++;
    }
    return anim>=n*95u/100u && beh>=n*98u/100u &&
        faces>=n*4u*99u/100u && solids>n/40u &&
        (partial>n/100u||items>n/100u) && items>=2u;
}
#endif
