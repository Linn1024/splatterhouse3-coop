/* Private P2 materials. Metadata uses reserved bytes in the existing save layout. */
#define SH_APPEARANCE (sh_players[0][0x314])
#define SH_APPEARANCE_MENU (sh_players[0][0x318])
static const unsigned sh_colors[]={0,0x247f91,0xa00070,0xd58b3b,0xa82020,0x355eb0,0x774ca5,0x438447,0xb7b7ae,0xd4ae45};
__declspec(dllexport) unsigned sh_coop_appearance_get(void) {
  return SH_APPEARANCE==1 ? sh_players[0][0x315]|(sh_players[0][0x316]<<8)|(sh_players[0][0x317]<<16) : 0;
}
__declspec(dllexport) void sh_coop_appearance_set(unsigned value) {
  unsigned i;SH_APPEARANCE=1;
  for(i=0;i<3;i++)sh_players[0][0x315+i]=((value>>(i*8))&255)%10;
}
__declspec(dllexport) unsigned sh_coop_appearance_menu_get(void) {
  return SH_APPEARANCE_MENU==1 ? (sh_p16(0,0x31a)<<16)|sh_p16(0,0x31c) : 0;
}
__declspec(dllexport) void sh_coop_appearance_menu_set(unsigned value) {
  SH_APPEARANCE_MENU=1;sh_setp16(0,0x31a,value>>16);sh_setp16(0,0x31c,value);
}
__declspec(dllexport) unsigned sh_coop_appearance_available(void) {
  /* Main title selector: native START / OPTIONS / PASSWORD, plus COOP OPTIONS. */
  if(sh_enabled && sh_r8(1)==1 && sh_r8(2)==3)return 2|(sh_r16(0x64)==3?4:0);
  return sh_enabled && sh_r8(1)==8 && sh_r8(2)==3;
}
static void sh_title_coop_label(void) {
  const char *label="COOP OPTIONS";unsigned i,offset=sh_r16(0x72),a=0x460e + offset;
  unsigned color=sh_r16(0x64)==3?0x0400:0x6400;
  sh_w16(a,17*2+22*((sh_r8(0x47)&2)?256:128));sh_w16(a+2,12);
  for(i=0;i<12;i++)sh_w16(a+4+i*2,color+(unsigned char)label[i]);
  sh_w16(a+28,65535);sh_w16(0x72,offset+28);
}
static unsigned sh_material(unsigned ink) {
  if(ink&16)return 3; /* Gray skin highlights share the mask's palette inks. */
  return ink>=10 && ink<=13?1:ink>=2 && ink<=5?2:ink>=6 && ink<=9?3:0;
}
__declspec(dllexport) unsigned sh_coop_appearance_rgb(unsigned ink,unsigned fallback) {
  unsigned material=sh_material(ink),choice,rgb,shade;
  if(!material)return fallback;
  choice=(sh_coop_appearance_get()>>((material-1)*8))&255;
  if(!choice || choice>=10)return fallback;
  /* The near-white specular inks are skin highlights, not colored midtones.
   * Keep their native value when changing the skin's hue. */
  if((ink&16) && (ink&15)<=3)return fallback;
  rgb=sh_colors[choice];
  if(ink&16)ink=(ink&15)+4;
  shade=material==1?(ink==10?255:ink==11?190:ink==12?125:60):
        material==2?(ink==2?255:ink==3?210:ink==4?145:75):
        (ink==6?255:ink==7?205:ink==8?150:85);
  return ((((rgb>>16)&255)*shade/255)<<16)|((((rgb>>8)&255)*shade/255)<<8)|((rgb&255)*shade/255);
}
static unsigned char sh_material_canvas[160*160],sh_material_seen[160*160],sh_material_face[160*160];
static unsigned char sh_material_sat[640],sh_material_native[0xc00],sh_material_private[0xc00],sh_material_owners[80];
static unsigned sh_material_x,sh_material_y,sh_material_frame=~0u;
static unsigned sh_material_head_matches(unsigned p,int dir,const unsigned char *crown,unsigned cn,const unsigned char *brow,unsigned bn,int brow_offset) {
  unsigned k;int ax=p%160,ay=p/160,xx;
  if(ay+1>=160)return 0;
  for(k=0;k<cn;k++) {
    xx=ax+dir*(int)k;
    if(xx<0 || xx>=160 || sh_material_canvas[ay*160+xx]!=crown[k])return 0;
  }
  for(k=0;k<bn;k++) {
    xx=ax+dir*((int)k+brow_offset);
    if(xx<0 || xx>=160 || sh_material_canvas[(ay+1)*160+xx]!=brow[k])return 0;
  }
  return 1;
}
extern void sh_render_material_invalidate(void);
static void sh_appearance_invalidate(void) {sh_material_frame=~0u;sh_render_material_invalidate();}
__declspec(dllexport) void sh_coop_material_prepare(void) {
  unsigned i,x,y,w,h,a,attr,flags,minx=512,miny=512;
  unsigned queue[160*160],best[160*160],bestn=0;
  unsigned char current_sat[640],current_owners[80];
  unsigned char active[80]={0},order[80];unsigned link=0,count=0;
  a=(reg[5]&127)<<9;
  for(i=0;i<640;i++)current_sat[i]=vram[(a+i)&65535];
  for(i=0;i<80;i++) {
    unsigned sprite_attr=(current_sat[(i*8+4)^1]<<8)|current_sat[(i*8+5)^1];
    current_owners[i]=sh_sprite_flags(i,sprite_attr);
  }
  /* Native DMA can move entrance sprites or replace jump art after the CPU
   * built their ownership list, without advancing the co-op frame counter.
   * Cache the actual rendered sprite list and both art sources instead. */
  if(sh_material_frame==sh_frames &&
     !memcmp(current_sat,sh_material_sat,sizeof(current_sat)) &&
     !memcmp(vram,sh_material_native,sizeof(sh_material_native)) &&
     !memcmp(sh_tiles[1],sh_material_private,sizeof(sh_material_private)) &&
     !memcmp(current_owners,sh_material_owners,sizeof(sh_material_owners)))return;
  sh_material_frame=sh_frames;
  memcpy(sh_material_sat,current_sat,sizeof(current_sat));
  memcpy(sh_material_native,vram,sizeof(sh_material_native));
  memcpy(sh_material_private,sh_tiles[1],sizeof(sh_material_private));
  /* Ownership is gated by the scene, not just saved SAT metadata. Exit to
   * fade can reuse identical SAT/art after briefly hiding gameplay sprites. */
  memcpy(sh_material_owners,current_owners,sizeof(sh_material_owners));
  memset(sh_material_canvas,0,sizeof(sh_material_canvas));memset(sh_material_seen,0,sizeof(sh_material_seen));memset(sh_material_face,0,sizeof(sh_material_face));
  for(i=0;i<80;i++) {
    if(link>=80 || active[link])break;active[link]=1;order[count++]=link;
    link=vram[(a+link*8+3)^1]&127;if(!link)break;
  }
  for(i=0;i<80;i++) {
    unsigned attr=(vram[(a+i*8+4)^1]<<8)|vram[(a+i*8+5)^1];
    unsigned flags=sh_sprite_flags(i,attr);
    /* Stale ownership bytes on palette-zero/unused entries are not Rick.
     * Use exactly the same ownership filter as the native sprite renderer. */
    if(!active[i] || !(flags&3) || !(flags&8))continue;
    x=((vram[(a+i*8+6)^1]<<8)|vram[(a+i*8+7)^1])&511;
    y=((vram[(a+i*8)^1]<<8)|vram[(a+i*8+1)^1])&511;
    if(x<minx)minx=x;if(y<miny)miny=y;
  }
  sh_material_x=minx;sh_material_y=miny;
  for(i=0;i<count;i++) {
    unsigned index=order[i],b=a+index*8,size=vram[(b+2)^1];
    attr=(vram[(b+4)^1]<<8)|vram[(b+5)^1];flags=sh_sprite_flags(index,attr);
    if(!(flags&3) || !(flags&8))continue;
    x=(((vram[(b+6)^1]<<8)|vram[(b+7)^1])&511)-minx;
    y=(((vram[b^1]<<8)|vram[(b+1)^1])&511)-miny;
    w=8+((size&12)<<1);h=8+((size&3)<<3);
    {unsigned yy,xx,col,tile;for(yy=0;yy<h && y+yy<160;yy++)for(col=0;col<w/8;col++) {
      unsigned char *row;
      tile=((attr&2047)+((attr&0x800)?w/8-1-col:col)*(h/8)+((attr&0x1000)?h/8-1-yy/8:yy/8))&2047;
      row=sh_sprite_row((attr&0xf800)|tile,(yy&7)<<3,flags);
      for(xx=0;xx<8 && x+col*8+xx<160;xx++) {
        unsigned ink;
        if(row)ink=row[xx];
        else {
          /* Entrance/shared body pieces use the native VRAM pattern cache,
           * just as sh_sprite_row's null return requests in the renderer. */
          unsigned sx=(attr&0x800)?7-xx:xx,sy=(attr&0x1000)?7-(yy&7):yy&7;
          unsigned off=tile*32+sy*4+sx/2;
          ink=(vram[off^1]>>((sx&1)?0:4))&15;
        }
        /* The native SAT is front-to-back. Behind pieces must not replace
         * already visible head pixels when reconstructing the silhouette. */
        if(ink && !sh_material_canvas[(y+yy)*160+x+col*8+xx])sh_material_canvas[(y+yy)*160+x+col*8+xx]=ink;
      }
    }}
  }
  /* Keep only the connected mask surface. A surrounding rectangle also
   * catches separate temple, neck and chest highlights that belong to skin. */
  for(i=0;i<160*160;i++)if(!sh_material_seen[i] && sh_material_canvas[i]>=2 && sh_material_canvas[i]<=5) {
    unsigned n=1,j;queue[0]=i;sh_material_seen[i]=1;
    for(j=0;j<n;j++) {
      unsigned p=queue[j];int dx,dy;
      /* Diagonal mask contours remain one material across jumping poses. */
      for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++) {
        int xx=(int)(p%160)+dx,yy=(int)(p/160)+dy;unsigned q;
        if(xx<0 || xx>=160 || yy<0 || yy>=160)continue;q=yy*160+xx;
        if(!sh_material_seen[q] && sh_material_canvas[q]>=2 && sh_material_canvas[q]<=5){sh_material_seen[q]=1;queue[n++]=q;}
      }
    }
    if(n>bestn){bestn=n;memcpy(best,queue,n*sizeof(unsigned));}
  }
  for(i=0;i<bestn;i++)sh_material_face[best[i]]=1;
  /* Rear view exposes the scalp but still shows the mask's side edges.
   * Color those disconnected edges, never the central scalp or forearms. */
  {
    static const unsigned char crown[]={4,2,4},brow[]={3,9,8,4,8,9,3};
    for(i=0;i<160*160;i++)if(sh_material_head_matches(i,1,crown,sizeof(crown),brow,sizeof(brow),-2)) {
      unsigned q;int ax=(int)(i%160)+1,ay=i/160;
      memset(sh_material_face,0,sizeof(sh_material_face));
      for(q=0;q<160*160;q++) {
        int x=(int)(q%160)-ax,y=(int)(q/160)-ay;
        if(y>=0 && y<8 && x>=-5 && x<=5 && sh_material_canvas[q]>=2 && sh_material_canvas[q]<=5 &&
           (x<=-2 || x>=2 || y==0))sh_material_face[q]=1;
      }
      break;
    }
  }
  /* Normal walking head: two pale neck/shoulder glints can meet the mask
   * diagonally as the arms move. The mask art is unchanged across these
   * frames, so identify it before excluding those two skin coordinates. */
  {
    static const unsigned char crown[]={5,5,4,3,4,5},brow[]={3,3,3,4,5},brow_alt[]={4,3,3,4,5};
    static const int dx[]={-2,3},dy[]={11,15};
    unsigned p,k;int dir;
    for(p=0;p<160*160;p++)if(sh_material_canvas[p]==5)for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160;
      if(!sh_material_head_matches(p,dir,crown,sizeof(crown),brow,sizeof(brow),3) &&
         !sh_material_head_matches(p,dir,crown,sizeof(crown),brow_alt,sizeof(brow_alt),3))continue;
      for(k=0;k<2;k++) {
        int x=ax+dir*dx[k],y=ay+dy[k];
        if(x>=0 && x<160 && y<160 && sh_material_canvas[y*160+x]==2)sh_material_face[y*160+x]=0;
      }
    }
  }
  /* Leaning powered guard: the raised fingers overlap the temple. Keep
   * their shared white ink outside the mask contour in either facing. */
  {
    static const unsigned char crown[]={4,4,3,2,2,4},brow[]={4,2,4,3,2,4,3,3,2,3,4};
    static const unsigned char edge[]={14,15,13,11,16,15,22,19,22,22,23};
    unsigned p,q;int dir;
    for(p=0;p<160*160;p++)if(sh_material_canvas[p]==4)for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160;
      if(!sh_material_head_matches(p,dir,crown,sizeof(crown),brow,sizeof(brow),-3))continue;
      for(q=0;q<160*160;q++)if(sh_material_face[q]) {
        int x=dir*((int)(q%160)-ax)+22,y=(int)(q/160)-ay;
        if(y>=16 || (y>=5 && x<edge[y-5]))sh_material_face[q]=0;
      }
    }
  }
  /* Powered guard: the raised hand touches the cheek's gray ink, even
   * within the same connected component. Identify the native head art in
   * either facing, then use its drawn cheek/hand boundary. Match art rather
   * than the animation counter: door DMA can temporarily show older art. */
  {
    static const unsigned char crown[]={4,2,3,3,4,4};
    static const unsigned char brow[]={4,3,3,3,4,3,2,3,4,2,4};
    static const unsigned char edge[]={25,26,26,28,23,24,20,20,21,17,16};
    unsigned p,k,yy;int dir;
    for(p=0;p<160*160;p++)if(sh_material_canvas[p]==4)for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160,xx;
      if(!sh_material_head_matches(p,dir,crown,sizeof(crown),brow,sizeof(brow),-2))continue;
      for(yy=5;yy<32 && ay+yy<160;yy++)for(k=0;k<40;k++) {
        xx=ax+dir*((int)k-12);
        if(xx>=0 && xx<160 && (yy>=16 || k>edge[yy-5]))sh_material_face[(ay+yy)*160+xx]=0;
      }
    }
  }
  /* Powered raised-arm pose: shoulder/chest glints join the face diagonally.
   * Recognize its native crown and apply the drawn neck/shoulder contour.
   * The reflected art shares this annotation. */
  {
    static const unsigned char crown[]={4,4,4,3,3,2,4};
    static const unsigned char brow[]={2,4,4,3,2,3,3,3,3,3,4};
    static const unsigned char edge[]={31,29,26,25,23,22,24,23,23,24,25,26,26,35,36,37};
    unsigned p,k,yy;int dir;
    for(p=0;p<160*160;p++)if(sh_material_canvas[p]==4)for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160,xx;
      if(!sh_material_head_matches(p,dir,crown,sizeof(crown),brow,sizeof(brow),-2))continue;
      for(yy=0;yy<40 && ay+yy<160;yy++)for(k=0;k<52;k++) {
        xx=ax+dir*((int)k-31);
        if(xx>=0 && xx<160 && (yy>=16 || k<edge[yy]))sh_material_face[(ay+yy)*160+xx]=0;
      }
    }
  }
  /* Raised-hands transformation sequence. The two palette phases share a
   * face contour but connect to different chest highlights; its final pose
   * also has scalp/shoulder glints beyond the back of the mask. */
  {
    static const unsigned char crown[3][7]={{3,3,3,3,3},{4,4,4,4,4},{3,2,3,4,3,4,4}};
    static const unsigned char brow[3][11]={{3,3,2,2,4,4,4,3,4},{4,4,2,2,5,5,5,4,5},{3,2,2,3,4,5,4,3,2,4,4}};
    static const unsigned char front[]={23,25,25,25,25,24,24,25};
    static const unsigned char back[]={23,22,22,23,21,16};
    unsigned pose,p,q;int dir;
    for(pose=0;pose<3;pose++)for(p=0;p<160*160;p++)if(sh_material_canvas[p]==crown[pose][0])for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160;
      if(!sh_material_head_matches(p,dir,crown[pose],pose==2?7:5,brow[pose],pose==2?11:9,-2))continue;
      for(q=0;q<160*160;q++)if(sh_material_face[q]) {
        int x=dir*((int)(q%160)-ax)+(pose==2?18:21),y=(int)(q/160)-ay+(pose==2?0:1);
        if(pose==2) {
          if(y>=13 || (y>=7 && x>back[y-7]))sh_material_face[q]=0;
        } else if(y>=17 || (y>=9 && x<front[y-9]))sh_material_face[q]=0;
      }
    }
  }
  /* Powered recovery pose: the lower jaw touches two chest glints, while
   * the back of the head meets a shoulder glint. Keep the drawn mask edge
   * through the chin, then return those highlights to the skin material. */
  {
    static const unsigned char crown[]={4,4,5,5},brow[]={4,14,14,14,3,4,5,5};
    static const unsigned char edge[]={32,32,31,30};
    unsigned p,q;int dir;
    for(p=0;p<160*160;p++)if(sh_material_canvas[p]==4)for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160;
      if(!sh_material_head_matches(p,dir,crown,sizeof(crown),brow,sizeof(brow),-2))continue;
      for(q=0;q<160*160;q++)if(sh_material_face[q]) {
        int x=dir*((int)(q%160)-ax)+29,y=(int)(q/160)-ay;
        if(y>=18 || (y>=14 && x>edge[y-14]))sh_material_face[q]=0;
      }
    }
  }
  /* Both powered floor poses rotate the cheek against chest/shoulder
   * highlights. Their native contours remain distinct materials despite
   * sharing gray ink and touching. Recognize the displayed art in either
   * facing; animation counters can lead the DMA upload by several frames. */
  {
    static const unsigned char crown[2][6]={{5,5,4,4,3,4},{4,3,2,0,0,0}};
    static const unsigned char brow[2][7]={{5,4,4,4,3,2,14},{4,9,4,3,14,4,0}};
    static const unsigned char edge[2][20]={
      {59,63,63,61,60,60,59,52,53,55,55,55,54,54,53,54,55,53,0,0},
      {61,62,62,62,61,61,60,59,58,56,58,60,61,62,62,63,63,63,63,63}
    };
    unsigned pose,p,q;int dir;
    for(pose=0;pose<2;pose++)for(p=0;p<160*160;p++)if(sh_material_canvas[p]==crown[pose][0])for(dir=-1;dir<=1;dir+=2) {
      int ax=p%160,ay=p/160,top=pose?6:24,height=pose?20:18;
      if(!sh_material_head_matches(p,dir,crown[pose],pose?3:6,brow[pose],pose?6:7,-1))continue;
      for(q=0;q<160*160;q++)if(sh_material_face[q]) {
        int x=dir*((int)(q%160)-ax)+61,y=(int)(q/160)-ay+(pose?6:27);
        if(y<top || y>=top+height || x<edge[pose][y-top])sh_material_face[q]=0;
      }
    }
  }
}
__declspec(dllexport) unsigned sh_coop_material_ink(unsigned x,unsigned y,unsigned ink) {
  x-=sh_material_x;y-=sh_material_y;
  return ink>=2 && ink<=5 && (x>=160 || y>=160 || !sh_material_face[y*160+x])?ink|16:ink;
}
/* The game's native compressed font, decoded from the supplied ROM. */
__declspec(dllexport) unsigned sh_coop_font_row(unsigned ch,unsigned row) {
  static unsigned char font[8192];static unsigned loaded;
  unsigned a=0x15ae1e,i,j,bits,word,out=0;
  if(!loaded) {
    for(i=0;i<256;i++) {
      bits=sh_rom16(a);a+=2;
      for(j=0;j<16;j++) {
        if((bits&0x8000) && j>=2 && (bits&(1u<<(15-j))))word=(font[i*32+j*2-4]<<8)|font[i*32+j*2-3];
        else {word=sh_rom16(a);a+=2;}
        font[i*32+j*2]=word>>8;font[i*32+j*2+1]=word;
      }
    }
    loaded=1;
  }
  for(i=0;i<8;i++)if(((font[(ch&255)*32+(row&7)*4+i/2]>>((i&1)?0:4))&15)==((row&8)?1:15))out|=128>>i;
  return out;
}
/* Native idle pose 95. Each DMA segment contains a two-byte command followed
 * by 384 bytes of art; sprite pieces cross these segment boundaries. */
static unsigned sh_preview_raw(unsigned x,unsigned y) {
  unsigned w,h,sx,tile,off,first,table;
  if(y<32){if(x<1)return 0;x--;first=1;w=32;h=32;}
  else if(y<64){y-=32;first=17;w=32;h=32;}
  else {y-=64;first=33;w=16;h=8;}
  if(x>=w || y>=h)return 0;
  sx=w-1-x;tile=first+(sx/8)*(h/8)+y/8;off=tile*32+(y&7)*4+(sx&7)/2;
  table=sh_rom32(0xc0000+95*4);
  return (sh_rom8(sh_rom32(table+(off/384)*4)+2+off%384)>>((sx&1)?0:4))&15;
}
__declspec(dllexport) unsigned sh_coop_preview_ink(unsigned x,unsigned y) {
  static unsigned char image[33*72],seen[33*72];static unsigned loaded;
  unsigned i;
  if(x>=33 || y>=72)return 0;
  if(!loaded) {
    unsigned queue[33*72],best[33*72],bestn=0;
    for(i=0;i<33*72;i++)image[i]=sh_preview_raw(i%33,i/33);
    for(i=0;i<33*72;i++)if(!seen[i] && image[i]>=2 && image[i]<=5) {
      unsigned n=1,j;queue[0]=i;seen[i]=1;
      for(j=0;j<n;j++) {
        unsigned p=queue[j];int dx,dy;
        for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++) {
          int xx=(int)(p%33)+dx,yy=(int)(p/33)+dy;unsigned q;
          if(xx<0 || xx>=33 || yy<0 || yy>=72)continue;q=yy*33+xx;
          if(!seen[q] && image[q]>=2 && image[q]<=5){seen[q]=1;queue[n++]=q;}
        }
      }
      if(n>bestn){bestn=n;memcpy(best,queue,n*sizeof(unsigned));}
    }
    memset(seen,0,sizeof(seen));
    for(i=0;i<bestn;i++)seen[best[i]]=1;
    for(i=0;i<33*72;i++)if(image[i]>=2 && image[i]<=5 && !seen[i])image[i]|=16;
    loaded=1;
  }
  return image[y*33+x];
}
