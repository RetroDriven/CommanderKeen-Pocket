/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "id_in.h"
#include "rd_platform.h"
int rd_in_play,rd_in_menu,rd_in_cards,rd_in_help,rd_in_retry;
void rd_galaxy_close_menu(void);
int rd_galaxy_back_root(void);
static unsigned previous,suppressed;
static IN_ScanCode pressed_keys[10];
static uint32_t repeated[4];
void rd_galaxy_input_dismiss(void){
 IN_ClearKeysDown();
 suppressed=rd_galaxy_buttons();previous=0;
 for(unsigned i=0;i<10;i++)pressed_keys[i]=IN_SC_None;
}
void rd_galaxy_input(void){
 unsigned raw=rd_galaxy_buttons();suppressed&=raw;
 unsigned now=raw&~suppressed;uint32_t time=rd_galaxy_us();
 if(rd_in_cards&&!rd_in_help&&(now&RD_START)&&!(previous&RD_START))rd_galaxy_close_menu();
 if(rd_in_cards&&!rd_in_help&&(now&RD_B)&&!(previous&RD_B))(void)rd_galaxy_back_root();
 unsigned keys[]={IN_SC_UpArrow,IN_SC_DownArrow,IN_SC_LeftArrow,IN_SC_RightArrow,
  rd_in_play&&!rd_in_menu?IN_SC_Control:IN_SC_Enter,rd_in_play&&!rd_in_menu?IN_SC_Space:IN_SC_Escape,
  IN_SC_Enter,IN_SC_Alt,IN_SC_Escape,IN_SC_F1};
 for(unsigned i=0;i<10;i++){
  unsigned bit=1u<<i;
  if((now&bit)&&!(previous&bit)){pressed_keys[i]=(IN_ScanCode)keys[i];IN_HandleKeyDown(pressed_keys[i],false);if(i<4)repeated[i]=time+350000;}
  else if(!(now&bit)&&(previous&bit)){IN_HandleKeyUp(pressed_keys[i],false);pressed_keys[i]=IN_SC_None;}
  else if(i<4&&(now&bit)&&(!rd_in_play||rd_in_menu)&&(int32_t)(time-repeated[i])>=0){IN_HandleKeyDown((IN_ScanCode)keys[i],false);repeated[i]=time+90000;}
 }
 previous=now;
}
#include "rd_input_base.inc"
