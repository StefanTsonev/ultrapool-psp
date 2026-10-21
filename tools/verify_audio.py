"""Verify a personal converted audio pack without shipping original content."""
from pathlib import Path
import argparse,hashlib,json,struct

def verify(folder,original=None):
 report=json.loads((folder/'conversion-report.json').read_text());paths=set();total=0
 for entry in report['resources']:
  path=folder/entry['output'];raw=path.read_bytes();magic,rate,channels,frames=struct.unpack('<4sIII',raw[:16])
  if magic!=b'UPA1' or rate!=22050 or channels not in (1,2) or frames!=entry['frames'] or len(raw)!=16+2*channels*frames:raise ValueError('Invalid PCM stream '+str(path))
  if entry['output'] in paths:raise ValueError('Duplicate output')
  paths.add(entry['output']);total+=len(raw)
  if original and hashlib.sha256((original/entry['source']).read_bytes()).hexdigest()!=entry['source_sha256']:raise ValueError('Original audio changed '+entry['source'])
 used={source for entry in report['entries'] for source in entry['paths']}
 if used!={entry['source'] for entry in report['resources']}:raise ValueError('Unaddressable audio resources')
 if {p.name for p in folder.glob('*.upa')}!=paths:raise ValueError('Extra or missing streams')
 return dict(streams=len(paths),cues=len(report['entries']),pcm_bytes=total,original_hashes_checked=bool(original))
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('folder',type=Path);parser.add_argument('--original',type=Path);args=parser.parse_args();print(json.dumps(verify(args.folder,args.original),indent=2))
