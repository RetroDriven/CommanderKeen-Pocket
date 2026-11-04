/* SPDX-License-Identifier: GPL-2.0-or-later
 * Pocket campaign policy; original game state/object serialization is retained. */
#include "rd_platform.h"
#include "ck_def.h"
#include "ck_play.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_rf.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_cfg.h"
#include "id_ti.h"
#include <string.h>
#include "rd_title_prompt.h"
extern int rd_in_menu,rd_in_play;
extern int ck_lastLevelFinished;
extern int ck_startingSavedGame;
extern US_CardCommand us_currentCommand;
extern bool command_confirmed;
unsigned rd_galaxy_options(void);
int rd_galaxy_hud_inventory(unsigned*needed,unsigned*held){
 static int level=-2;static unsigned required;
 *needed=*held=0;
 if((rd_galaxy_options()&0x00280000u)!=0x00280000u||!rd_in_play||rd_in_menu||ca_mapOn<=0)return 0;
 if(level!=(int)ca_mapOn){
  required=0;level=ca_mapOn;
  for(unsigned y=0;y<CA_GetMapHeight();y++)for(unsigned x=0;x<CA_GetMapWidth();x++){
   unsigned misc=TI_ForeMisc(CA_TileAtPos(x,y,1))&0x7f;
   if(misc>=MISCFLAG_GEMHOLDER0&&misc<=MISCFLAG_GEMHOLDER3)required|=1u<<(misc-MISCFLAG_GEMHOLDER0);
  }
 }
 *needed=required;for(unsigned i=0;i<4;i++)if(ck_gameState.keyGems[i])*held|=1u<<i;
 return required!=0;
}
bool rd_fs_commit(FS_File f);
static uint32_t notice_until;
static const char*notice;
static int checkpoint_pending;
int rd_in_confirm;
const char*rd_galaxy_notice(void){return notice&&(int32_t)(notice_until-rd_galaxy_us())>0?notice:NULL;}
static void rd_message(const char*s){notice=s;notice_until=rd_galaxy_us()+3000000;}
static int rd_galaxy_window(const char*title,int question){
 rd_in_confirm=1;
 US_WindowRec window;US_SaveWindow(&window);int font=US_GetPrintFont(),color=US_GetPrintColour();
 US_CenterWindow(30,6);US_SetPrintFont(0);US_SetPrintColour(4);US_CPrint(title);US_SetPrintY(US_GetPrintY()+12);US_SetPrintColour(0);US_CPrint(question?"A: Yes     B: Cancel":"A / B: Continue");VH_UpdateScreen();
 unsigned b;do{b=rd_galaxy_buttons();rd_galaxy_pump();}while(b&(RD_A|RD_B|RD_START));
 do{b=rd_galaxy_buttons();rd_galaxy_pump();}while(!(b&(RD_A|RD_B|RD_START)));
 int yes=!!(b&RD_A);do{b=rd_galaxy_buttons();rd_galaxy_pump();}while(b&(RD_A|RD_B|RD_START));
 IN_ClearKeysDown();US_RestoreWindow(&window);US_SetPrintFont(font);US_SetPrintColour(color);SD_SetLastTimeCount(SD_GetTimeCount());rd_in_confirm=0;
 return question?yes:1;
}
int rd_galaxy_confirm(const char*s){return rd_galaxy_window(s,1);}
int rd_galaxy_ack(const char*s){return rd_galaxy_window(s,0);}
void rd_galaxy_enter_level(void){checkpoint_pending=ca_mapOn==0&&ck_lastLevelFinished>0&&ck_gameState.levelState!=LS_LoadedGame&&IN_DemoGetMode()==IN_Demo_Off;}
void rd_galaxy_after_frame(void){
#ifdef RD_GALAXY_TEST
 extern void rd_galaxy_test_tick(void);rd_galaxy_test_tick();
#endif
 if(!checkpoint_pending||vl_screenFaded)return;
 checkpoint_pending=0;unsigned mode=(rd_galaxy_options()>>22)&3;
 if(mode>=2)return;
 if(mode==1&&!rd_galaxy_confirm("Auto save this progress?")){RF_ForceRefresh();return;}
 FS_File f=FS_CreateUserFile(FS_AdjustExtension("AUTOSAVE.CK4"));bool ok=f&&CK_SaveGame(f)&&rd_fs_commit(f);FS_CloseFile(f);
 rd_message(ok?"AUTO SAVE COMPLETE":"AUTO SAVE FAILED");RF_ForceRefresh();SD_SetLastTimeCount(SD_GetTimeCount());
}
void rd_galaxy_policy_tick(void){
 rd_galaxy_view_update();
 bool on=!!(rd_galaxy_options()&0x00200000u);
 if(ck_scoreBoxEnabled!=on&&ck_scoreBoxObj){ck_scoreBoxEnabled=on;
 if(!on&&ck_scoreBoxObj->sde)RF_RemoveSpriteDraw(&ck_scoreBoxObj->sde);
 if(on)ck_scoreBoxObj->user1=ck_scoreBoxObj->user2=ck_scoreBoxObj->user3=ck_scoreBoxObj->user4=-1;}
}
bool rd_galaxy_load_auto(bool fromMenu){
 FS_File f=FS_OpenUserFile(FS_AdjustExtension("AUTOSAVE.CK4"));
 if(!f){rd_message("NO AUTO SAVE FOUND");
 return false;}
 bool ok=CK_LoadGame(f,fromMenu);FS_CloseFile(f);
 if(ok){ck_startingSavedGame=1;ck_gameState.levelState=LS_LoadedGame;rd_message("AUTO SAVE LOADED");}else rd_message("AUTO SAVE LOAD FAILED");
 return ok;
}
static bool rd_auto_proc(US_CardMsg msg,US_CardItem*item){
 (void)item;
 if(msg!=US_MSG_CardEntered)return false;
 if(!FS_IsUserFilePresent(FS_AdjustExtension("AUTOSAVE.CK4"))){US_CenterWindow(24,4);US_SetPrintColour(0);US_CPrint("No auto save found.");VH_UpdateScreen();IN_WaitButton();
 return true;}
 if(rd_galaxy_confirm("Load auto save?")){
  if(rd_galaxy_load_auto(true)){us_currentCommand=US_Comm_LoadGame;command_confirmed=true;}
 }
 return true;
}
US_Card rd_auto_card={0,0,0,0,NULL,rd_auto_proc,0,0,0};

extern US_Card ck_us_mainMenu,*us_currentCard;
void rd_galaxy_close_menu(void){us_currentCommand=US_Comm_ReturnToGame;command_confirmed=true;}
int rd_galaxy_back_root(void){if(us_currentCard==&ck_us_mainMenu){rd_galaxy_close_menu();return 1;}return 0;}

void rd_galaxy_save_notice(void){rd_message("GAME SAVED");}
int rd_in_text;
bool rd_galaxy_name_input(uint16_t x,uint16_t y,char*buf,char*def,bool escok,uint16_t maxchars,uint16_t maxwidth){
 (void)x;(void)y;const char*letters=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.!-";
 char value[21];unsigned cap=maxchars<20?maxchars:20;if(maxwidth&&cap>maxwidth/8)cap=maxwidth/8;if(!cap)return false;
 snprintf(value,sizeof(value),"%.*s",(int)cap,def&&*def?def:"PLAYER");unsigned cursor=0;rd_in_text=1;
 US_WindowRec window;US_SaveWindow(&window);int font=US_GetPrintFont(),color=US_GetPrintColour();unsigned prev=rd_galaxy_buttons();uint32_t next=rd_galaxy_us()+350000;bool result=false;
 for(;;){
  US_CenterWindow(28,7);US_SetPrintFont(0);US_SetPrintColour(4);US_CPrint("Enter your name");US_SetPrintY(US_GetWindowY()+24);US_SetPrintColour(0);US_CPrint(value);
  uint16_t width,height;char left[21];memcpy(left,value,cursor);left[cursor]=0;VH_MeasurePropString(left,&width,&height,0);uint16_t total;VH_MeasurePropString(value,&total,&height,0);VH_DrawPropString("^",US_GetWindowX()+US_GetWindowW()/2-total/2+width,US_GetWindowY()+36,0,4);VH_UpdateScreen();
  unsigned now=rd_galaxy_buttons(),press=now&~prev;uint32_t time=rd_galaxy_us();
  if((now&(RD_UP|RD_DOWN|RD_LEFT|RD_RIGHT))&&now==prev&&(int32_t)(time-next)>=0){press|=now&(RD_UP|RD_DOWN|RD_LEFT|RD_RIGHT);next=time+80000;}else if(now!=prev)next=time+350000;
  unsigned length=(unsigned)strlen(value);
  if(press&RD_A){while(length&&value[length-1]==' ')value[--length]=0;if(!length)strcpy(value,"PLAYER");snprintf(buf,(size_t)maxchars+1,"%s",value);result=true;break;}
  if(escok&&(press&(RD_B|RD_START)))break;
  if((press&RD_LEFT)&&cursor)cursor--;
  if((press&RD_RIGHT)&&cursor+1<cap){if(cursor+1==length){value[length]='A';value[length+1]=0;}cursor++;}
  if(press&(RD_UP|RD_DOWN)){const char*c=strchr(letters,value[cursor]);unsigned index=c?(unsigned)(c-letters):1;unsigned count=(unsigned)strlen(letters);index=(index+count+((press&RD_UP)?1u:count-1u))%count;value[cursor]=letters[index];}
  if((press&RD_X)&&length){memmove(value+cursor,value+cursor+1,length-cursor);if(cursor&&cursor>=(unsigned)strlen(value))cursor--;if(!value[0]){value[0]='A';value[1]=0;cursor=0;}}
  prev=now;rd_galaxy_pump();
 }
 while(rd_galaxy_buttons()&(RD_A|RD_B|RD_START))rd_galaxy_pump();
 rd_in_text=0;IN_ClearKeysDown();US_RestoreWindow(&window);US_SetPrintFont(font);US_SetPrintColour(color);if(ca_mapOn>=0&&CA_MapHeaders[ca_mapOn])RF_ForceRefresh();return result;
}

void US_SaveConfig(void);
void rd_galaxy_store_config(void){US_SaveConfig();CFG_SaveConfig("OMNISPK.CFG");}

void rd_galaxy_save_failed_notice(void){rd_message("SAVE FAILED - OLD SAVE KEPT");}

void rd_galaxy_title_prompt(void){
 /* Keen 6's retail title has no F1 caption or built-in Help/Order menu. */
 if(rd_galaxy_episode()!=4&&rd_galaxy_episode()!=5)return;
 int chunk=CK_CHUNKNUM(PIC_TITLESCREEN);
 if(!ca_graphChunks[ca_gfxInfoE.hdrBitmaps])return;
 VH_BitmapTableEntry d=VH_GetBitmapTableEntry(chunk-ca_gfxInfoE.offBitmaps);
 if(d.width==40&&d.height==200&&ca_graphChunks[chunk])rd_title_prompt_pixels(ca_graphChunks[chunk],rd_galaxy_episode());
}

void rd_galaxy_watch_prompt(void){
 int chunk=CK_CHUNKNUM(PIC_WRISTWATCH);
 if(!ca_graphChunks[ca_gfxInfoE.hdrBitmaps]||!ca_graphChunks[chunk])return;
 VH_BitmapTableEntry d=VH_GetBitmapTableEntry(chunk-ca_gfxInfoE.offBitmaps);
 if(d.width==40&&d.height==200)rd_watch_prompt_pixels(ca_graphChunks[chunk]);
}
