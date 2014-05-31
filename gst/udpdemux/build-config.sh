#!/bin/bash
# 2013 Copyright Remotium Inc.
# Jose Pereira <jose@remotium.com>

# Custom iOS configuration for x264 build
LIBNAME="libgstudpdemux"
BUILDDIR="/Users/davidchen/Workspace/prebuilt/$TARGET"

#CONFIGOPTS="--enable-static --disable-maintainer-mode --disable-silent-rules --enable-static-plugins --enable-shared=no"
CONFIGOPTS=

MAKE_OPTS=
MAKE_INSTALL_OPTS="install"

OUTPUT_LIBS="libgstudpdemux"



function pre_config {

	if [[ "$TARGET" == "ios" ]]; then

		if [[ "$ARCH" == "armv7" || "$ARCH" == "armv7s" ]]; then
			ARCH_HOST="arm-apple-darwin12"
		fi

		if [[ "$ARCH" == "i386" ]]; then
			ARCH_HOST="i386"
		fi

	    CONFIGOPTS="$CONFIGOPTS --host=$ARCH_HOST"

	elif [[ "$TARGET" == "macosx" ]]; then
		ARCH_HOST="x86_64"

		CONFIGOPTS="$CONFIGOPTS"

	elif [[ "$TARGET" == "android" ]]; then

		#LDFLAGS="$LDFLAGS --gc-sections"
		LDFLAGS="$LDFLAGS -lgnustl_static -lsupc++ "
		CONFIGOPTS="$CONFIGOPTS --host=$BUILD_HOST"
	fi
}

function post_make_install {
	make $MAKE_INSTALL_OPTS
}
