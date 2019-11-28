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
    declare -rg PRJ_DIR=`pwd`
popd > /dev/null

declare -rg DST_DIR=$PRJ_DIR/_dst/$os_name
declare -rg BUILD_DIR=$PRJ_DIR/_build/$os_name

project_build()
{
    mkdir -p $DST_DIR > /dev/null
    mkdir -p $BUILD_DIR > /dev/null

    res_dir=$PRJ_DIR/res
    src_dir=$PRJ_DIR/src

    cc_flags+=' -DOS_MACOS -DRELEASE -std=c++17 -mmacosx-version-min=10.11'

    link_libs+='-lc++ -framework AppKit -framework IOKit'

    pushd $BUILD_DIR > /dev/null

        cat $res_dir/Mask@2+s.png > RES_Circle_Mask
        xxd -i RES_Circle_Mask > RES_Circle_Mask.cxx
        cc $cc_flags -c RES_Circle_Mask.cxx -o Resource.o

        cc $cc_flags -c -ObjC++ $src_dir/Instance.cxx -o Instance.o
        cc $cc_flags -c -ObjC++ $src_dir/macOS/ColorPicker.cxx -o ColorPicker.o
        cc $cc_flags $link_libs *.o  -o $DST_DIR/ColorPicker

    popd > /dev/null
}
project_build

# Sign
# declare -r key_id='Developer ID Application: Yuanyi Zhang (YMUB8PUSZ5)'
# codesign -s $key_id $DST_DIR/ColorPicker

