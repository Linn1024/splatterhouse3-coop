"""Two reported normal neck pixels and the upward-door top mask pixel."""
from probe import *
from appearance_probe import actor_pixels,gray_components,normal_neck_mask,rear_mask_edges
from native_rick_pose import stage_pose
import zipfile

source=ROOT/'diagnostics/appearance-neck-tip-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))

def actual_mask(raw):
 c.sh_coop_material_prepare()
 return {xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}

p.restore(seed);p.run(1);raw=actor_pixels(c);expected=normal_neck_mask(raw)
assert expected is not None and len(expected)==122
assert len(gray_components(raw,True)[0]-expected)==2
assert actual_mask(raw)==expected,'Reported neck pixels still use mask color'
p.screenshot(ROOT/'diagnostics/neck-pixels-qs1-fixed.png')
for frame in (6,7,8,0x44,0x45,0x46):
 p.restore(seed);p.run(1);stage_pose(c,frame);raw=actor_pixels(c);expected=normal_neck_mask(raw)
 assert expected is not None,(hex(frame),'Native walking head not recognized')
 assert actual_mask(raw)==expected,(hex(frame),'Neck/mask boundary')
 count={6:107,7:121,8:124,0x44:107,0x45:121,0x46:124}[frame]
 assert len(expected)==count,(hex(frame),len(expected))
 print('PASS normal walking pose',hex(frame),count,'mask pixels; annotated neck glints stay skin')

p.restore((ROOT/'diagnostics/checked-door-up-0-emerging.bin').read_bytes())
c.sh_coop_appearance_set(0x040501);checked=0
for t in range(35):
 p.run(1);raw=actor_pixels(c);expected=rear_mask_edges(raw)
 if expected is None:continue
 assert actual_mask(raw)==expected and len(expected)==23
 y=min(y for x,y in expected);top=sorted(x for x,yy in expected if yy==y)
 assert len(top)==3
 center=top[1];ink=c.sh_coop_material_ink(center,y,raw[center,y])
 assert ink==2 and c.sh_coop_appearance_rgb(ink,0xeeeecc)==0x355eb0,'Top mask pixel still white'
 # The pale scalp below the mask tip remains a skin highlight.
 scalp=c.sh_coop_material_ink(center,y+3,raw[center,y+3])
 assert scalp==18 and c.sh_coop_appearance_rgb(scalp,0xeeeecc)==0xeeeecc
 if t==10:p.screenshot(ROOT/'diagnostics/upward-door-top-pixel-fixed.png')
 checked+=1
assert checked>=20
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS upward door:',checked,'frames; colored top mask pixel and preserved scalp highlight; source QS1 unchanged')
