"""No uncolored mask islands in QS1 entrances or either form's jump poses."""
from probe import *
from appearance_probe import actor_pixels,gray_components,raised_hands_mask,leaning_guard_mask,normal_neck_mask
import zipfile

source=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'diagnostics/appearance-reported-qs1-v3.State'
original_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
checked=0;poses=set();screens=set()

def check_surface(label):
 global checked
 raw=actor_pixels(c);components=gray_components(raw,True)
 if not components:return
 # Ask for a fresh view of the same native SAT/art now exposed by the core.
 # During entrance DMA these can change even though sh_frames is constant.
 c.sh_coop_material_prepare()
 frame=int.from_bytes(c.sh_probe_player(1)[0x57a0:0x57a2],'big')
 mask=set(components[0])
 y0=min(y for x,y in raw);top=sorted(x for x,y in raw if y==y0)
 crown=[raw[x,y0] for x in top]
 if crown in ([4,2,3,3,4,4],[4,4,3,3,2,4]):
  # Native powered guard head reused by idle and landing frames. Independently
  # annotated cheek boundary; fingers must remain skin in either facing.
  facing_left=crown==[4,2,3,3,4,4];anchor=top[0] if facing_left else top[-1]
  limits={5:25,6:26,7:26,8:28,9:23,10:24,11:20,12:20,13:21,14:17,15:16}
  mask={xy for xy in mask if xy[1]-y0<5 or
        (xy[1]-y0<16 and (xy[0]-anchor+12 if facing_left else anchor-xy[0]+12)<=limits[xy[1]-y0])}
 raised=raised_hands_mask(raw)
 if raised is not None:mask=raised
 leaning=leaning_guard_mask(raw)
 if leaning is not None:mask=leaning
 normal=normal_neck_mask(raw)
 if normal is not None:mask=normal
 missing=[xy for xy in mask if c.sh_coop_material_ink(*xy,raw[xy])&16]
 assert not missing,(label,hex(frame),'White mask islands',len(missing),missing[:6])
 leaked=[xy for xy in raw if 2<=raw[xy]<=5 and xy not in mask
         if not c.sh_coop_material_ink(*xy,raw[xy])&16]
 assert not leaked,(label,'Mask color leaked onto skin',leaked[:6])
 checked+=1
 poses.add(frame)
 if label not in screens:
  screens.add(label);p.screenshot(ROOT/f'diagnostics/appearance-jump-fixed-{label}.png')

p.restore(d[4:4+int.from_bytes(d[:4],'little')])
for t in range(90):
 p.run(1);check_surface('entrance')
p.core.retro_reset()
for t in range(2400):
 p.run(1,(3,) if t%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('Cold campaign did not start')
p.run(120);assert c.sh_coop_debug(1);p.run(1);base=p.save()
families={}
for powered in (False,True):
 for direction in (6,7):
  for attack in (False,True):
   p.restore(base);c.sh_coop_appearance_set(0x030201)
   p.buttons[1]={direction};p.run(2);p.buttons[1]=set();p.run(2)
   if powered:
    assert c.sh_coop_debug(6);p.run(2);p.buttons[1]={1};p.run(1);p.buttons[1]=set();p.run(60)
   seen=set()
   for t in range(150):
    step=t%60;buttons={8,direction} if step<24 else set()
    if attack and 8<=step<30:buttons.add(0)
    p.buttons[1]=buttons;p.run(1)
    frame=int.from_bytes(c.sh_probe_player(1)[0x57a0:0x57a2],'big');seen.add(frame)
    check_surface(f'{int(powered)}-{direction}-{int(attack)}-{frame:03x}')
   families[powered,direction,attack]=seen
# Actual native airborne animation triples, for both facing directions/forms.
for family,expected in (((False,6,False),{0xd,0xe,0xf}),((False,7,False),{0x4b,0x4c,0x4d}),
                        ((True,6,False),{0x8b,0x8c,0x8d}),((True,7,False),{0xd8,0xd9,0xda})):
 assert expected<=families[family],(family,expected-families[family])
assert hashlib.sha256(source.read_bytes()).hexdigest()==original_hash
print('PASS QS1 entrance, normal/powered jumps, both facings and jump attacks:',checked,'complete mask/skin checks across',len(poses),'native poses; source save preserved')
