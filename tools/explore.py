from probe import *
p=Probe();p.restore((ROOT/'diagnostics/boot-11.state').read_bytes());p.run(600)
p.screenshot(ROOT/'diagnostics/room.png');(ROOT/'diagnostics/room.state').write_bytes(p.save());r=p.ram();(ROOT/'diagnostics/room.ram').write_bytes(r)
p.run(20,(7,));s=p.ram();p.screenshot(ROOT/'diagnostics/right.png')
for a in range(0,65536,2):
 x=int.from_bytes(r[a:a+2],'big'); y=int.from_bytes(s[a:a+2],'big')
 if 8<=y-x<=80 and 0<x<400: print(hex(a),x,y)
