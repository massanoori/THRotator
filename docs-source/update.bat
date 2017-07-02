@echo off

setlocal

set DOCS_OUTPUT_DIR=..\docs
set ROBOCOPY_FLAGS=/NJH /NFL /NDL /NJS /PURGE /S

robocopy %ROBOCOPY_FLAGS% en\_build\html %DOCS_OUTPUT_DIR%\en
if %ERRORLEVEL% GEQ 8 exit /b 1

robocopy %ROBOCOPY_FLAGS% ja\_build\html %DOCS_OUTPUT_DIR%\ja
if %ERRORLEVEL% GEQ 8 exit /b 1

exit /b 0
