#!/bin/sh

# -------------------------------------------------------------------------------------------------------------------- #
# Build gui-warp.app macOS application                                                                                 #
# -------------------------------------------------------------------------------------------------------------------- #

pv='/Library/Frameworks/Python.framework/Versions/Current'

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv tmp GUI-Warp *.dmg

echo "-- Making binaries ----------------------------------------------------------------------------------------------" &&
# NB! Static libraries must be located in the current directory! If changing, adjust Makefile as well!
[ -n "$WITH_LIBSSH2" ] && {
	echo "-- Downloading/unpacking SSL and SSH2 libraries -----------------------------------------------------------------"
	mkdir tmp
	wget -O tmp/head-openssl.tgz https://github.com/openssl/openssl/archive/refs/heads/master.tar.gz
	wget -O tmp/head-libssh2.tgz https://github.com/libssh2/libssh2/archive/refs/heads/master.tar.gz
	cd tmp
	tar zxvf head-openssl.tgz
	tar zxvf head-libssh2.tgz

	echo "-- Compiling SSL for ARM64 processors ---------------------------------------------------------------------------"
	cd openssl-master

	./Configure darwin64-arm64-cc no-shared -mmacosx-version-min=11 -fembed-bitcode
	make depend
	make -j 4 build_libs
	cp libcrypto.a ../arm64_libcrypto.a
	cp libssl.a  ../arm64_libssl.a

	echo "-- Compiling SSL for x86_64 processors --------------------------------------------------------------------------"
	make clean
	./Configure darwin64-x86_64-cc no-shared -mmacosx-version-min=11 -fembed-bitcode
	make depend
	make -j 4 build_libs
	cp libcrypto.a ../x86_64_libcrypto.a
	cp libssl.a  ../x86_64_libssl.a

	echo "-- Compiling SSH2 for ARM64 processors --------------------------------------------------------------------------"
	cd ../libssh2-master
	export CFLAGS="-arch arm64 -pipe -no-cpp-precomp  -mmacosx-version-min=11 -fembed-bitcode"
	export CPPFLAGS="-arch arm64 -pipe -no-cpp-precomp -mmacosx-version-min=11"
	autoreconf -fi
	./configure --host=aarch64-apple-darwin --disable-debug --disable-dependency-tracking --disable-silent-rules --disable-examples-build --with-libz --with-crypto=openssl --with-libssl-prefix=../openssl-master --disable-shared --enable-static
	make -j 4
	cp src/.libs/libssh2.a ../arm64_libssh2.a

	make clean
	export CFLAGS="-arch x86_64 -pipe -no-cpp-precomp -mmacosx-version-min=11 -fembed-bitcode"
	export CPPFLAGS="-arch x86_64 -pipe -no-cpp-precomp -mmacosx-version-min=11"
	autoreconf -fi
	./configure --host=x86_64-apple-darwin --disable-debug --disable-dependency-tracking --disable-silent-rules --disable-examples-build --with-libz --with-crypto=openssl -with-libssl-prefix=../openssl-master --disable-shared --enable-static
	make -j 4
	cp src/.libs/libssh2.a ../x86_64_libssh2.a

	unset CFLAGS
	unset CPPFLAGS

	echo "-- Making universal libraries -----------------------------------------------------------------------------------"
	cd ..
	lipo -create arm64_libssl.a x86_64_libssl.a -output ../libssl.a
	lipo -create arm64_libcrypto.a x86_64_libcrypto.a -output ../libcrypto.a
	lipo -create arm64_libssh2.a x86_64_libssh2.a -output ../libssh2.a
	cd ..

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
ln -s /Applications GUI-Warp
hdiutil create GUI-Warp-tmp.dmg -ov -volname "GUI-Warp" -fs HFS+ -srcfolder "GUI-Warp"
hdiutil convert GUI-Warp-tmp.dmg -format UDZO -o GUI-Warp.dmg

echo "-- Cleanup ------------------------------------------------------------------------------------------------------" &&
make clean &&
rm -rf build dist venv tmp GUI-Warp GUI-Warp-tmp.dmg
