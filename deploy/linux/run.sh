#!/bin/sh
chmod +x ./libexec/QtWebEngineProcess
chmod +x ./MetaGate
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./lib/" QT_QPA_PLATFORM_PLUGIN_PATH="./plugins/" ./MetaGate
