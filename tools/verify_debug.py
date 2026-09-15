exec(compile(open(__file__.replace('verify_debug.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for command in (1,2,3,4):assert c.sh_coop_debug(command)
initial=p.ram()[0xb6:0xba];p.run(500)
assert p.ram()[0xb6:0xba]==initial,(initial,p.ram()[0xb6:0xba])
s=status();assert s[6:8]==[252,252] and s[8:10]==[80,80],s
assert p.ram()[0xb4]==99
assert c.sh_coop_debug(5);p.run(2);assert status()[6:8]==[256,256]
assert c.sh_coop_debug(7);p.run(2);assert status()[6]==224
print('PASS: invincibility, infinite power/lives, freeze timer, heal and damage')
for level in range(6):
 assert c.sh_coop_debug_stage(level)
 for i in range(3200):
  p.run(1,(3,) if i%120==100 else ())
  if i>5 and c.sh_coop_ready() and c.sh_coop_level()==level:break
 else:raise AssertionError(('stage failed',level,status(),p.ram()[:16].hex()))
 p.screenshot(ROOT/f'diagnostics/debug-stage-{level+1}.png');print('PASS stage',level+1,'frames',i)
