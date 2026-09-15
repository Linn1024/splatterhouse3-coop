"""Independent meter changes in the two native-style footer rows."""
from probe import *

p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
for i in range(2400):
    p.run(1,(3,) if i%120==119 else ())
    if c.sh_coop_ready():break
else:raise AssertionError('game did not start')
p.run(2)
def region(x,y,w,h):
    data,fw,fh,pitch=p.frame
    return b''.join(data[yy*pitch+x*4:yy*pitch+(x+w)*4] for yy in range(y,y+h))
def life(player):return region(210,205+30*player,80,6)
def power(player):return region(60,205+30*player,100,6)
initial=[life(0),life(1)]
assert initial[0]==initial[1]
assert c.sh_coop_debug(7)
p.run(2)
assert life(0)!=initial[0] and life(1)==initial[1]
blue=life(0)
assert c.sh_coop_debug(8)
p.run(2)
assert life(0)==blue and life(1)!=initial[1]
empty=[power(0),power(1)]
assert c.sh_coop_debug(6)
p.run(2)
assert all(power(i)!=empty[i] for i in range(2))
assert power(0)==power(1)
p.screenshot(ROOT/'diagnostics/stacked-original-hud.png')
seed=p.save()
p.run(1,(3,));p.run(90)
assert life(0)==blue and life(1)==blue
assert power(0)==power(1)
p.restore(seed);p.run(1)
assert life(0)==blue and life(1)==blue
assert p.frame[1:]==(320,254,1280)
print('PASS stacked LIFE meters change independently; native-style POW fills both rows; map and old-size serialized frames retain taller output')
