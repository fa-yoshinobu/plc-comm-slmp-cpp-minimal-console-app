@echo off
setlocal

if "%PLATFORMIO_CORE_DIR%"=="" set "PLATFORMIO_CORE_DIR=%~d0\pio"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

set "TARGET=%~1"
if /I "%TARGET%"=="" set "TARGET=all"

where pio >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%USERPROFILE%\.platformio\penv\Scripts" (
        set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"
    )
)

if /I "%TARGET%"=="all" goto build_all
if /I "%TARGET%"=="atom" goto build_atom
if /I "%TARGET%"=="w6300" goto build_w6300

echo Usage: build_console.bat [all^|atom^|w6300]
exit /b 1

:build_all
echo Using PLATFORMIO_CORE_DIR=%PLATFORMIO_CORE_DIR%
echo [1/2] Building Atom Matrix console...
python -m platformio run -e m5stack-atom-console
if %errorlevel% neq 0 exit /b %errorlevel%

echo [2/2] Building W6300 console...
python -m platformio run -e wiznet_6300_evb_pico2
if %errorlevel% neq 0 exit /b %errorlevel%

echo [SUCCESS] All console targets built successfully.
exit /b 0

:build_atom
echo Using PLATFORMIO_CORE_DIR=%PLATFORMIO_CORE_DIR%
echo Building Atom Matrix console...
python -m platformio run -e m5stack-atom-console
exit /b %errorlevel%

:build_w6300
echo Using PLATFORMIO_CORE_DIR=%PLATFORMIO_CORE_DIR%
echo Building W6300 console...
python -m platformio run -e wiznet_6300_evb_pico2
exit /b %errorlevel%
