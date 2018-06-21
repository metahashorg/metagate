#!/bin/bash

export PATH=/opt/Qt/5.10.1/gcc_64/bin/:$PATH
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/opt/Qt/5.10.1/gcc_64/lib/"
./linuxdeployqt-continuous-x86_64.AppImage $1 -extra-plugins=imageformats/libqsvg.so,iconengines/libqsvgicon.so
