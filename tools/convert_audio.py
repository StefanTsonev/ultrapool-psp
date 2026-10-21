"""Convert owned audio to bounded-memory PSP PCM streams; exports no game scripts."""
import argparse,concurrent.futures,hashlib,json,math,re,struct,subprocess
from pathlib import Path

def sections(path):
 text=path.read_text(encoding='utf-8');result=[]
 for match in re.finditer(r'^\[([^\n]+)\]\s*\n([^\[]*)',text,re.M):
  header,body=match.groups();attrs=dict(re.findall(r'(\w+)="([^"]*)"',header));values={}
  for line in body.splitlines():
   if ' = ' in line:k,v=line.split(' = ',1);values[k]=v
  result.append((header.split()[0],attrs,values))
 return result

def bank_entries(root):
 entries=[]
 for scene in ['singletons/audio_manager.tscn','ball.tscn','main.tscn','ui/daily_end_bar.tscn']:
  parts=sections(root/scene);ext={a['id']:a['path'][6:] for kind,a,v in parts if kind=='ext_resource' and a.get('type')=='AudioStream'}
  subs={a['id']:v for kind,a,v in parts if kind=='sub_resource'}
  for kind,a,v in parts:
   if kind!='node' or not a.get('type','').startswith('AudioStreamPlayer'):continue
   key=a['name'];stream=v.get('stream','');ref=re.search(r'(Ext|Sub)Resource\("([^"]+)"\)',stream)
   if not ref:continue
   group=subs.get(ref[2],{}) if ref[1]=='Sub' else {};refs=[ref[2]] if ref[1]=='Ext' else [re.search(r'ExtResource\("([^"]+)"\)',value)[1] for k,value in group.items() if re.fullmatch(r'stream_\d+/stream',k)]
   paths=[ext[value] for value in refs];assert paths,('Empty audio group',key)
   entries.append(dict(key=key,paths=paths,gain=10**(float(v.get('volume_db','0'))/20),pitch=1.0 if a.get('parent')=='SFX' else float(v.get('pitch_scale','1')),random_pitch=float(group.get('random_pitch','1')),random_db=float(group.get('random_volume_offset_db','0')),mode=int(group.get('playback_mode','0')),polyphony=int(v.get('max_polyphony','1'))))
 # Named scene cue; the animation starts it at time zero, unity gain/pitch.
 if (root/'audio/BeastFromTheEast.wav').exists():
  entries.append(dict(key='tiger_cut_in',paths=['audio/BeastFromTheEast.wav'],gain=1,pitch=1,random_pitch=1,random_db=0,mode=0,polyphony=1))
 # Keep unused/demo assets addressable too, without assigning invented game triggers.
 used={path for entry in entries for path in entry['paths']}
 for index,path in enumerate(sorted(p for p in (root/'audio').rglob('*') if p.suffix.lower() in ('.wav','.ogg','.mp3'))):
  source=path.relative_to(root).as_posix()
  if source not in used:entries.append(dict(key=f'asset_{index:03}',paths=[source],gain=1,pitch=1,random_pitch=1,random_db=0,mode=0,polyphony=1))
 return entries

def convert(root,destination,ffmpeg):
 root=root.resolve();destination=destination.resolve()
 if root==destination or root in destination.parents:raise ValueError('Output must be outside the original project')
 if destination.exists():raise ValueError('Destination must be new; existing audio is preserved')
 files=sorted(p for p in (root/'audio').rglob('*') if p.suffix.lower() in ('.wav','.ogg','.mp3'))
 entries=bank_entries(root);mapping={p.relative_to(root).as_posix():f'{i:03}.upa' for i,p in enumerate(files)}
 for e in entries:
  for path in e['paths']:
   if path not in mapping:raise ValueError('Missing audio '+path)
 destination.mkdir(parents=True)
 def one(pair):
  path,out=pair;channels=2 if path.suffix.lower() in ('.ogg','.mp3') and 'seal_hurt' not in path.name else 1
  raw=subprocess.run([ffmpeg,'-v','error','-i',str(path),'-f','s16le','-acodec','pcm_s16le','-ar','22050','-ac',str(channels),'-'],capture_output=True,check=True).stdout
  if not raw or len(raw)%(2*channels):raise ValueError('Invalid decoded PCM '+str(path))
  (destination/out).write_bytes(struct.pack('<4sIII',b'UPA1',22050,channels,len(raw)//(2*channels))+raw)
  return dict(source=path.relative_to(root).as_posix(),output=out,channels=channels,frames=len(raw)//(2*channels),bytes=len(raw)+16,source_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:reports=list(pool.map(one,[(p,mapping[p.relative_to(root).as_posix()]) for p in files]))
 lines=['UPBANK1']
 for e in entries:
  lines.append(' '.join([e['key'],str(e['gain']),str(e['pitch']),str(e['random_pitch']),str(e['random_db']),str(e['mode']),str(e['polyphony']),str(len(e['paths']))]+[mapping[p] for p in e['paths']]))
 (destination/'bank.txt').write_text('\n'.join(lines)+'\n')
 report=dict(files=len(files),cues=len(entries),bytes=sum(v['bytes'] for v in reports),rate=22050,resources=reports,entries=entries)
 (destination/'conversion-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('resources','entries')}))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--ffmpeg',default='ffmpeg');a=p.parse_args();convert(a.original,a.output,a.ffmpeg)
