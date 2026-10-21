"""Convert owner-supplied descriptions/dialogue. Never exports proprietary prose to source."""
import argparse,csv,re,struct
from pathlib import Path
from build_catalog import prop
from convert_ui_text import plain
ROOT=Path(__file__).resolve().parents[1]
def pack(path,rows):
 if path.exists():raise FileExistsError(path)
 data=bytearray(b"UTX1"+struct.pack("<I",len(rows)))
 for identity,title,text in rows:
  a=title.encode("ascii");b=text.encode("ascii")
  if len(a)>255 or len(b)>1023:raise ValueError("Text exceeds PSP bounds")
  data+=struct.pack("<HHH",identity,len(a),len(b))+a+b
 path.write_bytes(data)
def clean(text,value="",value2=""):
 text=re.sub(r"#[A-Z_]+","",text).replace("{2}",value2).replace("{}",value).replace("[set]","{set}")
 for token,amount in [("p",value),("p2",value2)]:
  singular=amount.strip() in ("1","1.0")
  text=re.sub(r"\{"+token+r":([^}]+)\}",lambda m:m[1].split("/")[0 if singular else min(1,len(m[1].split("/"))-1)],text)
 return plain(text)
def convert(original,output):
 original=Path(original).resolve();output=Path(output);output.mkdir(parents=True,exist_ok=True)
 with (original/"localization/translations.csv").open(encoding="utf-8-sig",newline="") as f:
  translations={r[0]:r[1] for r in csv.reader(f) if len(r)>1}
 ids=__import__("json").loads((ROOT/"tools/catalog_ids.json").read_text())
 for kind,folder,name in [("balls","data/balls_data","ball_text.uat"),("snacks","data/passives_data","snack_text.uat")]:
  keys={key:i for i,key in enumerate(ids[kind])};rows=[]
  for p in sorted((original/folder).rglob("*.tres")):
   data=p.read_text(encoding="utf-8-sig");key=prop(data,"id","")
   if key not in keys:continue
   title=translations.get(prop(data,"name",""),"")
   desc=translations.get(prop(data,"description",""),"")
   first=str(prop(data,"level_modifiers","")).split("/")[0]
   second=str(prop(data,"level_modifiers2","")).split("/")[0]
   if title or desc:rows.append((keys[key],plain(title),clean(desc,first,second)))
  pack(output/name,rows)
 groups=[["DIAL_INTRO"+str(i) for i in range(1,5)],
 ["DIAL_BUY"+str(i) for i in range(1,4)],["DIAL_SELL"+str(i) for i in range(1,4)],
 ["DIAL_REROLL"+str(i) for i in range(1,4)],["DIAL_MANY_REROLL"+str(i) for i in range(1,4)],
 ["DIAL_LOST_ROUND"+str(i) for i in range(1,4)],["DIAL_WINSTREAK"+str(i) for i in range(1,4)],
 ["DIAL_LAST_ROUND1"],["DIAL_RARE"+str(i) for i in range(1,4)],["DIAL_LEGENDARY1","DIAL_LEGENDARY2"],
 ["DIAL_WELCOME_BACK1","DIAL_WELCOME_BACK2"],["DIAL_COCKTAIL-MAKE"],["DIAL_COCKTAIL-DONE"],
 ["DIAL_NEW_BALLS"],["DIAL_FIRST_INTRO1","DIAL_FIRST_INTRO2"],["DIAL_COCKTAIL-LATER"]]
 rows=[]
 for group,keys in enumerate(groups):
  for i,key in enumerate(keys):
   value=translations.get(key,"")
   if value:rows.append((group*4+i,key,clean(value,"{value}")))
 pack(output/"dialogue_text.uat",rows)
 return {p.name:p.stat().st_size for p in output.glob("*.uat")}
if __name__=="__main__":
 parser=argparse.ArgumentParser();parser.add_argument("--original",type=Path,required=True);parser.add_argument("--output",type=Path,required=True);a=parser.parse_args();print(convert(a.original,a.output))
