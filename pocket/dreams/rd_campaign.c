/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "kdreams/kd_def.h"
#include "rd_dreams.h"
#include "rd_stream.h"
#include <string.h>
extern int rd_dreams_in_play,rd_dreams_in_menu,rd_dreams_in_help,rd_dreams_in_status;
unsigned rd_dreams_options(void);
bool rd_fs_commit(RD_Stream*);
id0_boolean_t SaveGame(BE_FILE_T),LoadGame(BE_FILE_T);
void rd_dreams_slot_committed(int,const char*);
int rd_dreams_intro_flat;
static int checkpoint_pending;
static const char *notice;
static uint32_t notice_until;
void rd_dreams_message(const char*s){notice=s;notice_until=rd_dreams_us()+3000000;}
int rd_dreams_confirm(const char*s){
 unsigned width=linewidth;size_t bytes=(size_t)width*200*4;
 uint8_t*backdrop=malloc(bytes);if(!backdrop)return 0;
 id0_unsigned_t saved_buffer=bufferofs;
 VW_ScreenToMem(saved_buffer,backdrop,width,200);
 WindowRec window;US_SaveWindow(&window);int color=fontcolor;rd_dreams_in_menu++;
 US_CenterWindow(30,6);fontcolor=F_FIRSTCOLOR;US_CPrint(s);PrintY+=12;fontcolor=F_BLACK;US_CPrint("A: Yes    B: Cancel");VW_UpdateScreen();rd_dreams_present_pending();
 unsigned b;do{b=rd_dreams_buttons();rd_dreams_pump();}while(b&(16|32|256));
 do{b=rd_dreams_buttons();rd_dreams_pump();}while(!(b&(16|32|256)));
 int yes=!!(b&16);do{b=rd_dreams_buttons();rd_dreams_pump();}while(b&(16|32|256));
 IN_ClearKeysDown();US_RestoreWindow(&window);fontcolor=color;
 VW_MemToScreen(backdrop,saved_buffer,width,200);free(backdrop);
 VW_MarkUpdateBlock(0,0,319,199);VW_UpdateScreen();rd_dreams_present_pending();
 rd_dreams_in_menu--;return yes;
}
void rd_dreams_completed(void){if(mapon>0)checkpoint_pending=1;}
void rd_dreams_cancel_checkpoint(void){checkpoint_pending=0;}
int rd_dreams_save(int slot){
 if(slot<0||slot>6)return 0;char name[24];snprintf(name,sizeof(name),"SAVEGM%d.KDR",slot);
 BE_FILE_T f=BE_Cross_open_rewritable_for_overwriting(name);if(!f)return 0;
 unsigned char header[40]={0};memcpy(header,"KDR",4);header[4]=1;snprintf((char*)header+6,33,slot==6?"Auto Save - Level %d":"Level %d",gamestate.mapon);
 int ok=BE_Cross_writeInt8LEBuffer(f,header,sizeof(header))==sizeof(header)&&SaveGame(f)&&rd_fs_commit(f);
 if(rd_ds_close(f)!=0)ok=0;
 if(ok)rd_dreams_slot_committed(slot,(const char*)header+6);
 return ok;
}
void rd_dreams_after_frame(void){
#ifdef RD_DREAMS_TEST
 extern void rd_dreams_test_tick(void);rd_dreams_test_tick();
#endif
 if(!checkpoint_pending||mapon!=0||screenfaded)return;checkpoint_pending=0;
 unsigned mode=(rd_dreams_options()>>22)&3;if(mode>=2)return;
 if(mode==1&&!rd_dreams_confirm("Auto save this progress?")){RF_ForceRefresh();return;}
 rd_dreams_message(rd_dreams_save(6)?"AUTO SAVE COMPLETE":"AUTO SAVE FAILED");
 RF_ForceRefresh();lasttimecount=SD_GetTimeCount();
}
void rd_dreams_hud(objtype*ob){if(!(rd_dreams_options()&0x00200000u)){if(ob->sprite)RF_RemoveSprite(&ob->sprite);return;}extern void ScoreReact_Original(objtype*);ScoreReact_Original(ob);}
static int hud_keys_needed,hud_bombs_present;
void rd_dreams_hud_context(void){
 hud_keys_needed=hud_bombs_present=0;
 if(mapon<=0||mapon>=17||!mapsegs[2])return;
 for(unsigned i=0;i<(unsigned)mapwidth*mapheight;i++){
  if(mapsegs[2][i]==33)hud_keys_needed=1;
  if(mapsegs[2][i]==31)hud_bombs_present=1;
 }
}
static void hud_tile8(uint8_t*fb,int stride,int x,int y,unsigned tile,int transparent){
 const uint8_t*bank=(const uint8_t*)grsegs[STARTTILE8];if(!bank)return;
 const uint8_t*src=bank+tile*32;
 for(unsigned row=0;row<8;row++)for(unsigned col=0;col<8;col++){
  unsigned color=0;for(unsigned plane=0;plane<4;plane++)color|=((src[plane*8+row]>>(7-col))&1u)<<plane;
  if((tile==24||tile==25)&&color==15)color=3;
  if(!transparent||color!=3)fb[(y+row)*stride+x+col]=(uint8_t)color;
 }
}
/* Copy scorebox cells with their masks and bevels; clear only the inventory area. */
static void hud_cell(uint8_t*fb,int stride,int x,int y,unsigned source_x,unsigned width){
 const uint8_t*block=(const uint8_t*)grsegs[SCOREBOXSPR];if(!block)return;
 unsigned pitch=VW_GetSpriteShiftWidth(block,0),plane_size=VW_GetSpriteShiftPlaneSize(block,0);
 const uint8_t*src=block+VW_GetSpriteShiftSourceOffset(block,0);
 for(unsigned row=0;row<16;row++)for(unsigned col=0;col<width;col++){
  unsigned offset=(row+16)*pitch+(source_x+col)/8,bit=128u>>((source_x+col)&7);
  if(src[offset]&bit)continue;
  unsigned color=0;for(unsigned plane=0;plane<4;plane++)if(src[(plane+1)*plane_size+offset]&bit)color|=1u<<plane;
  if(row>=2&&row<14&&col>=2&&col<width-2)color=3;
  fb[(y+row)*stride+x+col]=(uint8_t)color;
 }
}
/* Boobus Bomb artwork uses status tiles 32-35, fitted to the 12-pixel icon area. */
static void hud_bomb(uint8_t*fb,int stride,int x,int y){
 const uint8_t*bank=(const uint8_t*)grsegs[STARTTILE8];if(!bank)return;
 for(unsigned row=0;row<12;row++)for(unsigned col=0;col<12;col++){
  unsigned sx=(col*2+1)*16/24,sy=(row*2+1)*16/24,tile=32+sx/8+(sy/8)*2,color=0;
  const uint8_t*src=bank+tile*32;
  for(unsigned plane=0;plane<4;plane++)color|=((src[plane*8+sy%8]>>(7-sx%8))&1u)<<plane;
  if(color!=15)fb[(y+row)*stride+x+col]=(uint8_t)color;
 }
}
static void hud_count(uint8_t*fb,int stride,int x,int y,int count){
 if(count<0)count=0;if(count>99)count=99;
 hud_tile8(fb,stride,x,y,count>=10?(unsigned)(count/10)+1:0,0);
 hud_tile8(fb,stride,x+8,y,(unsigned)(count%10)+1,0);
}
unsigned rd_dreams_inventory_flags(void){
 if((rd_dreams_options()&0x00280000u)!=0x00280000u||!rd_dreams_in_play||(rd_dreams_in_menu&&!rd_dreams_in_status)||mapon<=0||mapon>=17)return 0;
 return (hud_keys_needed?1u:0u)|((hud_bombs_present&&mapon!=15)?2u:0u);
}
void rd_dreams_inventory_overlay(uint8_t*fb,int stride){
 unsigned flags=rd_dreams_inventory_flags();int keys=flags&1,bombs=flags&2;
 if(!keys&&!bombs)return;
 int y=(240-(int)rd_dreams_view_height())/2+36,x=4;
 if(keys){
  hud_cell(fb,stride,x,y,0,32);
  hud_tile8(fb,stride,x+2,y+4,24,1);hud_tile8(fb,stride,x+10,y+4,25,1);
  int count=gamestate.keys;if(count<0)count=0;if(count>9)count=9;
  hud_tile8(fb,stride,x+16,y+4,(unsigned)count+1,0);x+=32;
 }
 if(bombs){
  hud_cell(fb,stride,x,y,32,40);
  hud_bomb(fb,stride,x+2,y+2);
  hud_count(fb,stride,x+16,y+4,gamestate.boobusbombs);
 }
}
static void text(uint8_t*fb,int stride,int y,const char*s,unsigned color){
 fontstruct*f=(fontstruct*)grsegs[STARTFONT];if(!f)return;unsigned w=0;for(const unsigned char*p=(const unsigned char*)s;*p;p++)w+=(unsigned char)f->width[*p];int x=(320-(int)w)/2;
 for(const unsigned char*p=(const unsigned char*)s;*p;p++){unsigned width=(unsigned char)f->width[*p],pitch=(width+7)/8;const unsigned char*glyph=(const unsigned char*)f+(uint16_t)f->location[*p];for(int row=0;row<f->height&&row+y<240;row++)for(unsigned col=0;col<width;col++)if(x+(int)col>=0&&x+(int)col<320&&(glyph[row*pitch+col/8]&(128>>(col&7))))fb[(y+row)*stride+x+col]=color;x+=width;}
}
static void notice_box(uint8_t*fb,int stride,const char*s){
 fontstruct*f=(fontstruct*)grsegs[STARTFONT];if(!f)return;
 int width=0;for(const unsigned char*p=(const unsigned char*)s;*p;p++)width+=(unsigned char)f->width[*p];
 int w=width+16,h=f->height+12;if(w>318)w=318;if(h>32)h=32;
 int x=(320-w)/2,y=238-h;
 for(int row=0;row<h;row++)for(int col=0;col<w;col++){
  unsigned color=17;
  if(!row||!col||row==h-1||col==w-1)color=19;
  else if(row==1||col==1)color=17;
  else if(row==h-2||col==w-2)color=21;
  else if(row==2||col==2)color=20;
  fb[(y+row)*stride+x+col]=(uint8_t)color;
 }
 text(fb,stride,y+6,s,strstr(s,"FAILED")?4:18);
}
void rd_dreams_overlay(uint8_t*fb,int stride){
 if(notice&&(int32_t)(notice_until-rd_dreams_us())>0)notice_box(fb,stride,notice);
 else if(rd_dreams_in_help)text(fb,stride,224,"D-PAD SCROLL  B BACK",17);else if(rd_dreams_in_menu)text(fb,stride,224,"A CHOOSE  B BACK  START MENU",17);
}
/* Center fixed menu artwork on the Pocket canvas. */
void rd_dreams_compose_frame(uint8_t*fb,const uint8_t*p){
 extern int rd_dreams_in_control_panel;
 unsigned height=(rd_dreams_in_control_panel||rd_dreams_intro_flat)?200:rd_dreams_view_height();
 int themed=(rd_dreams_in_control_panel||rd_dreams_intro_flat)&&(rd_dreams_options()&0x00100000u);
 memset(fb,themed?FIRSTCOLOR:0,320*240);
 memcpy(fb+320*((240-height)/2),p,320*height);
}
id0_boolean_t rd_dreams_name_input(id0_int_t x,id0_int_t y,char*buf,const char*def,id0_boolean_t escok,id0_int_t maxchars,id0_int_t maxwidth){
 (void)x;(void)y;(void)maxwidth;const char*letters=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.!-";char value[21];unsigned cap=maxchars<20?(unsigned)maxchars:20;if(!cap)return false;
 snprintf(value,sizeof(value),"%.*s",(int)cap,def&&*def?def:"PLAYER");unsigned cursor=0,prev=rd_dreams_buttons();uint32_t next=rd_dreams_us()+350000;WindowRec win;US_SaveWindow(&win);int color=fontcolor;rd_dreams_in_menu++;int result=0;
 for(;;){US_CenterWindow(30,8);fontcolor=F_FIRSTCOLOR;US_CPrint("Enter your name");PrintY+=12;fontcolor=F_BLACK;US_CPrint(value);PrintY+=12;US_CPrint("UP/DOWN LETTER  LEFT/RIGHT MOVE");US_CPrint("X DELETE   A OK   B BACK");VW_UpdateScreen();rd_dreams_present_pending();unsigned b=rd_dreams_buttons(),press=b&~prev;uint32_t now=rd_dreams_us();if((b&15)&&b==prev&&(int32_t)(now-next)>=0){press|=b&15;next=now+80000;}else if(b!=prev)next=now+350000;
 unsigned len=strlen(value);if(press&16){while(len&&value[len-1]==' ')value[--len]=0;if(!len)strcpy(value,"PLAYER");snprintf(buf,(size_t)maxchars+1,"%s",value);result=1;break;}if(escok&&(press&(32|256)))break;
 if((press&4)&&cursor)cursor--;if((press&8)&&cursor+1<cap){if(cursor+1==len){value[len]='A';value[len+1]=0;}cursor++;}
 if(press&3){const char*p=strchr(letters,value[cursor]);unsigned i=p?(unsigned)(p-letters):1,n=strlen(letters);value[cursor]=letters[(i+n+((press&1)?1:n-1))%n];}
 if((press&64)&&len){memmove(value+cursor,value+cursor+1,len-cursor);if(cursor&&cursor>=strlen(value))cursor--;if(!value[0])strcpy(value,"A");}prev=b;rd_dreams_pump();}
 IN_ClearKeysDown();US_RestoreWindow(&win);fontcolor=color;rd_dreams_in_menu--;return result;
}

int rd_dreams_expand_save(const uint16_t*src,uint16_t*dst,unsigned compressed,unsigned expanded,unsigned tag){
 if((compressed&1)||(expanded&1))return 0;unsigned in=0,out=0,words=compressed/2,target=expanded/2;
 while(in<words&&out<target){unsigned value=src[in++],count=1;if(value==tag){if(words-in<2)return 0;count=src[in++];value=src[in++];if(!count||count>target-out)return 0;}while(count--)dst[out++]=(uint16_t)value;}
 return in==words&&out==target;
}

void rd_dreams_policy_tick(void){
 extern void rd_dreams_resize_view(void);
 if(rd_dreams_in_play&&!rd_dreams_in_menu)rd_dreams_resize_view();
 static int previous=-1;int enabled=!!(rd_dreams_options()&0x00200000u);
 if(scoreobj&&enabled!=previous){if(!enabled&&scoreobj->sprite)RF_RemoveSprite(&scoreobj->sprite);scoreobj->needtoreact=true;previous=enabled;}
}


void rd_dreams_return_to_title(void){playstate=resetgame;checkpoint_pending=0;}

/* The expanded viewport reveals additional level rows without scaling. */
int rd_dreams_port_rows=14;
static unsigned view_height=200;
unsigned rd_dreams_view_height(void){return view_height;}
void rd_dreams_view_begin(void){
 int enhanced=(rd_dreams_options()&0x00100000u)&&(mapon<17||mapon==20)&&mapheaderseg[mapon]->height>=19;
 rd_dreams_port_rows=enhanced?16:14;view_height=enhanced?240:200;
}
