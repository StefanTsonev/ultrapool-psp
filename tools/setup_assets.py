"""Build a personal PSP installation from owner-supplied recovered resources."""
import argparse, contextlib, hashlib, json, shutil, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def inside(path, root):
 return path==root or root in path.parents

def preflight(source, manifest):
 source=source.resolve()
 required={"localization/translations.csv","singletons/audio_manager.tscn","ball.tscn","main.tscn","ui/daily_end_bar.tscn"}
 for e in json.loads(manifest.read_text()):
  required.add(e["source"])
  if "overlay" in e:required.add(e["overlay"])
  required.update(x["source"] for x in e.get("layers",[]))
 rules=json.loads((ROOT/"docs/PROGRESSION_RULES.json").read_text())
 required.update(x["source"] for x in rules["achievements"])
 missing=[]
 for name in sorted(required):
  p=(source/name).resolve()
  if not inside(p,source):raise ValueError("Resource path escapes recovered folder: "+name)
  if not p.is_file():missing.append(name)
 for folder in ("audio","data/balls_data","data/passives_data"):
  if not (source/folder).is_dir():missing.append(folder+"/")
 if missing:raise ValueError("Incomplete or incompatible recovery. Use GDRE Recover project (not raw Extract). Missing:\n"+"\n".join(missing[:30]))
 return len(required)

def build(source, output, eboot, ffmpeg="ffmpeg", manifest=None):
 from convert_assets import convert as textures
 from convert_audio import convert as audio
 from convert_ui_text import convert as achievements
 from convert_tutorial_text import convert as tutorial
 from convert_presentation_text import convert as descriptions
 from verify_audio import verify
 source=Path(source).resolve();output=Path(output).resolve();eboot=Path(eboot).resolve()
 manifest=Path(manifest or ROOT/"tools/manifest_psp.json")
 if output.exists():raise FileExistsError("Output already exists; choose a new folder. Your saves are never overwritten.")
 if inside(output,source) or inside(source,output):raise ValueError("Output must be separate from the recovered project")
 if not eboot.is_file():raise ValueError("EBOOT.PBP not found; use the prototype release or build it first")
 preflight(source,manifest)
 subprocess.run([str(ffmpeg),"-version"],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 output.parent.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix=".ultrapool-convert-",dir=output.parent) as tmp:
  stage=Path(tmp)/"Ultrapool";private=stage/"assets/private"
  print("[1/5] Converting textures...",flush=True);textures(source,private,manifest)
  print("[2/5] Converting music and sounds...",flush=True);audio(source,private/"audio",str(ffmpeg));report=verify(private/"audio")
  print("[3/5] Converting interface and tutorial text...",flush=True)
  achievements(source,private/"achievement_text.uat");tutorial(source,private/"tutorial_text.uat")
  print("[4/5] Converting ball descriptions and dialogue...",flush=True);descriptions(source,private)
  shutil.copyfile(eboot,stage/"EBOOT.PBP");shutil.copyfile(ROOT/"assets/definitions.txt",stage/"assets/definitions.txt")
  files=[p for p in stage.rglob("*") if p.is_file()]
  report.update(prototype=True,private_resources=True,files={p.relative_to(stage).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files})
  (stage/"asset-setup-report.json").write_text(json.dumps(report,indent=2))
  if output.exists():raise FileExistsError(output)
  stage.rename(output)
 print("[5/5] Ready: "+str(output)+" ? copy this folder into PSP/GAME. Do not redistribute converted resources.")
 return report

def main():
 p=argparse.ArgumentParser(description=__doc__)
 g=p.add_mutually_exclusive_group(required=True);g.add_argument("--source",type=Path,help="GDRE recovered project folder");g.add_argument("--exe",type=Path,help="Your original Windows game EXE or PCK (not executed)")
 p.add_argument("--gdre",type=Path,help="GDRE Tools executable, required with --exe")
 p.add_argument("--output",type=Path,required=True,help="New destination folder, normally ending in Ultrapool")
 p.add_argument("--eboot",type=Path,default=ROOT/"EBOOT.PBP");p.add_argument("--ffmpeg",default="ffmpeg")
 a=p.parse_args()
 try:
  if a.source:build(a.source,a.output,a.eboot,a.ffmpeg)
  else:
   if not a.gdre or not a.gdre.is_file():raise ValueError("--exe requires --gdre pointing to GDRE Tools")
   if not a.exe.is_file():raise ValueError("Game executable/PCK not found")
   if a.output.resolve().exists():raise FileExistsError("Choose a new output folder")
   with tempfile.TemporaryDirectory(prefix="ultrapool-recovery-") as tmp:
    recovered=Path(tmp)/"recovered"
    subprocess.run([str(a.gdre.resolve()),"--headless","--recover="+str(a.exe.resolve()),"--output="+str(recovered)],check=True)
    build(recovered,a.output,a.eboot,a.ffmpeg)
 except (OSError,ValueError,subprocess.SubprocessError,ImportError) as e:
  p.exit(1,"Setup failed: "+str(e)+"\nInputs and existing saves were preserved. Install tools/requirements.txt and FFmpeg; use a complete GDRE recovery.\n")
if __name__=="__main__":main()
