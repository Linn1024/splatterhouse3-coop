"""Reported blue skin stains, mirrored art, and vertical door materials."""
from probe import *
from appearance_probe import actor_pixels,gray_components,rear_mask_edges
import zipfile

source=ROOT/'diagnostics/appearance-blue-skin-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
c.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)

def tagged(raw):
 c.sh_coop_material_prepare()
 return {xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}

p.restore(seed);p.run(1);raw=actor_pixels(c)
x0=min(x for x,y in raw);y0=min(y for x,y in raw)
# Hand-annotated raised-arm mask outline, independently of connectivity.
left=[31,29,26,25,23,22,24,23,23,24,25,26,26,35,36,37]
expected={xy for xy,i in raw.items() if 2<=i<=5 and 6<=xy[1]-y0<=21
          and left[xy[1]-y0-6]<=xy[0]-x0<=41}
assert tagged(raw)==expected,('Reported shoulder/chest boundary',tagged(raw)^expected)
assert len(expected)==198,len(expected)
p.screenshot(ROOT/'diagnostics/blue-skin-fixed.png')

# Reflect the actual complete P2 SAT image, preserving all piece ordering and
# sources. This verifies the boundary when the same native art faces left.
v=c.al_probe_vram();base=(c.sh_probe_vdp_regs()[5]&127)<<9
def word(a):return v[a^1]*256+v[(a+1)^1]
def write_word(a,value):v[a^1]=(value>>8)&255;v[(a+1)^1]=value&255
link=0;pieces=[];visited=set()
while link not in visited and link<80:
 visited.add(link);a=base+link*8;attr=word(a+4)
 if c.sh_sprite_flags(link,attr)&3:
  pieces.append((a,word(a+6)&511,8+((v[(a+2)^1]&12)<<1),attr))
 link=v[(a+3)^1]&127
 if not link:break
axis=min(x for a,x,w,attr in pieces)+max(x+w-1 for a,x,w,attr in pieces)
for a,x,w,attr in pieces:write_word(a+6,axis-x-w+1);write_word(a+4,attr^0x800)
mirrored=actor_pixels(c)
assert tagged(mirrored)=={(axis-x,y) for x,y in expected},'Mirrored shoulder/chest boundary'

checks=rear=front=0
for name,file,frames in (('up','checked-door-up-0-emerging.bin',90),
                         ('down','down-door-seed.bin',300)):
 p.restore((ROOT/'diagnostics'/file).read_bytes());c.sh_coop_appearance_set(0x040501)
 for t in range(frames):
  p.run(1);raw=actor_pixels(c)
  if not raw:continue
  components=gray_components(raw,True)
  if not components:continue
  # Rear view still shows the mask's side edges. Scalp/limb highlights
  # remain skin; front/side masks retain their complete native surfaces.
  largest=components[0]
  back=rear_mask_edges(raw)
  expected=back if back is not None else largest
  assert tagged(raw)==expected,(name,t,'Door material boundary',len(tagged(raw)),len(expected),p.ram()[1:10].hex(),tagged(raw)^expected)
  if back is not None:assert len(back)==23
  rear+=back is not None;front+=back is None;checks+=1
  if t in (10,30,190):p.screenshot(ROOT/f'diagnostics/vertical-door-{name}-{t}-fixed.png')
assert rear>=20 and front>=100,(rear,front)
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS QS1: 198 mask pixels; shoulder/chest stay skin in both facings;',checks,
      'vertical door frames checked, including',rear,'rear-facing frames; source save unchanged')
