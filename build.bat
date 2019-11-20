@echo off
setlocal

set PRJ_DIR=%cd%

set DEFAULT_PATH=C:\Windows\system32;C:\Windows;

@rem VS2019 Command Promot need this to stop annoying dir change
set VSCMD_START_DIR=%PRJ_DIR%
@rem VS2019 Command Promot need this to stop annoying telemetry
set VSCMD_SKIP_SENDTELEMETRY=YES
set VS2019_INSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community

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
::                       The env clean function                               ::
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
::                          The real build function                           ::
::                                                                            ::
:BUILD
mkdir %DST_DIR% 2> nul
mkdir %BUILD_DIR% 2> nul

set res_dir=%PRJ_DIR%\res
set src_dir=%PRJ_DIR%\src\Windows

set cc_flags=/nologo /DRELEASE /DUNICODE /DOS_WINDOWS /MT /EHsc /O2
set link_libs=user32.lib gdi32.lib ole32.lib gdiplus.lib magnification.lib

pushd %BUILD_DIR% > nul

    type "%res_dir%\Mask@2.png" > RES_Circle_Mask
    xxd -i RES_Circle_Mask > RES_Circle_Mask.cxx

    cl %cc_flags% RES_Circle_Mask.cxx %src_dir%\ColorPicker.cxx ^
       %link_libs% /Fe:%DST_DIR%\ColorPicker

popd > nul

goto :EOF
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

