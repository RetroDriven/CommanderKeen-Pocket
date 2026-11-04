/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rd_platform.h"
#include "../keen_view.h"
#include "id_ca.h"
#include "id_rf.h"
#include "id_sd.h"
#include "id_vl.h"
unsigned rd_galaxy_options(void);
extern int rd_in_play,rd_in_menu,rd_in_retry;
int rd_galaxy_scene_active;
extern int rf_dirtyBufferOffset,rf_freeSpriteEraserIndex[];
void rd_galaxy_camera_resized(int);
static unsigned view_height=240;
unsigned rd_galaxy_buffer_rows=16;
unsigned rd_galaxy_view_height(void){return view_height;}
void rd_galaxy_view_begin(void){
 view_height=(rd_galaxy_options()&0x00100000u)?240:200;
 rd_galaxy_buffer_rows=view_height==240?16:14;
}
void rd_galaxy_view_update(void){
 unsigned desired=(rd_galaxy_options()&0x00100000u)?240:200;
 if(desired==view_height||!rd_in_play||rd_in_menu)return;
 int old=(int)view_height,x=rf_scrollXUnit,y=rf_scrollYUnit;
 rd_galaxy_view_begin();
 rf_scrollYMaxUnit=((int)CA_GetMapHeight()-(int)RF_SCREEN_HEIGHT_TILES-2)*256;
 if(rf_scrollYMaxUnit<rf_scrollYMinUnit)rf_scrollYMaxUnit=rf_scrollYMinUnit;
 rf_dirtyBufferOffset=0;
 for(int page=0;page<VL_GetNumBuffers();page++)rf_freeSpriteEraserIndex[page]=0;
 RF_Reposition(x,y+(old-(int)view_height)/2*16);
 rd_galaxy_camera_resized(old);
 SD_SetLastTimeCount(SD_GetTimeCount());
}
void rd_galaxy_compose_frame(uint8_t*fb,const uint8_t*p,int stride,int height,int x,int y){
 int enhanced=!!(rd_galaxy_options()&0x00100000u);
 int scene=(rd_galaxy_scene_active||rd_in_retry)&&height>=240;
 unsigned rows=scene?view_height:200,offset=(240-rows)/2;
 memset(fb,0,320*240);
 for(unsigned sy=0;sy<rows;sy++)if(y+(int)sy>=0&&y+(int)sy<height&&x>=0&&x+320<=stride)
  memcpy(fb+(sy+offset)*320,p+(y+sy)*stride+x,320);
 if(!scene)keen_view_flat_margins(fb,enhanced,0);
}
