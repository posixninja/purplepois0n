@echo off
REM Windows launcher for PurpleDoctor.
set ROOT=%~dp0..
cd /d "%ROOT%"
python "%ROOT%\doctors\doctor_gui.py"
