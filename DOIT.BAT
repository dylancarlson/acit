@echo off
if exist errors del errors
nmake CFLAGS="/AL /c /W4 /Gm /Gt /nologo"  DEFS="/DNETWORK" 
be beep

