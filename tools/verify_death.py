exec(compile(open(__file__.replace('verify_death.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
p2=c.sh_probe_player(1);p2[0xb2]=0;p2[0xb3]=4
dead=False;recovered=False
for t in range(12):
 p.run(60);print(t,status(),'lives',p.ram()[0xb4],'flags',bytes(c.sh_probe_player(1)[0xa0:0xa4]).hex())
 if status()[7]==0:dead=True
 if dead and status()[7]==256:recovered=True
assert dead and recovered and p.ram()[0xb4]==1
p.screenshot(ROOT/'diagnostics/death.png')
