@echo off

REM USED COMPILER OPTIONS:
REM /Od      : Disables optimization.
REM /MTd     : Links to the static version of the CRT (debug version).
REM /fp:fast : Specifies how the compiler treats floating-point expressions.
REM /GM-     : Enables minimal rebuild.
REM /GR-     : Disables RTTI (C++ feature).
REM /EHa-    : Disables C++ stack unwinding for catch(...).
REM /Oi      : Generates intrinsic functions.
REM /WX      : Treats all warnings as errors.
REM /W4      : Sets warning level 4 to output.
REM /wd####  : Disables the specified warning.
REM /FC      : Displays full path of source code files passed to cl.exe in diagnostic text.
REM /Z7      : Generates C 7.0-compatible debugging information.
REM /Zo      : Generates enhanced debugging information for optimized code (release build).
REM /Fm      : Creates a map file.
REM /opt:ref : Eliminates functions and data that are never referenced.
REM /LD      : Creates a .dll.
REM /I       : Additional include directories.


REM CF: common compiler flags; LF: common linker flags.
REM
set CF=/nologo /fp:fast /Gm- /GR- /EHa- /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4456 /FC /Z7 /Zo /I..\src\vendor
REM set CF=/fsanitize=address %CF%
set LF=/incremental:no /opt:ref user32.lib gdi32.lib winmm.lib shell32.lib 


REM pushd takes you to a directory you specify and popd takes you back to where you were.
REM "%~dp0" is the drive letter and path combined of the batch file being executed.
REM
pushd %~dp0
if not exist ..\build mkdir ..\build
pushd ..\build

REM Debug build
cl /Od /MTd /DDEVELOPER=1 /Fenur_dev.exe %CF% ..\src\win32_main.cpp /link /MANIFEST:EMBED /MANIFESTINPUT:../src/nur.manifest /entry:WinMainCRTStartup /subsystem:windows %LF%

REM Release build
cl /O2 /MT  /DDEVELOPER=0 /Fenur.exe %CF% ..\src\win32_main.cpp /link /MANIFEST:EMBED /MANIFESTINPUT:../src/nur.manifest /entry:WinMainCRTStartup /subsystem:windows %LF%

REM Delete intermediates
del *.obj *.res >NUL

REM Copy data folder to build folder
rd /S /Q .\data
xcopy ..\data .\data /E /I /Y >NUL

popd
popd
