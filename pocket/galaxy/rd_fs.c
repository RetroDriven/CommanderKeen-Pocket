/* SPDX-License-Identifier: GPL-2.0-or-later
 * Bounded packed resources and checksum-verified, two-bank save journals. */
#include "id_fs.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "rd_platform.h"
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#define RD_BANK_BYTES 131072u
#define RD_SAVE_HEADER 32u
#define RD_SAVE_CAP (RD_BANK_BYTES-RD_SAVE_HEADER)
struct RD_GalaxyFile {unsigned char*data;size_t size,pos;char name[48];unsigned sequence,bank;bool writing,committed,failed;};
static unsigned rd_fs_hash(const unsigned char*p,size_t size){unsigned h=2166136261u;for(size_t i=0;i<size;i++)h=(h^p[i])*16777619u;
 return h;}
static unsigned rd_fs_word(const unsigned char*p){return p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);}
static void rd_fs_put(unsigned char*p,unsigned v){for(unsigned n=0;n<4;n++)p[n]=(unsigned char)(v>>(n*8));}
static FS_File rd_fs_resource(const char*n){size_t size;void*p=rd_galaxy_asset(n,&size);
 if(!p)return NULL;
 FS_File f=calloc(1,sizeof(*f));
 if(!f){free(p);
 return NULL;}f->data=p;f->size=size;
 return f;}
static const char*rd_fs_name(const char*n){
 unsigned ep=rd_galaxy_episode();char config[16],automatic[16],ext[5];snprintf(config,sizeof(config),"CONFIG.CK%u",ep);snprintf(automatic,sizeof(automatic),"AUTOSAVE.CK%u",ep);snprintf(ext,sizeof(ext),".CK%u",ep);
 static char name[48];
 if(!strncmp(n,"SAVEGAM",7)&&n[7]>='0'&&n[7]<='5'&&!strcmp(n+8,ext))snprintf(name,sizeof(name),"RetroDrivenKeen%u_%c.sav",ep,n[7]);
 else if(!strcmp(n,automatic))snprintf(name,sizeof(name),"RetroDrivenK%u_state.sav",ep);
 else if(!strcmp(n,config))snprintf(name,sizeof(name),"RetroDrivenKeen%u_6.sav",ep);
 else if(!strcmp(n,"OMNISPK.CFG"))snprintf(name,sizeof(name),"RetroDrivenKeen%u_7.sav",ep);
 else return NULL;
 return name;
}
static void rd_fs_magic(unsigned char*m){
 memcpy(m,"RDK4J01",8);m[3]=(unsigned char)('0'+rd_galaxy_episode());
 if(rd_galaxy_episode()==7){
  size_t size=0;unsigned char*p=rd_galaxy_asset("DREAMVER.KDR",&size);
  if(p&&size==4){unsigned v=p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);if(v==113)m[5]='S';else if(v==193)m[5]='R';}
  free(p);
 }
}
static bool rd_fs_bank(FILE*p,unsigned bank,FS_File f){
 unsigned char magic[8];rd_fs_magic(magic);
 unsigned char h[RD_SAVE_HEADER];
 if(fseek(p,(long)(bank*RD_BANK_BYTES),SEEK_SET)||fread(h,1,sizeof(h),p)!=sizeof(h)||memcmp(h,magic,8))return false;
 unsigned size=rd_fs_word(h+12);
 if(size>RD_SAVE_CAP||rd_fs_word(h+24)!=rd_fs_hash(h,24))return false;
 unsigned char*data=malloc(size?size:1);
 if(!data)return false;
 if(fread(data,1,size,p)!=size||rd_fs_word(h+16)!=rd_fs_hash(data,size)){free(data);
 return false;}
 unsigned sequence=rd_fs_word(h+8);
 if(!f->data||(int32_t)(sequence-f->sequence)>0){free(f->data);f->data=data;f->size=size;f->sequence=sequence;f->bank=bank;}else free(data);
 return true;
}
static FS_File rd_fs_read(const char*name){
 FILE*p=fopen(name,"rb");
 if(!p)return NULL;
 FS_File f=calloc(1,sizeof(*f));
 if(!f){fclose(p);
 return NULL;}
 (void)rd_fs_bank(p,0,f);(void)rd_fs_bank(p,1,f);fclose(p);
 if(!f->data){free(f);
 return NULL;}snprintf(f->name,sizeof(f->name),"%s",name);
 return f;
}
static FS_File rd_fs_user(const char*n,bool write){
 const char*name=rd_fs_name(n);
 if(!name)return NULL;
 if(!write)return rd_fs_read(name);
 FS_File f=calloc(1,sizeof(*f));
 if(!f)return NULL;
 f->data=malloc(RD_SAVE_CAP);
 if(!f->data){free(f);
 return NULL;}snprintf(f->name,sizeof(f->name),"%s",name);f->writing=true;
 FS_File prev=rd_fs_read(name);
 if(prev){f->sequence=prev->sequence;f->bank=prev->bank;free(prev->data);free(prev);}else f->bank=1;
 return f;
}
bool rd_fs_commit(FS_File f){
 if(!f){errno=EINVAL;return false;}
 if(!f->writing)return true;
 if(f->failed){if(!errno)errno=EIO;return false;}
 if(f->committed)return true;
 unsigned char h[RD_SAVE_HEADER]={0};rd_fs_magic(h);rd_fs_put(h+8,f->sequence+1);rd_fs_put(h+12,(unsigned)f->size);rd_fs_put(h+16,rd_fs_hash(f->data,f->size));rd_fs_put(h+24,rd_fs_hash(h,24));
 FILE*p=fopen(f->name,"r+b");
 if(!p)p=fopen(f->name,"w+b");
 if(!p){f->failed=true;if(!errno)errno=EIO;return false;}
 unsigned bank=f->bank^1u;
 bool ok=!fseek(p,(long)(bank*RD_BANK_BYTES),SEEK_SET)&&fwrite(h,1,sizeof(h),p)==sizeof(h)&&fwrite(f->data,1,f->size,p)==f->size;
 if(fclose(p))ok=false;
 if(ok){FS_File check=rd_fs_read(f->name);ok=check&&check->sequence==f->sequence+1&&check->size==f->size&&!memcmp(check->data,f->data,f->size);
 if(check){free(check->data);free(check);}}
 f->committed=ok;f->failed=!ok;if(!ok&&!errno)errno=EIO;
 return ok;
}
bool FS_IsFileValid(FS_File f){return f!=NULL;}
size_t FS_GetFileSize(FS_File f){return f?f->size:0;}
size_t FS_Read(void*p,size_t size,size_t count,FS_File f){if(!f||!size||f->pos>f->size)return 0;
 if(count>(f->size-f->pos)/size)count=(f->size-f->pos)/size;memcpy(p,f->data+f->pos,size*count);f->pos+=size*count;
 return count;}
size_t FS_Write(const void*p,size_t size,size_t count,FS_File f){if(!f||!f->writing||!size||f->pos>RD_SAVE_CAP)return 0;
 if(count>(RD_SAVE_CAP-f->pos)/size){f->failed=true;errno=ENOSPC;
 return 0;}memcpy(f->data+f->pos,p,size*count);f->pos+=size*count;
 if(f->pos>f->size)f->size=f->pos;f->committed=false;
 return count;}
size_t FS_SeekTo(FS_File f,size_t offset){if(!f)return 0;size_t old=f->pos;
 if(offset<=f->size)f->pos=offset;
 return old;}
void FS_CloseFile(FS_File f){if(!f)return;
 if(f->writing&&!f->failed)(void)rd_fs_commit(f);free(f->data);free(f);}
FS_File FS_OpenKeenFile(const char*n){return rd_fs_resource(n);}
FS_File FS_OpenOmniFile(const char*n){return rd_fs_resource(n);}
FS_File FS_OpenUserFile(const char*n){return rd_fs_user(n,false);}
FS_File FS_CreateUserFile(const char*n){return rd_fs_user(n,true);}
bool FS_IsKeenFilePresent(const char*n){FS_File f=FS_OpenKeenFile(n);
 if(!f)return false;FS_CloseFile(f);
 return true;}
bool FS_IsOmniFilePresent(const char*n){return FS_IsKeenFilePresent(n);}
bool FS_IsUserFilePresent(const char*n){FS_File f=FS_OpenUserFile(n);
 if(!f)return false;FS_CloseFile(f);
 return true;}
char*FS_AdjustExtension(const char*n){static char b[FS_MAX_FILENAME_LEN];snprintf(b,sizeof(b),"%s",n);size_t l=strlen(b);
 if(l>=3)memcpy(b+l-3,ck_currentEpisode->ext,3);
 return b;}
void FS_Startup(void){}
static int rd_fs_vprintf(FS_File f,const char*fmt,va_list args){char text[4096];int n=vsnprintf(text,sizeof(text),fmt,args);
 if(n<0||(size_t)n>=sizeof(text))return -1;
 return FS_Write(text,1,(size_t)n,f)==(size_t)n?n:-1;}
#include "rd_fs_endian.inc"

size_t rd_fs_position(FS_File f){return f?f->pos:0;}
