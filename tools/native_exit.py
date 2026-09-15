from probe import *
p=Probe();p.restore((ROOT/'diagnostics/doors.state').read_bytes());p.core.sh_coop_enable(0)
for i in range(25):
 p.run(30,(7,));r=p.ram();print(i,r[:4].hex(),r[0x166:0x17e].hex());p.screenshot(ROOT/f'diagnostics/native-exit-{i}.png')
