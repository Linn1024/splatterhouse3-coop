#pragma once
// Shared player-facing menu, using the game's own ROM font and idle artwork.
struct AppearanceMenu {
 template<class Core> static bool update(Core& c,unsigned buttons,bool cheats) {
  unsigned state=c.appearance_menu_get(),previous=state>>8,edge=buttons&~previous;
  unsigned available=c.appearance_available();
  bool open=state&1,release=state&8;unsigned row=(state>>1)&3;
  if(open && !available){open=false;release=true;} // Older map-menu saves.
  if(release) {if(!buttons)release=false;}
  else if(open) {
   if(edge&(1<<4))row=(row+3)%4;
   if(edge&(1<<5))row=(row+1)%4;
   if((edge&((1<<3)|1)) || (row==3 && (edge&(1<<8)))) {open=false;release=true;}
   else if(row<3 && (edge&((1<<6)|(1<<7)|(1<<8)))) {
    unsigned value=c.appearance_get(),shift=row*8,color=(value>>shift)&255;
    color=(color+((edge&(1<<6))?9:1))%10;
    c.appearance_set((value&~(255u<<shift))|(color<<shift));
   }
  } else if(!cheats &&
            (((available&4) && (edge&((1<<3)|1|(1<<8)))) ||
             ((available&1) && (edge&(1<<1)) && (buttons&0x103)!=0x103))) {open=true;row=0;}
  c.appearance_menu_set((buttons<<8)|(row<<1)|(open?1:0)|(release?8:0));
  return open || release;
 }
 template<class Core> static void draw(Core& c,std::vector<uint32_t>& out,unsigned w,unsigned h,bool cheats) {
  if(w!=320 || h<224 || out.size()!=w*h)return;
  bool open=c.appearance_menu_get()&1;
  if(!open && (!(c.appearance_available()&1) || cheats))return;
  auto box=[&](unsigned x,unsigned y,unsigned ww,unsigned hh,uint32_t rgb) {
   for(unsigned yy=y;yy<std::min(h,y+hh);yy++)for(unsigned xx=x;xx<std::min(w,x+ww);xx++)out[yy*w+xx]=rgb;
  };
  auto text=[&](unsigned x,unsigned y,const char *s,uint32_t rgb) {
   for(;*s;s++,x+=8)for(unsigned yy=0;yy<8;yy++) {
    unsigned shadow=c.font_row((unsigned char)*s,yy|8);
    unsigned bits=c.font_row((unsigned char)*s,yy);
    for(unsigned xx=0;xx<8;xx++)if(shadow&(128>>xx))box(x+xx,y+yy,1,1,0x620000);
    for(unsigned xx=0;xx<8;xx++)if(bits&(128>>xx))box(x+xx,y+yy,1,1,rgb);
   }
  };
  const uint32_t gold=0xcdce00,selected=0xeeeecc;
  if(!open){box(16,213,288,11,0);text(32,215,"B: COOP OPTIONS",gold);return;}
  box(8,24,304,184,gold);box(10,26,300,180,0);
  text(32,40,"PLAYER 2 APPEARANCE",gold);
  const char *names[]={"CLOTHES","MASK","SKIN","BACK"};
  const char *colors[]={"ORIGINAL","TEAL","MAGENTA","TAN","RED","BLUE","PURPLE","GREEN","GRAY","GOLD"};
  unsigned value=c.appearance_get(),row=(c.appearance_menu_get()>>1)&3;
  for(unsigned i=0;i<4;i++) {
   uint32_t color=i==row?selected:gold;
   text(32,64+i*24,names[i],color);
   if(i<3)text(112,64+i*24,colors[(value>>(i*8))%256%10],color);
  }
  // Native idle pose with the same per-material shading as gameplay.
  const uint32_t native[]={0,0,0xeeeecc,0xccccaa,0x888888,0x444444,0xeebb66,0xcc8844,0xaa6622,0x442200,0x446688,0x224466,0x002244,0x220000,0xaa0000,0x660000};
  for(unsigned y=0;y<72;y++)for(unsigned x=0;x<33;x++) {
   unsigned material=c.preview_ink(x,y),ink=material&15,legacy=ink==10?14:ink==11 || ink==12?15:ink==13?1:ink;
   if(ink)box(224+x*2,62+y*2,2,2,c.appearance_rgb(material,native[legacy]));
  }
  text(24,168,"LEFT/RIGHT: COLOR",gold);
  text(24,184,"A: BACK  START: BACK",gold);
 }
};
