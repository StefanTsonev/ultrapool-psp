<h1 align="center">Ultrapool PSP</h1>
<p align="center"><strong>An unofficial native PSP prototype of Ultrapool.</strong><br>C++ | PSPSDK | 480 x 272 | Work in progress</p>

This project brings Ultrapool's pool-and-roguelike loop to the PlayStation Portable. Play rounds, build a team of unusual balls, combine abilities, buy snacks and chase higher scores.

**This is a prototype.** Gameplay differences, unfinished interactions and visual issues remain. Feedback, bug reports and contributions are welcome. It is independently implemented; the PSP does not run the original Godot scripts.

## Ownership and resources

This project is not affiliated with or endorsed by Ultrapool's creators. The original game, artwork, music, fonts and text belong to their respective owners. The MIT license covers this project's independently written code, not the original game's resources.

**You need your own legally acquired Windows copy of Ultrapool.** Public downloads contain the engine and conversion tools only. Do not upload the original executable, recovered project, converted asset packs or personal installation folders to GitHub.

## What's included

- Native pool physics, aiming guides and PSP controls.
- Round progression, shops, ball upgrades, mixed balls and snacks.
- Achievements, collection views, checkpoints and offline daily challenges.
- Owner-converted table art, ball textures, dialogue, sound effects and streamed music.
- Custom seeds, tutorial replay, fine aim settings and separate audio controls.

The goal is fidelity to the original, but complete rule and visual parity is not claimed. The latest prototype compiles for PSP and passes host tests; the new additions still need emulator and physical hardware validation. Earlier builds have received user testing on PSP.

## Getting started

### 1. Download the prototype

Download the **public prototype ZIP** from this repository's Releases page and extract it on your PC. Keep its folders together. Do not download or redistribute a personal asset package.

### 2. Install the PC tools

- [Python 3.10 or newer](https://www.python.org/downloads/) - enable **Add Python to PATH** on Windows.
- [GDRE Tools](https://github.com/GDRETools/gdsdecomp/releases) - download and extract the build for your PC.
- [FFmpeg](https://ffmpeg.org/download.html) - put `ffmpeg` on PATH, or pass its full executable path with `--ffmpeg`.

Open a terminal in the extracted release folder and install the converter dependency:

```sh
python -m pip install -r tools/requirements.txt
```

### 3. Prepare your assets

On Windows, double-click **Setup-Assets.cmd** for the setup window. Choose your recovered project folder, or select EXE/PCK mode and browse to GDRE Tools. Set a new installation folder and click **Build my PSP installation**. The equivalent terminal commands are below. On Linux/macOS the terminal tool works without Tkinter.

Choose either method. The tool reads your files, converts only needed resources and produces a new installation folder. Conversion can take several minutes. Keep the original game and recovered files on your PC.

**Option A - you already recovered the game with GDRE Tools:**

Use GDRE's **Recover project** feature, not only raw archive extraction. Imported textures/audio must have been restored and the recovered project should contain `localization`, `data`, `audio` and `singletons` folders. Select that project root:

```sh
python tools/setup_assets.py --source "C:/Games/Ultrapool-recovered" --output "output/Ultrapool"
```

**Option B - start with your own Windows executable:**

The setup tool invokes your installed GDRE Tools to recover resources in a temporary folder. It does not launch the game executable. If the installation uses a separate PCK, pass that PCK instead of the EXE.

```sh
python tools/setup_assets.py --exe "C:/Games/Ultrapool/Ultrapool.exe" --gdre "C:/Tools/GDRE/gdre_tools.exe" --output "output/Ultrapool"
```

The output folder must be new; the tool refuses to overwrite an existing installation. Failed conversions do not replace an existing installation. GDRE recovery support and game versions vary; incompatible layouts are reported with missing resource paths. Current conversion supports the recovered Windows resource layout used during development, not arbitrary future game updates.

### 4. Copy to your PSP

Use a PSP configured to run homebrew. Copy the generated **Ultrapool** folder into `PSP/GAME` on your Memory Stick:

```text
ms0:/PSP/GAME/Ultrapool/
+-- EBOOT.PBP
+-- asset-setup-report.json
+-- assets/
    +-- definitions.txt
    +-- private/
        +-- *.upt          converted textures
        +-- *.uat          converted text
        +-- audio/
            +-- bank.txt
            +-- *.upa      converted music and sounds
```

Launch it from **Game > Memory Stick**. The original Windows EXE and recovered scripts do not belong on the PSP. To try the same installation in [PPSSPP](https://www.ppsspp.org/), open its `EBOOT.PBP` and keep `assets` beside it.

For updates, back up your saves, create a fresh converted installation and merge it into your PSP folder. Keep your existing `ultrapool.sav`, profile, daily records and settings. This prototype writes run-save format v12; older builds cannot read those newer checkpoints. Quitting during a round returns to the last shop checkpoint rather than resuming mid-shot. PC/Godot saves are not supported.

## Controls

| Action | PSP control |
| --- | --- |
| Aim | Analog stick |
| Fine aim | D-pad left / right |
| Prepare / shoot / confirm | Cross (X) |
| Cancel | Circle (O) |
| Adjust shot power | L / R |
| Inspect / merge in shop | Square, then follow the prompts |
| Finish a cleared round early | Triangle |
| Pause and settings | Start |
| Custom seed on home screen | L |
| Daily calendar on home screen | R |
| Resume checkpoint on home screen | Triangle |

The shop has context-sensitive prompts for buying, selling, combining and moving balls. Settings include music/SFX volume, fine aim speed, analog aiming/deadzone, tutorial hints, reduced motion and tutorial replay.

## Troubleshooting

- **Missing resources:** use GDRE's full project recovery and select the recovered root, not its parent or `.godot` folder. Raw `.ctex` files alone are insufficient.
- **FFmpeg not found:** add it to PATH or append `--ffmpeg "C:/Tools/ffmpeg/bin/ffmpeg.exe"`.
- **Pillow not found:** run the pip installation using the same Python command used for setup.
- **Output exists:** choose a different output folder. The tool protects existing files and saves.
- **Placeholder graphics or no audio:** copy the entire generated folder; EBOOT alone does not contain game resources.
- **Game-version mismatch:** include the game version and missing resource names in your issue. Do not attach copyrighted files or your recovered project.

## Build from source

Install [PSPSDK](https://pspdev.github.io/pspsdk/) and run:

```sh
make -j2
```

This produces `EBOOT.PBP`. Run the asset setup command above before installing it. GDRE Tools, Python and FFmpeg are PC-side tools and are not PSP dependencies.

Host-side checks:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release --target core_tests
ctest --test-dir build/cmake -C Release --output-on-failure
python -m unittest discover -s tests -p 'test_public*.py'
```

See [prototype release notes](docs/PUBLIC_PROTOTYPE.md) for validation scope and known limitations.

## Contributing and reports

Please include the prototype version, PSP model or PPSSPP version, reproduction steps and the balls/snacks involved. Screenshots are useful for layout bugs; never attach original assets or recovered source. See [CONTRIBUTING.md](CONTRIBUTING.md).

## Thanks

Ultrapool's creators for the original game; the PSPSDK and PPSSPP communities for their tools; [GDRE Tools](https://github.com/GDRETools/gdsdecomp) for owner-side recovery. [PSPalatro](https://github.com/Luxotick/pspalatro) inspired the public setup flow and README organization; this does not imply shared gameplay code or affiliation.
