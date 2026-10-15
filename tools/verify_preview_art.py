"""Compare the appearance preview with the actual native DMA/SAT idle pose."""
from probe import *

p=Probe();p.restore((ROOT/'diagnostics/room.state').read_bytes())
c=p.core;c.sh_coop_enable(1);p.run(2)
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
c.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
assert p.ram()[0x57a0:0x57a2]==bytes([0,95])
v=c.al_probe_vram();base=(c.sh_probe_vdp_regs()[5]&127)<<9
def word(a):return (v[a^1]<<8)|v[(a+1)^1]
pieces=[]
for i in range(80):
 flags=c.sh_probe_player(0)[0x550+i];attr=word(base+i*8+4)
 if flags&8 and not flags&3 and (attr&2047)<96:
  pieces.append((word(base+i*8+6),word(base+i*8),v[(base+i*8+2)^1],attr))
assert len(pieces)==3
ox=min(q[0] for q in pieces);oy=min(q[1] for q in pieces)
expected=[0]*(33*72)
for x,y,size,attr in pieces:
 width=8+((size&12)<<1);height=8+((size&3)<<3)
 for yy in range(height):
  for xx in range(width):
   sx=width-1-xx if attr&0x800 else xx
   sy=height-1-yy if attr&0x1000 else yy
   tile=(attr&2047)+(sx//8)*(height//8)+sy//8
   a=tile*32+(sy%8)*4+(sx%8)//2
   ink=(v[a^1]>>(0 if sx&1 else 4))&15
   dx=x-ox+xx;dy=y-oy+yy
   if ink and dx<33 and dy<72:expected[dy*33+dx]=ink
actual=[c.sh_coop_preview_ink(i%33,i//33)&15 for i in range(33*72)]
assert actual==expected,'Preview does not match native sprite artwork'
# These native skin glints sit close to the mask, but outside its surface.
# An expanded face rectangle previously recolored all eight with the mask.
skin_glints=((15,3),(13,7),(13,8),(14,12),(15,16),(19,16),(14,17),(16,17))
for x,y in skin_glints:
 assert c.sh_coop_preview_ink(x,y)==18,('Skin glint classified as mask',x,y)
assert sum(2<=c.sh_coop_preview_ink(x,y)<=5 for y in range(72) for x in range(33))==107
# The live P2 renderer must use the same boundary as the native idle preview.
p2pieces=[]
for i in range(80):
 flags=c.sh_probe_player(0)[0x550+i]
 if flags&8 and flags&3:p2pieces.append((word(base+i*8+6),word(base+i*8)))
px=min(x for x,y in p2pieces);py=min(y for x,y in p2pieces)
for y in range(72):
 for x in range(33):
  material=c.sh_coop_preview_ink(x,y)
  if 2<=material&15<=5:
   assert c.sh_coop_material_ink(px+x,py+y,material&15)==material,('Preview/gameplay material mismatch',x,y)
print('PASS preview matches every pixel of the native DMA/SAT pose, including art spanning segment command headers')
print('PASS mask-only region excludes temple, neck and chest glints in both preview and gameplay')
