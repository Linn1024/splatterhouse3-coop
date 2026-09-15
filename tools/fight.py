from probe import *
p=Probe();p.restore((ROOT/'diagnostics/room.state').read_bytes());p.core.al_probe_cram.restype=C.POINTER(C.c_ubyte);r=bytes(p.core.al_probe_cram()[:128]);print(r.hex(' '));p.core.sh_coop_enable(1);p.run(2)
for t in range(10):
 p.run(120);s=(C.c_uint*10)();p.core.sh_coop_status(s);print(t,list(s));p.screenshot(ROOT/f'diagnostics/fight-{t}.png')
