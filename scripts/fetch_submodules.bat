@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1

if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

echo Downloading submodules

cd %ROOT_DIR%

git submodule sync
if errorlevel 1 goto fail
git submodule foreach git reset --hard
if errorlevel 1 goto fail
for /d %%i in ("%ROOT_DIR%submodules\*") do (
  call :FetchSubmodule submodules\%%~nxi
)
exit /b 0

:FetchSubmodule
set SUBMODULE_REL_PATH=%1
echo Fetching %SUBMODULE_REL_PATH%...
git submodule update --init --depth=10 -- %SUBMODULE_REL_PATH%
if errorlevel 1 (
  git submodule update --init --depth=50 --force -- %SUBMODULE_REL_PATH%
  if errorlevel 1 (
    cd %SUBMODULE_REL_PATH%
    if errorlevel 1 goto fail
    rem Shallow copy does not honour default branch config
    git config --add remote.origin.fetch +refs/heads/*:refs/remotes/origin/*
    if errorlevel 1 goto fail
    cd %ROOT_DIR%
    if errorlevel 1 goto fail
    git submodule deinit --force -- %SUBMODULE_REL_PATH%
    if errorlevel 1 goto fail
    git submodule update --init --force -- %SUBMODULE_REL_PATH%
    rem We tried T_T
    if errorlevel 1 goto fail
  )
)
goto :eof

:fail
echo Failed!
exit /b 1
