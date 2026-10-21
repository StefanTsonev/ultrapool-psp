# v0.9.5.1 public prototype

Unofficial native PSP implementation. This release is a prerelease, not a finished recreation.

## Changes

Tooling-only patch: fix text encoding damage in the README and asset setup window. The PSP executable is unchanged from v0.9.5.

Owner-side asset setup now accepts a GDRE-recovered folder or invokes an installed GDRE Tools against an owned Windows EXE/PCK. A Windows setup window and terminal tool share the conversion pipeline. The generated installation includes texture, audio, achievement, tutorial, ball-description and dialogue banks. Existing output folders are refused and conversion is staged before installation is published.

The engine includes the latest seed editor, expanded settings, tutorial replay, collection details, calendar, result/achievement presentation and NPC improvements.

## Validation and limitations

The EBOOT compiles with PSPSDK; the host core suite passes 5,341,555 assertions. New engine additions have not yet completed PPSSPP or physical PSP validation. Earlier builds received user hardware testing. Complete gameplay parity is not claimed.

The recovered-folder conversion path is tested end to end against the owner's local Windows recovery. EXE invocation arguments are covered by an automated test; actual EXE recovery depends on GDRE Tools and has not been validated here against a fresh game executable. Other releases/platform asset layouts may require converter changes.

English presentation text is converted. Native checkpoints are version 12 and read older supported versions; older executables cannot read new checkpoints. Back up saves before updating. Daily challenges are offline and do not connect to an official service.

## Publishing

Use only the staged `dist/public/v0.9.5.1/ultrapool-psp` source folder for the repository. Upload the `prototype.zip` and checksums from that same directory as release assets and mark the release **Pre-release**. Do not upload `dist/private`, the working directory, recovered resources, game executable, personal saves, or a generated asset installation. The source ZIP is optional because GitHub also generates source downloads.

The public packager uses a source allowlist and checks archive entries for resource and save formats. MIT covers independent project code; original game resources remain owned by their respective holders.
