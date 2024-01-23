#!/bin/sh

# -------------------------------------------------------------------------------------------------------------------- #
# Build gui-warp.app macOS application                                                                                 #
# -------------------------------------------------------------------------------------------------------------------- #

pv='/Library/Frameworks/Python.framework/Versions/Current'

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv GUI-Warp *.dmg

echo "-- Making binaries ----------------------------------------------------------------------------------------------" &&
# NB! Static libraries are in current directory! If changing, adjust Makefile as well!
[ -n "$WITH_LIBSSH2" ] && {
	[ ! -f ./libssh2.a -o ! -f ./libcrypto.a -o ! -f libssl.a ] && {
		echo "Unable to find static SSH2 library, skipping it"
		make
	} || make ts-warp-ssh2
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

echo "-- Creating DMG -------------------------------------------------------------------------------------------------" &&
mkdir GUI-Warp
mv dist/gui-warp.app GUI-Warp
hdiutil create GUI-Warp-tmp.dmg -ov -volname "GUI-Warp" -fs HFS+ -srcfolder "GUI-Warp"
hdiutil convert GUI-Warp-tmp.dmg -format UDZO -o GUI-Warp.dmg

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv GUI-Warp GUI-Warp-tmp.dmg
