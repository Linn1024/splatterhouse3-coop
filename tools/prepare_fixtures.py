"""Local, reproducible native boot and injected cleared-room test fixtures."""
from probe import *

out=ROOT/'diagnostics';out.mkdir(exist_ok=True)
p=Probe()
for _ in range(12):
    p.run(120)
    p.run(1,(3,))
p.run(600)
(out/'room.state').write_bytes(p.save())
(out/'room.ram').write_bytes(p.ram())
p.screenshot(out/'room.png')
p.core.sh_coop_enable(1);p.run(2)
# This fixture deliberately removes enemies to isolate door coordination.
# It does not establish that the room has been cleared through combat.
for slot in range(2,64):
    p.write(0x551e+slot*2,b'\0\0')
p.run(120)
(out/'doors.state').write_bytes(p.save())
n=p.core.sh_coop_state_size();extra=C.create_string_buffer(n)
assert p.core.sh_coop_save(extra,n)
(out/'doors.extra').write_bytes(extra.raw)
print('Prepared native opening-room and injected exit fixtures.')
