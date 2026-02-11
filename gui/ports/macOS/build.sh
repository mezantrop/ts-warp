#!/bin/sh

# -------------------------------------------------------------------------------------------------------------------- #
# Build gui-warp.app macOS application                                                                                 #
# -------------------------------------------------------------------------------------------------------------------- #

# Python3 to build virtual environmen
pv='/Library/Frameworks/Python.framework/Versions/Current'
pv_major=3
pv_minor=11

# iSSH2-head to build in current versions of OpenSSL/libssh2 libraries statically
issh2_home='./iSSH2'
issh2_platform_name='macosx'
issh2_platform_version=11
issh2_architectures="arm64 x86_64"

l_libssh2="$issh2_home/libssh2_macosx/lib/libssh2.a"
h_libssh2="$issh2_home/libssh2_macosx/include/libssh2.h"
l_libssl="$issh2_home/openssl_macosx/lib/libssl.a"
l_libcrypto="$issh2_home/openssl_macosx/lib/libcrypto.a"

which $pv/bin/python3 > /dev/null || { echo "No Python interpreter detected"; exit 1; }
$pv/bin/python3 -V |
	awk -v maj=$pv_major -v min=$pv_minor -F '[ .]' '
								{ print($0, "detected") }
		$2 != maj || $3 > min	{ exit 1 }
	' || {
		echo "Unsupported Python version. Use Python <= $pv_major.$pv_minor.x"
		exit 1
	}

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv tmp GUI-Warp *.dmg

echo "-- Making binaries ----------------------------------------------------------------------------------------------" &&
# NB! Static libraries and header files must be located in the current directory! If changing, adjust Makefile as well!
[ -n "$WITH_LIBSSH2" ] && {
	sh $issh2_home/iSSH2-head.sh \
		--platform="$issh2_platform_name" \
		--min-version="$issh2_platform_version" \
		--archs="$issh2_architectures"

	[ ! -f "$l_libssh2" -o ! -f "$h_libssh2" -o ! -f "$l_libssl" -o ! -f "$l_libcrypto" ] && {
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		echo "!!! Unable to find static OpenSSL/libssh2 libraries or header files          !!!"
		echo "!!! Building with NO SSH2 support!                                           !!!"
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		make
	} || {
		cp $l_libssh2 $h_libssh2 $l_libssl $l_libcrypto .
		make ts-warp-ssh2
	}
} || make

echo "-- Making environment -------------------------------------------------------------------------------------------" &&
$pv/bin/python3 -m venv venv &&
cd venv/bin &&
source activate &&
$pv/bin/pip3 install --upgrade pip &&
$pv/bin/pip3 install tk &&
$pv/bin/pip3 install py2app &&

echo "-- Setting TCL/Tk -----------------------------------------------------------------------------------------------" &&
cd ../.. &&
cp -r $pv/lib/tcl8 venv/lib &&
cp -r $pv/lib/tcl8.6 venv/lib &&
cp -r $pv/lib/tk8.6 venv/lib &&

echo "-- Setting permissions ------------------------------------------------------------------------------------------" &&
chmod 755 ts-warp &&
chmod 755 ts-pass &&
chmod 755 ts-warp.sh &&
chmod 755 ts-warp_autofw.sh &&

echo "-- Building the app ---------------------------------------------------------------------------------------------" &&
$pv/bin/python3 setup.py py2app &&

# echo "-- Archiving ----------------------------------------------------------------------------------------------------" &&
# tar cvf - -C dist gui-warp.app | gzip --best > gui-warp.app.tgz &&

# Installing the app-launcher - starter
mv dist/gui-warp.app/Contents/MacOS/gui-warp dist/gui-warp.app/Contents/MacOS/app
cp starter dist/gui-warp.app/Contents/MacOS/gui-warp

echo "-- Creating DMG -------------------------------------------------------------------------------------------------" &&
mkdir GUI-Warp
mv dist/gui-warp.app GUI-Warp
ln -s /Applications GUI-Warp

hdiutil create GUI-Warp-tmp.dmg -ov -volname "GUI-Warp" -fs HFS+ -srcfolder "GUI-Warp"
hdiutil convert GUI-Warp-tmp.dmg -format UDZO -o GUI-Warp.dmg

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv tmp GUI-Warp GUI-Warp-tmp.dmg \
	libcrypto.a libssh2.a libssl.a libssh2.h \
	iSSH2/libssh2_macosx iSSH2/openssl_macosx iSSH2/log
