#!/bin/bash
FN="/Library/LaunchDaemons/$1.plist"
echo $FN
cat <<EOT >> $FN
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
<key>Label</key>
<string>$1</string>
<key>ProgramArguments</key>
<array>
<string>$2</string>
<string>-m</string>
<string>$3</string>
</array>
<key>RunAtLoad</key>
<true/>
</dict>
</plist>
EOT
