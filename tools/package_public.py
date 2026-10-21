"""Stage only allowlisted independent source and an asset-free prototype download."""
import hashlib,json,shutil,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
VERSION="v0.9.5"
TOOLS=["setup_assets.py","setup_assets_gui.py","convert_assets.py","convert_audio.py","convert_ui_text.py","convert_tutorial_text.py","convert_presentation_text.py","verify_audio.py","build_catalog.py","catalog_ids.json","manifest_psp.json","requirements.txt","visual_audit.cpp","package_public.py"]
DOCS=["PROGRESSION_RULES.json","TUTORIAL_RULES.json","PUBLIC_PROTOTYPE.md"]
BASE=["README.md","Setup-Assets.cmd","LICENSE","CONTRIBUTING.md","Makefile","CMakeLists.txt",".gitignore","assets/definitions.txt"]
FORBIDDEN={".gd",".gdc",".tscn",".tres",".png",".jpg",".webp",".wav",".ogg",".mp3",".ttf",".otf",".ctex",".exe",".dll",".pck",".upt",".uat",".upa",".ups",".sav",".cfg",".profile",".daily",".bak"}

def source_files(root=ROOT):
 files=[root/p for p in BASE]+[root/"tools"/p for p in TOOLS]+[root/"docs"/p for p in DOCS]
 for folder,suffixes in [("src",{".cpp"}),("include",{".hpp"}),("tests",{".cpp",".hpp"})]:
  files.extend(p for p in (root/folder).rglob("*") if p.suffix in suffixes)
 files.extend((root/"tests").glob("test_public*.py"))
 files.extend((root/".github").rglob("*.yml"))
 for p in files:
  if not p.is_file() or not p.resolve().is_relative_to(root.resolve()):raise ValueError("Missing or unsafe public file: "+str(p))
  if p.suffix.lower() in FORBIDDEN:raise ValueError("Private file in source list: "+str(p))
 return sorted(set(files))

def main():
 out=ROOT/"dist/public"/VERSION
 if out.exists():raise FileExistsError("Release staging exists; preserve it and choose a new version")
 files=source_files();eboot=ROOT/"EBOOT.PBP"
 if not eboot.is_file():raise ValueError("Build EBOOT.PBP first")
 out.mkdir(parents=True);repo=out/"ultrapool-psp";repo.mkdir()
 for p in files:
  target=repo/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
 archives=[]
 for name,selected,prefix in [("source",files,"ultrapool-psp/"),("prototype",[ROOT/p for p in BASE if p!=".gitignore"]+[ROOT/"tools"/p for p in TOOLS]+[ROOT/"docs"/p for p in DOCS]+[eboot],"")]:
  path=out/("Ultrapool-PSP-"+VERSION+("-source" if name=="source" else "-prototype")+".zip")
  with zipfile.ZipFile(path,"x",zipfile.ZIP_DEFLATED,compresslevel=9) as z:
   for p in selected:z.write(p,prefix+p.relative_to(ROOT).as_posix())
  with zipfile.ZipFile(path) as z:
   if z.testzip():raise ValueError("Corrupt ZIP")
   for n in z.namelist():
    if Path(n).suffix.lower() in FORBIDDEN or "/private/" in n or "/build/" in n:raise ValueError("Private archive entry: "+n)
  archives.append(dict(name=path.name,bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
 report=dict(version=VERSION,prerelease=True,eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest(),source_files=len(files),archives=archives,physical_psp_tested=False,new_features_ppsspp_tested=False)
 (out/"release-manifest.json").write_text(json.dumps(report,indent=2))
 (out/"SHA256SUMS.txt").write_text("".join(x["sha256"]+"  "+x["name"]+"\n" for x in archives))
 print(json.dumps(report,indent=2));print("GitHub-ready source folder: "+str(repo))
if __name__=="__main__":main()
