@echo off
setlocal

if "%~1"=="" (
    echo Usage: %~nx0 ^<config_file^> [mode_name]
    echo Example: %~nx0 q3r_racing.cfg "Racing"
    exit /b 1
)

set "CONFIG_FILE=%~1"
set "MODE_NAME=%~2"

set "BASE_DIR=%~dp0.."
pushd "%BASE_DIR%" >nul
if errorlevel 1 goto :PushdFailed

set "SERVER_EXE="
for %%E in (
    q3rally.x86_64.exe
    ioq3rallyded.x86_64.exe
    ioq3rally.x86_64.exe
) do (
    if exist "%%E" (
        set "SERVER_EXE=%%E"
        goto :FoundExe
    )
)

:FoundExe
if not defined SERVER_EXE (
    echo Could not find q3rally.x86_64.exe or ioq3rallyded.x86_64.exe in %CD%.
    echo Copy these batch files into your installed Q3Rally directory.
    call :PauseAndExit 1
)

if defined MODE_NAME (
    echo Starting Q3Rally dedicated server for %MODE_NAME% using %CONFIG_FILE%...
) else (
    echo Starting Q3Rally dedicated server using %CONFIG_FILE%...
)

echo.
"%SERVER_EXE%" +set dedicated 1 +exec "baseq3r/%CONFIG_FILE%"
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo The dedicated server exited with code %EXIT_CODE%.
)

call :PauseAndExit %EXIT_CODE%

:PauseAndExit
echo.
echo Press any key to close this window.
pause >nul
popd
endlocal & exit /b %~1

:PushdFailed
echo Failed to change directory to %BASE_DIR%.
echo Ensure this script remains inside the Q3Rally\baseq3r folder.
echo.
echo Press any key to close this window.
pause >nul
endlocal & exit /b 1
