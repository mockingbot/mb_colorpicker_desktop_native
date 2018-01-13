#! /usr/bin/env bash

################################################################################
os_check()
{
    case `uname` in
        Darwin)
            export os_name=macOS
            ;;
        *)
            echo "none support os"
            exit -1
            ;;
    esac
}
os_check

pushd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null
    PRJ_DIR=`pwd`
popd > /dev/null

DST_DIR=$PRJ_DIR/_dst/$os_name
BUILD_DIR=$PRJ_DIR/_build/$os_name

project_build()
{
    mkdir -p $DST_DIR > /dev/null
    mkdir -p $BUILD_DIR > /dev/null

    #set RC_FLAG=/nologo /i%PRJ_DIR%\res
    CL_FLAG='-DRELEASE -std=c++14 -mmacosx-version-min=10.11'
    LINK_FLAG='-macosx_version_min 10.11 -arch x86_64'
    LINK_LIBS='-lSystem -framework Cocoa'

    SRC_DIR=$PRJ_DIR/src/$os_name

    APP_DIR=$DST_DIR/ColorPicker.app/Contents/
    mkdir -p $APP_DIR > /dev/null

    pushd $BUILD_DIR > /dev/null
        cc -c $CL_FLAG $SRC_DIR/ColorPicker.mm
        cp -f $SRC_DIR/Info.plist $APP_DIR
        ld $LINK_FLAG $LINK_LIBS *.o -o $APP_DIR/MacOS/ColorPicker
    popd > /dev/null
}
project_build

