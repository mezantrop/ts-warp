"""
Py2app setup file for TS-Warp GUI-Frontend macOS Application

To build the application install py2app and run:
    python3 setup.py py2app
"""

from setuptools import setup

APP = ['gui-warp.py']
OPTIONS = {
    'iconfile': 'media/gui-warp.icns',
    'plist': {
        'LSUIElement': True,
    },
}

setup(
    app=APP,
    options={'py2app': OPTIONS},
    setup_requires=['py2app'],
)
