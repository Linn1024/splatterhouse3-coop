exec(compile(open(__file__.replace('verify_attacks.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for who in (0,1):
 restore(base)
 if who==0:p.buttons[1]=set();p.run(40,(7,))
 for t in range(500):
  p.buttons[1]={0} if who==1 and t%20<10 else set();p.run(1,{0} if who==0 and t%20<10 else set())
 r=p.ram();print('attacker',who,status(),[(i,int.from_bytes(r[0x551e+2*i:0x5520+2*i],'big'),int.from_bytes(r[0x5e9e+2*i:0x5ea0+2*i],'big')) for i in (19,20)])
 assert all(not(int.from_bytes(r[0x551e+2*i:0x5520+2*i],'big')&0x8000) for i in (19,20)), 'Opening enemies survived attack sequence'
 p.screenshot(ROOT/f'diagnostics/attack-p{who+1}.png')

