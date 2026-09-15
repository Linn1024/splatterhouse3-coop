/* Emulator-assisted Splatterhouse 3 USA. Explicit opt-in; original ROM untouched. */
#include <string.h>
extern unsigned char zstate;
extern void gen_zbusreq_w(unsigned int data,unsigned int cycles);
extern unsigned char work_ram[65536], vram[65536], cram[128], reg[32];
static unsigned sh_enabled, sh_ready, sh_second, sh_render_second, sh_frames;
static unsigned sh_enemy_active, sh_target, sh_collision_repeat;
static unsigned sh_transition, sh_saved_hp, sh_saved_power;
static unsigned sh_camera_x, sh_camera_active;
static unsigned sh_render_done,sh_render_resume,sh_render_index;
static unsigned sh_regs[16], sh_input[3];
static unsigned char sh_players[2][65536], sh_tiles[2][0xc00];
/* Reserved snapshot metadata, outside every installed game-RAM range. */
#define SH_SAT_SCHEMA (sh_players[0][0x306])
#define SH_DOOR_LEADER (sh_players[0][0x307]) /* 1 blue, 2 red; zero legacy save */
#define SH_MAP_BACKUP (sh_players[0][0x308])
#define SH_MAP_SCHEMA (sh_players[0][0x309])
#define SH_DOOR_ENTRY (sh_players[0][0x30a]) /* Remains set through exit's last visible frame. */
#define SH_AI_TARGET (sh_players[0][0x300])
#define SH_OBJECT_OWNER(i) (sh_players[0][0x400+(i)])
#define SH_DEBUG_FLAGS (sh_players[0][0x301])
#define SH_DEBUG_PENDING (sh_players[0][0x302])
#define SH_DEBUG_STAGE (sh_players[0][0x303])
#define SH_SAT_BUILD(i) (sh_players[0][0x500+(i)])
#define SH_SAT_LIVE(i) (sh_players[0][0x550+(i)])
extern unsigned char bg_name_dirty[0x800];
extern unsigned short bg_name_list[0x800], bg_list_index;
static unsigned sh_r8(unsigned a) { return work_ram[(a & 65535)^1]; }
static unsigned sh_r16(unsigned a) { return (sh_r8(a)<<8)|sh_r8(a+1); }
static unsigned sh_r32(unsigned a) { return (sh_r16(a)<<16)|sh_r16(a+2); }
static void sh_w8(unsigned a,unsigned v) { work_ram[(a & 65535)^1]=(unsigned char)v; }
static void sh_w16(unsigned a,unsigned v) { sh_w8(a,v>>8);sh_w8(a+1,v); }
static void sh_w32(unsigned a,unsigned v) { sh_w16(a,v>>16);sh_w16(a+2,v); }
__declspec(dllexport) unsigned char *sh_probe_vdp_regs(void) { return reg; }
static void sh_copy_range(unsigned p,unsigned a,unsigned n,unsigned install) {
  unsigned i;for(i=0;i<n;i++) {
    if(install)sh_w8(a+i,sh_players[p][a+i]);
    else sh_players[p][a+i]=sh_r8(a+i);
  }
}
static void sh_context(unsigned p,unsigned install) {
  unsigned o;
  sh_copy_range(p,0xa0,0x14,install); /* lives at B4 remain shared */
  sh_copy_range(p,0xba,10,install);
  sh_copy_range(p,0xcd,6,install);
  sh_copy_range(p,0x18e,4,install);
  sh_copy_range(p,0x21d2,0x43,install);
  for(o=0;o<=0xa00;o+=0x80)sh_copy_range(p,0x5520+o,2,install);
  for(o=0xa80;o<=0xd80;o+=0x100)sh_copy_range(p,0x5522+o,4,install);
  sh_copy_range(p,0x64a0,2,install);
  sh_copy_range(p,0x6520,2,install);
}
static unsigned sh_p16(unsigned p,unsigned a) { return (sh_players[p][a]<<8)|sh_players[p][a+1]; }
static void sh_setp16(unsigned p,unsigned a,unsigned v) { sh_players[p][a]=v>>8;sh_players[p][a+1]=v; }
static unsigned sh_scene(void) { return sh_r8(1)==4 && sh_r8(2)>=8 && sh_r8(2)<=10; }
static void sh_dirty_vram(void) {
  unsigned i;bg_list_index=0;
  for(i=0;i<0x800;i++){bg_name_dirty[i]=255;bg_name_list[bg_list_index++]=i;}
}
static unsigned sh_rom8(unsigned a) { return m68k_read_pcrelative_8(a); }
static unsigned sh_rom16(unsigned a) { return (m68k_read_pcrelative_8(a)<<8)|m68k_read_pcrelative_8(a+1); }
static unsigned sh_rom32(unsigned a) { return (sh_rom16(a)<<16)|sh_rom16(a+2); }
/* Rebuild a native 7726 graphics list for old saves whose pause map overwrote
 * enemy patterns. No object state, health, palette or room progression changes. */
static void sh_repair_sheet(unsigned sheet) {
  unsigned a,dst,n,block,src,i,j,bits,word,count,start;
  unsigned char unpacked[8192];
  if(sheet>=128)return;
  a=sh_rom32(0x55b1c+sheet*4);if(a>=0x200000)return;
  dst=sh_rom16(a)*32;a+=2;
  for(n=0;n<2048 && a<0x200000;n++) {
    block=sh_rom8(a++);
    if(block==255)break;
    if(block==253){dst=sh_rom16(a)*32;a+=2;continue;}
    start=0;count=32;
    if(block==254)memset(unpacked,0,32);
    else {
      src=sh_rom32(0x120000+block*4);if(src>=0x200000)return;
      for(i=0;i<256;i++) {
        bits=sh_rom16(src);src+=2;
        for(j=0;j<16;j++) {
          if((bits&0x8000) && j>=2 && (bits&(1u<<(15-j))))
            word=(unpacked[i*32+j*2-4]<<8)|unpacked[i*32+j*2-3];
          else {if(src+2>0x200000)return;word=sh_rom16(src);src+=2;}
          unpacked[i*32+j*2]=word>>8;unpacked[i*32+j*2+1]=word;
        }
      }
      start=sh_rom8(a++)*32;count=sh_rom8(a++)*32;
    }
    if(dst+count>65536 || start+count>8192)return;
    for(i=0;i<count;i++)vram[(dst+i)^1]=unpacked[start+i];
    dst+=count;
  }
}
static void sh_repair_map_art(void) {
  unsigned i,a,room=sh_r32(0x184),stage=sh_r8(0x166);
  SH_MAP_SCHEMA=1;
  if(stage>6 || room>=0x200000)return;
  /* The native map's name table leaves this repeated fill in enemy VRAM. */
  for(i=0;i<64;i+=2)if(vram[0x6000+i]!=0 || vram[0x6001+i]!=0xe4)return;
  a=sh_rom32(sh_rom32(0x5610e + stage*4)+m68k_read_pcrelative_8(room)*4);
  if(a>=0x200000 || sh_rom16(a)!=0xfffe)return;
  a+=2;
  for(i=0;i<32 && a+6<0x200000 && sh_rom16(a)!=0xfffd;i++,a+=6)
    if(sh_rom16(a)!=65535)sh_repair_sheet(sh_rom16(a));
  sh_dirty_vram();
}
static void sh_repair_map_palette(void) {
  unsigned i,different=0;
  if(sh_r8(1)!=4 || sh_r8(2)!=10 || sh_r8(0x78) || (sh_r8(9)&64))return;
  /* Old map exits left palette 8 active while the room's native target
   * palette remained correct. This shared bank belongs to regular enemies
   * as well as bosses. Recover only that exact stale-map signature; current
   * map cycles preserve both palette buffers before entering the map. */
  for(i=0;i<32;i++) {
    if(sh_r8(0x4d6e + i)!=sh_rom8(0x361f2+8*32+i))return;
    if(sh_r8(0x4d6e + i)!=sh_r8(0x4dee + i))different=1;
  }
  if(different) {
    for(i=0;i<32;i++)sh_w8(0x4d6e + i,sh_r8(0x4dee + i));
    sh_w8(6,sh_r8(6)|8);
  }
}
static void sh_restore_map(void) {
  if(!SH_MAP_BACKUP)return;
  memcpy(vram,&sh_players[0][0x8000],0x8000);
  memcpy(vram+0x8000,&sh_players[1][0x8000],0x8000);
  memcpy(&SH_SAT_BUILD(0),&sh_players[0][0x7000],160);
  if(SH_MAP_BACKUP==2) {
    unsigned i;for(i=0;i<256;i++)sh_w8(0x4d0e + i,sh_players[0][0x70a0+i]);
    sh_w8(6,sh_r8(6)|15);
  }
  SH_MAP_BACKUP=0;sh_dirty_vram();
  sh_repair_map_palette();
}
/* Entering a doorway: separate along the travel axis. Emerging into the next
 * room retains the existing leading-step arrangement. Native exit commands
 * are 1..4; room-entry commands use the following command group. */
static unsigned sh_door_entering(void) {
  unsigned command=sh_r16(0xc2);
  return sh_transition && (SH_DOOR_ENTRY || (command>=1 && command<=4));
}
static int sh_door_dx(void) {
  unsigned direction=sh_r8(0x18e);
  if(sh_door_entering())return direction==1?-32:direction==2?32:0;
  return direction==1?-32:direction==2?32:direction==4?-24:24;
}
static int sh_door_dy(void) {
  unsigned direction=sh_r8(0x18e);
  if(sh_door_entering())return direction==4?40:direction==8?-40:0;
  return direction==4?20:direction==8?-20:0;
}
static unsigned sh_door_visible(void) {
  return sh_transition && (sh_r16(0x5520)&0x8000);
}
static unsigned sh_door_leader_visible(void) {
  int x,y;unsigned direction;
  if(!sh_door_entering() || !SH_DOOR_LEADER)return 1;
  direction=sh_r8(0x18e);
  x=(int)sh_r16(0x5fa2)-(int)sh_r16(0xf0)+128+sh_door_dx();
  y=(int)sh_r16(0x60a2)-(int)sh_r16(0xf8)+128+sh_door_dy();
  /* Native exit bounds (D648, D6AA, D70C, D778), with one short step into
   * side doors before hiding the leader. The follower completes the script. */
  return direction==1?x>0x98:direction==2?x<0x168:direction==4?y<0x140:y>0x110;
}
/* Private P2 pattern rows avoid consuming background VRAM. Only marked sprite
 * names use this path; background tiles retain the original pattern cache. */
__declspec(dllexport) unsigned sh_sprite_flags(unsigned index,unsigned attr) {
  /* Door fade mode still displays the last gameplay SAT. Keep its ownership
   * until the room loader replaces it; map/story sprites remain native. */
  unsigned scene=sh_scene() || (sh_transition && sh_r8(1)==5 && sh_r8(3)==4);
  return sh_enabled && scene && (sh_ready || sh_transition) && index<80 && (attr&0x6000)==0x2000 ? SH_SAT_LIVE(index):0;
}
__declspec(dllexport) unsigned char *sh_recolor_row(unsigned char *source,unsigned flags) {
  static unsigned char pixels[8];unsigned x,ink;
  if(!(flags&3))return source;
  for(x=0;x<8;x++){ink=source[x];pixels[x]=(ink==10)?14:(ink==11 || ink==12)?15:(ink==13)?1:ink;}
  return pixels;
}
__declspec(dllexport) unsigned char *sh_sprite_row(unsigned attr,unsigned row,unsigned flags) {
  static unsigned char pixels[8];unsigned x,y,ink,off,name=attr&2047,source=0;
  if(!sh_enabled || !(sh_scene() || (sh_transition && sh_r8(1)==5 && sh_r8(3)==4)) || (!sh_ready && !sh_transition))return 0;
  /* Native loader table 7934: normal / powered / gut-blast sheets share tile 96.
   * Read each actor's immutable ROM art instead of the last globally loaded
   * form. This also repairs mismatched VRAM in existing co-op saves. */
  if((flags&8) && name>=96 && name<96+((flags&16)?109:(flags&4)?72:63)) {
    source=((flags&16)?0xad0e0:(flags&4)?0xac7e0:0xac000)+(name-96)*32;
  } else {
    if((flags&1) && name>=0x7a0)name-=0x7a0; /* Legacy private tile aliases. */
    else if(!(flags&1) || (flags&2) || name>=96)return 0;
  }
  y=(row>>3)&7;if(attr&0x1000)y=7-y;
  for(x=0;x<8;x++) {
    unsigned sx=(attr&0x800)?7-x:x;
    off=y*4+sx/2;
    ink=((source?m68k_read_pcrelative_8(source+off):sh_tiles[1][(name*32+off)^1])>>((sx&1)?0:4))&15;
    pixels[x]=(flags&3)?((ink==10)?14:(ink==11 || ink==12)?15:(ink==13)?1:ink):ink;
  }
  return pixels;
}
__declspec(dllexport) void sh_coop_enable(unsigned v) { sh_enabled=v;sh_ready=sh_second=sh_render_second=sh_frames=sh_enemy_active=sh_target=sh_collision_repeat=sh_transition=sh_camera_active=sh_render_done=sh_render_resume=sh_render_index=0; memset(sh_players,0,sizeof(sh_players));memset(sh_tiles,0,sizeof(sh_tiles));SH_SAT_SCHEMA=1; }
__declspec(dllexport) unsigned sh_coop_paused(void) {return sh_ready && ((sh_r8(1)==11 && sh_r8(2)==3) || (sh_r8(9)&128));}
__declspec(dllexport) unsigned sh_coop_ready(void) { return sh_ready; }
__declspec(dllexport) unsigned sh_coop_sprite_budget(void) { return sh_enabled && (sh_ready || sh_transition); }
__declspec(dllexport) unsigned sh_coop_debug_flags(void) { return SH_DEBUG_FLAGS; }
__declspec(dllexport) unsigned sh_coop_menu_get(void) {return ((unsigned)sh_p16(0,0x310)<<16)|sh_p16(0,0x312);}
__declspec(dllexport) void sh_coop_menu_set(unsigned value) {sh_setp16(0,0x310,value>>16);sh_setp16(0,0x312,value);}
__declspec(dllexport) int sh_coop_debug(unsigned command) {
  if(!sh_enabled || !sh_ready)return 0;
  if(command>=1 && command<=4)SH_DEBUG_FLAGS^=1u<<(command-1);
  else if(command>=5 && command<=9)SH_DEBUG_PENDING=command;
  else if(command==10 && sh_scene() && sh_r8(2)==10 && !sh_transition && !(sh_r8(9)&0xc0))SH_DEBUG_PENDING=command;
  else return 0;
  return 1;
}
__declspec(dllexport) int sh_coop_debug_stage(unsigned stage) {
  if(!sh_enabled || !sh_ready || stage>5)return 0;
  SH_DEBUG_STAGE=stage+1;return 1;
}
__declspec(dllexport) unsigned sh_coop_level(void) { return sh_r8(0x166); }
static void sh_debug_apply_player(unsigned p,unsigned action) {
  unsigned hp=sh_r16(0xb2),power=sh_r16(0xba),tier=sh_r8(0xbd);
  if(p==0 && action==10) {
    unsigned slot;
    /* Native combat group 26..2C (word offsets). Use the native fatal-hit
     * reaction table (12104) so deaths, boss scripts and room clear run. */
    for(slot=19;slot<23;slot++) {
      unsigned a=0x551e + slot*2,b=0x551e + slot*4,type=sh_r16(a+0x300),script;
      if(!(sh_r16(a)&0x8000) || type<0x30 || type>0x60)continue;
      script=sh_rom32(sh_rom32(sh_rom32(0x2c752+type*4)+12)+8);
      if(script>=0x200000 || !script)continue;
      sh_w16(a+0x980,0);
      sh_w8(a+0x80,sh_r8(a+0x80)&~4u);
      sh_w16(a+0x1700,3);sh_w16(a+0x1780,2);
      sh_w32(b+0x1100,script);sh_w32(b+0x1200,script);
      sh_w16(a+0x1400,0);sh_w16(a+0x1480,0);
      sh_w32(b+0x1300,0);sh_w32(b+0x1600,0);
      if(sh_r16(0x190)==slot*2) {
        sh_w16(0x190,0);sh_w32(0xa0,sh_r32(0xa0)&0xfd07ff7f);sh_w8(0xcd,0);
      }
      if(sh_p16(1,0x190)==slot*2) {
        unsigned flags=((unsigned)sh_p16(1,0xa0)<<16)|sh_p16(1,0xa2);
        flags&=0xfd07ff7f;sh_setp16(1,0xa0,flags>>16);sh_setp16(1,0xa2,flags);
        sh_setp16(1,0x190,0);sh_players[1][0xcd]=0;
      }
    }
  }
  /* Older room/stage transitions can leave P2's ground coordinate in the
   * previous 256-pixel room row. Native CE E0 / D08C clamps preserve that
   * wrong row forever. Rebase both Y coordinates, preserving jump height
   * and fractional movement; scripted doorway positions remain native. */
  if(p==1 && sh_r8(1)==4 && sh_r8(2)==10 && !sh_transition && !(sh_r8(9)&64)) {
    unsigned floor=sh_r16(0xf8)&0xff00,ground=sh_r16(0x61a2)&0xff00;
    if(floor!=ground && (sh_r8(0x16b)<<8)==floor) {
      unsigned delta=(floor-ground)<<16;
      sh_w32(0x60a2,sh_r32(0x60a2)+delta);
      sh_w32(0x61a2,sh_r32(0x61a2)+delta);
      sh_w16(0x5720,sh_r16(0x60a2)-sh_r16(0xf8)+128);
    }
  }
  /* Old room transitions retained the meter but copied P1's tier to P2.
   * Native depletion clamps the meter to tier*20, so tier zero erases it on
   * the first tick. Repair inconsistent legacy states before either update. */
  if(power && (power>tier*20 || tier>4))sh_w8(0xbd,(power>=80)?4:(power+19)/20);
  if((SH_DEBUG_FLAGS&2) && hp) { sh_w16(0xba,80);sh_w8(0xbd,4); }
  if(action==5 && hp)sh_w16(0xb2,256);
  if(action==6 && hp) {sh_w16(0xba,80);sh_w8(0xbd,4);}
  if((action==7 && p==0) || (action==8 && p==1) || action==9) {
    SH_DEBUG_FLAGS&=~1;
    hp=(action==9 || hp<=32)?0:hp-32;sh_w16(0xb2,hp);
    if(!hp) {
      unsigned flags=sh_r32(0xa0);
      sh_w32(0xa0,(flags&3)|0x20008000);sh_w16(0xce,0x15);
      sh_w8(0x55a1,(sh_r8(0x55a1)&~1)|6);sh_w32(0xa4,0xdd60);
      sh_w32(0xa8,(flags&2)?0x60000:0x20000);
    }
  }
}
__declspec(dllexport) unsigned char *sh_probe_player(unsigned p) { return p<2?sh_players[p]:0; }
__declspec(dllexport) void sh_coop_status(unsigned *s) {
  unsigned active=(sh_second || sh_render_second || (sh_enemy_active && sh_target))?1:0;
  s[0]=sh_ready && (sh_scene() || sh_r8(1)==11);s[1]=sh_frames;
  s[2]=active?sh_p16(0,0x5fa2):sh_r16(0x5fa2);s[3]=active?sh_p16(0,0x60a2):sh_r16(0x60a2);
  s[4]=active?sh_r16(0x5fa2):sh_p16(1,0x5fa2);s[5]=active?sh_r16(0x60a2):sh_p16(1,0x60a2);
  s[6]=active?sh_p16(0,0xb2):sh_r16(0xb2);s[7]=active?sh_r16(0xb2):sh_p16(1,0xb2);
  s[8]=active?sh_p16(0,0xba):sh_r16(0xba);s[9]=active?sh_r16(0xba):sh_p16(1,0xba);
}
typedef struct {
  unsigned version, flags[16], regs[16], input[3];
  unsigned char players[2][65536],tiles[2][0xc00];
} sh_save_state;
__declspec(dllexport) unsigned sh_coop_state_size(void) { return sizeof(sh_save_state); }
__declspec(dllexport) int sh_coop_save(void *data,unsigned n) {
  sh_save_state *s=(sh_save_state*)data;
  unsigned values[]={sh_enabled,sh_ready,sh_second,sh_render_second,sh_frames,sh_enemy_active,
    sh_target,sh_collision_repeat,sh_transition,sh_saved_hp,sh_saved_power,sh_camera_x,sh_camera_active,sh_render_done,sh_render_resume,sh_render_index};
  if(!data || n!=sizeof(*s))return 0;memset(s,0,sizeof(*s));s->version=1;
  memcpy(s->flags,values,sizeof(values));memcpy(s->regs,sh_regs,sizeof(sh_regs));memcpy(s->input,sh_input,sizeof(sh_input));
  memcpy(s->players,sh_players,sizeof(sh_players));memcpy(s->tiles,sh_tiles,sizeof(sh_tiles));return 1;
}
__declspec(dllexport) int sh_coop_restore(const void *data,unsigned n) {
  const sh_save_state *s=(const sh_save_state*)data;const unsigned *v=s->flags;
  if(!data || n!=sizeof(*s) || s->version!=1)return 0;
  if(v[0]>1 || v[1]>1 || v[2]>1 || v[3]>1 || v[5]>1 || v[6]>1 || v[7]>1 || v[8]>1 || v[12]>1 || v[13]>1)return 0;
  sh_enabled=v[0];sh_ready=v[1];sh_second=v[2];sh_render_second=v[3];sh_frames=v[4];sh_enemy_active=v[5];
  sh_target=v[6];sh_collision_repeat=v[7];sh_transition=v[8];sh_saved_hp=v[9];sh_saved_power=v[10];sh_camera_x=v[11];sh_camera_active=v[12];
  sh_render_done=v[13];sh_render_resume=v[14];sh_render_index=v[15];
  memcpy(sh_regs,s->regs,sizeof(sh_regs));memcpy(sh_input,s->input,sizeof(sh_input));
  memcpy(sh_players,s->players,sizeof(sh_players));memcpy(sh_tiles,s->tiles,sizeof(sh_tiles));
  if(SH_SAT_SCHEMA!=1) {
    unsigned i;
    for(i=0;i<80;i++) {
      unsigned live=SH_SAT_LIVE(i),build=SH_SAT_BUILD(i);
      unsigned lp=(live&1)?1:0,bp=(build&1)?1:0;
      SH_SAT_LIVE(i)=live|8|((sh_p16(lp,0x57a0)>=125)?4:0);
      SH_SAT_BUILD(i)=build|8|((sh_p16(bp,0x57a0)>=125)?4:0);
    }
    SH_SAT_SCHEMA=1;
  }
  return 1;
}
static void sh_coop_hook(void) {
  unsigned pc=REG_PC;
  if(!sh_enabled)return;
  /* Infer the latch for older saves captured partway into a doorway. */
  if(sh_transition && sh_r8(2)==10 && sh_r16(0xc2)>=1 && sh_r16(0xc2)<=4)SH_DOOR_ENTRY=1;

  /* Co-op can extend a frame across the sound driver's BUSREQ/poll pair.
   * An intervening sound interrupt can release BUSREQ, leaving the main
   * driver waiting forever. Reassert through the bus controller (including
   * Z80 synchronization and memory mapping), only at this ROM's wait loop.
   * This also recovers existing saves captured in the stalled loop. */
  if(pc==0x60c82 && zstate==1)gen_zbusreq_w(1,m68k.cycles);

  /* Keep each Rick's power until his meter expires, including cleared rooms.
   * Native death handling at B6D6 and empty-meter handling at C898 still run. */
  if(sh_ready && pc==0xb6b6)m68ki_jump(0xb6d6);
  if(sh_ready && pc==0xc8a4 && (m68k.dar[7]&0x4000))m68ki_jump(0xc92a);
  /* Start opens the native map even while enemies remain in the room. */
  if(pc==0x327e && sh_ready && sh_r8(0x166)<5 && !(sh_r8(9)&128) && (sh_r8(0x1f)&128))m68ki_jump(0x32a2);
  if(pc==0x32b6 && sh_ready && !SH_MAP_BACKUP) {
    memcpy(&sh_players[0][0x8000],vram,0x8000);
    memcpy(&sh_players[1][0x8000],vram+0x8000,0x8000);
    memcpy(&sh_players[0][0x7000],&SH_SAT_BUILD(0),160);
    {unsigned i;for(i=0;i<256;i++)sh_players[0][0x70a0+i]=sh_r8(0x4d0e + i);}
    SH_MAP_BACKUP=2; /* Version 1 snapshots contain only VRAM/SAT metadata. */
  }
  /* Native map exit has restored the gameplay mode here. Restore before
   * returning to the frame loop, not at the following player update. */
  if(pc==0x743a && sh_scene())sh_restore_map();
  if(pc==0x3002){sh_ready=sh_transition=0;SH_DOOR_ENTRY=0;SH_MAP_BACKUP=0;SH_MAP_SCHEMA=1;}
  /* Every native room load replaces the primary actor, including floor-entry
   * paths that bypass stage setup at 3002. Invalidate P2 at this common
   * boundary so the first playable update initializes the new actor pair.
   * Keep an accepted door transition's saved health/power for its handoff. */
  if(pc==0x3112) {
    sh_ready=sh_second=sh_enemy_active=sh_render_second=sh_camera_active=0;
    SH_DOOR_ENTRY=0;
    SH_MAP_BACKUP=0;
  }
  /* A regular purple monster back in its normal AI must be hittable. Older
   * co-op saves can retain the scripted collision exclusion after that script
   * has ended. Restore the native category and remove the stale list entry. */
  if(pc==0x13bf4 && sh_enemy_active && sh_r16(m68k.dar[12]+0x300)==0x46 &&
     sh_r16(m68k.dar[12]+0x380)==4 && !(sh_r8(m68k.dar[12]+0x80)&4)) {
    unsigned i,j=0,slot=sh_r16(0x12c);
    sh_w16(m68k.dar[12]+0x380,3);
    for(i=0;i<64;i++) {
      unsigned entry=sh_r8(0x226e + i);
      if(entry==255)break;
      if(entry!=slot)sh_w8(0x226e + j++,entry);
    }
    sh_w8(0x226e + j,255);
  }
  if(pc==0x23868 && sh_ready && (SH_DEBUG_FLAGS&1))m68ki_jump(0x23a64);
  if(pc==0x2a7b2 && sh_ready && (SH_DEBUG_FLAGS&8))m68ki_jump(0x2a7ca);
  /* P2 DMA must never replace P1's live VDP tiles, even for part of a frame. */
  if(pc==0xb5d0 && sh_ready && (sh_second || (sh_enemy_active && sh_target))) {
    unsigned dst=m68k.dar[5],src=m68k.dar[6],n=m68k.dar[7]&65535,i;
    if(dst<0xc00 && n<=0xc00-dst && src<0x200000 && n<=0x200000-src) {
      for(i=0;i<n;i++)sh_tiles[1][(dst+i)^1]=m68k_read_pcrelative_8(src+i);
      /* Native code uses MOVE.W for length and its return value: the high
       * word can hold action flags during transformation and must survive. */
      m68k.dar[7]=(m68k.dar[7]&0xffff0000u)|1;m68ki_jump(0xb5d6);
    }
  }
  if(pc==0xaf2) { sh_w8(0x1f,sh_r8(0x1f)|(sh_r8(0x20)&128));sh_w8(0x21,sh_r8(0x21)|(sh_r8(0x22)&128)); }
  if(pc==0x2326e && sh_ready && sh_enemy_active) {
    unsigned partner=sh_target^1;
    int x=sh_r16((m68k.dar[13]+0xa80)&65535),y=sh_r16((m68k.dar[13]+0xc80)&65535);
    int dx=(int)sh_p16(partner,0x5fa2)-x,dy=(int)sh_p16(partner,0x61a2)-y;
    if(sh_transition || dx < -24 || dx > 24 || dy < -16 || dy > 16)m68ki_jump(0x23476);
  }
  if(pc==0x2346e && sh_ready && sh_enemy_active && !sh_transition) {
    sh_saved_hp=sh_target?sh_r16(0xb2):sh_p16(1,0xb2);
    sh_saved_power=sh_target?(sh_r16(0xba)|(sh_r8(0xbc)<<16)|(sh_r8(0xbd)<<24)):
      (sh_p16(1,0xba)|(sh_players[1][0xbc]<<16)|(sh_players[1][0xbd]<<24));
    if(sh_target) {
      /* The original scripted door walk runs through P1's native slot. */
      sh_setp16(0,0xc2,sh_r16(0xc2));sh_setp16(0,0xac,sh_r16(0xac));
      sh_players[0][0x18e]=sh_r8(0x18e);
      sh_players[0][0xa2]|=0x10;
    }
    sh_transition=1;SH_DOOR_ENTRY=1;
    {
      unsigned direction=sh_r8(0x18e),a=(direction==1 || direction==2)?0x5fa2:0x61a2;
      int p0=sh_p16(0,a),p1=sh_p16(1,a);
      SH_DOOR_LEADER=(direction==1 || direction==8)?(p1<p0?2:1):(p1>p0?2:1);
    }
    /* Run the native script at the follower's position so room loading waits
     * for both actors. The extra sprite is the leader, one step ahead. Copy
     * the native doorway alignment even when red initiated the transition. */
    {
      unsigned a;
      for(a=0x5fa2;a<=0x61a2;a+=0x100) {
        int delta=a==0x5fa2?sh_door_dx():sh_door_dy();
        unsigned value=(delta?sh_p16(SH_DOOR_LEADER-1,a):sh_r16(a))-delta;
        sh_setp16(0,a,value);sh_setp16(0,a+2,sh_r16(a+2));
        if(!sh_target)sh_w16(a,value);
      }
      sh_setp16(0,0x56a0,sh_p16(0,0x5fa2)-sh_r16(0xf0)+128);
      sh_setp16(0,0x5720,sh_p16(0,0x60a2)-sh_r16(0xf8)+128);
      if(!sh_target) {
        sh_w16(0x56a0,sh_p16(0,0x56a0));sh_w16(0x5720,sh_p16(0,0x5720));
      }
    }
  }
  if(pc==0x11d0e && sh_ready) {
    int x=sh_r16((m68k.dar[13]+0xa80)&65535),y=sh_r16((m68k.dar[13]+0xc80)&65535);
    int dx0=x-(int)sh_r16(0x5fa2),dy0=y-(int)sh_r16(0x61a2);
    int dx1=x-(int)sh_p16(1,0x5fa2),dy1=y-(int)sh_p16(1,0x61a2);
    sh_context(0,0);
    sh_target=(dx1*dx1+4*dy1*dy1 < dx0*dx0+4*dy0*dy0);
    /* An enemy already being grappled stays with its owner. */
    if(sh_p16(0,0x190)==(m68k.dar[0]&65535))sh_target=0;
    else if(sh_p16(1,0x190)==(m68k.dar[0]&65535))sh_target=1;
    if((sh_players[0][0xa3]&32) && sh_players[0][0xd1]==(m68k.dar[0]&127))sh_target=0;
    if((sh_players[1][0xa3]&32) && sh_players[1][0xd1]==(m68k.dar[0]&127))sh_target=1;
    SH_AI_TARGET=sh_target;
    SH_OBJECT_OWNER((m68k.dar[0]&127)/2)=sh_target;
    sh_context(sh_target,1);sh_enemy_active=1;sh_collision_repeat=0;
  }
  /* Each world object advances once; test its contact against both Ricks. */
  if(pc==0x11f44 && sh_enemy_active) {
    sh_context(sh_target,0);
    if(!sh_collision_repeat) {
      sh_target^=1;sh_context(sh_target,1);sh_collision_repeat=1;m68ki_jump(0x11f3e);
    } else {
      sh_target=SH_AI_TARGET;sh_context(sh_target,1);
    }
  }
  if(pc==0x11f90 && sh_enemy_active) {
    sh_context(sh_target,0);sh_context(0,1);sh_enemy_active=0;
  }
  if(pc==0x32f0 && !sh_second) {
    sh_restore_map(); /* Fallback for snapshots already past native map exit. */
    if(!SH_MAP_SCHEMA)sh_repair_map_art();
    sh_repair_map_palette();
    if(SH_DEBUG_STAGE) {
      unsigned stage=SH_DEBUG_STAGE-1;SH_DEBUG_STAGE=0;
      sh_ready=sh_transition=sh_enemy_active=sh_render_second=0;
      sh_w8(0x166,stage);sh_w8(1,4);sh_w8(2,0);sh_w8(9,0);
      m68ki_jump(0x3494);return;
    }
    if(sh_transition) {
      if(sh_r8(9)&0x40) { sh_ready=0;return; }
      sh_ready=0;
    }
    if(!sh_ready && (sh_r8(9)&0x40))return;
    if(!sh_ready) {
      sh_context(0,0);memcpy(sh_players[1],sh_players[0],65536);
      memcpy(sh_tiles[0],vram,0xc00);memcpy(sh_tiles[1],vram,0xc00);
      sh_setp16(1,0x5fa2,sh_r16(0x5fa2)+(sh_transition?sh_door_dx():32));
      sh_setp16(1,0x56a0,sh_r16(0x56a0)+(sh_transition?sh_door_dx():32));
      if(sh_transition) {
        sh_setp16(1,0x60a2,sh_r16(0x60a2)+sh_door_dy());
        sh_setp16(1,0x61a2,sh_r16(0x61a2)+sh_door_dy());
        sh_setp16(1,0x5720,sh_r16(0x5720)+sh_door_dy());
      }
      if(sh_transition) {
        sh_setp16(1,0xb2,sh_saved_hp);sh_setp16(1,0xba,sh_saved_power&65535);
        sh_players[1][0xbc]=(sh_saved_power>>16)&255;sh_players[1][0xbd]=sh_saved_power>>24;
        sh_transition=0;
      }
      sh_ready=1;
    }
    if(SH_DEBUG_FLAGS&4)sh_w8(0xb4,99);
    sh_debug_apply_player(0,SH_DEBUG_PENDING);
    memcpy(sh_regs,m68k.dar,sizeof(sh_regs));
  }
  if(pc==0x32f6 && sh_ready) {
    if(!sh_second) {
      sh_context(0,0);memcpy(sh_tiles[0],vram,0xc00);
      sh_input[0]=sh_r8(0x21);sh_input[1]=sh_r8(0x1f);sh_input[2]=sh_r8(0x23);
      sh_context(1,1);
      sh_debug_apply_player(1,SH_DEBUG_PENDING);SH_DEBUG_PENDING=0;
      sh_w8(0x21,sh_r8(0x22));sh_w8(0x1f,sh_r8(0x20));sh_w8(0x23,sh_r8(0x24));
      memcpy(m68k.dar,sh_regs,sizeof(sh_regs));sh_second=1;m68ki_jump(0x32f0);
    } else {
      sh_context(1,0);sh_context(0,1);
      sh_w8(0x21,sh_input[0]);sh_w8(0x1f,sh_input[1]);sh_w8(0x23,sh_input[2]);
      sh_second=0;sh_frames++;
    }
  }
  /* Keep P2 from independently moving the shared camera. */
  if(pc==0xcf24 && sh_second)m68ki_jump(0xd030);
  if(pc==0xcf24 && sh_ready && !sh_second) {
    sh_camera_x=sh_r32(0x5fa2);sh_camera_active=1;
    sh_w16(0x5fa2,(sh_r16(0x5fa2)+sh_p16(1,0x5fa2))/2);
  }
  if(pc==0xd030 && sh_camera_active) { sh_w32(0x5fa2,sh_camera_x);sh_camera_active=0; }
  if(pc==0x79bc)sh_render_done=0;
  if(sh_scene() && (sh_ready || sh_door_visible()) && !sh_render_done && !sh_render_second && (pc==0x7a02 || pc==0x7a7e)) {
    unsigned index=m68k.dar[2]&65535;
    unsigned ground=sh_transition?sh_r16(0x61a2)+sh_door_dy():sh_p16(1,0x61a2);
    if(sh_transition && !sh_door_leader_visible())sh_render_done=1;
    else if(pc==0x7a7e || (m68k.dar[1]>=sh_r8(0x192) && ground>=sh_r16(0x619e + index*2))) {
      sh_context(0,0);
      if(sh_transition) {
        sh_w16(0x5fa2,sh_r16(0x5fa2)+sh_door_dx());
        sh_w16(0x60a2,sh_r16(0x60a2)+sh_door_dy());
        sh_w16(0x61a2,sh_r16(0x61a2)+sh_door_dy());
      } else sh_context(1,1);
      sh_w16(0x56a0,sh_r16(0x5fa2)-sh_r16(0xf0)+128);
      sh_w16(0x5720,sh_r16(0x60a2)-sh_r16(0xf8)+128);
      sh_render_resume=pc;sh_render_index=m68k.dar[2];m68k.dar[2]=2;
      sh_render_done=sh_render_second=1;m68ki_jump(0x7a3c);
    }
  }
  if(pc==0x7a74 && sh_render_second) {
    sh_context(0,1);sh_render_second=0;m68k.dar[2]=sh_render_index;m68ki_jump(sh_render_resume);
  }
  if(pc==0x7b50 && (sh_ready || sh_transition)) {
    unsigned slot=(m68k.dar[2]&127)/2,offset=m68k.dar[13]&65535;
    unsigned type=sh_r16(0x581e + slot*2);
    if(offset>=0x5262 && offset<0x54e2) {
      unsigned owner=sh_render_second?1:(slot==1?0:SH_OBJECT_OWNER(slot));
      unsigned powered=(slot==1)?(sh_r16(0x57a0)>=125):(sh_p16(owner,0x57a0)>=125);
      unsigned frame=(slot==1)?sh_r16(0x57a0):sh_p16(owner,0x57a0);
      /* Native left BA-BC / right 107-109 gut-blast poses use table entry 2,
       * whose 109 tiles replace the ordinary 72-tile powered sheet. */
      unsigned guts=(frame>=0xba && frame<=0xbc) || (frame>=0x107 && frame<=0x109);
      unsigned flags=0;
      unsigned attr=m68k.dar[4]&65535,tile=attr&2047;
      /* Composite enemy frames can contain Rick's grabbing arms, but their
       * own pieces and foreground scenery must keep their native colors. */
      if((attr&0x6000)==0x2000 && (tile<(guts?205:168) || (owner && tile>=0x7a0)) &&
         (slot==1 || !(type>=5 && type<=7))) {
        flags=8|(powered?4:0)|(guts?16:0);
        if(sh_render_second)flags|=sh_transition?2:1;
        else if(!sh_transition && slot!=1 && owner)flags|=1;
        if(slot==1 && sh_door_entering() && SH_DOOR_LEADER==1)
          flags=(flags&~3u)|(sh_render_second?0:2);
      }
      SH_SAT_BUILD((offset-0x5262)/8)=flags;
    }
  }
  if(pc==0x7bdc && (sh_ready || sh_transition))memcpy(&SH_SAT_LIVE(0),&SH_SAT_BUILD(0),80);
}




