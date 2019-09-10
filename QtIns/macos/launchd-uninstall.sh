#!/bin/bash
FN="/Library/LaunchDaemons/$1.plist"
echo $FN
rm -f $FN
#killall $2
