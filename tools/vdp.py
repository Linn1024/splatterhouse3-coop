from probe import *
p=Probe();p.restore((ROOT/'diagnostics/room.state').read_bytes());p.run(1)
for fn in ('al_probe_vram','sh_probe_vdp_regs'):getattr(p.core,fn).restype=C.POINTER(C.c_ubyte)
r=bytes(p.core.sh_probe_vdp_regs()[:32]);v=bytes(p.core.al_probe_vram()[:65536]);print('VDP',r.hex(' '))
def w(a): return v[a]+v[a+1]*256
for start,end in ((0xc000,0xe000),(0xe000,0xf000),(0xf000,0x10000)):
 used={w(a)&2047 for a in range(start,end,2)}
 print(hex(start),min(used),max(used),sorted(used)[-30:])
print('objects')
r=p.ram()
for a in range(0x551e,0x671e,0x80): print(hex(a),r[a:a+12].hex(' '))
