#!/bin/sh
chmod +x ./libexec/QtWebEngineProcess
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./lib" ./WalletMetahash
