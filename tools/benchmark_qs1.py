"""Measure host frame cost and native co-op update cadence from an archived QS1."""
from probe import *
import zipfile,time,statistics,json

directory=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'bizhawk'
output=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/'diagnostics/performance-current.json'
source=ROOT/'diagnostics/performance-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(directory/'splatterhouse_coop_libretro.dll');c=C.CDLL(str(directory/'splatterhouse_engine.dll'))
results={}
for mode in (('native',) if len(sys.argv)>3 and sys.argv[3]=='native' else ('idle','combat')):
 p.restore(seed);durations=[];updates=[];hashes=[];s=(C.c_uint*10)();c.sh_coop_status(s);last=s[1]
 if mode=='native':c.sh_coop_enable(0);last=0
 for t in range(900):
  p.buttons[1]={0} if mode=='combat' and t%24<12 else set()
  start=time.perf_counter_ns();p.run(1,(0,) if mode=='combat' and t%24<12 else ());durations.append((time.perf_counter_ns()-start)/1e6)
  c.sh_coop_status(s);updates.append((s[1]-last)&0xffffffff);last=s[1]
  hashes.append(hashlib.sha256(p.frame[0]).hexdigest())
 results[mode]={'median_ms':statistics.median(durations),'p95_ms':sorted(durations)[854],
                'max_ms':max(durations),'over_16ms':sum(t>16.67 for t in durations),
                'game_updates':sum(updates),'no_update_frames':updates.count(0),
                'durations_ms':durations,'update_trace':updates,'video_hashes':hashes}
 print(mode, {k:v for k,v in results[mode].items() if k not in ('durations_ms','update_trace','video_hashes')})
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
output.write_text(json.dumps(results))
