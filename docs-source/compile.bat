@echo off

setlocal

echo Checking Python executable.
echo[

python --version
if errorlevel 9009 (
  echo[
  echo Failed to find Python executable.
  echo Make sure you have installed Python or added Python directory to PATH.
  exit /b 1
)

echo[

pushd en
call make html
popd

pushd ja
call make html
popd

