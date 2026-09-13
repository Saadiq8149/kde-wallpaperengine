import QtQuick
import org.kde.plasma.plasmoid

WallpaperItem {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "red"

        Text {
            anchors.centerIn: parent
            text: "KDE WALLPAPER ENGINE"
            color: "white"
            font.pixelSize: 64
        }
    }
}
