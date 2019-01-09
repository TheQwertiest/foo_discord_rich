@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%~dp0..\
if not '%1'=='' (
    set ROOT_DIR=%1
)


echo Preparing project repo

call %CUR_DIR%fetch_submodules.bat %ROOT_DIR%
if errorlevel 1 goto fail
call %CUR_DIR%prepare_discord_rpc.bat %ROOT_DIR%
if errorlevel 1 goto fail
call %CUR_DIR%generate_commit_hash_header.bat %ROOT_DIR%
if errorlevel 1 goto fail

echo Setup complete!
exit /b 0

:fail
echo Setup failed!
exit /b 1