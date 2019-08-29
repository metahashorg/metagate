#!/bin/bash
UNITDIR="/etc/systemd/system"
#UNITDIR="."
FN="$UNITDIR/$1.service"
#echo $FN
cat <<EOT > "$FN"
[Unit]
Description=$1
After=network.target

[Service]
Type=forking
WorkingDirectory=$2
ExecStart=$2/$3
ExecStop=$2/$3 -t

[Install]
WantedBy=multi-user.target
EOT
