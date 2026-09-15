from probe import *
import struct
p=Probe(ROOT/'backups/before-sprite-debug-fixes/genesis_plus_gx_libretro.dll');c=p.core
p.run(2)
d=(ROOT/'diagnostics/user-save/Core.bin').read_bytes()[4:];h=struct.unpack('<8I',d[:32]);p.restore(d[32:32+h[2]]);s=C.create_string_buffer(d[32+h[2]:32+h[2]+h[3]]);c.sh_coop_restore(s,len(s)-1)
v=(C.c_uint*10)();c.sh_coop_status(v);print('old before',list(v));p.run(120);c.sh_coop_status(v);print('old after',list(v))
print('zstate',d[32+16+65536+8192:32+16+65536+8192+8].hex())

