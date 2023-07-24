#!/bin/sh

# -------------------------------------------------------------------------------------------------------------------- #
# Build gui-warp.app macOS application                                                                                 #
# -------------------------------------------------------------------------------------------------------------------- #

make &&

python3 -m venv venv &&
cd venv/bin &&
source activate &&
pip3 install tk &&
pip3 install py2app &&
cd ../.. &&
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8 venv/lib &&
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8.6 venv/lib &&
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tk8.6 venv/lib &&

python3 setup.py py2app &&

tar cvf - -C dist gui-warp.app | gzip --best > gui-warp.app.tgz &&

make clean &&
rm -rf build dist venv