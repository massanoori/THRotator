@echo off

setlocal

python --version 2>NUL
if errorlevel 9009 (
  echo Failed to find Python executable.
  echo Make sure you have installed Python or added Python directory to PATH.
  exit /b 1
)

pushd en
echo ##### Start building English manual #####
call make html
echo[
popd

pushd ja
echo ##### Start building Japanese manual #####
call make html
echo[
popd

