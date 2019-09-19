#!/bin/sh
DIR=$(dirname "$0")
sed "s?sssssss?$DIR/MetaGate.app/Contents/MacOS/MetaGate?g" < $DIR/mgtemp.plist > ~/Library/LaunchAgents/com.metahash.metagate.plist
