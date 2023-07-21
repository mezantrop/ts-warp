"""
Py2app setup file for TS-Warp GUI-Frontend macOS Application

------------------------------------------------------------------------------------------------------------------------
Building instructions
------------------------------------------------------------------------------------------------------------------------
Python3 with "universal" architecture is required

1. Create virtual environment:
    python3 -m venv venv
2. Switch into venv
    cd venv/bin
    source activate.csh
3. Install modules:
    pip3 install tk
    pip3 install py2app
    cd ../..
4. Copy TCL/Tk libraris into venv/lib (see the bug discussion/workaround https://stackoverflow.com/a/62430022/9127614):
    cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8 venv/lib
    cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tcl8.6 venv/lib
    cp -r /Library/Frameworks/Python.framework/Versions/Current/lib/tk8.6 venv/lib
    ls -la venv/lib
    total 0
    drwxr-xr-x   6 mez  staff   192 Jul 20 12:59 .
    drwxr-xr-x   6 mez  staff   192 Jul 20 12:21 ..
    drwxr-xr-x   3 mez  staff    96 Jul 20 12:21 python3.11
    drwxr-xr-x   5 mez  staff   160 Nov 21  2022 tcl8
    drwxr-xr-x  41 mez  staff  1312 Dec  2  2022 tcl8.6
    drwxr-xr-x  40 mez  staff  1280 Nov 21  2022 tk8.6

5. To build the application run:
    python3 setup.py py2app
"""

from setuptools import setup

APP = ['gui-warp.py']
OPTIONS = {
    'iconfile': 'media/gui-warp.icns',
    'arch': 'universal2',
    'packages': ['tkinter'],
}

setup(
    app=APP,
    options={'py2app': OPTIONS},
    setup_requires=['py2app'],
)
