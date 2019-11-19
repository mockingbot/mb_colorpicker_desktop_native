#! /usr/bin/env bash
# set -o xtrace

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

    CL_FLAG='-DRELEASE -std=c++14 -mmacosx-version-min=10.11'
    LINK_LIBS='-lc++ -lSystem -framework Cocoa -framework AppKit -framework IOKit'

    RES_DIR=$PRJ_DIR/res
    SRC_DIR=$PRJ_DIR/src/macOS

    pushd $BUILD_DIR > /dev/null
        cat $RES_DIR/Mask@2+s.png > RES_Circle_Mask
        xxd -i RES_Circle_Mask > RES_Circle_Mask.cxx

        cc -c $CL_FLAG RES_Circle_Mask.cxx
        cc -c $CL_FLAG $SRC_DIR/ColorPicker.mm

        cc $CL_FLAG $LINK_LIBS *.o -o $DST_DIR/ColorPicker

        # mkdir -p $APP_DIR/Contents > /dev/null
        # cp -f $SRC_DIR/Info.plist $APP_DIR/Contents

        # mkdir -p $APP_DIR/Contents/Resources > /dev/null
        # cp -f $RES_DIR/Mask@2+s.png $APP_DIR/Contents/Resources/Mask.png
        # cp -f $RES_DIR/Mask@2+s.png $APP_DIR/Contents/Resources/Mask.png

        # mkdir -p $APP_DIR/Contents/MacOS > /dev/null

    popd > /dev/null
}
project_build

# Sign
# codesign -s 'Developer ID Application: Yuanyi Zhang (YMUB8PUSZ5)' $DST_DIR/ColorPicker.app

