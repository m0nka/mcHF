@echo off
rem Debug UART terminal. Every session is also captured in full to
rem claude\uart\uart_<timestamp>.log - the on-screen scrollback is
rem limited, the capture file is not
if not exist "%~dp0claude\uart" mkdir "%~dp0claude\uart"
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set ts=%%i
rem capdirect=0 keeps the incoming data on screen as well - without it
rem Realterm's 'direct capture' bypasses the display entirely
"C:\Program Files (x86)\BEL\Realterm\realterm.exe" baud=115200 port=9 rows=55 colors=RYLRYK scrollback=5000 fontname="Lucida Console" fontsize=16 capdirect=0 capture="%~dp0claude\uart\uart_%ts%.log"
