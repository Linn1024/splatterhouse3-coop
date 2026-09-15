exec(compile(open(__file__.replace('explore_exit.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for slot in range(2,64):p.write(0x551e+slot*2,b'\0\0')
p.run(120)
r=p.ram()
for slot in range(1,64):
 a=0x551e+slot*2
 if int.from_bytes(r[a:a+2],'big')&0x8000:
  print(slot,r[a:a+2].hex(), 'type',int.from_bytes(r[a+0x300:a+0x302],'big'),'category',int.from_bytes(r[a+0x380:a+0x382],'big'),'x',int.from_bytes(r[0x5f9e+slot*4:0x5fa0+slot*4],'big'),'y',int.from_bytes(r[0x619e+slot*4:0x61a0+slot*4],'big'))
p.screenshot(ROOT/'diagnostics/doors.png');st=save();(ROOT/'diagnostics/doors.state').write_bytes(st[0]);(ROOT/'diagnostics/doors.extra').write_bytes(st[1]);print('flags',r[0x166:0x180].hex())
