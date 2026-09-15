exec(compile(open(__file__.replace('verify_debug_respawn.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
assert c.sh_coop_debug(9);p.run(2);assert status()[6:8]==[0,0],status()
for i in range(900):
 p.run(1)
 if status()[6:8]==[256,256]:break
else:raise AssertionError(('Respawn failed',status()))
assert p.ram()[0xb4]==0,p.ram()[0xb4]
assert c.sh_coop_debug(8);p.run(2);assert status()[7]==224
c.sh_coop_enable(0);c.sh_coop_enable(1);assert c.sh_coop_debug_flags()==0 and c.sh_coop_menu_get()==0
print('PASS: defeat both, independent native respawns, shared lives, P2 damage, reset clears cheats')
