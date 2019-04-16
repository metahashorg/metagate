#!/bin/sh

sed "s?sssssss?${PWD}/MetaGate?g" < ./mgtemp.plist > ~/Library/LaunchAgents/com.metahash.metagate.plist
