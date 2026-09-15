"""Headless outer Libretro adapter boot, HUD and save replay verification."""
from probe import *

p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
for i in range(2400):
    p.run(1,(3,) if i%120==119 else ())
    if c.sh_coop_ready():break
else:raise AssertionError('game did not start')
p.run(2)
s=(C.c_uint*10)();c.sh_coop_status(s)
assert s[0] and s[1]>0,list(s)
seed=p.save()
def replay():
    p.buttons[1]={7,0};p.run(40,(4,));c.sh_coop_status(s)
    return list(s),hashlib.sha256(p.frame[0]).hexdigest()
before=list(s);a=replay();assert a[0][1]>before[1] and a[0][4]>before[4];p.restore(seed);b=replay()
assert a==b,(a,b)
p.screenshot(ROOT/'diagnostics/adapter-coop.png')
print('PASS: adapter boot, two-player input, video and serialized replay',list(s))
