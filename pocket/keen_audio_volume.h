/* Stereo gain at the output boundary; source samples remain unmodified. */
#ifndef KEEN_AUDIO_VOLUME_H
#define KEEN_AUDIO_VOLUME_H
#include "keen_core_options.h"
static inline int keen_core_stream_write(const int16_t*p,int pairs,int (*write_pairs)(const int16_t*,int)){
 unsigned gain=keen_core_audio_gain();if(gain==256)return write_pairs(p,pairs);
 static int16_t scaled[1024];if(pairs>512)pairs=512;
 for(int i=0;i<pairs*2;i++)scaled[i]=keen_core_scale_sample(p[i],gain);
 return write_pairs(scaled,pairs);
}
#endif
