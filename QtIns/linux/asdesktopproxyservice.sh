#!/bin/bash
DIR=$(dirname "$0")
LD_LIBRARY_PATH="$DIR:$LD_LIBRARY_PATH" $DIR/asdesktopproxyservice $1
