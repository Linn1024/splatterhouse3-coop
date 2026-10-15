"""Read an actor's native indexed sprite image for appearance regressions."""
from probe import C

def actor_pixels(core,owner=1):
 core.al_probe_vram.restype=C.POINTER(C.c_ubyte)
 core.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)
 core.sh_sprite_row.argtypes=[C.c_uint,C.c_uint,C.c_uint]
 core.sh_sprite_row.restype=C.POINTER(C.c_ubyte)
 v=core.al_probe_vram();base=(core.sh_probe_vdp_regs()[5]&127)<<9
 def word(a):return (v[a^1]<<8)|v[(a+1)^1]
 pixels={};link=0;visited=set()
 for _ in range(80):
  if link>=80 or link in visited:break
  visited.add(link);a=base+link*8;attr=word(a+4);flags=core.sh_sprite_flags(link,attr)
  if flags&8 and bool(flags&3)==bool(owner):
   x,y=word(a+6)&511,word(a)&511;size=v[(a+2)^1]
   width=8+((size&12)<<1);height=8+((size&3)<<3)
   for yy in range(height):
    for column in range(width//8):
     tile=((attr&2047)+(width//8-1-column if attr&0x800 else column)*(height//8)+(height//8-1-yy//8 if attr&0x1000 else yy//8))&2047
     row=core.sh_sprite_row((attr&0xf800)|tile,(yy&7)<<3,flags)
     data=bytes(row[:8]) if row else None
     for xx in range(8):
      if data is not None:ink=data[xx]
      else:
       sx=7-xx if attr&0x800 else xx;sy=7-(yy&7) if attr&0x1000 else yy&7
       ink=(v[(tile*32+sy*4+sx//2)^1]>>(0 if sx&1 else 4))&15
      if ink:pixels.setdefault((x+column*8+xx,y+yy),ink)
  link=v[(a+3)^1]&127
  if not link:break
 return pixels

def gray_components(pixels,diagonal=False):
 remaining={xy for xy,ink in pixels.items() if 2<=ink<=5};components=[]
 neighbors=[(dx,dy) for dx in (-1,0,1) for dy in (-1,0,1)
            if (dx or dy) and (diagonal or abs(dx)+abs(dy)==1)]
 while remaining:
  queue=[min(remaining,key=lambda xy:(xy[1],xy[0]))];remaining.remove(queue[0])
  for x,y in queue:
   for dx,dy in neighbors:
    xy=x+dx,y+dy
    if xy in remaining:remaining.remove(xy);queue.append(xy)
  components.append(set(queue))
 return sorted(components,key=len,reverse=True)


def raised_hands_mask(pixels):
 """Annotated native AF/B0/B1 face outlines, including reflected art."""
 profiles=[([3]*5,[3,3,2,2,4,4,4,3,4]),
           ([4]*5,[4,4,2,2,5,5,5,4,5]),
           ([3,2,3,4,3,4,4],[3,2,2,3,4,5,4,3,2,4,4])]
 for kind,(crown,brow) in enumerate(profiles):
  for (ax,ay),ink in pixels.items():
   if ink!=crown[0]:continue
   for direction in (-1,1):
    if [pixels.get((ax+direction*k,ay)) for k in range(len(crown))]!=crown:continue
    if [pixels.get((ax+direction*(k-2),ay+1)) for k in range(len(brow))]!=brow:continue
    mask=gray_components(pixels,True)[0];result=set()
    for xy in mask:
     x=direction*(xy[0]-ax)+(18 if kind==2 else 21);y=xy[1]-ay+(0 if kind==2 else 1)
     if kind==2:
      keep=y<7 or y<13 and x<=[23,22,22,23,21,16][y-7]
     else:keep=y<9 or y<17 and x>=[23,25,25,25,25,24,24,25][y-9]
     if keep:result.add(xy)
    return result
 return None


def rear_mask_edges(pixels):
 """The 23 visible mask-edge pixels, including the top-center pixel."""
 for (x,y),ink in pixels.items():
  if [pixels.get((x+k,y)) for k in range(3)]!=[4,2,4]:continue
  if [pixels.get((x+k-2,y+1)) for k in range(7)]!=[3,9,8,4,8,9,3]:continue
  center=x+1
  return {xy for xy,i in pixels.items() if 2<=i<=5 and 0<=xy[1]-y<8
          and abs(xy[0]-center)<=5 and (abs(xy[0]-center)>=2 or xy[1]==y)}
 return None


def normal_neck_mask(pixels):
 """Normal walking mask with two annotated diagonal skin glints removed."""
 for (ax,ay),ink in pixels.items():
  if ink!=5:continue
  for direction in (-1,1):
   if [pixels.get((ax+direction*k,ay)) for k in range(6)]!=[5,5,4,3,4,5]:continue
   if [pixels.get((ax+direction*(k+3),ay+1)) for k in range(5)] not in ([3,3,3,4,5],[4,3,3,4,5]):continue
   skin={(ax+direction*dx,ay+dy) for dx,dy in ((-2,11),(3,15))
         if pixels.get((ax+direction*dx,ay+dy))==2}
   return gray_components(pixels,True)[0]-skin
 return None


def leaning_guard_mask(pixels):
 """Annotated AA/F7 cheek boundary where the hand overlaps the temple."""
 for (ax,ay),ink in pixels.items():
  if ink!=4:continue
  for direction in (-1,1):
   if [pixels.get((ax+direction*k,ay)) for k in range(6)]!=[4,4,3,2,2,4]:continue
   if [pixels.get((ax+direction*(k-3),ay+1)) for k in range(11)]!=[4,2,4,3,2,4,3,3,2,3,4]:continue
   edge=[14,15,13,11,16,15,22,19,22,22,23]
   return {xy for xy in gray_components(pixels,True)[0] if xy[1]-ay<5 or
           xy[1]-ay<16 and direction*(xy[0]-ax)+22>=edge[xy[1]-ay-5]}
 return None
