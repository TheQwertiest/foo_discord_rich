@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1
if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\
set DISCORD_RPC_DIR=%ROOT_DIR%submodules\discord-rpc\

echo Preparing Discord RPC

xcopy /r/y/q "%CUR_DIR%additional_files\discord-rpc.vcxproj" "%DISCORD_RPC_DIR%src\"
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1
