@echo off

REM USED COMPILER OPTIONS:
REM -Od      : Disables optimization.
REM -MTd     : Links to the static version of the CRT (debug version).
REM -fp:fast : Specifies how the compiler treats floating-point expressions.
REM -GM-     : Enables minimal rebuild.
REM -GR-     : Disables RTTI (C++ feature).
REM -EHa-    : Disables C++ stack unwinding for catch(...).
REM -Oi      : Generates intrinsic functions.
REM -WX      : Treats all warnings as errors.
REM -W4      : Sets warning level 4 to output.
REM -wd####  : Disables the specified warning.
REM -FC      : Displays full path of source code files passed to cl.exe in diagnostic text.
REM -Z7      : Generates C 7.0-compatible debugging information.
REM -Zo      : Generates enhanced debugging information for optimized code (release build).
REM -Fm      : Creates a map file.
REM -opt:ref : Eliminates functions and data that are never referenced.
REM -LD      : Creates a .dll.

set COMPILER_FLAGS=-Od -MTd -nologo -fp:fast -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4456 -FC -Z7 -Zo
set COMPILER_FLAGS=-DDEVELOPER=1 %COMPILER_FLAGS%
REM set COMPILER_FLAGS=-fsanitize=address %COMPILER_FLAGS%
set LINKER_FLAGS=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib shell32.lib 

REM pushd takes you to a directory you specify and popd takes you back to where you were.
REM "%~dp0" is the drive letter and path combined, of the batch file being executed.
pushd %~dp0
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl %COMPILER_FLAGS% /I..\src\vendor -Fenur.exe ..\src\win32_main.cpp -Fmwin32_main.map /link /MANIFEST:EMBED /MANIFESTINPUT:nur.manifest /entry:WinMainCRTStartup /subsystem:windows %LINKER_FLAGS% 
popd
popd
