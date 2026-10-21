"""Convert owner-supplied tutorial prose into a private native bank."""
from pathlib import Path
import argparse,csv,json,struct
from convert_ui_text import plain,ROOT
PSP_PROMPTS={
 'daily-1':'Offline daily challenges are now available. Press R on the menu to choose a date.',
 'intro-3':'Press {cross} to aim. Use the analog stick or {dpad}, adjust power with {L}/{R}, then press {cross} again to shoot.',
 'aim-reminder':'Press {cross} to aim, then press {cross} again to shoot. {circle} cancels aiming.',
 'shop-play':'Select Play with the {dpad} and press {cross} when your build is ready for the next round.',
 'shop-intro-3':'Select an offered ball with the {dpad} and press {cross} to buy it for your build.',
 'shop-intro-4':'Press {square} on a ball, choose a matching ball at the same level, then press {cross} to merge.',
 'shop-reserve-2':'Select an owned ball, press {select}, choose a reserve slot, then press {cross} to move it.',
 'mix-1':'In the cocktail bar, choose two different owned balls with {cross} to combine their abilities.',
 'passives-3':'Select a snack offer and press {cross} to spend a snack ticket and add it to your build.'
}
def convert(original,output):
 rows=json.loads((ROOT/'docs/TUTORIAL_RULES.json').read_text())['steps']
 with (original/'localization/translations.csv').open(encoding='utf-8-sig',newline='') as f:translations={r['key']:r['en']for r in csv.DictReader(f)}
 data=bytearray(struct.pack('<4sI',b'UAT1',len(rows)))
 for i,row in enumerate(rows):
  value=plain(PSP_PROMPTS.get(row['id'],translations.get(row['text_key'],row['text_key']).split('|')[0])).encode('ascii')
  if len(value)>=384:raise ValueError('Tutorial exceeds native capacity: '+row['id'])
  data+=struct.pack('<HHH',i,0,len(value))+value
 output.parent.mkdir(parents=True,exist_ok=True)
 with output.open('xb')as f:f.write(data)
 print(json.dumps({'entries':len(rows),'bytes':len(data),'private_resource':True}))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();convert(a.original,a.output)
