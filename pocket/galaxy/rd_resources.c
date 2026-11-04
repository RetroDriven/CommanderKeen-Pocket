/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rd_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rd_metadata.inc"
static unsigned rd_word(const unsigned char*p){return p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);}
static unsigned episode=4,variant,pack_headers=1;
unsigned rd_galaxy_episode(void){return episode;}
void rd_galaxy_select_episode(unsigned n){episode=n>=4&&n<=7?n:4;variant=0;pack_headers=episode==4;}
static void*rd_pack_asset(const char*name,size_t*size){
 FILE*f=fopen("keenpack.bin","rb");if(!f)return NULL;unsigned char head[16],entry[48];void*data=NULL;
 if(fread(head,1,16,f)!=16||memcmp(head,"RDKP019",7))goto done;
 unsigned count=rd_word(head+12);if(count>256)goto done;
 for(unsigned i=0;i<count;i++){
  if(fread(entry,1,48,f)!=48)break;
  if(rd_word(entry)!=episode||strncmp((const char*)entry+4,name,24))continue;
  unsigned offset=rd_word(entry+28),length=rd_word(entry+32);if(length>2000000)break;
  data=malloc(length);if(!data)break;
  if(fseek(f,(long)offset,SEEK_SET)||fread(data,1,length,f)!=length){free(data);data=NULL;}else *size=length;break;
 }
 done:fclose(f);return data;
}

void*rd_galaxy_asset(const char*name,size_t*size){
 if(episode==6&&!variant){size_t length=0;void*p=rd_pack_asset("EGAGRAPH.CK6",&length);free(p);
  if(length&&length!=464726&&length!=464662&&length!=457309)rd_galaxy_fatal("Unsupported Keen 6 data revision");
  variant=length==464662?15:14;pack_headers=length==457309;
 }
 /* Load derived headers from the pack; use linked-in action tables and strings. */
 const char*headers[]={"EGADICT.","EGAHEAD.","MAPHEAD.","TILEINFO.","AUDIODCT.","AUDIOHHD."};
 if(pack_headers)for(unsigned i=0;i<6;i++)if(!strncmp(name,headers[i],strlen(headers[i]))){void*p=rd_pack_asset(name,size);if(p)return p;break;}
 for(unsigned i=0;i<sizeof(rd_metadata)/sizeof(rd_metadata[0]);i++){
  if(rd_metadata[i].episode!=episode||(rd_metadata[i].variant&&rd_metadata[i].variant!=variant)||strcmp(name,rd_metadata[i].name))continue;
  *size=rd_metadata[i].size;void*p=malloc(*size);if(p)memcpy(p,rd_metadata[i].data,*size);return p;
 }
 return rd_pack_asset(name,size);
}
