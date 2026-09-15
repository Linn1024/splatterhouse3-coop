from probe import *
import struct
p=Probe();c=p.core
raw=(ROOT/'diagnostics/user-save/Core.bin').read_bytes();d=raw[4:];h=struct.unpack('<8I',d[:32]);native=bytearray(d[32:32+h[2]]);extra=d[32+h[2]:32+h[2]+h[3]]
def restore(bus):
 n=bytearray(native);n[16+65536+8192]=bus;p.restore(bytes(n));b=C.create_string_buffer(extra);assert c.sh_coop_restore(b,len(extra))
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
for bus in (1,3):
 restore(bus);print('before',bus,status());p.run(120);print('after',bus,status());p.screenshot(ROOT/f'diagnostics/recover-bus{bus}.png')

