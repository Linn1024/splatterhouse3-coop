from verify_exit import *
c.al_probe_start();p.run(1);ptr=C.POINTER(C.c_uint)();n=c.al_probe_stop(C.byref(ptr));from collections import Counter
print('pc',Counter(ptr[:n]).most_common(15));print('state',p.ram()[:16].hex());print('stack',p.ram()[0xff00:].hex());p.screenshot(ROOT/'diagnostics/exit-end.png')

