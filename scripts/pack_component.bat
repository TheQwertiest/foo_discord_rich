@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip

set ROOT_DIR=%~dp0..\
if not '%1'=='' if not '%1'=='--debug' (
    set ROOT_DIR=%1
)

set CONFIGURATION=Release
if '%1'=='--debug' (
    set CONFIGURATION=Debug
)
if '%2'=='--debug' (
    set CONFIGURATION=Debug
)

set COMPONENT_DIR_NO_SLASH=%ROOT_DIR%component
set RESULT_CONFIGURATION_DIR=%ROOT_DIR%_result\Win32_%CONFIGURATION%\
set DOC_DIR_NO_SLASH=%ROOT_DIR%_result\html
set COMPONENT_LICENSE=%ROOT_DIR%LICENSE
set COMPONENT_DLL=%RESULT_CONFIGURATION_DIR%\bin\foo_spider_monkey_panel.dll
set COMPONENT_PDB=%RESULT_CONFIGURATION_DIR%\dbginfo\foo_spider_monkey_panel.pdb
set SAMPLES_COMPLETE_DIR_NO_SLASH=%ROOT_DIR%submodules\smp_2003
set MOZ_JS_BIN_DIR=%ROOT_DIR%mozjs\%CONFIGURATION%\bin\

set COMPONENT_OUT_DIR_NO_SLASH=%RESULT_CONFIGURATION_DIR%component
set COMPONENT_OUT_DIR=%COMPONENT_OUT_DIR_NO_SLASH%\
set COMPONENT_PDB_PACKAGE=%RESULT_CONFIGURATION_DIR%foo_spider_monkey_panel_pdb.zip
set FB2K_ARCHIVE=%RESULT_CONFIGURATION_DIR%foo_spider_monkey_panel.fb2k-component

echo Packing component to .fb2k-component

if exist "%COMPONENT_OUT_DIR_NO_SLASH%" rmdir /s/q "%COMPONENT_OUT_DIR_NO_SLASH%"
mkdir "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y/s/q/i "%COMPONENT_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%"
if errorlevel 1 goto fail
if exist "%DOC_DIR_NO_SLASH%" (
    xcopy /r/y/s/q/i "%DOC_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%\docs\html"
    if errorlevel 1 goto fail
) else (
    echo No docs found. Skipping...
)
xcopy /r/y/s/q/i "%SAMPLES_COMPLETE_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%\samples\complete"
if errorlevel 1 goto fail
xcopy /r/y/s/q "%MOZ_JS_BIN_DIR%*.dll" "%COMPONENT_OUT_DIR%"
if errorlevel 1 goto fail
xcopy /r/y/q "%COMPONENT_LICENSE%" "%COMPONENT_OUT_DIR%"
if errorlevel 1 goto fail
xcopy /r/y/q "%COMPONENT_DLL%" "%COMPONENT_OUT_DIR%"
if errorlevel 1 goto fail
if '%CONFIGURATION%'=='Debug' (
    rem Only debug package should have pdbs inside
    xcopy /r/y/s/q "%MOZ_JS_BIN_DIR%*.pdb" "%COMPONENT_OUT_DIR%"
    if errorlevel 1 goto fail
    xcopy /r/y/q "%COMPONENT_PDB%" "%COMPONENT_OUT_DIR%"
    if errorlevel 1 goto fail
) else (
    rem Release pdbs are packed in a separate package
    if exist "%COMPONENT_PDB_PACKAGE%" del /f/q "%COMPONENT_PDB_PACKAGE%"
    7z a -tzip -mx=9 "%COMPONENT_PDB_PACKAGE%" "%MOZ_JS_BIN_DIR%*.pdb" > NUL
    if errorlevel 1 goto fail
    7z a -tzip -mx=9 "%COMPONENT_PDB_PACKAGE%" "%COMPONENT_PDB%" > NUL
    if errorlevel 1 goto fail
    echo Pdb's were sucessfuly packed: %COMPONENT_PDB_PACKAGE%
)

if exist "%FB2K_ARCHIVE%" del /f/q "%FB2K_ARCHIVE%"
7z a -tzip -mx=9 "%FB2K_ARCHIVE%" "%COMPONENT_OUT_DIR%*" > NUL
if errorlevel 1 goto fail
echo Component was sucessfuly packed: %FB2K_ARCHIVE%
exit /b 0

:fail
echo Failed!
exit /b 1
