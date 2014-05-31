#!/bin/bash
# 2013 Copyright Remotium Inc.
# Jose Pereira <jose@remotium.com>

ARCHS="armv7 armv7s i386"
TARGET_OS=ios
TARGET="ios"
LIBNAME=libgstlibyuvscaler
OUTPUT_LIBS="libgstlibyuvscaler"
XCODE_SELECT="xcode-select"
XCODE=$(${XCODE_SELECT} --print-path)
SDK_VERSION="7.0"
COMPILER="gcc"

CONFIGOPTS="--enable-static --disable-maintainer-mode --disable-silent-rules --enable-static-plugins --enable-shared=no"
BUILDDIR=/Users/davidchen/Workspace/Builds/GStreamer

CFLAGS=
CXXFLAGS=
LDFLAGS=


if [ ! -d "$XCODE" ]; then
  echo "xcode path is not set correctly $XCODE does not exist (most likely because of xcode > 4.3)"
  echo "run"
  echo "sudo xcode-select -switch <xcode path>"
  echo "for default installation:"
  echo "sudo xcode-select -switch /Applications/Xcode.app/Contents/Developer"
  exit 1
fi
