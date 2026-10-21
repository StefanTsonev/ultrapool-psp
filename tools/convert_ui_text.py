"""Convert owner-supplied English achievement prose into a private bounded text bank."""
import argparse,csv,json,re,struct,unicodedata
from pathlib import Path
from build_catalog import prop
ROOT=Path(__file__).resolve().parents[1]
def plain(text,amount=0):
 text=text.replace('[br]',' ').replace('\n',' ')
 if amount>0:text=text.replace('{}',str(amount))
 text=re.sub(r'\{p:([^}]+)\}',lambda m:m[1].split('/')[min(0 if amount==1 else 1,len(m[1].split('/'))-1)],text)
 text=re.sub(r'\[/?[a-z_]+(?:=[^]]*)?\]','',text)
 text=re.sub(r'#([A-Z_]+)',lambda m:m[1].replace('_',' ').lower(),text)
 text=text.translate(str.maketrans({'\u2019':"'",'\u2018':"'",'\u201c':'"','\u201d':'"','\u2013':'-','\u2014':'-','\u00d7':'x','\u20ac':'EUR'}))
 text=unicodedata.normalize('NFKD',text).encode('ascii','ignore').decode()
 return ' '.join(text.split())
def convert(original,output,rules=None):
 original=original.resolve();rows=rules if rules is not None else json.loads((ROOT/'docs/PROGRESSION_RULES.json').read_text())['achievements']
 with (original/'localization/translations.csv').open(encoding='utf-8-sig',newline='') as f:translations={r['key']:r['en'] for r in csv.DictReader(f)}
 def tr(key):return translations.get(key,key) if key else ''
 result=bytearray(struct.pack('<4sI',b'UAT1',len(rows)))
 for index,row in enumerate(rows):
  path=(original/row['source']).resolve()
  if not path.is_relative_to(original):raise ValueError('Resource path escapes original root')
  resource=path.read_text(encoding='utf-8-sig');title=tr(prop(resource,'title',''))
  desc=tr('ACH_MIX_BALLS' if row.get('mix_a') else prop(resource,'description',''))
  match=re.search(r'^extra_localization_tokens\s*=.*$',resource,re.M)
  tokens=[f"BALL_{row['mix_a']}_NAME",f"BALL_{row['mix_b']}_NAME"] if row.get('mix_a') else re.findall(r'"([^"\n]+)"',match[0]) if match else []
  for n,key in enumerate(tokens,1):desc=desc.replace(f'[{n}]',tr(key))
  title=plain(title);desc=plain(desc,row.get('amount',0));a,b=title.encode('ascii'),desc.encode('ascii')
  if len(a)>=96 or len(b)>=384 or '\0' in title+desc:raise ValueError('Achievement text exceeds native bounds: '+row['id'])
  result+=struct.pack('<HHH',index,len(a),len(b))+a+b
 output.parent.mkdir(parents=True,exist_ok=True)
 with output.open('xb') as f:f.write(result)
 return {'entries':len(rows),'bytes':len(result),'language':'en','private_resource':True,'resident_bytes':len(rows)*480}
def main():
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();print(json.dumps(convert(a.original,a.output),indent=2))
if __name__=='__main__':main()
