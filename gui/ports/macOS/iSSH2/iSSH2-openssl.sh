#!/bin/bash
                                   #########
#################################### iSSH2 #####################################
#                                  #########                                   #
# Copyright (c) 2013 Tommaso Madonia. All rights reserved.                     #
#                                                                              #
# Permission is hereby granted, free of charge, to any person obtaining a copy #
# of this software and associated documentation files (the "Software"), to deal#
# in the Software without restriction, including without limitation the rights #
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell    #
# copies of the Software, and to permit persons to whom the Software is        #
# furnished to do so, subject to the following conditions:                     #
#                                                                              #
# The above copyright notice and this permission notice shall be included in   #
# all copies or substantial portions of the Software.                          #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,#
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN    #
# THE SOFTWARE.                                                                #
################################################################################

################################################################################
#                                                                              #
# Modifications for "bleeding-edge" OpenSSL and libssh2 versions               #
# by Mikhail Zakharov, Copyright (c) 2024-2026                                 #
# Licensed under BSD-2-Clause License                                          #
#                                                                              #
################################################################################

XCODE_VERSION=`xcodebuild -version | grep Xcode | cut -d' ' -f2`

version () {
  printf "%02d%02d%02d" ${1//./ }
}

source "$BASEPATH/iSSH2-commons"

set -e

mkdir -p "$LIBSSLDIR"

LIBSSL_TAR="head-openssl.tgz"

downloadFile "https://github.com/openssl/openssl/archive/refs/heads/master.tar.gz" "$LIBSSLDIR/$LIBSSL_TAR"

LIBSSLSRC="$LIBSSLDIR/src/"
mkdir -p "$LIBSSLSRC"

set +e
echo "Extracting $LIBSSL_TAR"
tar -zxkf "$LIBSSLDIR/$LIBSSL_TAR" -C "$LIBSSLSRC" --strip-components 1 2>&-
set -e

LIBSSL_VERSION=$(awk -F '=' '
  $1 == "MAJOR" {major=$2};
  $1 == "MINOR" {minor=$2};
  $1 == "PATCH" {patch=$2};
  $1 == "PRE_RELEASE_TAG" {tag=$2};
  END {printf("%s.%s.%s_%s", major, minor, patch, tag)}' $LIBSSLDIR/src/VERSION.dat)
echo "Building OpenSSL $LIBSSL_VERSION, please wait..."

for ARCH in $ARCHS
do
  if [[ "$SDK_PLATFORM" == "macosx" ]]; then
    CONF="no-shared"
  else
    CONF="no-asm no-hw no-shared no-async"
  fi

  PLATFORM="$(platformName "$SDK_PLATFORM" "$ARCH")"
  OPENSSLDIR="$LIBSSLDIR/${PLATFORM}_$SDK_VERSION-$ARCH"
  LIPO_LIBSSL="$LIPO_LIBSSL $OPENSSLDIR/libssl.a"
  LIPO_LIBCRYPTO="$LIPO_LIBCRYPTO $OPENSSLDIR/libcrypto.a"

  if [[ -f "$OPENSSLDIR/libssl.a" ]] && [[ -f "$OPENSSLDIR/libcrypto.a" ]]; then
    echo "libssl.a and libcrypto.a for $ARCH already exist."
  else
    rm -rf "$OPENSSLDIR"
    cp -R "$LIBSSLSRC"  "$OPENSSLDIR"
    cd "$OPENSSLDIR"

    LOG="$LOGPATH/build-openssl.log"
    : > $LOG

    if [[ "$SDK_PLATFORM" == "macosx" ]]; then
      if [[ "$ARCH" == "x86_64" ]]; then
        HOST="darwin64-x86_64-cc"
      elif [[ "$ARCH" == "arm64" ]] && [[ $(version "$XCODE_VERSION") -ge $(version "12.0") ]]; then
        HOST="darwin64-arm64-cc"
      else
        HOST="darwin-$ARCH-cc"
      fi
    else
      HOST="iphoneos-cross"
      if [[ "${ARCH}" == *64 ]] || [[ "${ARCH}" == arm64* ]]; then
        CONF="$CONF enable-ec_nistp_64_gcc_128"
      fi
    fi

    export CROSS_TOP="$DEVELOPER/Platforms/$PLATFORM.platform/Developer"
    export CROSS_SDK="$PLATFORM$SDK_VERSION.sdk"
    export SDKROOT="$CROSS_TOP/SDKs/$CROSS_SDK"
    export CC="$CLANG -arch $ARCH"

    CONF="$CONF -m$SDK_PLATFORM-version-min=$MIN_VERSION $EMBED_BITCODE"

    ./Configure $HOST $CONF >> "$LOG" 2>&1

    if [[ "$ARCH" == "x86_64" ]]; then
      sed -ie "s!^CFLAG=!CFLAG=-isysroot $SDKROOT !" "Makefile"
    fi

    make depend >> "$LOG" 2>&1
    make -j "$BUILD_THREADS" build_libs >> "$LOG" 2>&1

    echo "- $PLATFORM $ARCH done!"
  fi
done

lipoFatLibrary "$LIPO_LIBSSL" "$BASEPATH/openssl_$SDK_PLATFORM/lib/libssl.a"
lipoFatLibrary "$LIPO_LIBCRYPTO" "$BASEPATH/openssl_$SDK_PLATFORM/lib/libcrypto.a"

importHeaders "$OPENSSLDIR/include/" "$BASEPATH/openssl_$SDK_PLATFORM/include"

echo "Building done."
