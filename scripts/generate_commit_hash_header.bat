@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%CUR_DIR%..\
set GENERATED_DIR=%ROOT_DIR%_result\AllPlatforms\generated\
set HASH_HEADER=%GENERATED_DIR%commit_hash.h


echo Generating header with commit hash

if exist "%HASH_HEADER%" del /f/q "%HASH_HEADER%"
if not exist "%GENERATED_DIR%" mkdir "%GENERATED_DIR%"

echo #pragma once>> "%HASH_HEADER%"
echo.>> "%HASH_HEADER%"
echo #define SMP_COMMIT_HASH \>> "%HASH_HEADER%"

call git rev-parse --short HEAD>> "%HASH_HEADER%"
if errorlevel 1 goto fail

echo Header was successfully generated: %HASH_HEADER%
exit /b 0

:fail
echo Header generation failed!
exit /b 1