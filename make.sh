#!/bin/sh

export PATH=/home/chengpg/bin/arm/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin:$PATH

DEST="../../Image/"

CROSS=arm-linux-

export ARCH=arm
export CROSS_COMPILE=$CROSS

build_sbc8600() {
    if ! [ -f ".config" ]; then
        make am335x_sbc8600_defconfig
        [ $? != 0 ] && exit 1
    fi

    make -j8
    [ $? != 0 ] && exit 1

    echo "Build Info: COPY MLO u-boot.img -> $DEST"
    cp -rf MLO u-boot.img $DEST
}

build_weidian() {
    if ! [ -f ".config" ]; then
        make am335x_weidian_defconfig
        [ $? != 0 ] && exit 1
    fi

    make -j8
    [ $? != 0 ] && exit 1

    echo "Build Info: COPY MLO u-boot.img -> $DEST"
    cp -rf MLO u-boot.img $DEST
}

build_sbc8600
# build_weidian
