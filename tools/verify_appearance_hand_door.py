"""Reported leaning hand and visible mask edges during upward door walks."""
from probe import *
from appearance_probe import actor_pixels,gray_components,rear_mask_edges
from native_rick_pose import stage_pose
import zipfile

source=ROOT/'diagnostics/appearance-hand-door-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))

def mask_pixels(raw):
 c.sh_coop_material_prepare()
 return {xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}

def check_hand(mirrored=False):
 raw=actor_pixels(c);x0=min(x for x,y in raw);x1=max(x for x,y in raw);y0=min(y for x,y in raw)
 largest=gray_components(raw,True)[0];edge=[14,15,13,11,16,15,22,19,22,22,23]
 expected={xy for xy in largest if xy[1]-y0<5 or xy[1]-y0<16 and
           (x1-xy[0] if mirrored else xy[0]-x0)>=edge[xy[1]-y0-5]}
 assert len(expected)==193 and len(largest-expected)==41
 assert mask_pixels(raw)==expected,'Mask color leaked onto the leaning hand'

p.restore(seed);p.run(1);check_hand();p.screenshot(ROOT/'diagnostics/hand-door-qs1-fixed.png')
for frame in (0xaa,0xf7):
 p.restore(seed);p.run(1);stage_pose(c,frame);check_hand(frame==0xaa)
 print('PASS leaning guard',hex(frame),'193 mask pixels; 41 touching skin pixels excluded')

# Replay the native upward door walk, including all three rear-facing poses.
p.restore((ROOT/'diagnostics/checked-door-up-0-emerging.bin').read_bytes())
c.sh_coop_appearance_set(0x040501)
checked=0
for t in range(35):
 p.run(1);raw=actor_pixels(c);expected=rear_mask_edges(raw)
 if expected is None:continue
 assert len(expected)==23
 assert mask_pixels(raw)==expected,'Rear mask edges white or mask color on scalp/arm'
 if t==10:p.screenshot(ROOT/'diagnostics/upward-door-mask-edges-fixed.png')
 checked+=1
assert checked>=20,checked
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS upward door walk:',checked,'frames with 23 colored mask-edge pixels; source QS1 unchanged')
