#!/bin/bash
DIR=$(dirname "$0")
LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH" $DIR/asdesktopproxyservice $1
