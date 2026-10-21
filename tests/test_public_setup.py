import json,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import setup_assets,package_public
class PublicSetupTests(unittest.TestCase):
 def test_setup_and_readme_have_no_encoding_placeholders(self):
  for name in ("README.md","tools/setup_assets.py","tools/setup_assets_gui.py"):
   text=(setup_assets.ROOT/name).read_text(encoding="utf-8")
   self.assertNotIn(" ? ",text,name)
   self.assertNotIn("Browse?",text,name)
   self.assertNotIn("???",text,name)
   self.assertNotIn(chr(0xfffd),text,name)
 def test_refuses_existing_installation_without_touching_it(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);out=root/"install";out.mkdir();save=out/"ultrapool.sav";save.write_bytes(b"player")
   with self.assertRaises(FileExistsError):setup_assets.build(root/"source",out,root/"EBOOT.PBP")
   self.assertEqual(save.read_bytes(),b"player")
 def test_refuses_output_inside_original(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d)
   with self.assertRaisesRegex(ValueError,"separate"):setup_assets.build(root,root/"output",root/"eboot")
 def test_reports_incomplete_recovery(self):
  with tempfile.TemporaryDirectory() as d:
   with self.assertRaisesRegex(ValueError,"Recover project"):setup_assets.preflight(Path(d),setup_assets.ROOT/"tools/manifest_psp.json")
 def test_rejects_escaping_manifest_path(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);manifest=root/"manifest.json";manifest.write_text(json.dumps([dict(source="../private.png")]))
   with self.assertRaisesRegex(ValueError,"escapes"):setup_assets.preflight(root,manifest)
 def test_exe_mode_invokes_gdre_without_launching_game(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);exe=root/"owned game.exe";gdre=root/"gdre tools.exe";exe.write_bytes(b"owner");gdre.write_bytes(b"tool")
   argv=["setup_assets.py","--exe",str(exe),"--gdre",str(gdre),"--output",str(root/"output")]
   with patch.object(sys,"argv",argv),patch("setup_assets.subprocess.run") as run,patch.object(setup_assets,"build") as build:
    setup_assets.main()
   command=run.call_args.args[0]
   self.assertEqual(command[0],str(gdre.resolve()))
   self.assertIn("--recover="+str(exe.resolve()),command)
   self.assertIn("--headless",command);self.assertTrue(any(x.startswith("--output=") for x in command))
   self.assertEqual(exe.read_bytes(),b"owner");build.assert_called_once()
 def test_public_files_have_no_resources_or_player_data(self):
  files=package_public.source_files()
  self.assertTrue(files)
  self.assertFalse(any(p.suffix.lower() in package_public.FORBIDDEN for p in files))
  self.assertFalse(any("private" in p.relative_to(package_public.ROOT).parts for p in files))
 def test_failed_conversion_does_not_publish_partial_installation(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);source=root/"source";source.mkdir();out=root/"installation";eboot=root/"EBOOT.PBP";eboot.write_bytes(b"test")
   with patch.object(setup_assets,"preflight"),patch("setup_assets.subprocess.run"),patch("convert_assets.convert",side_effect=ValueError("bad texture")):
    with self.assertRaisesRegex(ValueError,"bad texture"):setup_assets.build(source,out,eboot)
   self.assertFalse(out.exists());self.assertEqual(list(source.iterdir()),[])
if __name__=="__main__":unittest.main()
