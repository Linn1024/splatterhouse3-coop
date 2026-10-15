"""Exact reported QS1 and the complete raised-hands transformation sequence."""
from probe import *
from appearance_probe import actor_pixels,gray_components,raised_hands_mask
from native_rick_pose import stage_pose
import zipfile

source=ROOT/'diagnostics/appearance-chest2-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))

def check(label):
 raw=actor_pixels(c);expected=raised_hands_mask(raw)
 if expected is None:return False
 c.sh_coop_material_prepare()
 actual={xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}
 assert actual==expected,(label,'Mask/skin boundary',actual^expected)
 return True

p.restore(seed);p.run(1);assert check('actual QS1')
p.screenshot(ROOT/'diagnostics/raised-hands-qs1-fixed.png')
for frame,mask_count,skin_count in ((0xaf,118,54),(0xb0,118,11),(0xb1,137,13),
                                    (0xfc,118,54),(0xfd,118,11),(0xfe,137,13)):
 p.restore(seed);p.run(1);stage_pose(c,frame);assert check(hex(frame))
 raw=actor_pixels(c);mask=raised_hands_mask(raw)
 assert len(mask)==mask_count and len(gray_components(raw,True)[0]-mask)==skin_count
 print('PASS native pose',hex(frame),mask_count,'mask pixels;',skin_count,'touching skin pixels excluded')
checks=0
for replay in range(2):
 p.restore(seed)
 for t in range(90):
  p.run(1);checks+=check(('actual QS1 replay',replay,t))
assert checks>=20,checks
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS exact QS1 sequence replay:',checks,'raised-hands frames; original save unchanged')
