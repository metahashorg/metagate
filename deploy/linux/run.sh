#!/bin/sh
echo > ./qt.conf
echo [Paths] >> ./qt.conf
echo Prefix = $PWD >> ./qt.conf
echo Plugins = plugins >> ./qt.conf
echo Imports = qml >> ./qt.conf
echo Qml2Imports = qml >> ./qt.conf

chmod +x ./libexec/QtWebEngineProcess
chmod +x ./MetaGate
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./lib/" QT_QPA_PLATFORM_PLUGIN_PATH="./plugins/" ./MetaGate
