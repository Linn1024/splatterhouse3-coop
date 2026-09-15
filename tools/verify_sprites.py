exec(compile(open(__file__.replace('verify_sprites.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for i in range(2,64):p.write(0x551e+i*2,b'\0\0')
p.buttons[1]={7};p.run(55);p.buttons[1]=set();p.run(4)
seen=set()
for t in range(65):
 p.buttons[1]={0} if t%20<10 else set();p.run(1)
 p.screenshot(ROOT/'diagnostics/sprite-check.png');im=Image.open(ROOT/'diagnostics/sprite-check.png')
 r=p.ram();camera=int.from_bytes(r[0xf0:0xf2],'big');s=status()
 def blue(who):
  x=s[2+2*who]-camera
  return sum(b>r+25 and b>g for r,g,b in im.crop((x-20,60,x+20,162)).getdata())
 assert blue(0)>5,('P1 blue clothing missing',t,blue(0))
 assert blue(1)==0,('P2 turned blue',t,blue(1))
 q=c.sh_probe_player(1);seen.add(int.from_bytes(q[0x57a0:0x57a2],'big'))
 if t==7:p.screenshot(ROOT/'diagnostics/red-rick-punch.png')
print('PASS: P1 stays blue and P2 stays red across',len(seen),'native attack frames',sorted(seen))
