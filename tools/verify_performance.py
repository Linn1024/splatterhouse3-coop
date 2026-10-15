"""Validate archived crowded-QS1 benchmarks without timing-dependent assertions."""
from pathlib import Path
import json

root=Path(__file__).resolve().parents[1]/'diagnostics'
def read(name):return json.loads((root/name).read_text())
before=read('performance-before.json');after=read('performance-after.json');repeat=read('performance-repeat.json')
for mode in ('idle','combat'):
 old,new=before[mode],after[mode]
 assert len(new['update_trace'])==900
 assert new['update_trace']==[1]*900,(mode,'Missed or doubled native game update')
 assert old['no_update_frames']>=100,(mode,'Fixture did not reproduce crowd slowdown')
 assert new['video_hashes']==repeat[mode]['video_hashes'],(mode,'Saved replay changed')
 print('PASS',mode,old['game_updates'],'->',new['game_updates'],'game updates per 900 video frames; deterministic replay')
old=read('performance-native-before.json')['native'];new=read('performance-native-after.json')['native']
assert old['video_hashes']==new['video_hashes'],'Single-player video/timing changed'
print('PASS co-op disabled: all 900 rendered frames identical before/after')
