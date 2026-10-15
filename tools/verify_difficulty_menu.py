"""Fifth native Options/LEVEL choice, wrapping, redraw, saves and campaign."""
from probe import *
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
def pulse(buttons):p.run(1,buttons);p.run(2)
def level():return 4 if c.sh_coop_debug_flags()&16 else p.ram()[0xf02a]
p.run(600);pulse((3,));p.run(180);pulse((3,));p.run(60)
pulse((5,));pulse((3,));p.run(100)
assert p.ram()[1:3]==bytes([8,3]) and level()==1
for expected in (2,3,4,0,1):
 pulse((7,));assert level()==expected
 assert p.ram()[0xf02a]==min(expected,3)
for expected in (0,4,3,2,1):
 pulse((6,));assert level()==expected
for _ in range(3):pulse((7,))
assert level()==4
p.screenshot(ROOT/'diagnostics/ultra-hard-native-menu.png')
def label_ink():
 data,w,h,pitch=p.frame
 return [data[y*pitch+x*4+2]>160 and data[y*pitch+x*4+1]>160 and data[y*pitch+x*4]<90 for y in range(64,72) for x in range(192,280)]
label=label_ink();assert sum(label)>80
seed=p.save();pulse((7,));assert level()==0
p.restore(seed);p.run(2);assert level()==4
# Leave options and reenter: the native initial draw must retain the label.
for _ in range(5):pulse((5,))
pulse((3,));p.run(120);pulse((3,));p.run(60)
# Title cursor returns to START. Select OPTIONS again.
pulse((5,));pulse((3,));p.run(100)
assert p.ram()[1:3]==bytes([8,3]),p.ram()[:8].hex()
assert level()==4
p.screenshot(ROOT/'diagnostics/ultra-hard-native-reentered.png')
assert label_ink()==label, 'Native menu reentry lost part of the Ultra Hard label'
# Return to title and start the campaign with the fifth choice.
for _ in range(5):pulse((5,))
pulse((3,));p.run(120);pulse((3,));p.run(60);pulse((3,))
for i in range(2400):
 p.run(1,(3,) if i%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('Campaign did not start')
assert level()==4 and p.ram()[0xf02a]==3
saved=p.save();p.core.retro_reset();p.run(2);assert not c.sh_coop_debug_flags()&16
p.restore(saved);p.run(2);assert level()==4
print('PASS native fifth difficulty, both-direction wrapping, options reentry, saved selection, campaign/Game Master base and reset')
