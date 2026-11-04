/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "kdreams/id_heads.h"
#include "be_cross_mem_internal.h"
#include "backend/video/be_video_emu.h"
#include "rd_dreams.h"
#include <stdlib.h>
#include <string.h>
int g_be_argc=3;
static const char *args[]={"Keen Dreams","/NOJOYS","/NOMOUSE",NULL}; const char **g_be_argv=args;
BE_GameVer_T refkeen_current_gamever=BE_GAMEVER_KDREAMSE100;
bool g_sdlDoRefreshGfxOutput;
const uint32_t g_sdlEGABGRAScreenColors[16]={0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff};
const uint32_t g_sdlVGADefaultBGRAScreenColors[256]={0};
int g_binding_value_button[2],g_binding_value_stats,g_binding_value_up,g_binding_value_down,g_binding_value_left,g_binding_value_right;
bool g_keybind_used_button[2],g_keybind_used_stats,g_keybind_used_up,g_keybind_used_down,g_keybind_used_left,g_keybind_used_right;
BE_ST_ControllerMapping g_beStControllerMappingTextInput,g_ingame_altcontrol_mapping_demoloop,g_ingame_altcontrol_mapping_inackback,g_ingame_altcontrol_mapping_menu,g_ingame_altcontrol_mapping_menu_help,g_ingame_altcontrol_mapping_notenoughmemorytostart,g_ingame_altcontrol_mapping_simpledialog;
void BE_ST_AltControlScheme_Push(void){}
void BE_ST_AltControlScheme_Pop(void){}
void BE_ST_AltControlScheme_PrepareControllerMapping(const BE_ST_ControllerMapping*p){(void)p;}
void BE_ST_AltControlScheme_UpdateVirtualMouseCursor(int x,int y){(void)x;(void)y;}
void PrepareGamePlayControllerMapping(void){}
void FinalizeControlPanelMappingsByMousePresence(bool b){(void)b;}
void UpdateGameplayMappingsByMousePresence(bool b){(void)b;}
void BEL_ST_SetGfxOutputRects(bool b){(void)b;}
void BEL_ST_RecreateMainTextures(void){}
void BE_ST_ResetEmuMouse(void){}
void BE_ST_GetEmuAccuMouseMotion(int16_t*x,int16_t*y){if(x)*x=0;if(y)*y=0;}
uint16_t BE_ST_GetEmuMouseButtons(void){return 0;}
void BE_ST_GetEmuJoyAxes(uint16_t j,uint16_t*x,uint16_t*y){(void)j;if(x)*x=65535;if(y)*y=65535;}
uint16_t BE_ST_GetEmuJoyButtons(uint16_t j){(void)j;return 0;}
bool BE_ST_HostGfx_CanToggleAspectRatio(void){return false;}
bool BE_ST_HostGfx_CanToggleFullScreen(void){return false;}
bool BE_ST_HostGfx_GetAspectRatioToggle(void){return true;}
bool BE_ST_HostGfx_GetFullScreenToggle(void){return true;}
void BE_ST_HostGfx_SetAbsMouseCursorToggle(bool b){(void)b;}
void BE_ST_HostGfx_SetAspectRatioToggle(bool b){(void)b;}
void BE_ST_HostGfx_SetFullScreenToggle(bool b){(void)b;}
void BE_ST_SetAppQuitCallback(void(*f)(void)){(void)f;}
void BE_ST_ExitWithErrorMsg(const char*s){rd_dreams_fatal(s);}
void BE_ST_HandleExit(int n){(void)n;rd_dreams_fatal("Keen Dreams stopped.");}
void BE_ST_QuickExit(void){rd_dreams_fatal("Keen Dreams stopped.");}
void BE_Cross_Bexecv(void(*f)(void),const char**a,void(*z)(void),bool b){(void)f;(void)a;(void)z;(void)b;rd_dreams_fatal("Unsupported DOS executable transition.");}
int loadscn2_main(int a,const char**b){(void)a;(void)b;return 0;}
void BE_ST_MarkGfxForUpdate(void){g_sdlDoRefreshGfxOutput=true;}
extern int rd_dreams_in_play;
static void rd_dreams_video(void){
 static unsigned previous_view=~0u;unsigned view=rd_dreams_options()&0x00100000u;
 if(view!=previous_view){previous_view=view;if(!rd_dreams_in_play)g_sdlDoRefreshGfxOutput=true;}
 if(!g_sdlDoRefreshGfxOutput||g_sdlScreenMode!=0xD)return;
 static uint8_t pixels[320*240],palette[16];
 unsigned start=(unsigned)g_sdlScreenStartAddress*8+g_sdlPelPanning;
 for(unsigned y=0;y<rd_dreams_view_height();y++)for(unsigned x=0;x<320;x++)pixels[y*320+x]=g_sdlVidMem.A000.gfxByByte[(start+y*g_sdlPixLineWidth+x)&0x7ffff];
 for(unsigned c=0;c<16;c++){palette[c]=0;for(unsigned i=0;i<16;i++)if(g_sdlEGACurrBGRAPalette[c]==g_sdlEGABGRAScreenColors[i])palette[c]=i;}
 rd_dreams_present(pixels,palette);g_sdlDoRefreshGfxOutput=false;
}
void rd_dreams_present_pending(void){rd_dreams_video();}
void BE_ST_WaitForNewVerticalRetraces(int16_t n){uint32_t t=rd_dreams_us(),duration=(n>0?n:1)*14286u;rd_dreams_video();while((uint32_t)(rd_dreams_us()-t)<duration)rd_dreams_pump();}
static void(*key_service)(uint8_t);
static unsigned old_buttons;
static unsigned held_scans[9];
static uint32_t next_repeat[4];
static int bios_scan;
void BE_ST_StartKeyboardService(void(*f)(uint8_t)){key_service=f;}
void BE_ST_StopKeyboardService(void){key_service=NULL;}
extern int rd_dreams_in_play,rd_dreams_in_menu;
void rd_dreams_input(void){
 unsigned b=rd_dreams_buttons();int menu=!rd_dreams_in_play||rd_dreams_in_menu;
 const unsigned masks[]={1,2,4,8,16,32,64,256,512};
 const unsigned keys[]={0x48,0x50,0x4b,0x4d,menu?0x1c:0x1d,menu?0x01:0x38,0x39,0x01,0x3b};
 uint32_t now=rd_dreams_us();
 for(unsigned i=0;i<9;i++){
  if((old_buttons^b)&masks[i]){
   if(b&masks[i]){unsigned k=keys[i];held_scans[i]=k;bios_scan=k;if(key_service)key_service(k);if(i<4)next_repeat[i]=now+350000;}
   else if(key_service)key_service(held_scans[i]|0x80);
  }else if(i<4&&(b&masks[i])){
   if(!menu)Keyboard[held_scans[i]]=true;
   else if((int32_t)(now-next_repeat[i])>=0){if(key_service)key_service(held_scans[i]);next_repeat[i]=now+80000;}
  }
 }
 old_buttons=b;
}
int16_t BE_ST_BiosScanCode(int16_t command){rd_dreams_input();int r=bios_scan;if(!command)bios_scan=0;return r;}
void BE_ST_ShortSleep(void){rd_dreams_pump();rd_dreams_input();rd_dreams_video();}
