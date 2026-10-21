# Contributing

This is an unofficial prototype. Keep gameplay code platform-independent where practical and document behavior assumptions. Test relevant changes with the host suite and identify whether hardware/emulator validation occurred.

Do not contribute recovered GDScript, proprietary textures, fonts, audio, localized prose, game executables or asset packs. Use synthetic fixtures for converter tests. The conversion pipeline must preserve input files and refuse to overwrite personal installations.

Bug reports should identify build, device/emulator, reproduction steps and relevant ball/snack combinations. Do not include personal saves unless requested; never include original assets.

Public releases must use `python tools/package_public.py`, which stages an allowlisted source tree and asset-free engine package. Never upload the working directory or `dist/private`. Mark GitHub releases as prereleases while prototype status applies.
