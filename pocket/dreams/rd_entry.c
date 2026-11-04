/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "kdreams/kd_def.h"
#include "be_cross_mem_internal.h"
#include "rd_dreams.h"
#include <stdlib.h>
void *rd_galaxy_asset(const char*,size_t*);
void rd_galaxy_select_episode(unsigned);
void RefKeen_Patch_id_ca(void),RefKeen_Patch_id_mm(void),RefKeen_Patch_id_us(void),RefKeen_Patch_id_rf(void),RefKeen_Patch_id_rf_a(void),RefKeen_Patch_id_vw(void),RefKeen_Patch_id_vw_ac(void),RefKeen_Patch_id_vw_ae(void),RefKeen_Patch_kd_demo(void),RefKeen_Patch_kd_keen(void),RefKeen_Patch_kd_play(void),RefKeen_FillObjStatesWithDOSPointers(void),RefKeen_Load_Embedded_Resources_From_kdreams_exe(void),InitGame(void);
void rd_dreams_init(void){
 rd_galaxy_select_episode(7);
 size_t length=0;unsigned char *version=rd_galaxy_asset("DREAMVER.KDR",&length);
 unsigned revision=100;
 if(version){if(length!=4){free(version);rd_dreams_fatal("Invalid Dreams data revision");}revision=version[0]|((unsigned)version[1]<<8)|((unsigned)version[2]<<16)|((unsigned)version[3]<<24);free(version);}
 if(revision==100)refkeen_current_gamever=BE_GAMEVER_KDREAMSE100;
 else if(revision==113)refkeen_current_gamever=BE_GAMEVER_KDREAMSE113;
 else if(revision==193)refkeen_current_gamever=BE_GAMEVER_KDREAMSE193;
 else rd_dreams_fatal("Unsupported Dreams data revision");
 g_be_current_exeTotalMem=256+(revision==100?175424-0x1a00:revision==113?213536-0x1c00:213200-0x1c00)+0x7b50;
 BEL_Cross_ClearMemory();
 RefKeen_Patch_id_ca();RefKeen_Load_Embedded_Resources_From_kdreams_exe();
 RefKeen_Patch_id_mm();RefKeen_Patch_id_us();RefKeen_Patch_id_rf();RefKeen_Patch_id_rf_a();RefKeen_Patch_id_vw();RefKeen_Patch_id_vw_ac();RefKeen_Patch_id_vw_ae();RefKeen_Patch_kd_demo();RefKeen_Patch_kd_keen();RefKeen_Patch_kd_play();RefKeen_FillObjStatesWithDOSPointers();
 InitGame();
}
void rd_dreams_run(void){rd_dreams_init();extern void(*DemoLoop)(void);DemoLoop();rd_dreams_fatal("Keen Dreams stopped.");}
