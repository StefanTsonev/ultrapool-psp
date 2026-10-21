@echo off
cd /d "%~dp0"
where py >nul 2>nul
if errorlevel 1 goto python
py -3 tools\setup_assets_gui.py
goto done
:python
python tools\setup_assets_gui.py
:done
if errorlevel 1 pause
