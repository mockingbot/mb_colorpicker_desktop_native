@echo off
setlocal

set PRJ_DIR=%cd%

set RC_FLAG=/nologo /i%PRJ_DIR%\res
set CL_FLAG=/nologo /DRELEASE /DUNICODE /MT /EHsc /Ox /c
set LINK_FLAG=/NOLOGO user32.lib gdi32.lib ole32.lib
set LINK_LIBS=gdiplus.lib d3d9.lib magnification.lib

set SRC_DIR=%PRJ_DIR%\src\Windows
set DST_DIR=%PRJ_DIR%\_dst\Windows
set BUILD_DIR=%PRJ_DIR%\_build\Windows

mkdir %DST_DIR% 2> nul
mkdir %BUILD_DIR% 2> nul

pushd %BUILD_DIR%

cl %CL_FLAG% %SRC_DIR%\ColorPicker.cxx
rc %RC_FLAG% /foResource.res %SRC_DIR%\Resource.rc
link %LINK_FLAG% %LINK_LIBS% *.res *.obj /OUT:%DST_DIR%\ColorPicker.exe

popd
