"""Native-font appearance menu, material isolation, old saves and replay."""
from probe import *
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
def pulse(buttons=(),who=0):
 p.buttons[1]=set(buttons) if who else set();p.run(1,buttons if not who else ());p.buttons[1]=set();p.run(2)
p.run(600);pulse((3,));p.run(180);pulse((3,));p.run(60);pulse((5,));pulse((3,));p.run(100)
assert p.ram()[1:3]==bytes([8,3]) and c.sh_coop_appearance_get()==0
pulse((1,),1);assert c.sh_coop_appearance_menu_get()&1
p.screenshot(ROOT/'diagnostics/appearance-original.png')
# Match the reference: teal clothes, magenta mask, tan skin; use P2 inputs.
pulse((7,),1);pulse((5,),1)
for _ in range(2):pulse((7,),1)
pulse((5,),1)
for _ in range(3):pulse((7,),1)
assert c.sh_coop_appearance_get()==0x030201
p.screenshot(ROOT/'diagnostics/appearance-costume.png')
seed=p.save();pulse((7,));assert c.sh_coop_appearance_get()!=0x030201
p.restore(seed);p.run(1);assert c.sh_coop_appearance_get()==0x030201 and c.sh_coop_appearance_menu_get()&1
# Wrapping both ways, independently of the other materials.
for _ in range(10):pulse((7,))
assert c.sh_coop_appearance_get()==0x030201
for _ in range(10):pulse((6,))
assert c.sh_coop_appearance_get()==0x030201
# Held Start must close the overlay without also leaving native Options.
p.run(1,(3,));p.run(12,(3,));assert p.ram()[1:3]==bytes([8,3])
p.run(2);assert not c.sh_coop_appearance_menu_get()&9
# Exit original options normally and start campaign.
for _ in range(5):pulse((5,))
pulse((3,));p.run(120);pulse((3,));p.run(60);pulse((3,))
for i in range(2400):
 p.run(1,(3,) if i%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('Campaign did not start')
p.run(120);assert c.sh_coop_appearance_get()==0x030201
# Isolate palette changes at the identical deterministic gameplay instant.
seed=p.save();p.run(1);colored=p.frame[0];p.screenshot(ROOT/'diagnostics/appearance-gameplay.png')
p.restore(seed);c.sh_coop_appearance_set(0);p.run(1);original=p.frame[0]
s=(C.c_uint*10)();c.sh_coop_status(s)
# Every changed playfield pixel must belong to the private P2 body.
changed=[]
for y in range(194):
 for x in range(320):
  a=y*320*4+x*4
  if colored[a:a+4]!=original[a:a+4]:changed.append((x,y))
assert len(changed)>100,len(changed)
assert max(x for x,y in changed)-min(x for x,y in changed)<65,changed[:5]
# Each material can change alone and leaves the other material pixels intact.
material_pixels=[]
for material,choice in enumerate((1,2,3)):
 p.restore(seed);c.sh_coop_appearance_set(choice<<(material*8));p.run(1)
 assert p.frame[0]!=original
 data=p.frame[0]
 material_pixels.append({(x,y) for y in range(194) for x in range(320) if data[y*1280+x*4:y*1280+x*4+4]!=original[y*1280+x*4:y*1280+x*4+4]})
assert all(not material_pixels[a]&material_pixels[b] for a,b in ((0,1),(0,2),(1,2)))
mask=material_pixels[1]
assert max(x for x,y in mask)-min(x for x,y in mask)<25
assert max(y for x,y in mask)-min(y for x,y in mask)<22
p.restore(seed)
def replay():
 p.buttons[1]={7,0};p.run(30,(4,));return p.save(),p.frame[0]
a=replay();p.restore(seed);assert replay()==a
# Reserved metadata migration preserves the old snapshot layout.
assert c.sh_coop_state_size()==137360
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
q=c.sh_probe_player(0)
for i in range(0x314,0x31e):q[i]=0
assert c.sh_coop_appearance_get()==0 and c.sh_coop_appearance_menu_get()==0
p.restore(seed);assert c.sh_coop_appearance_get()==0x030201
# P2 powered transformation retains all independently selected materials.
p.buttons[1]=set();assert c.sh_coop_debug(6);p.run(2)
p.buttons[1]={1};p.run(1);p.buttons[1]=set();p.run(50)
assert int.from_bytes(c.sh_probe_player(1)[0x57a0:0x57a2],'big')>=125
assert c.sh_coop_appearance_get()==0x030201
p.screenshot(ROOT/'diagnostics/appearance-powered.png')
# Appearance options are absent from the map, including either pad's Transform.
p.buttons[1]=set();p.run(150);pulse((3,));p.run(100);assert c.sh_coop_paused(),p.ram()[:12].hex()
assert not c.sh_coop_appearance_available()
for who in (0,1):
 pulse((1,),who);assert not c.sh_coop_appearance_menu_get()&1
assert c.sh_coop_paused()
# An older save with the removed map overlay open closes it safely.
c.sh_coop_appearance_menu_set(1);p.run(2)
assert not c.sh_coop_appearance_menu_get()&9 and c.sh_coop_paused()
p.screenshot(ROOT/'diagnostics/appearance-map.png')
# Exercise the standalone host with the same inputs and compare its menu pixels.
import subprocess
standalone=ROOT/'Splatterhouse3-Coop.exe'
if standalone.exists():
 actions=[(600,0,3),(783,0,3),(846,0,5),(849,0,3),(952,1,1),(955,1,7),(958,1,5),(961,1,7),(964,1,7),(967,1,5),(970,1,7),(973,1,7),(976,1,7)]
 script=ROOT/'diagnostics/appearance-standalone.txt'
 script.write_text(''.join(f'{frame} {frame+1} {player} {button}\n' for frame,player,button in actions))
 subprocess.run([str(standalone),'--test','1000','--script',str(script)],cwd=ROOT,check=True,creationflags=subprocess.CREATE_NO_WINDOW)
 assert Image.open(ROOT/'diagnostics/launcher.bmp').convert('RGB').tobytes()==Image.open(ROOT/'diagnostics/appearance-costume.png').convert('RGB').tobytes()
print('PASS appearance: native font/preview, reference colors, P2 menu input, wrap, material isolation, input release, gameplay, removed map access and saved deterministic replay')
