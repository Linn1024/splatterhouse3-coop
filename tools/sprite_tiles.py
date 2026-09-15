from verify_attacks import *
seen=bytes(c.sh_probe_player(1)[0x800:0x1000]);print('P2 sprite tiles', [i for i,v in enumerate(seen) if v])
