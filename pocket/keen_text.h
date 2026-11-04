/* Bounded Vorticons text preparation and word wrapping.
 * Behavioral reference: Chocolate Keen menus.c. SPDX-License-Identifier: MIT */
#ifndef KEEN_TEXT_H
#define KEEN_TEXT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {uint16_t start,length;} KeenTextLine;
static inline size_t keen_text_prepare(uint8_t *text,size_t size){
 if(!text||size>65535)return 0;
 uint8_t *end=memchr(text,26,size);if(!end)return 0;size_t n=(size_t)(end-text),p=0;
 while(p<n){
  if(text[p]==31){
   text[p]=13;
   if(p+2<n&&text[p+1]==13&&text[p+2]==10){memmove(text+p+1,text+p+2,n-p-1);n--;}
   p++;
  }
  else if(text[p]==13){
   if(p+2<n&&text[p+2]==13){do{p+=2;}while(p<n&&text[p]==13);}
   else if(p+1<n&&text[p+1]==10){text[p]=32;memmove(text+p+1,text+p+2,n-p-1);n--;p++;}
   else p++;
  }else p++;
 }
 return n+1;
}
static inline unsigned keen_text_lines(const uint8_t *text,size_t size,KeenTextLine*lines,unsigned cap,unsigned columns){
 if(!text||!lines||!columns||!cap||size>65535)return 0;
 size_t p=0;unsigned count=0;
 while(p<size&&text[p]!=26&&count<cap){
  size_t end=p;while(end<size&&text[end]!=26&&text[end]!=13&&end-p<columns)end++;
  if(end>=size)return 0;
  size_t next=end;
  if(text[end]==13){next=end+1;if(next<size&&text[next]==10)next++;}
  else if(text[end]!=26){
   size_t word=end;while(word>p&&text[word]!=32)word--;
   if(word>p){end=word;next=word+1;}
  }
  lines[count++]=(KeenTextLine){(uint16_t)p,(uint16_t)(end-p)};
  if(next==p)return 0;
  p=next;
 }
 return count;
}
#endif
