#!/bin/bash
MY_PATH="`dirname \"$0\"`"
cd ${MY_PATH}

#create dirs if them are not exist
mkdir -p ~/.local/share/applications/
mkdir -p ~/.config/autostart/
sed "s?sssssss?${PWD}/run.sh?g" < ./metagate_template.desktop > metagate.desktop
mv metagate.desktop ~/.local/share/applications/
sed "s?sssssss?${PWD}/run.sh?g" < ./metagate_atemplate.desktop > metagate.desktop
mv metagate.desktop ~/.config/autostart/
if type "xdg-mime" > /dev/null; then
  xdg-mime default metagate.desktop x-scheme-handler/metapay
fi
mkdir -p ~/.local/share/icons/
cp metagate.png ~/.local/share/icons/

echo > ./qt.conf
echo [Paths] >> ./qt.conf
echo -n "Prefix = " >> ./qt.conf
for (( i = 0; i < ${#PWD}; i++ )); do
c="${PWD:i:1}"
printf '\\x%04x' "'$c" >> ./qt.conf
done
    
#echo $PWD | 
#    while read -n 1 u
#    do [[ -n "$u" ]] && [ "$u" != "/" ] && printf '\\x%04x' "'$u" >> ./qt.conf || [[ -n "$u" ]] && [ "$u" == "/" ] && printf "$u" >> ./qt.conf
#    done;
echo >> ./qt.conf
echo Plugins = plugins >> ./qt.conf
echo Imports = qml >> ./qt.conf
echo Qml2Imports = qml >> ./qt.conf

chmod +x ./libexec/QtWebEngineProcess
chmod +x ./MetaGate
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./lib/" QT_QPA_PLATFORM_PLUGIN_PATH="./plugins/" ./MetaGate $* --remote-debugging-port=8081
