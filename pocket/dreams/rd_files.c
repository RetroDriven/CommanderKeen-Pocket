/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "refkeen.h"
#include "rd_stream.h"
#include "rd_dreams.h"
#include <stdlib.h>
#include <string.h>
extern RD_Stream *FS_OpenKeenFile(const char*),*FS_OpenUserFile(const char*),*FS_CreateUserFile(const char*);
extern size_t FS_Read(void*,size_t,size_t,RD_Stream*),FS_Write(const void*,size_t,size_t,RD_Stream*),FS_GetFileSize(RD_Stream*),FS_SeekTo(RD_Stream*,size_t),rd_fs_position(RD_Stream*);
extern void FS_CloseFile(RD_Stream*);
extern bool rd_fs_commit(RD_Stream*);
size_t rd_ds_read(void*p,size_t n,size_t c,RD_Stream*f){return FS_Read(p,n,c,f);}
size_t rd_ds_write(const void*p,size_t n,size_t c,RD_Stream*f){return FS_Write(p,n,c,f);}
int rd_ds_seek(RD_Stream*f,long n,int origin){long pos=origin==SEEK_SET?n:origin==SEEK_END?(long)FS_GetFileSize(f)+n:(long)rd_fs_position(f)+n;if(pos<0||(size_t)pos>FS_GetFileSize(f))return -1;FS_SeekTo(f,(size_t)pos);return 0;}
long rd_ds_tell(RD_Stream*f){return (long)rd_fs_position(f);}
int rd_ds_close(RD_Stream*f){if(!f)return -1;bool ok=rd_fs_commit(f);FS_CloseFile(f);return ok?0:-1;}
int rd_ds_getc(RD_Stream*f){unsigned char c;return FS_Read(&c,1,1,f)==1?c:EOF;}
int rd_ds_putc(int c,RD_Stream*f){unsigned char b=(unsigned char)c;return FS_Write(&b,1,1,f)==1?b:EOF;}
static const char *name(const char*s){static char n[32];
 if(!strcmp(s,"CONFIG.KDR")||!strcmp(s,"KDREAMS.CFG"))return "CONFIG.CK7";
 if(!strcmp(s,"AUTOSAVE.KDR"))return "AUTOSAVE.CK7";
 if(!strncmp(s,"SAVEGM",6)&&s[6]>='0'&&s[6]<='6'&&!strcmp(s+7,".KDR")){if(s[6]=='6')return "AUTOSAVE.CK7";snprintf(n,sizeof(n),"SAVEGAM%c.CK7",s[6]);return n;}return NULL;}
BE_FILE_T BE_Cross_open_readonly_for_reading(const char*n){return FS_OpenKeenFile(n);}
BE_FILE_T BE_Cross_open_rewritable_for_reading(const char*n){const char*p=name(n);return p?FS_OpenUserFile(p):NULL;}
BE_FILE_T BE_Cross_open_rewritable_for_overwriting(const char*n){const char*p=name(n);return p?FS_CreateUserFile(p):NULL;}
int BE_Cross_load_embedded_rsrc_to_mem(const char*n,void**p){BE_FILE_T f=FS_OpenKeenFile(n);if(!f)return -1;size_t size=FS_GetFileSize(f);*p=calloc(1,size+5600);if(!*p){FS_CloseFile(f);return -1;}int ok=FS_Read(*p,1,size,f)==size;((char*)*p)[size]=0;FS_CloseFile(f);if(!ok){free(*p);*p=NULL;return -1;}return (int)size;}
void BE_Cross_free_mem_loaded_embedded_rsrc(void*p){free(p);}
void *BE_Cross_GetFarEmbeddedData(const char*n,uint32_t*size){void*p;int len=BE_Cross_load_embedded_rsrc_to_mem(n,&p);if(len<0)return NULL;if(size)*size=(uint32_t)len;return p;}
void *BE_Cross_GetNearEmbeddedData(const char*n,uint16_t*size){uint32_t len;void*p=BE_Cross_GetFarEmbeddedData(n,&len);if(p&&size)*size=(uint16_t)len;return p;}
