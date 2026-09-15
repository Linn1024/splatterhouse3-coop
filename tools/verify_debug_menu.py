from probe import *
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
for i in range(2400):
 p.run(1,(3,) if i%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('game did not start')
p.run(2)
def pulse(buttons):p.run(1,buttons);p.run(1)
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
pulse((3,));before=status();p.run(90);assert status()==before
assert c.sh_coop_paused();p.screenshot(ROOT/'diagnostics/pause-map.png')
pulse((0,1,8));assert c.sh_coop_menu_get()&64
for i in range(4):
 pulse((5,));pulse((8,));assert c.sh_coop_debug_flags()&(1<<i)
p.screenshot(ROOT/'diagnostics/debug-menu.png')
seed=p.save();p.run(10);p.restore(seed);assert c.sh_coop_menu_get()&65==65
assert c.sh_coop_debug_flags()==15
pulse((3,));p.run(150);assert status()[1]>before[1]
assert status()[8:10]==[80,80]
print('PASS: Start shows native map, A+B+C opens menu, toggles work, menu saves/loads, Start resumes')
