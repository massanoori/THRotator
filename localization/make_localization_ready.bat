@echo off
setlocal

muirct -q "%~dp0\AllNeutral.rcconfig" -v 2 -x 0x0411 -g 0x0409 %1 %1
