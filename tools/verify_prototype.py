from probe import *
p=Probe();p.restore((ROOT/'diagnostics/room.state').read_bytes());c=p.core;c.sh_coop_enable(1)
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
p.run(2);print(status());p.screenshot(ROOT/'diagnostics/coop-idle.png')
p.buttons[1]={7};p.run(20);print(status());p.screenshot(ROOT/'diagnostics/coop-right.png')
p.buttons[1]={0};p.run(20);print(status());p.screenshot(ROOT/'diagnostics/coop-attack.png')
