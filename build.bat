@echo off
setlocal

set PRJ_DIR=%cd%

set DEFAULT_PATH=C:\Windows\system32;C:\Windows;

@rem VS2019 Command Promot have need this
set VSCMD_START_DIR=%PRJ_DIR%
set VS2019_INSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community

echo %VS2019_INSTALLDIR%
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::                            The main function                               ::
::                                                                            ::
call :CLEAN_ENV
call "%VS2019_INSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"
echo Build X64 Static MT
set DST_DIR=%PRJ_DIR%\_dst\Windows\x64
set BUILD_DIR=%PRJ_DIR%\_build\Windows\x64
call :BUILD

call :CLEAN_ENV
call "%VS2019_INSTALLDIR%\VC\Auxiliary\Build\vcvars32.bat"
echo Build X32 Static MT
set DST_DIR=%PRJ_DIR%\_dst\Windows\x32
set BUILD_DIR=%PRJ_DIR%\_build\Windows\x32
call :BUILD

goto :EOF
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::                       The clean env function                               ::
::                                                                            ::
:CLEAN_ENV

set VSINSTALLDIR=
set INCLUDE=
set LIB=
set LIBPATH=
set PATH=%DEFAULT_PATH%

goto :EOF
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::                          The build function                                ::
::                                                                            ::
:BUILD
mkdir %DST_DIR% 2> nul
mkdir %BUILD_DIR% 2> nul

echo %CD%

set SRC_DIR=%PRJ_DIR%\src\Windows

set RC_FLAG=/nologo /i%PRJ_DIR%\res
set CL_FLAG=/nologo /DRELEASE /DUNICODE /MT /EHsc /Ox /c
set LINK_FLAG=/NOLOGO user32.lib gdi32.lib ole32.lib
set LINK_LIBS=gdiplus.lib magnification.lib

pushd %BUILD_DIR%

cl %CL_FLAG% %SRC_DIR%\ColorPicker.cxx
rc %RC_FLAG% /foResource.res %SRC_DIR%\Resource.rc
link %LINK_FLAG% %LINK_LIBS% *.res *.obj /OUT:%DST_DIR%\ColorPicker.exe

popd

goto :EOF
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

