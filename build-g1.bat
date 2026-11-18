@echo off
rem Builds YAGA for Gothic 1 Classic (x86, Release) on a machine with VS 2022.
rem Usage: build-g1.bat [extra cmake configure args]
setlocal
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "usebackq delims=" %%i in (`vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
    echo Visual Studio with C++ tools not found.
    exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul || exit /b 1
set "PATH=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cd /d "%~dp0"
if not exist "out\build\G1-Release\CMakeCache.txt" (
    cmake --preset G1-Release %* || exit /b 1
)
cmake --build --preset G1-Release || exit /b 1
endlocal
