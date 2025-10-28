@echo off
REM Wrapper script to install and build the www files on Windows

REM Capture the first two arguments and shift them out
set "DIR=%~1"
set "CMD=%~2"
shift
shift
REM Collect remaining arguments
set EXTRA_ARGS=
:loop
if "%~1"=="" goto endloop
set EXTRA_ARGS=%EXTRA_ARGS% %1
shift
goto loop
:endloop

REM Navigate to the correct directory and run the necessary commands
cd /d "%DIR%" || exit /b 1
call "%CMD%" install
call "%CMD%" build %EXTRA_ARGS%
