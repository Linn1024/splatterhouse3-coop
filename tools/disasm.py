from pathlib import Path
from capstone import *
import sys
r=Path('Splatterhouse 3 (USA).md').read_bytes();m=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
a=int(sys.argv[1],16);b=int(sys.argv[2],16)
for i in m.disasm(r[a:b],a): print(f'{i.address:06x}: {i.mnemonic:10} {i.op_str}')
