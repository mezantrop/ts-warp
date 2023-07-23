#!/bin/sh

# -------------------------------------------------------------------------------------------------------------------- #
python3 -m venv venv
cd venv/bin
source activate.csh
pip3 install tk
pip3 install py2app
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8 venv/lib
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8.6 venv/lib
cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tk8.6 venv/lib

python3 setup.py py2app