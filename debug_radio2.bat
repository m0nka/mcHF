@echo off
rem Debug UART terminal. Every session is also captured in full to
rem claude\uart\uart_<timestamp>.log - the on-screen scrollback is
rem limited, the capture file is not
if not exist "%~dp0claude\uart" mkdir "%~dp0claude\uart"
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set ts=%%i
rem Capture syntax: the file name goes in capfile=, the bare 'capture'
rem switch starts it (capture=<file> crashes with an access violation on
rem the first received byte - no valid file handle). capsecs caps a
rem session at 24 h, capdirect=0 keeps the data on screen as well
"C:\Program Files (x86)\BEL\Realterm\realterm.exe" baud=115200 port=7 rows=55 colors=RYLRYK scrollback=5000 fontname="Lucida Console" fontsize=16 capfile="%~dp0claude\uart\uart2_%ts%.log" capsecs=86400 capdirect=0 capture
