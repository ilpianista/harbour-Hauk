import QtQuick 2.2
import Sailfish.Share 1.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: qsTr("Share link")
                enabled: client.shareLink != ""
                onClicked: {
                    share.trigger()
                }
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            anchors.left: parent.left
            anchors.right: parent.right

            PageHeader {
                title: "Hauk"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: client.status
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: formatTime(client.remainingSeconds)
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraLarge
                horizontalAlignment: Text.AlignHCenter
                visible: client.tracking
            }

            Column {
                width: parent.width
                visible: client.tracking
                spacing: Theme.paddingMedium

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Latitude")
                    value: client.latitude.toFixed(6)
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Longitude")
                    value: client.longitude.toFixed(6)
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Accuracy")
                    value: client.accuracy.toFixed(1) + " m"
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Short ID")
                    value: client.shareLink.substring(client.shareLink.indexOf("?") + 1)
                }
            }

            // Start/Stop button
            Button {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: client.tracking ? qsTr("Stop sharing") : qsTr(
                                            "Start sharing")
                enabled: !client.tracking || client.sessionId.length > 0
                onClicked: {
                    if (client.tracking) {
                        client.stopTracking()
                    } else {
                        client.startTracking()
                    }
                }
            }

            // Duration selector (when not tracking)
            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Duration")
                visible: !client.tracking
                currentIndex: {
                    switch (client.duration) {
                    case 1800:
                        return 0
                    case 3600:
                        return 1
                    case 7200:
                        return 2
                    case 14400:
                        return 3
                    case 43200:
                        return 4
                    default:
                        return 1
                    }
                }
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("30 minutes")
                        onClicked: client.duration = 1800
                    }
                    MenuItem {
                        text: qsTr("1 hour")
                        onClicked: client.duration = 3600
                    }
                    MenuItem {
                        text: qsTr("2 hours")
                        onClicked: client.duration = 7200
                    }
                    MenuItem {
                        text: qsTr("4 hours")
                        onClicked: client.duration = 14400
                    }
                    MenuItem {
                        text: qsTr("12 hours")
                        onClicked: client.duration = 43200
                    }
                }
            }

            // Interval selector (when not tracking)
            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Update interval")
                visible: !client.tracking
                currentIndex: {
                    switch (client.interval) {
                    case 10:
                        return 0
                    case 30:
                        return 1
                    case 60:
                        return 2
                    case 120:
                        return 3
                    case 300:
                        return 4
                    default:
                        return 1
                    }
                }
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("10 seconds")
                        onClicked: client.interval = 10
                    }
                    MenuItem {
                        text: qsTr("30 seconds")
                        onClicked: client.interval = 30
                    }
                    MenuItem {
                        text: qsTr("1 minute")
                        onClicked: client.interval = 60
                    }
                    MenuItem {
                        text: qsTr("2 minutes")
                        onClicked: client.interval = 120
                    }
                    MenuItem {
                        text: qsTr("5 minutes")
                        onClicked: client.interval = 300
                    }
                }
            }

            // Server info
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr(
                          "Server:") + " " + (client.serverUrl ? client.serverUrl : qsTr(
                                                                     "Not configured"))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                visible: !client.tracking
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            Label {
                id: errorMsg
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                topPadding: Theme.paddingMedium
                wrapMode: Text.Wrap
                color: Theme.errorColor
                visible: text.length > 0
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        VerticalScrollDecorator {}
    }

    ShareAction {
        id: share

        title: qsTr("Share link")
        mimeType: "text/x-url"
        resources: [{
                "type": "text/x-url",
                "linkTitle": "Hauk",
                "status": client.shareLink
            }]
    }

    function formatTime(seconds) {
        if (seconds <= 0)
            return ""
        var hours = Math.floor(seconds / 3600)
        var mins = Math.floor((seconds % 3600) / 60)
        var secs = seconds % 60
        if (hours > 0) {
            return hours + ":" + (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs
        }
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }

    Connections {
        target: client
        onError: {
            errorMsg.text = errorMsg
        }
    }
}
