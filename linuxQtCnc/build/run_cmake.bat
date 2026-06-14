@echo off
SETLOCAL
SET CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
SET SRC=d:\cnclx\linuxcnc\linuxQtCnc
cd /d "%SRC%\build"
del /S /Q CMakeCache.txt >nul 2>&1
rd /S /Q CMakeFiles >nul 2>&1
echo Running cmake...
"%CMAKE%" -G "Visual Studio 15 2017 Win64" %SRC% > cmake_full_output.txt 2>&1
echo CMAKE EXIT=%ERRORLEVEL%
echo.
echo === Last 60 lines of output ===
type cmake_full_output.txt | findstr /N "^" | tail -60
echo === end ===
