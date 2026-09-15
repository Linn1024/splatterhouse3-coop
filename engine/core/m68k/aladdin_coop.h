/* Experimental emulator-assisted Aladdin (USA) campaign co-op.
 * Addresses are specific to SHA256 a3779fc7...0014ded3.
 * Disabled until the frontend explicitly enables it for that ROM.
 */
#include <string.h>
extern unsigned char work_ram[65536];
extern unsigned char cram[128], vram[65536];
extern unsigned char bg_name_dirty[0x800];
extern unsigned short bg_name_list[0x800], bg_list_index;
__declspec(dllexport) unsigned char *al_probe_cram(void) {return cram;}
__declspec(dllexport) unsigned char *al_probe_vram(void) {return vram;}
#define AL_SLOT (0x7e40 + 31 * 0x42)
#define AL_CARPET_SLOT (AL_SLOT-0x42)
static unsigned al_enabled, al_ready, al_second, al_anim_second, al_enemy_second;
static unsigned al_buttons[2], al_previous[2], al_ticks, al_rejoins[2], al_restart, al_complete;
static unsigned al_regs[16];
static unsigned al_hud[4]; /* second pass, sprite start, original HP, animation cursor */
static unsigned al_debug_flags, al_debug_pending;
/* Two players and two lamp meters exceed both native scanline budgets. */
__declspec(dllexport) unsigned al_coop_sprite_budget(void) {return al_enabled && al_ready;}
typedef struct { unsigned char ram[65536]; int camera_x, camera_y; } al_player;
static al_player al_players[2];
__declspec(dllexport) unsigned char *al_probe_player(unsigned p) {return p<2?al_players[p].ram:0;}
static const unsigned al_ranges[][2] = {
  {0x7dfa,12}, {0x7e40,0x42}, {0xeffa,1}, {0xeffc,12},
  {0xf07c,0x36}, {0xf0b6,0x48}, {0xf100,0x40}, {0xf167,1}, {0xf16a,1}
};
static unsigned al_r8(unsigned a) { return work_ram[a^1]; }
static unsigned al_r16(unsigned a) { return (al_r8(a)<<8)|al_r8(a+1); }
static unsigned al_r32(unsigned a) { return (al_r16(a)<<16)|al_r16(a+2); }
static void al_w8(unsigned a,unsigned v) { work_ram[a^1]=(unsigned char)v; }
static void al_w16(unsigned a,unsigned v) { al_w8(a,v>>8); al_w8(a+1,v); }
static void al_w32(unsigned a,unsigned v) { al_w16(a,v>>16); al_w16(a+2,v); }
static void al_release_sprite_tiles(unsigned a) {
  unsigned start=al_r32(a+0x2a)&65535,n=al_r8(a+0x29)+1,i;
  if(start<0xf008 || start>=0xf07c || n>0xf07c-start)return;
  if(al_r32(a+0x2e)!=0x8600+(start-0xf008)*128)return;
  for(i=0;i<n;i++)al_w8(start+i,0);
  al_w32(a+0x2a,0);al_w32(a+0x2e,0);
}
static void al_reclaim_iago_tiles(void) {
  unsigned char owned[0x74]={0};unsigned a,i,start,n;
  if(!al_ready || al_r8(0x7e26)!=12)return;
  /* Old Iago saves can contain allocations orphaned by the fixed-slot effect.
   * The sprite allocator's entire pool is owned by these 32 object records. */
  for(a=0x7e40;a<=AL_SLOT;a+=0x42)if(al_r8(a)) {
    start=al_r32(a+0x2a)&65535;n=al_r8(a+0x29)+1;
    if(start<0xf008 || start>=0xf07c || n>0xf07c-start)continue;
    for(i=0;i<n;i++)owned[start-0xf008+i]=1;
  }
  for(i=0;i<sizeof(owned);i++)if(!owned[i])al_w8(0xf008+i,0);
}
/* Recover the original animation tiles, including when loading a save made by
 * an older recolor. Trouser folds share skin inks and the sword shares white:
 * classify connected cloth regions before changing those colors. */
static unsigned al_rom8(unsigned a) {return m68k_read_pcrelative_8(a);}
static unsigned al_rom16(unsigned a) {return (al_rom8(a)<<8)|al_rom8(a+1);}
static void al_recolor_player_two(void) {
  static unsigned cached_frame, cached_size, cached_abu;
  static unsigned char tiles[4096], props[4096], pixels[65536], region[65536];
  static unsigned short positions[8192], queue[65536];
  static const unsigned char cloth[16]={0,7,7,7,7,5,6,7,4,9,10,15,15,7,8,15};
  unsigned frame=al_r32(AL_SLOT+0x14),start=al_r32(AL_SLOT+0x2e);
  unsigned bytes=(al_r8(AL_SLOT+0x29)+1)*128,i,j,n,used=0;
  unsigned abu=al_r8(0x7e26)==2 || al_r8(0x7e26)==6;
  if(al_r8(AL_SLOT)!=0x83 || !start || start>=65536 || bytes>65536-start ||
     bytes>sizeof(tiles) || frame<0x10000 || frame>0x1ffff0)return;
  al_w16(AL_SLOT+0x1e,(al_r16(AL_SLOT+0x1e)&~0x6000)|0x6000);
  if(frame!=cached_frame || bytes!=cached_size || abu!=cached_abu) {
    cached_frame=0;cached_size=0;
    n=al_rom16(frame)+1;
    if(n>32 || frame+6+n*12>0x200000)return;
    memset(pixels,0,sizeof(pixels));memset(region,0,sizeof(region));
    memset(tiles,0,sizeof(tiles));memset(props,0,sizeof(props));
    for(i=0;i<n;i++) {
      unsigned a=frame+6+i*12,shape=al_rom16(a),w,h,size,source,x,y;
      if(shape>0xfff6)return;
      w=al_rom8(shape+8);h=al_rom8(shape+9);size=al_rom16(shape+4);
      source=(al_rom8(a+5)|(al_rom8(a+7)<<8)|((al_rom8(a+9)&127)<<16))*2;
      if(!w || !h || w>32 || h>32 || (w&7) || (h&7) ||
         size!=w*h/2 || used+size>bytes || source+size>0x200000)return;
      for(j=0;j<size;j++) {
        tiles[used+j]=al_rom8(source+j);
        /* The idle apple has four independent 8x8 artwork tiles. */
        if(size==32 && source>=0xad9a0 && source<=0xada00)props[used+j]=1;
        /* Separate torso pieces in the idle cycle contain vest, not face. */
        if(source==0x7d780 || source==0x93880)props[used+j]=2;
        /* Apple-toss torso: only the first 8x16 column is vest. The
         * remaining columns include a purple hand shadow to preserve. */
        if(source==0xfb2a0 && j<64)props[used+j]=2;
      }
      for(x=0;x<w;x++)for(y=0;y<h;y++) {
        unsigned offset=used+((x/8)*(h/8)+y/8)*32+(y&7)*4+(x&7)/2;
        unsigned pos=((al_rom8(a+3)+y)&255)*256+((al_rom8(a+2)+x)&255);
        unsigned ink=(tiles[offset]>>((x&1)?0:4))&15;
        positions[offset*2+(x&1)]=pos;
        if(ink)pixels[pos]=ink;
      }
      used+=size;
    }
    /* A broad white interior distinguishes trousers from the thin blade and
     * tiny facial highlights. Follow the cream/grey folds across tile seams. */
    for(i=0;i<65536;i++)if(!region[i] &&
        (pixels[i]==1 || pixels[i]==2 || pixels[i]==14)) {
      unsigned head=0,tail=1,white=0,warm=0,broad=0,k;
      queue[0]=i;region[i]=3;
      while(head<tail) {
        unsigned pos=queue[head++],neighbors[4]={pos-256,pos+256,pos-1,pos+1};
        if(pixels[pos]==14) {
          white++;
          if((pos&255)<255 && pos<65280 && pixels[pos+1]==14 &&
             pixels[pos+256]==14 && pixels[pos+257]==14)broad=1;
        }
        if(pixels[pos]!=14)warm++;
        for(k=0;k<4;k++) {
          unsigned next=neighbors[k],ink;
          if(next>=65536 || (k==2 && !(pos&255)) || (k==3 && (pos&255)==255))continue;
          ink=pixels[next];
          if(!region[next] && (ink==1 || ink==2 || ink==14)) {
            region[next]=3;queue[tail++]=next;
          }
        }
      }
      for(j=0;j<tail;j++)region[queue[j]]=(broad && (white>=16 || (white>=4 && warm>=2)))?2:1;
    }
    /* Bent legs and overlapping swords split off small fabric fragments.
     * Recover cream-edged white fragments near the established trousers, even
     * when they are too small for the main broad-interior test. Keep secondary
     * regions separate so proximity cannot spread repeatedly toward the face. */
    {
      unsigned top=256;
      for(i=0;i<65536;i++)if(region[i]==2 && (i>>8)<top)top=i>>8;
      for(i=0;i<65536;i++)if(region[i]==1) {
        unsigned head=0,tail=1,white=0,warm=0,near=0,min_y=255,k;
        queue[0]=i;region[i]=5;
        while(head<tail) {
          unsigned pos=queue[head++],neighbors[4]={pos-256,pos+256,pos-1,pos+1};
          if(pixels[pos]==14)white++;else warm++;
          if((pos>>8)<min_y)min_y=pos>>8;
          for(k=0;k<4;k++) {
            unsigned next=neighbors[k];
            if(next>=65536 || (k==2 && !(pos&255)) || (k==3 && (pos&255)==255))continue;
            if(region[next]==1){region[next]=5;queue[tail++]=next;}
          }
        }
        if(white && warm && min_y+3>=top) {
          for(j=0;j<tail && !near;j++) {
            int dx,dy,x=queue[j]&255,y=queue[j]>>8;
            for(dy=-8;dy<=8 && !near;dy++)for(dx=-8;dx<=8;dx++)
              if(x+dx>=0 && x+dx<256 && y+dy>=0 && y+dy<256 && region[(y+dy)*256+x+dx]==2){near=1;break;}
          }
        }
        for(j=0;j<tail;j++)region[queue[j]]=near?6:5;
      }
    }
    /* Grey outline inks are also used by the sword and climbing details.
     * Only recolor the immediate cloth border; never flood through greys into
     * another material. Exposed limbs retain skin colors; enclosed warm islands
     * within trousers are handled separately below. */
    for(i=0;i<65536;i++)if(pixels[i]==12 || pixels[i]==13) {
      int dx,dy;
      for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++) {
        int x=(i&255)+dx,y=(i>>8)+dy;
        if(x>=0 && x<256 && y>=0 && y<256 && (region[y*256+x]==2 || region[y*256+x]==6))
          region[i]=4;
      }
    }
    /* Work inside the fabric silhouette rather than following warm colors
     * into the waistband or limbs. The same patch includes cream interior
     * pixels and a tan rim, which must be classified as a complete shape. */
    unsigned cloth_top=256,cloth_bottom=0,left[256],right[256];
    for(i=0;i<256;i++){left[i]=256;right[i]=0;}
    for(i=0;i<65536;i++)if(region[i]==2 || region[i]==6) {
      unsigned x=i&255,y=i>>8;
      if(y<cloth_top)cloth_top=y;if(y>cloth_bottom)cloth_bottom=y;
      if(x<left[y])left[y]=x;if(x>right[y])right[y]=x;
    }
    for(i=0;i<256;i++)if(left[i]<256) {
      while(left[i] && pixels[i*256+left[i]-1]>=1 && pixels[i*256+left[i]-1]<=4)left[i]--;
      while(right[i]<255 && pixels[i*256+right[i]+1]>=1 && pixels[i*256+right[i]+1]<=4)right[i]++;
    }
    for(i=0;i<65536;i++)if(pixels[i]>=1 && pixels[i]<=4 &&
        (i&255)>=left[i>>8] && (i&255)<=right[i>>8])region[i]=15;
    for(i=0;i<65536;i++)if(region[i]==15 && (pixels[i]==3 || pixels[i]==4) &&
        (i>>8)*2>=cloth_top+cloth_bottom) {
      unsigned head=0,tail=1,minx=i&255,maxx=minx,miny=i>>8,maxy=miny;
      queue[0]=i;region[i]=18;
      while(head<tail) {
        unsigned pos=queue[head++];int dx,dy,x=pos&255,y=pos>>8;
        if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
        for(dy=-2;dy<=2;dy++)for(dx=-2;dx<=2;dx++) {
          unsigned next;
          if(x+dx<0 || x+dx>255 || y+dy<0 || y+dy>255)continue;
          next=(y+dy)*256+x+dx;
          if(region[next]==15 && (pixels[next]==3 || pixels[next]==4) &&
             (y+dy)*2>=cloth_top+cloth_bottom){region[next]=18;queue[tail++]=next;}
        }
      }
      if(tail>=3 && tail<=40 && maxx-minx<=12 && maxy-miny<=12 && maxx>minx && maxy>miny) {
        unsigned row;
        for(row=miny?miny-1:0;row<=maxy+1 && row<256;row++) {
          unsigned lo=256,hi=0,x;
          for(j=0;j<tail;j++)if(abs((int)(queue[j]>>8)-(int)row)<=1) {
            x=queue[j]&255;if(x<lo)lo=x;if(x>hi)hi=x;
          }
          if(lo==256)continue;if(lo)lo--;if(hi<255)hi++;
          for(x=lo;x<=hi;x++) {
            unsigned pos=row*256+x;
            if(pixels[pos]>=1 && pixels[pos]<=4 &&
               (region[pos]==15 || region[pos]==18))region[pos]=10;
          }
        }
      }
    }
    /* Purple is shared by vest panels and thin sword edges. Only cloth-sized
     * regions or regions adjoining the body use the blue vest color. */
    for(i=0;i<65536;i++)if(pixels[i]==11 && !region[i]) {
      unsigned head=0,tail=1,broad=0,skin=0,k;
      queue[0]=i;region[i]=8;
      while(head<tail) {
        unsigned pos=queue[head++],neighbors[4]={pos-256,pos+256,pos-1,pos+1};
        if((pos&255)<255 && pos<65280 && pixels[pos+1]==11 &&
           pixels[pos+256]==11 && pixels[pos+257]==11)broad=1;
        for(k=0;k<4;k++) {
          unsigned next=neighbors[k];
          if(next>=65536 || (k==2 && !(pos&255)) || (k==3 && (pos&255)==255))continue;
          if(pixels[next]==3 || pixels[next]==4)skin++;
          if(pixels[next]==11 && !region[next]){region[next]=8;queue[tail++]=next;}
        }
      }
      for(j=0;j<tail;j++)region[queue[j]]=(broad || (tail>=4 && skin>=3))?7:8;
    }
    /* Recover narrow vest panels near established cloth, but do not walk
     * upward into facial shadows or across silver sword-edge pixels. */
    {
      unsigned vest_top=256;
      for(i=0;i<65536;i++)if(region[i]==7 && (i>>8)<vest_top)vest_top=i>>8;
      for(i=0;i<65536;i++)if(pixels[i]==11 && region[i]==8 && (i>>8)>=vest_top) {
        int dx,dy,x=i&255,y=i>>8,metal=0,near=0,skin=0,hair=0;
        for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)if(x+dx>=0 && x+dx<256 && y+dy>=0 && y+dy<256) {
          unsigned ink=pixels[(y+dy)*256+x+dx];if(ink==12 || ink==13)metal=1;
          if(ink>=1 && ink<=7)skin++;if(ink==15)hair++;
        }
        if(metal || skin>=5 || hair>=4)continue;
        for(dy=-2;dy<=2 && !near;dy++)for(dx=-2;dx<=2;dx++)
          if(x+dx>=0 && x+dx<256 && y+dy>=0 && y+dy<256 && region[(y+dy)*256+x+dx]==7){near=1;break;}
        if(near)region[i]=16;
      }
    }
    for(i=0;i<used*2;i++) {
      unsigned shift=(i&1)?0:4,ink=(tiles[i/2]>>shift)&15,pos=positions[i];
      unsigned x=pos&255,y=pos>>8;
      unsigned value=(region[pos]==2 || region[pos]==4 || region[pos]==6 || region[pos]==15 || region[pos]==18)?cloth[ink]:(ink==11?((region[pos]==7 || region[pos]==16)?10:11):ink==8?8:ink==3?2:ink==4?3:ink);
      if(region[pos]==10)value=ink; /* Keep the native tan patch. */
      /* Vertical-rope poses split tiny cuff/fold highlights from the main
       * trouser region with the sword and exposed legs. Below the waist,
       * native cream/white inks are fabric; skin uses 3..7 and steel 12/13. */
      if(frame>=0x1e6ae6 && frame<=0x1e6d50 && y>=114 &&
         (ink==1 || ink==2 || ink==14))value=cloth[ink];
      /* Nine bending/recovering idle poses put the knee fold next to the
       * patch. Use the native patch outline, not proximity to warm inks:
       * those inks also draw the adjacent folds and trouser border. */
      if(frame>=0x1ec8e8 && frame<=0x1ecbac) {
        static const unsigned char patch_left[4][5]={
          {127,126,126,126,127}, {126,125,125,126,0},
          {127,125,125,125,127}, {127,125,125,125,127}};
        static const unsigned char patch_right[4][5]={
          {129,130,131,131,130}, {129,130,130,130,0},
          {128,129,130,130,129}, {128,130,130,130,129}};
        unsigned layout=frame==0x1ec8e8?0:frame==0x1ec936?1:frame==0x1ecbac?3:2;
        unsigned top=(layout==0 || layout==1 || layout==3)?130:129;
        unsigned patch=y>=top && y<top+5 && patch_left[layout][y-top] &&
          x>=patch_left[layout][y-top] && x<=patch_right[layout][y-top];
        if(ink>=1 && ink<=4 && patch)value=ink;
        else if(region[pos]==10 || (ink>=1 && ink<=4 &&
          x>=121 && x<=130 && y>=129 && y<=137))value=cloth[ink];
      }
      /* The Genie shares this composite descriptor with Aladdin. */
      if(frame==0x1e8034 && (pos&255)>=130 && (pos>>8)<124)value=ink;
      /* Final teleport frames contain only magic, not trouser fabric. */
      if(frame>=0x1ed230 && frame<=0x1ed2e4)value=ink;
      if(props[i/2]==1)value=ink;
      if(props[i/2]==2 && ink==11)value=10;
      /* The apple-toss cycle keeps the torso at these coordinates while
       * repacking it with different head/arm tiles. Its narrow front panel
       * has no broad purple region for the general classifier to follow.
       * This material mask excludes the face, hand, apple and sword. */
      if(frame>=0x1ec27c && frame<=0x1ec6b4 && ink==11 &&
         ((x>=127 && x<=131 && y>=101 && y<=113) || (x==121 && y==100)))value=10;
      /* One descending-apple frame packs the apple into an arm/body tile. */
      if(frame==0x1ec5dc && (pos&255)>=144 && (pos&255)<=148 &&
         (pos>>8)>=93 && (pos>>8)<=97)value=ink;
      if(abu)value=(ink>=1 && ink<=6)?ink+1:ink; /* Darker fur; keep eyes and clothing. */
      tiles[i/2]=(tiles[i/2]&~(15<<shift))|(value<<shift);
    }
    cached_frame=frame;cached_size=bytes;cached_abu=abu;
  }
  for(i=0;i<bytes;i++)if(vram[start+(i^1)]!=tiles[i]) {
    unsigned tile=(start+i)>>5;
    vram[start+(i^1)]=tiles[i];
    if(!bg_name_dirty[tile])bg_name_list[bg_list_index++]=tile;
    bg_name_dirty[tile]=255;
  }
}
__declspec(dllexport) void al_probe_recolor(void) {al_recolor_player_two();}
static void al_capture(al_player *p) {
  unsigned i,j; for(i=0;i<sizeof(al_ranges)/sizeof(al_ranges[0]);i++)
    for(j=0;j<al_ranges[i][1];j++) p->ram[al_ranges[i][0]+j]=al_r8(al_ranges[i][0]+j);
  p->camera_x=al_r16(0x7df6); p->camera_y=al_r16(0x7df8);
}
static unsigned al_p16(al_player *p,unsigned a) {return (p->ram[a]<<8)|p->ram[a+1];}
static void al_pw16(al_player *p,unsigned a,unsigned v) {p->ram[a]=v>>8;p->ram[a+1]=v;}
static void al_install(al_player *p) {
  unsigned i,j; for(i=0;i<sizeof(al_ranges)/sizeof(al_ranges[0]);i++)
    for(j=0;j<al_ranges[i][1];j++) {
      unsigned a=al_ranges[i][0]+j;
      /* Rooftops flutes unlock shared ropes, regardless of who collected them. */
      if(al_r8(0x7e26)==0 && a>=0xf126 && a<=0xf12a)continue;
      /* The optional Rooftops flute selects the shared snake's supply route. */
      if(al_r8(0x7e26)==0 && a==0xf116)continue;
      /* The palace key removes one shared carpet covering the passage. */
      if(al_r8(0x7e26)==10 && a==0xf11c)continue;
      /* Scripted carpets have one shared destination, even when P2 boards. */
      if(a>=0xf094 && a<=0xf097)continue;
      /* Rug Ride duck cues belong to the shared obstacle script. */
      if(al_r8(0x7e26)==8 && a==0xf10b)continue;
      al_w8(a,p->ram[a]);
    }
  al_w16(0x7dfa,al_r16(0x7dfa)+p->camera_x-al_r16(0x7df6));
  al_w16(0x7dfc,al_r16(0x7dfc)+p->camera_y-al_r16(0x7df8));
}
static void al_input(unsigned p) {
  unsigned b=al_players[p].ram[0xeffa]?al_buttons[p]:0,hi=0xff,lo=0xff;
  /* Frontend bits use Libretro joypad IDs. */
  if(b&(1<<4)) hi&=~1; if(b&(1<<5)) hi&=~2;
  if(b&(1<<6)) hi&=~4; if(b&(1<<7)) hi&=~8;
  if(b&(1<<0)) hi&=~16; /* Genesis B */
  if(b&(1<<8)) hi&=~32; /* Genesis C */
  if(b&(1<<1)) lo&=~16; /* Genesis A */
  al_w8(0xf155,lo);al_w8(0xf156,hi);
}
static void al_return(void) { m68k.pc=al_r32(m68k.dar[15]&65535);m68k.dar[15]+=4; }
static int al_grounded(al_player *p) {
  return p->ram[0xf0c1] && !p->ram[0xf0be] && !p->ram[0xf0e6] &&
         !p->ram[0xf0f6] && p->ram[0xeffa];
}
/* Genie and Abu token collection both play SFX 64 and leave the 1B7B5C
 * sparkle animation. Use disposable native objects, without collecting a token
 * or touching bonus flags, score, camera targets, or either player's tiles. */
static void al_teleport_sparkles(unsigned x,unsigned y) {
  static const signed char offsets[4][2]={{-10,-28},{10,-16},{-4,-8},{4,-40}};
  unsigned a,n=0,i,t=0x1b7b5c;
  for(a=0x7e82;a<AL_CARPET_SLOT && n<4;a+=0x42)if(!al_r8(a)) {
    al_release_sprite_tiles(a);
    for(i=0;i<0x42;i++)al_w8(a+i,0);
    al_w16(a,al_rom16(t));
    for(i=0;i<4;i++)al_w8(a+6+i,al_rom8(t+2+i));
    for(i=0;i<4;i++)al_w8(a+10+i,al_rom8(t+6+i));
    al_w16(a+0x1e,al_rom16(t+10));
    for(i=0;i<4;i++)al_w8(a+0x20+i,al_rom8(t+12+i));
    al_w8(a+0x29,al_rom8(t+16));al_w8(a+0x35,al_rom8(t+17));al_w8(a+0x3c,al_rom8(t+18));
    al_w16(a+2,x+offsets[n][0]);al_w16(a+4,y+offsets[n][1]);n++;
  }
}
static void al_teleport_effect(al_player *p) {
  al_teleport_sparkles(al_p16(p,0x7e42),al_p16(p,0x7e44));
  al_players[0].ram[1]=1; /* Pending native SFX call; serialized outside installed RAM. */
}
/* Actions are queued at a frontend frame boundary and applied at the next
 * gameplay loop, never halfway through a player's simulation. */
__declspec(dllexport) unsigned al_coop_debug_flags(void) {return al_debug_flags;}
__declspec(dllexport) int al_coop_debug(unsigned action) {
  if(!al_enabled || !al_ready || al_restart || al_complete || action<1 || action>10 || action==9) return 0;
  if(action==10) {al_debug_flags^=8;return 1;} /* Death: partner / checkpoint. */
  if(action==8) {
    al_debug_flags^=4;
    if(!(al_debug_flags&4))al_debug_pending|=1u<<8;
    return 1;
  }
  if((al_debug_flags&4) && action>=4) return 0;
  if(action==1 || action==2) {al_debug_flags^=1u<<(action-1);return 1;}
  if(action==4 && !al_grounded(&al_players[0])) return 0;
  al_debug_pending|=1u<<action;
  if(action>=5) al_debug_flags&=~1u; /* Damage tests turn protection off. */
  return 1;
}
static void al_debug_apply(void) {
  unsigned p;
  if(al_debug_pending&(1u<<8)) {
    /* Preserve each player's health and private sprite allocation while placing
     * both at the scouting position. Gravity/collisions resume from this point. */
    unsigned char allocation[8],health=al_players[1].ram[0xeffa];
    memcpy(allocation,al_players[1].ram+0x7e6a,8);
    al_players[1]=al_players[0];
    memcpy(al_players[1].ram+0x7e6a,allocation,8);
    memset(al_players[1].ram+0x7e54,0,4);
    al_players[1].ram[0xeffa]=health;
    for(p=0;p<2;p++) {
      memset(al_players[p].ram+0xf0b6,0,0x36);
      al_pw16(&al_players[p],0x7e60,0x12);al_pw16(&al_players[p],0x7e62,al_r8(0x7e26)==8?0x2350:0x1e7c);
      al_players[p].ram[0x7e77]=0;
      al_pw16(&al_players[p],0x7e58,0);al_pw16(&al_players[p],0x7e5a,0);
      al_players[p].ram[0xf0f2]=60;
    }
    al_teleport_effect(&al_players[0]);
  }
  if(al_debug_pending&(1u<<4)) {
    al_w16(0x7e0a,al_r16(0x7dfa)+(al_r16(0x7df6)&15));
    al_w16(0x7e0c,al_r16(0x7dfc)+(al_r16(0x7df8)&15));
    al_w16(0x7e0e,al_r16(0x7df6)&0xfff0);
    al_w16(0x7e10,al_r16(0x7df8)&0xfff0);
  }
  for(p=0;p<2;p++) {
    al_player *v=&al_players[p];
    if(al_debug_pending&(1u<<3)) {v->ram[0xeffa]=8;v->ram[0xf0e6]=0;}
    if(al_debug_pending&(1u<<(6+p))) {
      if(v->ram[0xeffa])v->ram[0xeffa]--;
    }
    if(al_debug_pending&(1u<<5)) {v->ram[0xeffa]=0;v->ram[0xf0e6]=255;}
    /* Damage protection must not cancel a scripted death. Drowning removes
     * the player object before the next gameplay loop; clearing its death
     * flag here used to strand a full-health, invisible player forever. */
    else if((al_debug_flags&1) && v->ram[0xeffa] && !v->ram[0xf0e6] &&
            v->ram[0x7e40]==0x83) {v->ram[0xeffa]=8;v->ram[0xf0f2]=60;}
  }
  if(al_debug_flags&2) {al_w8(0xefe0,'9');al_w8(0xefe1,'9');}
  al_debug_pending=0;
}
static void al_noclip_move(void) {
  al_player *p=&al_players[0];unsigned b=al_buttons[0];
  int x=al_p16(p,0x7e42),y=al_p16(p,0x7e44);
  x+=4*((!!(b&(1<<7)))-(!!(b&(1<<6))));
  y+=4*((!!(b&(1<<5)))-(!!(b&(1<<4))));
  if(x<16)x=16;if(x>(int)al_r16(0x7db8)-32)x=al_r16(0x7db8)-32;
  if(y<288)y=288;if(y>(int)al_r16(0x7dbc)+240)y=al_r16(0x7dbc)+240;
  al_pw16(p,0x7e42,x);al_pw16(p,0x7e44,y);
  al_pw16(p,0x7e02,x);al_pw16(p,0x7e04,y);
  al_pw16(p,0x7dfa,x-p->camera_x);al_pw16(p,0x7dfc,y-p->camera_y);
  al_pw16(p,0x7e58,0);al_pw16(p,0x7e5a,0);
  if(b&(1<<6))p->ram[0x7e49]=255;if(b&(1<<7))p->ram[0x7e49]=0;
  p->ram[0xf167]=0;
}
/* Snapshot RAM 0..4 and 0200..02FF are metadata, outside installed ranges.
 * Existing snapshots leave them zero; keep the state layout compatible. */
/* Native spring animations temporarily disable collision for everybody.
 * Keep their visual animation shared, but give each player its own refractory
 * period. Metadata is outside installed RAM and already covered by saves. */
static unsigned al_spring(unsigned a) {
  unsigned t=al_r8(a),anim=al_r32(a+0x20);
  if(t==1 || (t==0x84 && anim>=0x122db2 && anim<0x122dd8))return 1;
  if(t==0x4e || (t==0x84 && anim>=0x124b1a && anim<0x124b3a))return 0x4e;
  if(t==0x4f || (t==0x84 && anim>=0x124b3e && anim<0x124b6e))return 0x4f;
  if(t==0x65 || t==0x66)return 0x65;
  return 0;
}
static unsigned al_spring_cooldown(unsigned a,unsigned set) {
  unsigned index=(a-0x7e40)/0x42,m=0x200+index*8,t=al_spring(a);
  al_player *p=&al_players[al_second];
  if(index>=32 || !t)return 0;
  if(p->ram[m+5]!=t || al_p16(p,m+1)!=al_r16(a+2) || al_p16(p,m+3)!=al_r16(a+4))p->ram[m]=0;
  if(set) {
    p->ram[m]=t==1?24:t==0x4f?36:20;
    al_pw16(p,m+1,al_r16(a+2));al_pw16(p,m+3,al_r16(a+4));p->ram[m+5]=t;
  }
  return p->ram[m];
}
static int al_window_hand(unsigned a) {
  unsigned animation=al_r32(a+0x20);
  if((al_r8(a)==0x84 || al_r8(a)==0x0e) && animation>=0x123d34 && animation<0x123de2)return 64;
  /* Hidden window knife thrower: FD $30, then native type $06. */
  if((al_r8(a)==0x84 || al_r8(a)==0x06) && animation>=0x123200 && animation<0x123274)return 48;
  return 0;
}
static unsigned al_carpet(unsigned a) {
  unsigned type=al_r8(a),linked=al_r32(a+0x3e)&65535;
  if(type==0x5e || type==0x60 || type==0x61)return a;
  if(type==0x84 && linked>=0x7e82 && linked<AL_CARPET_SLOT &&
     (linked-0x7e40)%0x42==0 && al_r32(linked+0x3e)==0xff0000+a &&
     (al_r8(linked)==0x5e || al_r8(linked)==0x60 || al_r8(linked)==0x61))return linked;
  return 0;
}
static int al_targeted_object(unsigned a) {
  return al_r8(a) && (al_r8(a)<0x7f || al_window_hand(a) || al_carpet(a));
}
static int al_enemy_targets_two(unsigned a) {
  int x=al_r16(a+2),y=al_r16(a+4),d0,d1;
  if(!al_players[1].ram[0xeffa])return 0;
  if(!al_players[0].ram[0xeffa])return 1;
  if(al_carpet(a)) {
    unsigned carpet=al_carpet(a),p;
    for(p=0;p<2;p++) {
      unsigned riding=al_players[p].ram[0xf0d3];
      if(al_players[p].ram[0xeffa] && (riding==0x5e || riding==0x60 || riding==0x61) &&
         abs((int)al_p16(&al_players[p],0x7e42)-(int)al_r16(carpet+2))<48 &&
         abs((int)al_p16(&al_players[p],0x7e44)-(int)al_r16(carpet+4))<48)return p;
    }
  }
  /* Native window triggers use a horizontal band. Prefer either live
   * player inside that band, even if the other is closer on a different floor. */
  if(al_window_hand(a)) {
    int range=al_window_hand(a);
    if(abs(x-(int)al_p16(&al_players[0],0x7e42))<=range)return 0;
    if(abs(x-(int)al_p16(&al_players[1],0x7e42))<=range)return 1;
  }

  d0=abs(x-(int)al_p16(&al_players[0],0x7e42))+abs(y-(int)al_p16(&al_players[0],0x7e44));
  d1=abs(x-(int)al_p16(&al_players[1],0x7e42))+abs(y-(int)al_p16(&al_players[1],0x7e44));
  return d1<d0;
}
static void al_rejoin(unsigned who, unsigned revive) {
  al_player *p=&al_players[who],*other=&al_players[1-who];
  unsigned char allocation[8],health=p->ram[0xeffa];unsigned i;
  if(!revive && !health)return;
  if(!al_grounded(other) && !(al_r8(0x7e26)==8 && other->ram[0xeffa] && !other->ram[0xf0e6])) return;
  for(i=0;i<8;i++) allocation[i]=p->ram[0x7e6a+i];
  *p=*other;
  for(i=0;i<8;i++) p->ram[0x7e6a+i]=allocation[i];
  for(i=0;i<4;i++) p->ram[0x7e54+i]=0;
  p->ram[0]=0;
  p->ram[0xeffa]=revive?8:health;
  p->ram[0xf0e6]=0;p->ram[0xf0f2]=60;
  al_pw16(p,0x7e5e,0x6000);
  al_rejoins[who]++;
  al_teleport_effect(p);
}
__declspec(dllexport) void al_coop_enable(unsigned enabled) {
  al_enabled=enabled;al_ready=al_second=al_anim_second=al_enemy_second=al_ticks=0;
  al_restart=al_complete=al_previous[0]=al_previous[1]=al_rejoins[0]=al_rejoins[1]=0;
  memset(al_hud,0,sizeof(al_hud));
  al_debug_flags=al_debug_pending=0;
}
__declspec(dllexport) void al_coop_buttons(unsigned p,unsigned buttons) { if(p<2) al_buttons[p]=buttons; }
__declspec(dllexport) void al_coop_status(unsigned *out) {
  out[0]=al_ready;out[1]=al_ticks;out[2]=al_second;
  out[3]=al_r16(0x7e42);out[4]=al_r16(0x7e44);
  out[5]=al_p16(&al_players[1],0x7e42);out[6]=al_p16(&al_players[1],0x7e44);
}
__declspec(dllexport) void al_coop_details(unsigned *out) {
  out[0]=al_players[0].ram[0xeffa];out[1]=al_players[1].ram[0xeffa];
  out[2]=al_rejoins[0];out[3]=al_rejoins[1];out[4]=0;out[5]=0;
}
__declspec(dllexport) unsigned al_coop_level(void) {return al_r8(0x7e26);}
__declspec(dllexport) unsigned al_coop_ready(void) {return al_ready;}
__declspec(dllexport) unsigned al_coop_transition_pending(void) {return al_complete;}
__declspec(dllexport) int al_coop_debug_stage(unsigned level) {
  if(!al_enabled || !al_ready || al_restart || level>12)return 0;
  al_complete=level+2;al_debug_flags&=11;al_debug_pending=0;return 1;
}
static int al_camera_real_x,al_camera_real_y;
/* Versioned extension to the upstream emulator state. Includes mid-frame passes. */
#include <string.h>
typedef struct {
  unsigned magic, version;
  unsigned enabled, ready, second, anim_second, enemy_second;
  unsigned buttons[2], previous[2], ticks, rejoins[2], restart, complete, regs[16];
  al_player players[2];
  int camera_x, camera_y;
  unsigned hud[4];
  unsigned debug_flags, debug_pending;
} al_snapshot;
__declspec(dllexport) unsigned al_coop_state_size(void) {return sizeof(al_snapshot);}
__declspec(dllexport) int al_coop_save(void *data, unsigned size) {
  al_snapshot s;
  if(!data || size!=sizeof(s)) return 0;
  memset(&s,0,sizeof(s));s.magic=0x414c4331;s.version=3;
  s.debug_flags=al_debug_flags;s.debug_pending=al_debug_pending;
  memcpy(s.hud,al_hud,sizeof(al_hud));
#define AL_SAVE(field) s.field=al_##field
  AL_SAVE(enabled);AL_SAVE(ready);AL_SAVE(second);AL_SAVE(anim_second);AL_SAVE(enemy_second);
  AL_SAVE(ticks);AL_SAVE(restart);AL_SAVE(complete);
#undef AL_SAVE
  memcpy(s.buttons,al_buttons,sizeof(al_buttons));memcpy(s.previous,al_previous,sizeof(al_previous));
  memcpy(s.rejoins,al_rejoins,sizeof(al_rejoins));memcpy(s.regs,al_regs,sizeof(al_regs));
  memcpy(s.players,al_players,sizeof(al_players));s.camera_x=al_camera_real_x;s.camera_y=al_camera_real_y;
  memcpy(data,&s,sizeof(s));return 1;
}
__declspec(dllexport) int al_coop_restore(const void *data, unsigned size) {
  al_snapshot s;
  if(!data || size!=sizeof(s)) return 0;
  memcpy(&s,data,sizeof(s));
  if(s.magic!=0x414c4331 || s.version!=3 || s.enabled>1 || s.ready>1 || s.second>1 || s.anim_second>1 || s.enemy_second>2 || s.restart>3 || s.hud[0]>1 || s.complete>14 || s.debug_flags>15 || (s.debug_pending&~0x1f8u)) return 0;
  al_debug_flags=s.debug_flags;al_debug_pending=s.debug_pending;
  memcpy(al_hud,s.hud,sizeof(al_hud));
#define AL_LOAD(field) al_##field=s.field
  AL_LOAD(enabled);AL_LOAD(ready);AL_LOAD(second);AL_LOAD(anim_second);AL_LOAD(enemy_second);
  AL_LOAD(ticks);AL_LOAD(restart);AL_LOAD(complete);
#undef AL_LOAD
  memcpy(al_buttons,s.buttons,sizeof(al_buttons));memcpy(al_previous,s.previous,sizeof(al_previous));
  memcpy(al_rejoins,s.rejoins,sizeof(al_rejoins));memcpy(al_regs,s.regs,sizeof(al_regs));
  memcpy(al_players,s.players,sizeof(al_players));al_camera_real_x=s.camera_x;al_camera_real_y=s.camera_y;
  /* Migrate old saves that kept a collected flute in only one player context. */
  if(al_ready && al_r8(0x7e26)==0) {
    unsigned a;for(a=0xf126;a<=0xf12a;a++)
      al_w8(a,al_r8(a)|al_players[0].ram[a]|al_players[1].ram[a]);
  }
  al_reclaim_iago_tiles();
  if(al_ready && al_r8(0x7e26)==10)
    al_w8(0xf11c,al_r8(0xf11c)|al_players[0].ram[0xf11c]|al_players[1].ram[0xf11c]);
  return 1;
}
/* The native game only tests flute-dependent rope spawns on a newly streamed
 * row/column. The midpoint camera can keep a basket visible throughout pickup,
 * so retry visible pending ropes at the end of the Rooftops stage update.
 * Use the original predicate, allocator, template and animation. The native
 * MOVEM/RTS epilogue restores our registers and returns to the stage caller;
 * all temporary state lives on the emulated stack, including in save states. */
static int al_rooftop_rope(void) {
  unsigned cols=al_r16(0x7db6),rows=al_r16(0x7dbc)/16,x,y,i;
  unsigned cx=al_r16(0x7df6)/16,cy=al_r16(0x7df8)/16;
  static const unsigned entry[5]={0x1b70f8,0x1b712c,0x1b7158,0x1b717c,0x1b71a0};
  for(y=cy;y<rows && y<cy+16;y++)for(x=cx;x<cols && x<cx+23;x++) {
    unsigned tile=al_r16((y*cols+x)*2)/2,tag;
    if(tile>65535-0xae87)continue;
    tag=al_r8(0xae87+tile);
    if(tag<0xb6 || tag>0xba || !al_r8(0xf126+tag-0xb6))continue;
    if(tag==0xb6 && (al_r8(0xf127)||al_r8(0xf128)))continue;
    if(tag==0xb7 && al_r8(0xf128))continue;
    m68k.dar[15]-=60;
    for(i=0;i<15;i++)al_w32((m68k.dar[15]+i*4)&65535,m68k.dar[i]);
    m68k.dar[15]-=4;al_w32(m68k.dar[15]&65535,0x1adb56);
    m68k.dar[2]=tile;m68k.dar[3]=tag;m68k.dar[10]=0xffae87;
    al_w16(0x7db0,x*16);al_w16(0x7db2,y*16);
    al_w16(0xf150,0xfff0);al_w16(0xf152,0xf0);
    m68k.pc=entry[tag-0xb6];return 1;
  }
  return 0;
}
static void al_coop_hook(void) {
  unsigned pc=m68k.pc,i;
  if(!al_enabled) return;
  /* The Genie slot machine polls controller port 1 directly, outside the
   * gameplay input routine. Combine either player's A/B/C at those reads. */
  if(pc==0x1b190e) {
    unsigned b=al_buttons[0]|al_buttons[1];
    if(b&(1<<0))m68k.dar[0]&=~16u;
    if(b&(1<<8))m68k.dar[0]&=~32u;
  }
  if(pc==0x1b1954 && ((al_buttons[0]|al_buttons[1])&(1<<1)))m68k.dar[0]&=~16u;
  /* Extend the original title-screen Options list. The extra row sits above
   * Difficulty, so Up from Difficulty reaches Death and Down returns to it. */
  if(pc==0x1b409a) {
    const char *label=(al_debug_flags&8)?"DEATH: CHECKPOINT":"DEATH: PARTNER   ";
    unsigned command=al_r32(0x8680+12*4),address=(command&0x3fff)|((command>>16&3)<<14);
    for(i=0;label[i];i++) {
      unsigned tile=0x87c0+((label[i]-32)|al_r16(0xeff0)),a=address+(10+i)*2;
      if(a<65535){vram[a^1]=tile>>8;vram[(a+1)^1]=tile;}
    }
  }
  if(pc==0x1b42be) {al_w16(0xf13c,6);m68k.pc=0x1b42c6;return;}
  if(pc==0x1b42f0) {m68k.pc=al_r16(0xf13c)==7?0x1b42fa:0x1b42c6;return;}
  if(pc==0x1b43e0 && al_r16(0xf13c)==6) {
    al_w16(0x7ec8,0x164);al_w16(0x7ec6,0xb4);al_return();return;
  }
  if(pc==0x1b410c && al_r16(0xf13c)==6) {
    if(!al_r8(0xeffd)){al_debug_flags^=8;al_w8(0xeffd,255);}
    m68k.pc=0x1b428a;return;
  }
  if(pc==0x1b5b92 && al_ready && !al_second && al_r8(0x7e26)==0 &&
     !(al_debug_flags&4) && al_rooftop_rope())return;
  if(pc==0x1a8b50) {
    /* Native level loading/cutscenes own RAM until the gameplay loop resumes. */
    al_ready=al_second=al_anim_second=al_enemy_second=0;
    al_restart=2; /* New stages spawn both players at the native safe position. */
    al_complete=0;al_debug_pending=0;al_debug_flags&=11;
    memset(al_hud,0,sizeof(al_hud));
  }
  if(pc==0x1a8c16 && !al_second) {
    if(!al_ready) {
      al_capture(&al_players[0]);memset(al_players[0].ram+0x200,0,288);memset(al_players[0].ram,0,5); al_players[1]=al_players[0];
      unsigned offset=al_restart==2?0:48;
      al_pw16(&al_players[1],0x7dfa,al_p16(&al_players[1],0x7dfa)+offset);
      al_pw16(&al_players[1],0x7e02,al_p16(&al_players[1],0x7e02)+offset);
      al_pw16(&al_players[1],0x7e42,al_p16(&al_players[1],0x7e42)+offset);
      for(i=0x7e6a;i<0x7e72;i++) al_players[1].ram[i]=0; /* Allocate independent sprite tiles. */
      for(i=0x7e54;i<0x7e58;i++) al_players[1].ram[i]=0; /* Force first tile upload. */
      al_pw16(&al_players[1],0x7e5e,0x6000); /* Original colors with private clothing-ink remap. */
      al_ready=1;al_restart=0;
    }
    al_capture(&al_players[0]);
    al_debug_apply();
    if(al_debug_flags&4)al_noclip_move();
    for(i=0;i<2;i++) {
      if(!(al_debug_flags&4) && (al_buttons[i]&(1<<10)) && !(al_previous[i]&(1<<10))) al_rejoin(i,0);
      al_previous[i]=al_buttons[i];
    }
    al_install(&al_players[0]);
    al_input(0);
  }
  if(!al_ready) return;
  /* The native key predicate suppresses this carpet on the next map stream.
   * With a midpoint camera it can remain loaded after pickup. Retire the
   * existing instance through the native allocator's cleanup as well. */
  if(pc==0x1ac7dc && al_r8(0x7e26)==10 && al_r8(0xf11c)) {
    unsigned a=m68k.dar[9]&65535;
    if(al_r8(a)==0x5f && al_r8(a+0x34)==0x82) {
      m68k.dar[15]-=4;al_w32(m68k.dar[15]&65535,0x1ac846);
      m68k.pc=0x1abe6e;return;
    }
  }
  /* Native DBRA counts 31..0. Render slots 0,31,1..30, including when a
   * blink/hidden branch skips directly to the native loop epilogue. */
  if(pc==0x1aba20) {
    unsigned remaining=m68k.dar[0]&65535;
    if(remaining<=31)m68k.dar[10]=0xff0000+(remaining==31?0x7e40:remaining==30?AL_SLOT:0x7e40+(30-remaining)*0x42);
  }
  if(pc==0x1a8c20 && !al_second && al_players[0].ram[1]) {
    al_players[0].ram[1]=0;
    /* Run the original sound-only tail of the token pickup. Its RTS reaches
     * the native MOVEM/RTS epilogue, restoring every register and this PC. */
    m68k.dar[15]-=4;al_w32(m68k.dar[15]&65535,pc);
    m68k.dar[15]-=60;
    for(i=0;i<15;i++)al_w32((m68k.dar[15]+i*4)&65535,m68k.dar[i]);
    m68k.dar[15]-=4;al_w32(m68k.dar[15]&65535,0x1adb56);
    m68k.pc=0x1af2d6;return;
  }

  /* Iago reinitializes the same effect slot every 128 ticks. Free its previous
   * allocation before native 1AE30A clears the ownership pointers. */
  if((pc==0x1b6302 || pc==0x1b632a) && al_r8(0x7e26)==12)
    al_release_sprite_tiles(0x7e82);
  /* Coal flames arm after a delay. Keep each player's delayed flame personal,
   * otherwise the trailing runner is burned by the leading runner's flame.
   * Snapshot metadata 0300..031F survives saves without changing their size. */
  if(pc==0x1b5332) {
    unsigned a,owner=1+(al_second || al_enemy_second);
    for(a=0x7e82;a<AL_SLOT;a+=0x42)if(al_r8(a)==0x8c) {
      unsigned pending=al_players[0].ram[0x300+(a-0x7e40)/0x42];
      if(!pending || pending==owner){m68k.pc=0x1b536a;return;}
    }
    m68k.pc=0x1b533e;return;
  }
  if(pc==0x1ae30a || pc==0x1b535a) {
    unsigned a=m68k.dar[13]&65535;
    if(a>=0x7e40 && a<=AL_SLOT && (a-0x7e40)%0x42==0)
      al_players[0].ram[0x300+(a-0x7e40)/0x42]=pc==0x1b535a?1+(al_second || al_enemy_second):0;
  }
  if(pc==0x1ae9d4) {
    unsigned a=m68k.dar[9]&65535;
    if(a>=0x7e82 && a<AL_SLOT && (a-0x7e40)%0x42==0 && al_r8(a)==0x7b) {
      unsigned owner=al_players[0].ram[0x300+(a-0x7e40)/0x42];
      if(owner && owner!=1+(al_second || al_enemy_second)){m68k.pc=0x1ae9d8;return;}
    }
  }
  /* Debug protection must not pin the native hurt-blink on its invisible frame. */
  if(pc==0x1aba14 && (al_debug_flags&5)){m68k.pc=0x1aba20;return;}
  if(al_r8(0x7e26)==8 && !(al_debug_flags&4)) {
    /* The native rug routine advances the shared camera and obstacle script.
     * Only P1 runs that part. P2 steers an independently rendered second rug. */
    if(pc==0x1a9d18 && al_second) {
      unsigned x=al_p16(&al_players[0],0x7e42)-48;
      int y=al_r16(0x7dfc);
      y+=4*((!!(al_buttons[1]&(1<<5)))-(!!(al_buttons[1]&(1<<4))));
      if(y<0x112)y=0x112;if(y>0x187)y=0x187;
      al_w16(0x7dfa,x-al_r16(0x7df6));al_w16(0x7dfc,y);
      al_w16(0x7e02,x);al_w16(0x7e42,x);al_return();return;
    }
    if(pc==0x1aa8fa){al_camera_real_x=al_camera_real_y=0;al_return();return;}
    if((pc==0x1ade5e || pc==0x1ac7dc) && (m68k.dar[9]&65535)==AL_CARPET_SLOT) {
      m68k.pc=pc==0x1ade5e?0x1ae0a6:0x1ac846;return;
    }
    if(pc==0x1ab7c4) {
      for(i=0;i<0x42;i++)al_w8(AL_CARPET_SLOT+i,al_r8(0x7e82+i));
      al_w16(AL_CARPET_SLOT+2,al_p16(&al_players[1],0x7e42));
      al_w16(AL_CARPET_SLOT+4,al_p16(&al_players[1],0x7e44));
    }
  }
  if(al_debug_flags&4) {
    /* Freeze simulation, including enemy collisions, but keep native camera,
     * background streaming and sprite rendering running while scouting. */
    if(pc==0x1a8c20){m68k.pc=al_second?0x1a8cc6:0x1a8c9e;return;}
    if(pc==0x1aa8fa) {
      int cx=al_r16(0x7df6),cy=al_r16(0x7df8);
      int tx=(int)al_r16(0x7e42)-160,ty=(int)al_r16(0x7e44)-368,dx,dy;
      if(tx<16)tx=16;if(tx>(int)al_r16(0x7db8)-352)tx=al_r16(0x7db8)-352;
      /* Match the native camera's strict height-240 bound. At that exact
       * boundary the column streamer reads a 16-tile column starting one
       * row too low, treating memory beyond the map as tile offsets. */
      if(ty<16)ty=16;if(ty>(int)al_r16(0x7dbc)-241)ty=al_r16(0x7dbc)-241;
      dx=tx-cx;dy=ty-cy;if(dx>4)dx=4;if(dx< -4)dx=-4;if(dy>4)dy=4;if(dy< -4)dy=-4;
      al_w16(0x7df6,cx+dx);al_w16(0x7df8,cy+dy);
      al_w16(0x7dfa,al_r16(0x7dfa)-dx);al_w16(0x7dfc,al_r16(0x7dfc)-dy);
      al_w16(0xf0b2,al_r16(0xf0b2)+dx);al_w16(0xf0b4,al_r16(0xf0b4)+dy);
      if(dx)al_w8(dx<0?0xf0b9:0xf0ba,255);
      if(dy)al_w8(dy<0?0xf0bb:0xf0bc,255);
      al_w16(0x7dfe,160);al_w16(0x7e00,368);al_return();return;
    }
    if(pc==0x1a8ca2){m68k.pc=0x1a8cc6;return;}
    if(pc==0x1aba20 && ((m68k.dar[10]&65535)==AL_SLOT ||
       (al_r8(0x7e26)==8 && (m68k.dar[10]&65535)==AL_CARPET_SLOT))){m68k.pc=0x1abb20;return;}
  }
  /* The native lives icon/counter now displays the shared respawn budget. */
  /* Draw the native lamp meter twice. Reuse its animation and health-dependent
   * tiles, then move P2's sprites into the former score area. */
  if(pc==0x1ab9bc) {
    if(!al_hud[0]) {
      al_hud[0]=1;al_hud[1]=m68k.dar[9]&65535;
      al_hud[2]=al_r8(0xeffa);al_hud[3]=al_r32(0x7da8);
      al_w8(0xeffa,al_players[1].ram[0xeffa]);m68k.pc=0x1ab90a;return;
    }
    for(i=al_hud[1];i<(m68k.dar[9]&65535);i+=8)
      al_w16(i+6,al_r16(i+6)+160);
    al_w8(0xeffa,al_hud[2]);al_w32(0x7da8,al_hud[3]);al_hud[0]=0;
    m68k.dar[1]&=~0x0f00;m68k.pc=0x1aba0a;return;
  }
  if(pc==0x1ac782) al_recolor_player_two(); /* Tile DMA has finished. */
  /* This terrain/gravity pass starts at slot 1: it advances shared objects,
   * never the primary player. A second pass doubles falling acceleration and
   * can resolve a barrel/stone bounce twice. Player collisions still run for
   * both players in the following 1ABB40 call. */
  if(pc==0x1abbe6 && al_r8(m68k.dar[9]&65535)==0x84 && al_spring(m68k.dar[9]&65535)) {
    m68k.pc=0x1abbee;return;
  }
  if(pc==0x1abc96) {
    unsigned t=al_spring(m68k.dar[9]&65535);
    if(t){m68k.dar[1]=t;al_w8(0xf0f6,t);}
  }
  if(pc==0x1abc9e && al_spring_cooldown(m68k.dar[9]&65535,0)) {
    m68k.pc=0x1abca0;return;
  }
  if(pc==0x1afc04 && !al_spring_cooldown(m68k.dar[9]&65535,0)) {m68k.pc=0x1afc0a;return;}
  if(pc==0x1afc22 || pc==0x1afc7a || pc==0x1afcf2 || pc==0x1afdbc)
    al_spring_cooldown(m68k.dar[9]&65535,1);
  if(pc==0x1adb5c && al_second) {al_return();return;}
  if(pc==0x1ade5e) {
    if(al_second) m68k.dar[4]=0; /* Only advance this player's object in second pass. */
    else if((m68k.dar[9]&65535)==AL_SLOT) m68k.pc=0x1ae0a6;
    else {
      unsigned a=m68k.dar[9]&65535;
      if(a!=0x7e40 && al_targeted_object(a)) {
        if(al_enemy_targets_two(a)) {
          al_capture(&al_players[0]);al_install(&al_players[1]);al_enemy_second=1;
        }
      }
    }
  }
  if(pc==0x1ae0a6 && al_enemy_second==1) {
    al_capture(&al_players[1]);al_install(&al_players[0]);al_enemy_second=0;
  }
  if(pc==0x1aa8fa) {
    int other_x=(int)al_p16(&al_players[1-al_second],0x7e42)-(int)al_r16(0x7df6);
    int other_y=(int)al_p16(&al_players[1-al_second],0x7e44)-(int)al_r16(0x7df8);
    int sx=al_r16(0x7dfa);
    unsigned live=al_r8(0xeffa) && !al_r8(0xf0e6) && al_r8(0x7e40);
    al_player *other=&al_players[1-al_second];
    unsigned other_live=other->ram[0xeffa] && !other->ram[0xf0e6] && other->ram[0x7e40];
    if(!live || !other_live) {
      if(al_second){al_return();return;}
      /* Native scrolling follows the survivor, with no separation limits
       * against the falling body. Preserve P1's real position afterwards. */
      al_camera_real_x=al_camera_real_y=0;
      if(!live && other_live) {
        al_camera_real_x=(int)al_r16(0x7dfa)-other_x;
        al_camera_real_y=(int)al_r16(0x7dfc)-other_y;
        al_w16(0x7dfa,other_x);al_w16(0x7dfc,other_y);
      }
      if(!live && !other_live){al_return();return;}
      al_w16(0x7dfe,160);
      return;
    }
    /* The original exit begins at screen X=300 once the camera reaches the
     * right map boundary. Keep both visible while allowing that final approach. */
    int right_limit=al_r16(0x7df6)>=(int)al_r16(0x7db8)-360?304:280;
    if(sx>other_x+240) sx=other_x+240;
    if(sx<other_x-240) sx=other_x-240;
    if(sx<32) sx=32; if(sx>right_limit) sx=right_limit;
    al_w16(0x7dfa,sx);
    /* Only limit an airborne ascent. A grounded player's zero velocity used
     * to satisfy this condition: a partner descending a slope then pushed
     * the stationary player downward through the floor to enforce separation. */
    if((int)al_r16(0x7dfc)<other_y-120 && (short)al_r16(0x7e5a)<0 &&
       al_r8(0xf0be) && !al_r8(0xf0c0)) {
      int previous_y=(int)al_p16(&al_players[al_second],0x7e44)-(int)al_r16(0x7df8);
      int limit=other_y-120;
      if(limit>previous_y)limit=previous_y; /* Never snap down through a ledge. */
      if((int)al_r16(0x7dfc)<limit)al_w16(0x7dfc,limit);
      al_w16(0x7e5a,0);al_w8(0xf0c0,255); /* End ascent; let gravity take over. */
    }
    if(al_second) {al_return();return;}
    /* A settled look-up pose from either player can request upward scrolling.
     * Save P1's own target so releasing P2 Up does not latch it permanently. */
    al_pw16(&al_players[0],2,al_r16(0x7e00));al_players[0].ram[4]=1;
    if(al_players[1].ram[0xf0df] && al_players[1].ram[0xeffa] &&
       al_p16(&al_players[1],0x7e00)>al_r16(0x7e00))
      al_w16(0x7e00,al_p16(&al_players[1],0x7e00));
    al_camera_real_x=al_r16(0x7dfa);al_camera_real_y=al_r16(0x7dfc);
    al_w16(0x7dfa,(al_camera_real_x+al_p16(&al_players[1],0x7e42)-(int)al_r16(0x7df6))/2);
    al_w16(0x7dfc,(al_camera_real_y+al_p16(&al_players[1],0x7e44)-(int)al_r16(0x7df8))/2);
    if((al_r16(0x7dfa)>160 && (al_camera_real_x<28 || other_x<28)) ||
       (al_r16(0x7dfa)<160 && (al_camera_real_x>292 || other_x>292))) al_w16(0x7dfa,160);
    if((al_r16(0x7dfc)<al_r16(0x7e00) && (al_camera_real_y>432 || other_y>432)) ||
       (al_r16(0x7dfc)>al_r16(0x7e00) && (al_camera_real_y<304 || other_y<304)))
      al_w16(0x7dfc,al_r16(0x7e00));
    al_camera_real_x-=al_r16(0x7dfa);al_camera_real_y-=al_r16(0x7dfc);
    al_w16(0x7dfe,160);
  }
  if(pc==0x1a8ca2 && !al_second) {
    if(al_players[0].ram[4]){al_w16(0x7e00,al_p16(&al_players[0],2));al_players[0].ram[4]=0;}
    al_w16(0x7dfa,al_r16(0x7dfa)+al_camera_real_x);
    al_w16(0x7dfc,al_r16(0x7dfc)+al_camera_real_y);
  }
  if(pc==0x1a8f0c) { /* MVP death handling is coordinated after both updates. */
    if((al_restart==1 || al_restart==3) && !al_second) {
      /* Enter the original world rebuild with this call's return address intact.
       * The game's shared checkpoint words at 7e0a..7e10 survive the rebuild. */
      unsigned exhausted=al_restart==3;
      al_ready=0;al_restart=2;
      m68k.pc=exhausted?0x1a9088:0x1a90ca;return;
    }
    /* Also recover old saves whose drowning flag was erased by invincibility.
     * A missing gameplay object cannot be revived by restoring HP alone. */
    if(al_r8(0xf0e6) || (!al_r8(0x7e40) && !al_r8(0xf0e9)))al_w8(0xeffa,0);
    if(al_r16(0x7e04)>al_r16(0x7dbc)+256) {al_w8(0xeffa,0);al_w8(0xf0e6,255);}
    m68k.pc=0x1a8f02;return;
  }
  if(pc==0x1a8e3e) {
    if(al_complete && !al_second && !(al_debug_flags&4)) {
      unsigned requested=al_complete;
      al_ready=0;al_complete=0;al_anim_second=al_enemy_second=0;
      if(requested>=2) {
        static const unsigned sequence_after[13]={0x408e,0x4082,0x4086,0x408a,0x4096,0x409a,0x409e,0x40a2,0x40a6,0x40aa,0x40ae,0x40b6,0x40b2};
        al_w8(0x7e26,requested-2);al_w32(0xf572,sequence_after[requested-2]);
        m68k.pc=0x1a8ed8;return;
      }
      al_w8(0xf0e9,255);m68k.pc=0x1a8e5c;return;
    }
    /* Retain each player's native exit request until the paired gate agrees. */
    if(al_r8(0xf0e9) && al_r8(0xf0e9)!=255 && !(al_debug_flags&4)) {
      unsigned delay=al_r8(0xf0e9)-1;al_w8(0xf0e9,delay?delay:255);
    }
    m68k.pc=0x1a8f02;return;
  }
  if(al_second && pc==0x1a8f04) {m68k.pc=0x1a8f02;return;}
  if(pc==0x1a8cca) {
    if(!al_second) {
      al_capture(&al_players[0]);
      for(i=0;i<16;i++) al_regs[i]=m68k.dar[i];
      al_install(&al_players[1]);al_input(1);al_second=1;m68k.pc=(al_debug_flags&4)?0x1a8cc6:0x1a8c20;
    } else {
      al_capture(&al_players[1]);
      al_pw16(&al_players[1],0x7e5e,0x6000);
      if(!(al_debug_flags&4)) {
        if(al_r8(0x7e26)==1) {
          if(al_p16(&al_players[0],0x7e42)>=0x1288 && al_p16(&al_players[1],0x7e42)>=0x1288 &&
             al_p16(&al_players[0],0x7e44)<0x1d6 && al_p16(&al_players[1],0x7e44)<0x1d6)al_complete=1;
        } else if((al_players[0].ram[0xf0e9]==255 || al_players[1].ram[0xf0e9]==255) &&
          ((al_r8(0x7e26)==12 || al_r8(0x7e26)==11) ||
          (abs((int)al_p16(&al_players[0],0x7e42)-(int)al_p16(&al_players[1],0x7e42))<=128 &&
          abs((int)al_p16(&al_players[0],0x7e44)-(int)al_p16(&al_players[1],0x7e44))<=128)))
          al_complete=1;
      }
      /* Keep the native fatal-hit / water-effect frames on screen before
       * resolving death. Timers belong to players and survive mid-death saves. */
      for(i=0;i<2;i++) {
        al_player *p=&al_players[i];
        unsigned j;for(j=0;j<32;j++)if(p->ram[0x200+j*8])p->ram[0x200+j*8]--;
        if(p->ram[0xeffa])p->ram[0]=0;
        else if(p->ram[0]<60)p->ram[0]++;
      }
      if(!al_restart) {
        unsigned dead0=!al_players[0].ram[0xeffa],dead1=!al_players[1].ram[0xeffa];
        unsigned checkpoint=(al_debug_flags&8) || (dead0 && dead1);
        unsigned who=dead0?0:1;
        al_player *survivor=&al_players[1-who];
        unsigned safe=al_grounded(survivor) || (al_r8(0x7e26)==8 &&
          survivor->ram[0xeffa] && !survivor->ram[0xf0e6]);
        if((dead0 || dead1) && (!dead0 || al_players[0].ram[0]>=60) &&
           (!dead1 || al_players[1].ram[0]>=60) && (checkpoint || safe)) {
          /* One payment per recovery, including a simultaneous team wipe.
           * Native lives are spare retries: at zero the next death opens the
           * original continue/game-over flow. Never wrap the ASCII counter. */
          unsigned lives=al_r8(0x7e3c);
          if(lives<='0') {al_w8(0x7e3c,'0');al_restart=3;}
          else {
            al_w8(0x7e3c,lives-1);
            if(checkpoint)al_restart=1;
            else al_rejoin(who,1);
          }
        }
      }
      for(i=0;i<0x42;i++) al_w8(AL_SLOT+i,al_players[1].ram[0x7e40+i]);
      al_install(&al_players[0]);al_input(0);al_second=0;al_ticks++;
      for(i=0;i<16;i++) m68k.dar[i]=al_regs[i];
    }
  }
  /* Keep a defeated character in the native hit sequence, then its final
   * stagger pose, instead of returning to the living idle animation. Water
   * deaths remove the object and use their own independently animated splash. */
  if(pc==0x1ac7dc && (m68k.dar[9]&65535)==AL_SLOT && al_r8(0x7e26)==8) {
    unsigned animation=al_r32(AL_SLOT+0x20);
    /* The second carpet's landing collision can select the free-ride loop,
     * which lacks the scripted duck branch used by this stage. */
    if(animation>=0x122336 && animation<0x122350) {
      al_w32(AL_SLOT+0x20,0x122350);al_w8(AL_SLOT+0x37,0);
    }
  }
  if(pc==0x1ac7dc) {
    unsigned a=m68k.dar[9]&65535,who=a==AL_SLOT?1:0;
    al_player *p=&al_players[who];
    if((a==0x7e40 || a==AL_SLOT) && al_r8(a)==0x83 && !p->ram[0xeffa] && p->ram[0]) {
      unsigned animation=al_r32(a+0x20);
      if(p->ram[0]>=20) {al_w32(a+0x20,0x1226e0);al_w8(a+0x37,255);}
      else if(animation<0x1226ce || animation>0x1226e0) {
        al_w32(a+0x20,0x1226ce);al_w8(a+0x37,0);
      }
    }
  }
  if(pc==0x1aba20) {
    unsigned a=m68k.dar[10]&65535,who=a==AL_SLOT?1:0;
    if(a==AL_SLOT && !(al_debug_flags&5) && (al_players[1].ram[0xf0f2]&1)) {
      m68k.pc=0x1abb20;return;
    }
    if((a==0x7e40 || a==AL_SLOT) && !al_players[who].ram[0xeffa] && al_players[who].ram[0]>=40) {
      m68k.pc=0x1abb20;return;
    }
  }
  /* Enemy attack decisions also execute in animation bytecode, outside the
   * movement pass. Use the same live target for both native interpreters. */
  if(pc==0x1ac7dc && !al_anim_second && !al_enemy_second) {
    unsigned a=m68k.dar[9]&65535;
    if(a!=0x7e40 && a!=AL_SLOT && al_targeted_object(a) && al_enemy_targets_two(a)) {
      al_capture(&al_players[0]);al_install(&al_players[1]);al_enemy_second=2;
    }
  }
  if(pc==0x1ac846 && al_enemy_second==2) {
    al_capture(&al_players[1]);al_install(&al_players[0]);al_enemy_second=0;
  }
  /* Animation bytecode also touches player globals. Give it the right context. */
  if(pc==0x1ac7dc && (m68k.dar[9]&65535)==AL_SLOT) {
    al_capture(&al_players[0]);al_install(&al_players[1]);al_anim_second=1;
    al_w8(0xf0d8,0);al_w8(0xf0d9,0);al_w8(0xf0da,0);
  }
  if(pc==0x1ac846 && al_anim_second) {
    al_capture(&al_players[1]);
    for(i=0;i<0x42;i++) al_players[1].ram[0x7e40+i]=al_r8(AL_SLOT+i);
    al_install(&al_players[0]);al_anim_second=0;
  }
}
