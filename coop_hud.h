#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
constexpr unsigned coopDisplayWidth=320,coopGameHeight=224,coopDisplayHeight=254;
// Fixed output geometry: preserve the game's height and add one footer row.
inline std::vector<uint32_t> coopDisplayFrame(const std::vector<uint32_t>& pixels,unsigned w,unsigned h) {
 std::vector<uint32_t> out(coopDisplayWidth*coopDisplayHeight,0);
 if(!w || !h || pixels.size()!=w*h)return out;
 for(unsigned y=0;y<coopGameHeight;y++)for(unsigned x=0;x<coopDisplayWidth;x++)
  out[y*coopDisplayWidth+x]=pixels[(y*h/coopGameHeight)*w+x*w/coopDisplayWidth];
 return out;
}
inline void drawCoopHud(std::vector<uint32_t>& pixels,unsigned w,unsigned h,const unsigned *s) {
 if(!s[0] || h<coopDisplayHeight || w<256 || pixels.size()!=w*h)return;
 // Reuse the native frame's gold border, lettering and meter surrounds.
 // Native gameplay is 256 pixels wide, normalized by the display composer.
 const std::vector<uint32_t> footer(pixels.begin()+194*w,pixels.begin()+224*w);
 auto box=[&](unsigned x,unsigned y,unsigned width,unsigned height,uint32_t color){
  for(unsigned yy=y;yy<std::min(h,y+height);yy++)for(unsigned xx=0;xx<w;xx++)
   if(xx*256/w>=x && xx*256/w<x+width)pixels[yy*w+xx]=color;
 };
 auto glyph=[&](unsigned x,unsigned y,const char *rows,uint32_t color){
  for(unsigned i=0;i<15;i++)if(rows[i]=='1')box(x+i%3,y+i/3,1,1,color);
 };
 const uint32_t power[6]={0x41658b,0x8b898b,0xcdcecd,0xcdcecd,0x8b898b,0x41658b};
 const uint32_t life[6]={0x620000,0x620000,0xac0000,0xac0000,0x620000,0x620000};
 for(unsigned p=0;p<2;p++) {
  unsigned dy=p*30;
  std::copy(footer.begin(),footer.end(),pixels.begin()+(194+dy)*w);
  // The original meters: 80 power pixels and 64 life pixels (4 HP each).
  for(unsigned y=0;y<6;y++) {
   box(48,205+dy+y,80,1,0);box(168,205+dy+y,64,1,0);
   box(48,205+dy+y,std::min(s[8+p],80u),1,power[y]);
   box(168,205+dy+y,(std::min(s[6+p],256u)+3)/4,1,life[y]);
  }
  uint32_t color=p?0xe05242:0x529be0;
  glyph(10,205+dy,"110101110100100",color);
  glyph(14,205+dy,p?"110001010100111":"010110010010111",color);
 }
}
