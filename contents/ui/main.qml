import QtQuick
import org.kde.plasma.plasmoid

WallpaperItem {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "black"

        Text {
            anchors.centerIn: parent
            text: "KDE WALLPAPER ENGINE"
            color: "red"
            font.pixelSize: 64
        }
    }
}
