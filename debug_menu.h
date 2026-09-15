#pragma once
// Shared by standalone and BizHawk. Native Start shows the map; ABC opens cheats.
struct DebugMenu {
 unsigned paused=0,selected=0,previous=0,waitingRelease=0,stage=0,debug=0;
 const char *notice="";
 unsigned pack() const {return paused|((selected&7)<<1)|((selected&8)<<2)|(waitingRelease<<4)|(debug<<6)|(previous<<8)|(stage<<24);}
 void unpack(unsigned v){paused=v&1;debug=(v>>6)&1;selected=((v>>1)&7)|((v>>2)&8);waitingRelease=(v>>4)&1;previous=(v>>8)&65535;stage=(v>>24)&7;if(selected>11)selected=0;if(stage>5)stage=0;notice="";}
 bool update(unsigned buttons,int (*command)(unsigned),int (*loadStage)(unsigned),unsigned currentStage,bool nativePaused) {
  unsigned edge=buttons&~previous;previous=buttons;
  if(!debug)paused=nativePaused;
  if(edge&(1<<3)){debug=0;paused=0;selected=0;notice="";return false;}
  if(!debug && !nativePaused && (edge&(1<<10)))command(10); // Genesis X
  if(!nativePaused && !debug){paused=0;return false;}
  if(!debug){
   if((buttons&0x103)==0x103 && (edge&0x103)){debug=paused=1;selected=0;stage=currentStage<6?currentStage:0;notice="";}
   else return false;
   return true;
  }
  if(edge&(1<<4)){selected=(selected+11)%12;notice="";}
  if(edge&(1<<5)){selected=(selected+1)%12;notice="";}
  if(selected==10){if(edge&(1<<6))stage=(stage+5)%6;if(edge&(1<<7))stage=(stage+1)%6;}
  if(edge&(1<<8)) {
   if(!selected){debug=0;return false;}
   else if(selected>=10)notice=loadStage(selected==10?stage:currentStage)?"QUEUED - RESUME TO LOAD":"UNAVAILABLE";
   else notice=command(selected)?(selected<=4?"SETTING UPDATED":"QUEUED - RESUME TO APPLY"):"UNAVAILABLE";
  }
  return true;
 }
 void draw(std::vector<uint32_t>& out,unsigned w,unsigned h,unsigned flags) const {
  if(!debug || out.size()!=w*h || w<256 || h<224)return;
  BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-(int)h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
  void *bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
  if(!dc||!bm){if(bm)DeleteObject(bm);if(dc)DeleteDC(dc);return;}
  auto old=SelectObject(dc,bm);memcpy(bits,out.data(),w*h*4);
  RECT box={6,6,(LONG)w-6,219};HBRUSH brush=CreateSolidBrush(RGB(18,22,32));FillRect(dc,&box,brush);DeleteObject(brush);
  HFONT font=CreateFontA(-11,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,FIXED_PITCH,"Courier New");
  auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);
  auto line=[&](int y,const char *s,COLORREF color){SetTextColor(dc,color);TextOutA(dc,14,y,s,(int)strlen(s));};
  {
   line(12,"DEBUG CHEATS",RGB(255,215,110));
   const char *names[]={"BACK TO MAP","INVINCIBILITY","INFINITE POWER","INFINITE LIVES","FREEZE TIMER","HEAL BOTH","FILL POWER","DAMAGE P1","DAMAGE P2","DEFEAT BOTH","STAGE","RESTART STAGE"};
   for(unsigned i=0;i<12;i++) {
    char s[64];snprintf(s,sizeof(s),"%c %s%s",i==selected?'>':' ',names[i],i>=1&&i<=4?((flags&(1u<<(i-1)))?" [ON]":" [OFF]"):"");
    if(i==10)snprintf(s,sizeof(s),"%c STAGE: %u",selected==10?'>':' ',stage+1);
    line(29+i*12,s,i==selected?RGB(255,215,110):RGB(230,235,245));
   }
   line(177,notice,RGB(130,215,255));
   line(192,selected==10?"LEFT/RIGHT: STAGE  C: LOAD":"UP/DOWN: SELECT  C: APPLY",RGB(170,185,205));
   line(205,"START: RESUME",RGB(170,185,205));
  }
  GdiFlush();memcpy(out.data(),bits,w*h*4);SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
 }
};
