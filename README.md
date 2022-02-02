# MetaHashGate

This repository contains Metahash wallet c++ source code. Wallet supports following currencies: MHC, BTC, ETC.

## Get the source code
Clone the repository by:
```shell
git clone https://github.com/metahashorg/metagate
```

## Build

Current Qt version - 10.1. For Windows, Qt 10.1 with visual studio 2015 compiler.
Detailed instructions for the project build on linux, mac or win can be found in the [deploy folder](https://github.com/metahashorg/metagate/tree/master/deploy).

## Api to connect with javascript

When `Q_INVOKABLE` qt-function returns some result, in javascript it must be got via callback, e.g.: 
```shell
mainWindow.openFolderDialog(beginPath, caption, function(returnValue) {
alert(returnValue);
});
