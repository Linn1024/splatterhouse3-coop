from probe import *
from capstone import *
from collections import Counter
p=Probe();p.restore((ROOT/'diagnostics/room.state').read_bytes());c=p.core
c.al_probe_start();p.run(1);ptr=C.POINTER(C.c_uint)();n=c.al_probe_stop(C.byref(ptr));pcs=list(ptr[:n]);(ROOT/'diagnostics/trace.json').write_text(__import__('json').dumps(pcs))
rom=p.rom_path.read_bytes();md=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
lines=[]
for pc,count in sorted(Counter(pcs).items()):
 if pc>=len(rom):continue
 ins=next(md.disasm(rom[pc:pc+16],pc,count=1),None)
 if ins:lines.append(f'{count:5} {pc:06x}: {ins.mnemonic:10} {ins.op_str}')
(ROOT/'diagnostics/trace.asm').write_text('\n'.join(lines))
for a in range(0x5680,0x5800,16):print(f'{a:04x}',p.ram()[a:a+16].hex(' '))
