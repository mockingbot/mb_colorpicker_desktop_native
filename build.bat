@echo off
setlocal

set PRJ_DIR=%cd%

set DEFAULT_PATH=C:\Windows\system32;C:\Windows;

@rem VS2019 Command Promot need this to stop annoying dir change
set VSCMD_START_DIR=%PRJ_DIR%
@rem VS2019 Command Promot need this to stop annoying telemetry
set VSCMD_SKIP_SENDTELEMETRY=YES


call :clean_env
call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
echo Build X64 Static MT
set DST_DIR=%PRJ_DIR%\_dst\Windows\x64
set BUILD_DIR=%PRJ_DIR%\_build\Windows\x64
call :build

call :clean_env
call "%VCINSTALLDIR%\Auxiliary\Build\vcvars32.bat"
echo Build X32 Static MT
set DST_DIR=%PRJ_DIR%\_dst\Windows\x32
set BUILD_DIR=%PRJ_DIR%\_build\Windows\x32
call :build

endlocal

exit /b 0

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::                       The env clean function                               ::
::                                                                            ::
:clean_env

set VSINSTALLDIR=
set INCLUDE=
set LIB=
set LIBPATH=
set PATH=%DEFAULT_PATH%

goto :eof
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::                          The real build function                           ::
::                                                                            ::
:build
mkdir %DST_DIR% 2> nul
mkdir %BUILD_DIR% 2> nul

set res_dir=%PRJ_DIR%\res
set src_dir=%PRJ_DIR%\src

set cc_flags=/nologo /DOS_WINDOWS /DRELEASE /DUNICODE /MT /EHsc /O2 /std:c++17
set link_libs=user32.lib gdi32.lib ole32.lib gdiplus.lib magnification.lib

pushd %BUILD_DIR% > nul

    type "%res_dir%\Mask@2.png" > RES_Circle_Mask
    xxd -i RES_Circle_Mask > RES_Circle_Mask.cxx

    cl %cc_flags% RES_Circle_Mask.cxx %src_dir%\Windows\ColorPicker.cxx ^
       %link_libs% /Fe:%DST_DIR%\ColorPicker

popd > nul

goto :eof
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

