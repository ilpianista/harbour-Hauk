import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Column {
        anchors.centerIn: parent
        width: parent.width
        spacing: Theme.paddingSmall

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.width - 5 * Theme.paddingLarge
            width: parent.width - 5 * Theme.paddingLarge
            source: "/usr/share/icons/hicolor/86x86/apps/harbour-hauk.png"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Hauk"
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.primaryColor
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: client.tracking ? client.status : qsTr("Not sharing")
            font.pixelSize: Theme.fontSizeSmall
            color: client.tracking ? Theme.highlightColor : Theme.secondaryColor
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: formatTime(client.remainingSeconds)
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.secondaryHighlightColor
            visible: client.tracking
        }
    }

    CoverActionList {
        enabled: !client.tracking
        CoverAction {
            iconSource: "image://theme/icon-cover-location"
            onTriggered: client.startTracking()
        }
    }

    CoverActionList {
        enabled: client.tracking
        CoverAction {
            iconSource: "image://theme/icon-cover-pause"
            onTriggered: client.stopTracking()
        }
    }

    function formatTime(seconds) {
        if (seconds <= 0)
            return "";
        var hours = Math.floor(seconds / 3600);
        var mins = Math.floor((seconds % 3600) / 60);
        var secs = seconds % 60;
        if (hours > 0) {
            return hours + ":" + (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs;
        }
        return mins + ":" + (secs < 10 ? "0" : "") + secs;
    }
}
