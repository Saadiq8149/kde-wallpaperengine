#!/usr/bin/env bash

set -e
cd ~/dev/kde-wallpaperengine
PACKAGE="$PWD/build/kde-wallpaperengine.plasmoid"
mkdir -p build
rm -f "$PACKAGE"
zip -r "$PACKAGE" metadata.json contents
if kpackagetool6 --type Plasma/Wallpaper --list | grep -q "com.saadiq.kde.wallpaperengine"; then
    kpackagetool6 --type Plasma/Wallpaper --upgrade "$PACKAGE"
else
    kpackagetool6 --type Plasma/Wallpaper --install "$PACKAGE"
fi
systemctl --user restart plasma-plasmashell.service
