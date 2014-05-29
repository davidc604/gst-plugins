#!/bin/sh
# 2013 Copyright Remotium Inc.
# Jose Pereira <jose@remotium.com>

# iOS automated build script

BASE_CONFIG_OPTS=$CONFIGOPTS
BASE_CFLAGS=$CFLAGS
BASE_CXXFLAGS=$CXXFLAGS
BASE_LDFLAGS=$LDFLAGS

# build process
for ARCH in ${ARCHS}; do
    #pre_clean
    make clean

    #reset config opts
    CONFIGOPTS=$BASE_CONFIG_OPTS

    if [[ $TARGET == "ios" ]]; then
        if [[ "${ARCH}" == "i386" ]]; then
            SDK_VERSION="6.1"
            PLATFORM="iPhoneSimulator"
        else
            SDK_VERSION="7.0"
            PLATFORM="iPhoneOS"
        fi
    elif [[ $TARGET == "macosx" ]]; then
        PLATFORM="MacOSX"
    fi

    XCRUN_SDK="xcrun -sdk $(echo ${PLATFORM} | tr '[:upper:]' '[:lower:]')"
    export CC="$($XCRUN_SDK -find ${COMPILER})"
    export CXX="$($XCRUN_SDK -find g++)"
    export LD="$($XCRUN_SDK -find ld)"
    export AR="$($XCRUN_SDK -find ar)"
    export NM="$($XCRUN_SDK -find nm)"
    export RANLIB="$($XCRUN_SDK -find ranlib)"

    SDK="${XCODE}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDK_VERSION}.sdk"

    INCLIBPATHS="-I$BUILDDIR/include"
    LIBPATHS="-L$BUILDDIR/lib/ -L$SDK/usr/lib -L$SDK/usr/lib/system"

    export CFLAGS="$BASE_CFLAGS -arch ${ARCH} -isysroot ${SDK}"
    export CXXFLAGS="$BASE_CXXFLAGS -arch ${ARCH} -isysroot ${SDK}"
    export LDFLAGS="$BASE_LDFLAGS -arch ${ARCH} -isysroot ${SDK}"

    echo ""
    echo "* Building ${LIBNAME} for ${PLATFORM} ${SDK_VERSION} (${ARCH})..."

    mkdir -p "${BUILDDIR}/bin/${ARCH}"

    # pre_config

    #if [[ "x$CONFIGURE_SCRIPT" != "x" ]]; then
    #    echo "Config"
    #    ./$CONFIGURE_SCRIPT --prefix="$BUILDDIR/bin/$ARCH" $CONFIGOPTS || exit 1
    #fi

    echo "Init Config"
    ./configure --prefix="$BUILDDIR/bin/$ARCH" --host="${ARCH}" $CONFIGOPTS

    #if [[ $NO_MAKE != "yes" ]]; then
    #    make $MAKE_OPTS || exit 1
    #fi

    # post_make_install
    make install
done

mkdir -p $BUILDDIR
cd $BUILDDIR
#BUILDDIR=$(pwd) #TODO builddir shall be an absolute dir


echo ""
echo "* Creating binaries for $LIBNAME..."

mkdir -p $BUILDDIR/lib

# Use lipo to create universal lib. Run per each lib on LIBS
echo $OUTPUT_LIBS
for LIB in $OUTPUT_LIBS; do
LIPO="$XCRUN_SDK lipo -create"
    for ARCH in $ARCHS; do
        LIPO="$LIPO $BUILDDIR/bin/$ARCH/$LIB.a"
    done
LIPO="$LIPO -output $BUILDDIR/lib/$LIB.a"
echo $LIPO
eval $LIPO
done

mkdir -p $BUILDDIR/include

FIRST_ARCH="${ARCHS%% *}"
cp -R "${BUILDDIR}/bin/${FIRST_ARCH}/include/" "${BUILDDIR}/include/"

echo ""
echo "* Finished; $LIBNAME binary created for platforms: $ARCHS"
