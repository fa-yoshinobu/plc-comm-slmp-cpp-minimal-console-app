@echo off
setlocal

if "%PLATFORMIO_CORE_DIR%"=="" set "PLATFORMIO_CORE_DIR=%~d0\pio"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

where pio >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%USERPROFILE%\.platformio\penv\Scripts" (
        set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"
    )
)

echo Using PLATFORMIO_CORE_DIR=%PLATFORMIO_CORE_DIR%
echo [1/2] Building Atom Matrix console...
python -m platformio run -e m5stack-atom-console
if %errorlevel% neq 0 exit /b %errorlevel%

echo [2/2] Building W6300 console...
python -m platformio run -e wiznet_6300_evb_pico2
if %errorlevel% neq 0 exit /b %errorlevel%

echo [SUCCESS] Console app build checks passed.
endlocal
