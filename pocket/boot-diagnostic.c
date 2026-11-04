/* SPDX-License-Identifier: MIT
 * Pocket framebuffer and button-input diagnostic. */
#include "of.h"
#include <stdint.h>

#define WIDTH 320
#define HEIGHT 240

static void fill(uint8_t *fb, int x, int y, int w, int h, uint8_t c) {
    for (int yy = y; yy < y+h; ++yy)
        for (int xx = x; xx < x+w; ++xx)
            fb[yy*WIDTH + xx] = c;
}
static void letter(uint8_t *fb, int x, int y, char ch, uint8_t ink) {
    /* Glyphs for the messages BOOT OK / KEEN / 017; 5 pixels wide. */
    static const struct { char c; uint8_t b[7]; } font[] = {
        {'B',{30,17,17,30,17,17,30}}, {'O',{14,17,17,17,17,17,14}},
        {'T',{31,4,4,4,4,4,4}}, {'K',{17,18,20,24,20,18,17}},
        {'E',{31,16,16,30,16,16,31}}, {'N',{17,25,21,19,17,17,17}},
        {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
        {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,4,4,4}}, {'A',{14,17,17,31,17,17,17}},
        {'P',{30,17,17,30,16,16,16}}, {'R',{30,17,17,30,20,18,17}},
        {'S',{15,16,16,14,1,1,30}}, {' ',{0,0,0,0,0,0,0}}
    };
    for (unsigned int i = 0; i < sizeof(font)/sizeof(font[0]); ++i)
        if (font[i].c == ch) {
            for (int r = 0; r < 7; ++r)
                for (int col = 0; col < 5; ++col)
                    if (font[i].b[r] & (1 << (4-col)))
                        fill(fb,x+col*3,y+r*3,3,3,ink);
            return;
        }
}
static void message(uint8_t *fb, int y, const char *s, uint8_t ink) {
    int n=0; while(s[n]) ++n;
    int x=(WIDTH-n*18)/2;
    for (int i=0;i<n;i++,x+=18) letter(fb,x,y,s[i],ink);
}
int main(void) {
    static const uint32_t palette[] = {
        0x0A1427,0xFAFAFA,0xE83D52,0x2FC3E0,0xF5C34B,0x67CD81
    };
    int swapped=0;
    of_video_init();
    of_video_palette_bulk(palette,6);
    for (;;) {
        of_input_poll();
        if (of_btn_pressed(OF_BTN_A)) swapped ^= 1;
        uint8_t *fb=of_video_surface();
        /* Render the fixed diagnostic without allocating memory or reading files. */
        fill(fb,0,0,WIDTH,HEIGHT,0);
        fill(fb,0,0,WIDTH,12,swapped?3:2);
        fill(fb,0,HEIGHT-12,WIDTH,12,swapped?2:3);
        message(fb,47,"KEEN",4);
        message(fb,87,"BOOT OK",1);
        message(fb,128,"017",5);
        fill(fb,16,182,288,7,swapped?5:2);
        fill(fb,16,196,288,7,swapped?3:4);
        of_video_flip();
        /* Wait for frame presentation. */
        of_video_wait_flip();
    }
}
